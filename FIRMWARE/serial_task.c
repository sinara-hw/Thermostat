//*****************************************************************************
//
// serial_task.c - Task to handle serial peripherals and their interrupts.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/uart.h"
#include "utils/ringbuf.h"
#include "config.h"
#include "priorities.h"
#include "serial.h"
#include "telnet.h"
#include "FreeRTOSConfig.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

//*****************************************************************************
//
// The stack size for the Serial task.
//
//*****************************************************************************
#define STACKSIZE_SerialTASK    512

//*****************************************************************************
//
// Queue used to pass information from the UART interrupt handlers to the task.
// This queue is also used to signal the task that an event has occurred.
//
//*****************************************************************************
xQueueHandle g_QueSerial;

//*****************************************************************************
//
//! Possible serial event types.
//
//*****************************************************************************
typedef enum
{
    RX,
    TX
}
tSerialEventType;

//*****************************************************************************
//
//! This structure is used for holding the state of a given serial interrupt.
//! On every event, an instance of this structure is passed from the serial
//! interrupt handler to the serial task.
//
//*****************************************************************************
typedef struct
{
    tSerialEventType eEventType;
    uint8_t ui8Port;
    uint8_t ui8Char;
}
tSerialEvent;

//*****************************************************************************
//
//! Handles the UART interrupt.
//!
//! \param ui8Port is the serial port number to be accessed.
//!
//! This function is called when either of the UARTs generate an interrupt.
//! An interrupt will be generated when data is received and when the transmit
//! FIFO becomes half empty.  The transmit and receive FIFOs are processed as
//! appropriate.
//!
//! \return None.
//
//*****************************************************************************
static void
SerialUARTIntHandler(uint8_t ui8Port)
{
    signed portBASE_TYPE bYield;
    uint32_t ui32Status;
    tSerialEvent sEvent;

    //
    // Set the port for which this interrupt occurred.
    //
    sEvent.ui8Port = ui8Port;

    //
    // Get the cause of the interrupt.
    //
    ui32Status = UARTIntStatus(g_ui32UARTBase[ui8Port], true);

    //
    // Clear the cause of the interrupt.
    //
    UARTIntClear(g_ui32UARTBase[ui8Port], ui32Status);

    //
    // See if there is data to be processed in the receive FIFO.
    //
    if(ui32Status & (UART_INT_RT | UART_INT_RX))
    {
        //
        // Loop while there are characters available in the receive FIFO.
        //
        while(UARTCharsAvail(g_ui32UARTBase[ui8Port]))
        {
            //
            // Set the interrupt type to help the serial task handle the
            // information appropriately.
            //
            sEvent.eEventType = RX;

            //
            // Get the next character from the receive FIFO.
            //
            sEvent.ui8Char = UARTCharGet(g_ui32UARTBase[ui8Port]);

            //
            // Send the information regarding this event to back of the Queue.
            // This also signals to the task that a serial event has occurred.
            //
            xQueueSendToBackFromISR(g_QueSerial, &sEvent, &bYield);

            //
            // If a context switch is necessary, schedule one for when the ISR
            // exits.
            //
            if(bYield)
            {
                portYIELD();
            }
        }
    }

    //
    // See if there is space to be filled in the transmit FIFO.
    //
    if(ui32Status & UART_INT_TX)
    {
        //
        // Set the interrupt type to help the serial task handle the
        // information appropriately.
        //
        sEvent.eEventType = TX;

        //
        // Send the information regarding this event to back of the Queue.
        // This also signals to the task that a serial event has occurred.
        //
        xQueueSendToBackFromISR(g_QueSerial, &sEvent, &bYield);

        //
        // If a context switch is necessary, schedule one for when the ISR
        // exits.
        //
        if(bYield)
        {
            portYIELD();
        }
    }
}

//*****************************************************************************
//
//! Handles the UART0 interrupt.
//!
//! This function is called when the UART generates an interrupt.  An interrupt
//! will be generated when data is received and when the transmit FIFO becomes
//! half empty.  These interrupts are handled by the SerialUARTIntHandler()
//! function.
//! \return None.
//
//*****************************************************************************
void
SerialPort0IntHandler(void)
{
    SerialUARTIntHandler(0);
}

//*****************************************************************************
//
//! Handles the UART1 interrupt.
//!
//! This function is called when the UART generates an interrupt.  An interrupt
//! will be generated when data is received and when the transmit FIFO becomes
//! half empty.  These interrupts are handled by the SerialUARTIntHandler()
//! function.
//!
//! \return None.
//
//*****************************************************************************
void
SerialPort1IntHandler(void)
{
    SerialUARTIntHandler(1);
}

//*****************************************************************************
//
//! The Serial task handles data flow from the serial interrupt handler(s) to
//! the ring buffers and vice versa.
//!
//! On receiving RX interrupt, the interrupt handler passes the data received
//! to the serial task using Queue.  The serial task copies this data into the
//! ring buffer.
//!
//! On receiving a TX interrupt, indicating that space is available in TX FIFO,
//! the interrupt handler passes this information to the serial task using
//! Queue.  The serial task writes data from the ring buffer to TX FIFO.
//!
//
//*****************************************************************************
static void
SerialTask(void *pvParameters)
{
    tSerialEvent sEvent;

    //
    // Loop forever.
    //
    while(1)
    {

        //
        // Block until a message is put on the g_QueSerial queue by the
        // interrupt handler.
        //
        xQueueReceive(g_QueSerial, (void*) &sEvent, portMAX_DELAY);

        //
        // The first part of the queue message is the type of event.  Check if
        // it's an RX or receive timeout interrupt.
        //
        if(sEvent.eEventType == RX)
        {
            //
            // If Telnet protocol is enabled, check for incoming IAC character,
            // and escape it.
            //
            if((g_sParameters.sPort[sEvent.ui8Port].ui8Flags &
                PORT_FLAG_PROTOCOL) == PORT_PROTOCOL_TELNET)
            {
                //
                // If this is a Telnet IAC character, write it twice.
                //
                if((sEvent.ui8Char == TELNET_IAC) &&
                   (RingBufFree(&g_sRxBuf[sEvent.ui8Port]) >= 2))
                {
                    RingBufWriteOne(&g_sRxBuf[sEvent.ui8Port], sEvent.ui8Char);
                    RingBufWriteOne(&g_sRxBuf[sEvent.ui8Port], sEvent.ui8Char);
                }

                //
                // If not a Telnet IAC character, only write it once.
                //
                else if((sEvent.ui8Char != TELNET_IAC) &&
                        (RingBufFree(&g_sRxBuf[sEvent.ui8Port]) >= 1))
                {
                    RingBufWriteOne(&g_sRxBuf[sEvent.ui8Port], sEvent.ui8Char);
                }
            }

            //
            // If not Telnet, then only write the data once.
            //
            else
            {
                RingBufWriteOne(&g_sRxBuf[sEvent.ui8Port], sEvent.ui8Char);
            }
        }

        //
        // Check if it's a TX interrupt, indicating that there is space
        // available in TX FIFO.
        //
        else
        {
            //
            // Loop while there is space in the transmit FIFO and characters to
            // be sent.
            //
            while(!RingBufEmpty(&g_sTxBuf[sEvent.ui8Port]) &&
                  UARTSpaceAvail(g_ui32UARTBase[sEvent.ui8Port]))
            {
                //
                // Write the next character into the transmit FIFO.
                //
                UARTCharPut(g_ui32UARTBase[sEvent.ui8Port],
                            RingBufReadOne(&g_sTxBuf[sEvent.ui8Port]));
            }
        }
    }
}

//*****************************************************************************
//
// Initialize the serial peripherals and the serial task.
//
//*****************************************************************************
uint32_t
SerialTaskInit(void)
{
    //
    // Initialize the serial port module.
    //
    SerialInit();

    //
    // Set the interrupt priorities.  As the UART interrupt handlers use
    // FreeRTOS APIs, the interrupt priorities of the UARTs should be between
    // configKERNEL_INTERRUPT_PRIORITY and
    // configMAX_SYSCALL_INTERRUPT_PRIORITY.
    //
    MAP_IntPrioritySet(INT_UART3, UART3_INT_PRIORITY);
    MAP_IntPrioritySet(INT_UART4, UART4_INT_PRIORITY);

    //
    // Create a queue to pass information from the UART interrupt handlers to
    // the serial task.  This queue is also used to signal the serial task that
    // an event has occurred.  A queue is used, instead of a Semaphore, as data
    // has to be sent from the interrupt handler to the task and also because
    // the frequency of the UART interrupts could be quite high.  The queue
    // size can be adjusted based on the UART baud rate.
    //
    g_QueSerial = xQueueCreate(64, sizeof(tSerialEvent));
    if(g_QueSerial == 0)
    {
        return(1);
    }

    //
    // Create the Serial task.
    //
    if(xTaskCreate(SerialTask, (const portCHAR *)"Serial",
                   STACKSIZE_SerialTASK, NULL, tskIDLE_PRIORITY +
                   PRIORITY_SERIAL_TASK, NULL) != pdTRUE)
    {
        return(1);
    }

    //
    // Success.
    //
    return(0);
}
