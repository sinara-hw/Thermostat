//*****************************************************************************
//
// serial.c - Serial port driver for S2E Module.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/ringbuf.h"
#include "config.h"
#include "serial.h"

//*****************************************************************************
//
// External References.
//
//*****************************************************************************
extern uint32_t g_ui32SysClock;

//*****************************************************************************
//
//! \addtogroup serial_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! The buffer used to hold characters received from the serial Port0.
//
//*****************************************************************************
static uint8_t g_pui8RX0Buffer[RX_RING_BUF_SIZE];

//*****************************************************************************
//
//! The buffer used to hold characters to be sent to the serial Port0.
//
//*****************************************************************************
static uint8_t g_pui8TX0Buffer[TX_RING_BUF_SIZE];

//*****************************************************************************
//
//! The buffer used to hold characters received from the serial Port1.
//
//*****************************************************************************
static uint8_t g_pui8RX1Buffer[RX_RING_BUF_SIZE];

//*****************************************************************************
//
//! The buffer used to hold characters to be sent to the serial Port1.
//
//*****************************************************************************
static uint8_t g_pui8TX1Buffer[TX_RING_BUF_SIZE];

//*****************************************************************************
//
//! The ring buffers used to hold characters received from the serial ports.
//
//*****************************************************************************
tRingBufObject g_sRxBuf[MAX_S2E_PORTS];

//*****************************************************************************
//
//! The ring buffers used to hold characters to be sent to the serial ports.
//
//*****************************************************************************
tRingBufObject g_sTxBuf[MAX_S2E_PORTS];

//*****************************************************************************
//
//! The base address for the UART associated with a port.
//
//*****************************************************************************
const uint32_t g_ui32UARTBase[MAX_S2E_PORTS] =
{
    S2E_PORT0_UART_PORT,
    S2E_PORT1_UART_PORT
};

//*****************************************************************************
//
//! The interrupt for the UART associated with a port.
//
//*****************************************************************************
static const uint32_t g_ui32UARTInterrupt[MAX_S2E_PORTS] =
{
    S2E_PORT0_UART_INT,
    S2E_PORT1_UART_INT
};

//*****************************************************************************
//
//! The current baud rate setting of the serial port
//
//*****************************************************************************
static uint32_t g_ui32CurrentBaudRate[MAX_S2E_PORTS] =
{
    0,
    0
};

//*****************************************************************************
//
//! Enables transmitting and receiving.
//!
//! \param ui32Port is the UART port number to be accessed.
//!
//! Sets the UARTEN, and RXE bits, and enables the transmit and receive
//! FIFOs.
//!
//! \return None.
//
//*****************************************************************************
static void
SerialUARTEnable(uint32_t ui32Port)
{
    //
    // Enable the FIFO.
    //
    HWREG(g_ui32UARTBase[ui32Port] + UART_O_LCRH) |= UART_LCRH_FEN;

    //
    // Enable RX, TX, and the UART.
    //
    HWREG(g_ui32UARTBase[ui32Port] + UART_O_CTL) |= (UART_CTL_UARTEN |
                                                     UART_CTL_RXE |
                                                     UART_CTL_TXE);
}

//*****************************************************************************
//
//! Checks the availability of the serial port output buffer.
//!
//! \param ui32Port is the UART port number to be accessed.
//!
//! This function checks to see if there is room on the UART transmit buffer
//! for additional data.
//!
//! \return Returns \b true if the transmit buffer is full, \b false
//! otherwise.
//
//*****************************************************************************
bool
SerialSendFull(uint32_t ui32Port)
{
    //
    // Check the arguments.
    //
    ASSERT(ui32Port < MAX_S2E_PORTS);

    //
    // Return the number of bytes available in the tx ring buffer.
    //
    return(RingBufFull(&g_sTxBuf[ui32Port]));
}

//*****************************************************************************
//
//! Sends a character to the UART.
//!
//! \param ui32Port is the UART port number to be accessed.
//! \param ui8Char is the character to be sent.
//!
//! This function sends a character to the UART.  The character will either be
//! directly written into the UART FIFO or into the UART transmit buffer, as
//! appropriate.
//!
//! \return None.
//
//*****************************************************************************
void
SerialSend(uint32_t ui32Port, uint8_t ui8Char)
{
    //
    // Check the arguments.
    //
    ASSERT(ui32Port < MAX_S2E_PORTS);

    //
    // Disable the UART transmit interrupt while determining how to handle this
    // character.  Failure to do so could result in the loss of this character,
    // or stalled output due to this character being placed into the UART
    // transmit buffer but never transferred out into the UART FIFO.
    //
    UARTIntDisable(g_ui32UARTBase[ui32Port], UART_INT_TX);

    //
    // See if the transmit buffer is empty and there is space in the FIFO.
    //
    if(RingBufEmpty(&g_sTxBuf[ui32Port]) &&
       (UARTSpaceAvail(g_ui32UARTBase[ui32Port])))
    {
        //
        // Write this character directly into the FIFO.
        //
        UARTCharPut(g_ui32UARTBase[ui32Port], ui8Char);
    }

    //
    // See if there is room in the transmit buffer.
    //
    else if(!RingBufFull(&g_sTxBuf[ui32Port]))
    {
        //
        // Put this character into the transmit buffer.
        //
        RingBufWriteOne(&g_sTxBuf[ui32Port], ui8Char);
    }

    //
    // Enable the UART transmit interrupt.
    //
    UARTIntEnable(g_ui32UARTBase[ui32Port], UART_INT_TX);
}

//*****************************************************************************
//
//! Receives a character from the UART.
//!
//! \param ui32Port is the UART port number to be accessed.
//!
//! This function receives a character from the relevant port's UART buffer.
//!
//! \return Returns -1 if no data is available or the oldest character held in
//! the receive ring buffer.
//
//*****************************************************************************
int32_t
SerialReceive(uint32_t ui32Port)
{
    uint32_t ui32Data;

    //
    // Check the arguments.
    //
    ASSERT(ui32Port < MAX_S2E_PORTS);

    //
    // See if the receive buffer is empty and there is space in the FIFO.
    //
    if(RingBufEmpty(&g_sRxBuf[ui32Port]))
    {
        //
        // Return -1 (EOF) to indicate no data available.
        //
        return(-1);
    }

    //
    // Read a single character.
    //
    ui32Data = (long)RingBufReadOne(&g_sRxBuf[ui32Port]);

    //
    // Return the data that was read.
    //
    return(ui32Data);
}

//*****************************************************************************
//
//! Returns number of characters available in the serial ring buffer.
//!
//! \param ui32Port is the UART port number to be accessed.
//!
//! This function will return the number of characters available in the
//! serial ring buffer.
//!
//! \return The number of characters available in the ring buffer..
//
//*****************************************************************************
uint32_t
SerialReceiveAvailable(uint32_t ui32Port)
{
    //
    // Check the arguments.
    //
    ASSERT(ui32Port < MAX_S2E_PORTS);

    //
    // Return the value.
    //
    return(RingBufUsed(&g_sRxBuf[ui32Port]));
}

//*****************************************************************************
//
//! Configures the serial port baud rate.
//!
//! \param ui32Port is the serial port number to be accessed.
//! \param ui32BaudRate is the new baud rate for the serial port.
//!
//! This function configures the serial port's baud rate.  The current
//! configuration for the serial port will be read.  The baud rate will be
//! modified, and the port will be reconfigured.
//!
//! \return None.
//
//*****************************************************************************
void
SerialSetBaudRate(uint32_t ui32Port, uint32_t ui32BaudRate)
{
    uint32_t ui32Div;

    //
    // Check the arguments.
    //
    ASSERT(ui32Port < MAX_S2E_PORTS);
    ASSERT(ui32BaudRate != 0);

    //
    // Save the baud rate for future reference.
    //
    g_ui32CurrentBaudRate[ui32Port] = ui32BaudRate;

    //
    // Get and check the clock use by the UART.
    //
    ASSERT(g_ui32SysClock >= (ui32BaudRate * 16));

    //
    // Stop the UART.
    //
    UARTDisable(g_ui32UARTBase[ui32Port]);

    //
    // Compute the fractional baud rate divider.
    //
    ui32Div = (((g_ui32SysClock * 8) / ui32BaudRate) + 1) / 2;

    //
    // Set the baud rate.
    //
    HWREG(g_ui32UARTBase[ui32Port] + UART_O_IBRD) = ui32Div / 64;
    HWREG(g_ui32UARTBase[ui32Port] + UART_O_FBRD) = ui32Div % 64;

    //
    // Clear the flags register.
    //
    HWREG(g_ui32UARTBase[ui32Port] + UART_O_FR) = 0;

    //
    // Start the UART.
    //
    SerialUARTEnable(ui32Port);
}

//*****************************************************************************
//
//! Queries the serial port baud rate.
//!
//! \param ui32Port is the serial port number to be accessed.
//!
//! This function will read the UART configuration and return the currently
//! configured baud rate for the selected port.
//!
//! \return The current baud rate of the serial port.
//
//*****************************************************************************
uint32_t
SerialGetBaudRate(uint32_t ui32Port)
{
    uint32_t ui32CurrentBaudRate, ui32CurrentConfig, ui32Dif, ui32Temp;

    //
    // Check the arguments.
    //
    ASSERT(ui32Port < MAX_S2E_PORTS);

    //
    // Get the current configuration of the UART.
    //
    UARTConfigGetExpClk(g_ui32UARTBase[ui32Port], g_ui32SysClock,
                        &ui32CurrentBaudRate, &ui32CurrentConfig);

    //
    // Calculate the difference between the reported baud rate and the
    // stored nominal baud rate.
    //
    if(ui32CurrentBaudRate > g_ui32CurrentBaudRate[ui32Port])
    {
        ui32Dif = ui32CurrentBaudRate - g_ui32CurrentBaudRate[ui32Port];
    }
    else
    {
        ui32Dif = g_ui32CurrentBaudRate[ui32Port] - ui32CurrentBaudRate;
    }

    //
    // Calculate the 1% value of nominal baud rate.
    //
    ui32Temp = g_ui32CurrentBaudRate[ui32Port] / 100;

    //
    // If the difference between calculated and nominal is > 1%, report the
    // calculated rate.  Otherwise, report the nominal rate.
    //
    if(ui32Dif > ui32Temp)
    {
        return(ui32CurrentBaudRate);
    }

    //
    // Return the current serial port baud rate.
    //
    return(g_ui32CurrentBaudRate[ui32Port]);
}

//*****************************************************************************
//
//! Configures the serial port data size.
//!
//! \param ui32Port is the serial port number to be accessed.
//! \param ui8DataSize is the new data size for the serial port.
//!
//! This function configures the serial port's data size.  The current
//! configuration for the serial port will be read.  The data size will be
//! modified, and the port will be reconfigured.
//!
//! \return None.
//
//*****************************************************************************
void
SerialSetDataSize(uint32_t ui32Port, uint8_t ui8DataSize)
{
    uint32_t ui32CurrentBaudRate, ui32CurrentConfig, ui32NewConfig;

    //
    // Check the arguments.
    //
    ASSERT(ui32Port < MAX_S2E_PORTS);
    ASSERT((ui8DataSize >= 5) && (ui8DataSize <= 8));

    //
    // Stop the UART.
    //
    UARTDisable(g_ui32UARTBase[ui32Port]);

    //
    // Get the current configuration of the UART.
    //
    UARTConfigGetExpClk(g_ui32UARTBase[ui32Port], g_ui32SysClock,
                        &ui32CurrentBaudRate, &ui32CurrentConfig);

    //
    // Update the configuration with a new data length.
    //
    switch(ui8DataSize)
    {
        case 5:
        {
            ui32NewConfig = (ui32CurrentConfig & ~UART_CONFIG_WLEN_MASK);
            ui32NewConfig |= UART_CONFIG_WLEN_5;
            g_sParameters.sPort[ui32Port].ui8DataSize = ui8DataSize;
            break;
        }

        case 6:
        {
            ui32NewConfig = (ui32CurrentConfig & ~UART_CONFIG_WLEN_MASK);
            ui32NewConfig |= UART_CONFIG_WLEN_6;
            g_sParameters.sPort[ui32Port].ui8DataSize = ui8DataSize;
            break;
        }

        case 7:
        {
            ui32NewConfig = (ui32CurrentConfig & ~UART_CONFIG_WLEN_MASK);
            ui32NewConfig |= UART_CONFIG_WLEN_7;
            g_sParameters.sPort[ui32Port].ui8DataSize = ui8DataSize;
            break;
        }

        case 8:
        {
            ui32NewConfig = (ui32CurrentConfig & ~UART_CONFIG_WLEN_MASK);
            ui32NewConfig |= UART_CONFIG_WLEN_8;
            g_sParameters.sPort[ui32Port].ui8DataSize = ui8DataSize;
            break;
        }

        default:
        {
            ui32NewConfig = ui32CurrentConfig;
            break;
        }
    }

    //
    // Set parity, data length, and number of stop bits.
    //
    HWREG(g_ui32UARTBase[ui32Port] + UART_O_LCRH) = ui32NewConfig;

    //
    // Clear the flags register.
    //
    HWREG(g_ui32UARTBase[ui32Port] + UART_O_FR) = 0;

    //
    // Start the UART.
    //
    SerialUARTEnable(ui32Port);
}

//*****************************************************************************
//
//! Queries the serial port data size.
//!
//! \param ui32Port is the serial port number to be accessed.
//!
//! This function will read the UART configuration and return the currently
//! configured data size for the selected port.
//!
//! \return None.
//
//*****************************************************************************
uint8_t
SerialGetDataSize(uint32_t ui32Port)
{
    uint32_t ui32CurrentBaudRate, ui32CurrentConfig;
    uint8_t ui8CurrentDataSize;

    //
    // Check the arguments.
    //
    ASSERT(ui32Port < MAX_S2E_PORTS);

    //
    // Get the current configuration of the UART.
    //
    UARTConfigGetExpClk(g_ui32UARTBase[ui32Port], g_ui32SysClock,
                        &ui32CurrentBaudRate, &ui32CurrentConfig);

    //
    // Determine the current data size.
    //
    switch(ui32CurrentConfig & UART_CONFIG_WLEN_MASK)
    {
        case UART_CONFIG_WLEN_5:
        {
            ui8CurrentDataSize = 5;
            break;
        }

        case UART_CONFIG_WLEN_6:
        {
            ui8CurrentDataSize = 6;
            break;
        }

        case UART_CONFIG_WLEN_7:
        {
            ui8CurrentDataSize = 7;
            break;
        }

        case UART_CONFIG_WLEN_8:
        {
            ui8CurrentDataSize = 8;
            break;
        }

        default:
        {
            ui8CurrentDataSize = 0;
            break;
        }
    }

    //
    // Return the current data size.
    //
    return(ui8CurrentDataSize);
}

//*****************************************************************************
//
//! Configures the serial port parity.
//!
//! \param ui32Port is the serial port number to be accessed.
//! \param ui8Parity is the new parity for the serial port.
//!
//! This function configures the serial port's parity.  The current
//! configuration for the serial port will be read.  The parity will be
//! modified, and the port will be reconfigured.
//!
//! \return None.
//
//*****************************************************************************
void
SerialSetParity(uint32_t ui32Port, uint8_t ui8Parity)
{
    uint32_t ui32CurrentBaudRate, ui32CurrentConfig, ui32NewConfig;

    //
    // Check the arguments.
    //
    ASSERT(ui32Port < MAX_S2E_PORTS);
    ASSERT((ui8Parity == SERIAL_PARITY_NONE) ||
           (ui8Parity == SERIAL_PARITY_ODD) ||
           (ui8Parity == SERIAL_PARITY_EVEN) ||
           (ui8Parity == SERIAL_PARITY_MARK) ||
           (ui8Parity == SERIAL_PARITY_SPACE));

    //
    // Stop the UART.
    //
    UARTDisable(g_ui32UARTBase[ui32Port]);

    //
    // Get the current configuration of the UART.
    //
    UARTConfigGetExpClk(g_ui32UARTBase[ui32Port], g_ui32SysClock,
                        &ui32CurrentBaudRate, &ui32CurrentConfig);

    //
    // Update the configuration with a new parity.
    //
    switch(ui8Parity)
    {
        case SERIAL_PARITY_NONE:
        {
            ui32NewConfig = (ui32CurrentConfig & ~UART_CONFIG_PAR_MASK);
            ui32NewConfig |= UART_CONFIG_PAR_NONE;
            g_sParameters.sPort[ui32Port].ui8Parity = ui8Parity;
            break;
        }

        case SERIAL_PARITY_ODD:
        {
            ui32NewConfig = (ui32CurrentConfig & ~UART_CONFIG_PAR_MASK);
            ui32NewConfig |= UART_CONFIG_PAR_ODD;
            g_sParameters.sPort[ui32Port].ui8Parity = ui8Parity;
            break;
        }

        case SERIAL_PARITY_EVEN:
        {
            ui32NewConfig = (ui32CurrentConfig & ~UART_CONFIG_PAR_MASK);
            ui32NewConfig |= UART_CONFIG_PAR_EVEN;
            g_sParameters.sPort[ui32Port].ui8Parity = ui8Parity;
            break;
        }

        case SERIAL_PARITY_MARK:
        {
            ui32NewConfig = (ui32CurrentConfig & ~UART_CONFIG_PAR_MASK);
            ui32NewConfig |= UART_CONFIG_PAR_ONE;
            g_sParameters.sPort[ui32Port].ui8Parity = ui8Parity;
            break;
        }

        case SERIAL_PARITY_SPACE:
        {
            ui32NewConfig = (ui32CurrentConfig & ~UART_CONFIG_PAR_MASK);
            ui32NewConfig |= UART_CONFIG_PAR_ZERO;
            g_sParameters.sPort[ui32Port].ui8Parity = ui8Parity;
            break;
        }

        default:
        {
            ui32NewConfig = ui32CurrentConfig;
            break;
        }
    }

    //
    // Set parity, data length, and number of stop bits.
    //
    HWREG(g_ui32UARTBase[ui32Port] + UART_O_LCRH) = ui32NewConfig;

    //
    // Clear the flags register.
    //
    HWREG(g_ui32UARTBase[ui32Port] + UART_O_FR) = 0;

    //
    // Start the UART.
    //
    SerialUARTEnable(ui32Port);
}

//*****************************************************************************
//
//! Queries the serial port parity.
//!
//! \param ui32Port is the serial port number to be accessed.
//!
//! This function will read the UART configuration and return the currently
//! configured parity for the selected port.
//!
//! \return Returns the current parity setting for the port.  This will be one
//! of \b SERIAL_PARITY_NONE, \b SERIAL_PARITY_ODD, \b SERIAL_PARITY_EVEN,
//! \b SERIAL_PARITY_MARK, or \b SERIAL_PARITY_SPACE.
//
//*****************************************************************************
uint8_t
SerialGetParity(uint32_t ui32Port)
{
    uint32_t ui32CurrentBaudRate, ui32CurrentConfig;
    uint8_t ui8CurrentParity;

    //
    // Check the arguments.
    //
    ASSERT(ui32Port < MAX_S2E_PORTS);

    //
    // Get the current configuration of the UART.
    //
    UARTConfigGetExpClk(g_ui32UARTBase[ui32Port], g_ui32SysClock,
                        &ui32CurrentBaudRate, &ui32CurrentConfig);

    //
    // Determine the current data size.
    //
    switch(ui32CurrentConfig & UART_CONFIG_PAR_MASK)
    {
        case UART_CONFIG_PAR_NONE:
        {
            ui8CurrentParity = SERIAL_PARITY_NONE;
            break;
        }

        case UART_CONFIG_PAR_ODD:
        {
            ui8CurrentParity = SERIAL_PARITY_ODD;
            break;
        }

        case UART_CONFIG_PAR_EVEN:
        {
            ui8CurrentParity = SERIAL_PARITY_EVEN;
            break;
        }

        case UART_CONFIG_PAR_ONE:
        {
            ui8CurrentParity = SERIAL_PARITY_MARK;
            break;
        }

        case UART_CONFIG_PAR_ZERO:
        {
            ui8CurrentParity = SERIAL_PARITY_SPACE;
            break;
        }

        default:
        {
            ui8CurrentParity = SERIAL_PARITY_NONE;
            break;
        }
    }

    //
    // Return the current data size.
    //
    return(ui8CurrentParity);
}

//*****************************************************************************
//
//! Configures the serial port stop bits.
//!
//! \param ui32Port is the serial port number to be accessed.
//! \param ui8StopBits is the new stop bits for the serial port.
//!
//! This function configures the serial port's stop bits.  The current
//! configuration for the serial port will be read.  The stop bits will be
//! modified, and the port will be reconfigured.
//!
//! \return None.
//
//*****************************************************************************
void
SerialSetStopBits(uint32_t ui32Port, uint8_t ui8StopBits)
{
    uint32_t ui32CurrentBaudRate, ui32CurrentConfig, ui32NewConfig;

    //
    // Check the arguments.
    //
    ASSERT(ui32Port < MAX_S2E_PORTS);
    ASSERT((ui8StopBits >= 1) && (ui8StopBits <= 2));

    //
    // Stop the UART.
    //
    UARTDisable(g_ui32UARTBase[ui32Port]);

    //
    // Get the current configuration of the UART.
    //
    UARTConfigGetExpClk(g_ui32UARTBase[ui32Port], g_ui32SysClock,
                        &ui32CurrentBaudRate, &ui32CurrentConfig);

    //
    // Update the configuration with a new stop bits.
    //
    switch(ui8StopBits)
    {
        case 1:
        {
            ui32NewConfig = (ui32CurrentConfig & ~UART_CONFIG_STOP_MASK);
            ui32NewConfig |= UART_CONFIG_STOP_ONE;
            g_sParameters.sPort[ui32Port].ui8StopBits = ui8StopBits;
            break;
        }

        case 2:
        {
            ui32NewConfig = (ui32CurrentConfig & ~UART_CONFIG_STOP_MASK);
            ui32NewConfig |= UART_CONFIG_STOP_TWO;
            g_sParameters.sPort[ui32Port].ui8StopBits = ui8StopBits;
            break;
        }

        default:
        {
            ui32NewConfig = ui32CurrentConfig;
            break;
        }
    }

    //
    // Set parity, data length, and number of stop bits.
    //
    HWREG(g_ui32UARTBase[ui32Port] + UART_O_LCRH) = ui32NewConfig;

    //
    // Clear the flags register.
    //
    HWREG(g_ui32UARTBase[ui32Port] + UART_O_FR) = 0;

    //
    // Start the UART.
    //
    SerialUARTEnable(ui32Port);
}

//*****************************************************************************
//
//! Queries the serial port stop bits.
//!
//! \param ui32Port is the serial port number to be accessed.
//!
//! This function will read the UART configuration and return the currently
//! configured stop bits for the selected port.
//!
//! \return None.
//
//*****************************************************************************
uint8_t
SerialGetStopBits(uint32_t ui32Port)
{
    uint32_t ui32CurrentBaudRate, ui32CurrentConfig;
    uint8_t ui8CurrentStopBits;

    //
    // Check the arguments.
    //
    ASSERT(ui32Port < MAX_S2E_PORTS);

    //
    // Get the current configuration of the UART.
    //
    UARTConfigGetExpClk(g_ui32UARTBase[ui32Port], g_ui32SysClock,
                        &ui32CurrentBaudRate, &ui32CurrentConfig);

    //
    // Determine the current data size.
    //
    switch(ui32CurrentConfig & UART_CONFIG_STOP_MASK)
    {
        case UART_CONFIG_STOP_ONE:
        {
            ui8CurrentStopBits = 1;
            break;
        }

        case UART_CONFIG_STOP_TWO:
        {
            ui8CurrentStopBits = 2;
            break;
        }

        default:
        {
            ui8CurrentStopBits = 0;
            break;
        }
    }

    //
    // Return the current data size.
    //
    return(ui8CurrentStopBits);
}

//*****************************************************************************
//
//! Configures the serial port flow control option.
//!
//! \param ui32Port is the UART port number to be accessed.
//! \param ui8FlowControl is the new flow control setting for the serial port.
//!
//! This function configures the serial port's flow control.  This function
//! will enable/disable the hardware flow control setting in the UART based on
//! the value of the flow control setting.
//!
//! \return None.
//
//*****************************************************************************
void
SerialSetFlowControl(uint32_t ui32Port, uint8_t ui8FlowControl)
{
    //
    // Check the arguments.
    //
    ASSERT(ui32Port < MAX_S2E_PORTS);
    ASSERT((ui8FlowControl == SERIAL_FLOW_CONTROL_NONE) ||
           (ui8FlowControl == SERIAL_FLOW_CONTROL_HW));

    //
    // Save the new flow control setting.
    //
    g_sParameters.sPort[ui32Port].ui8FlowControl = ui8FlowControl;

    //
    // Enable flow control in the UART.
    //
    if(g_sParameters.sPort[ui32Port].ui8FlowControl == SERIAL_FLOW_CONTROL_HW)
    {
        HWREG(g_ui32UARTBase[ui32Port] + UART_O_CTL) |=
            (UART_FLOWCONTROL_TX | UART_FLOWCONTROL_RX);

        //
        // Debug. Remove later.
        //
        if(ui32Port == 0)
        {
            GPIOPadConfigSet(GPIO_PORTN_BASE, GPIO_PIN_4,
                             GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
        }
        else
        {
            GPIOPadConfigSet(GPIO_PORTK_BASE, GPIO_PIN_2,
                             GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
        }
    }

    //
    // Disable flow control in the UART.
    //
    else
    {
        HWREG(g_ui32UARTBase[ui32Port] + UART_O_CTL) &=
            ~(UART_FLOWCONTROL_TX | UART_FLOWCONTROL_RX);

        //
        // Debug. Remove later.
        //
        if(ui32Port == 0)
        {
            GPIOPadConfigSet(GPIO_PORTN_BASE, GPIO_PIN_4,
                             GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);
        }
        else
        {
            GPIOPadConfigSet(GPIO_PORTK_BASE, GPIO_PIN_2,
                             GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);
        }
    }
}

//*****************************************************************************
//
//! Queries the serial port flow control.
//!
//! \param ui32Port is the serial port number to be accessed.
//!
//! This function will return the currently configured flow control for
//! the selected port.
//!
//! \return None.
//
//*****************************************************************************
uint8_t
SerialGetFlowControl(uint32_t ui32Port)
{
    //
    // Check the arguments.
    //
    ASSERT(ui32Port < MAX_S2E_PORTS);

    //
    // Return the current flow control.
    //
    return(g_sParameters.sPort[ui32Port].ui8FlowControl);
}

//*****************************************************************************
//
//! Purges the serial port data queue(s).
//!
//! \param ui32Port is the serial port number to be accessed.
//! \param ui8PurgeCommand is the command indicating which queue's to purge.
//!
//! This function will purge data from the tx, rx, or both serial port queues.
//!
//! \return None.
//
//*****************************************************************************
void
SerialPurgeData(uint32_t ui32Port, uint8_t ui8PurgeCommand)
{
    //
    // Check the arguments.
    //
    ASSERT(ui32Port < MAX_S2E_PORTS);
    ASSERT((ui8PurgeCommand >= 1) && (ui8PurgeCommand <= 3));

    //
    // Disable the UART.
    //
    UARTDisable(g_ui32UARTBase[ui32Port]);

    //
    // Purge the receive data if requested.
    //
    if(ui8PurgeCommand & 0x01)
    {
        RingBufFlush(&g_sRxBuf[ui32Port]);
    }

    //
    // Purge the transmit data if requested.
    //
    if(ui8PurgeCommand & 0x02)
    {
        RingBufFlush(&g_sTxBuf[ui32Port]);
    }

    //
    // Re-enable the UART.
    //
    SerialUARTEnable(ui32Port);
}

//*****************************************************************************
//
//! \internal
//!
//! Configures the serial port to a default setup.
//!
//! \param ui32Port is the UART port number to be accessed.
//!
//! This function resets the serial port to a default configuration.
//!
//! \return None.
//
//*****************************************************************************
static void
_SerialSetConfig(uint32_t ui32Port, const tPortParameters *psPort)
{
    //
    // Disable interrupts.
    //
    IntDisable(g_ui32UARTInterrupt[ui32Port]);

    //
    // Set the baud rate.
    //
    SerialSetBaudRate(ui32Port, psPort->ui32BaudRate);

    //
    // Set the data size.
    //
    SerialSetDataSize(ui32Port, psPort->ui8DataSize);

    //
    // Set the parity.
    //
    SerialSetParity(ui32Port, psPort->ui8Parity);

    //
    // Set the stop bits.
    //
    SerialSetStopBits(ui32Port, psPort->ui8StopBits);

    //
    // Set the flow control.
    //
    SerialSetFlowControl(ui32Port, psPort->ui8FlowControl);

    //
    // Purge the Serial Tx/Rx Ring Buffers.
    //
    SerialPurgeData(ui32Port, 0x03);

    //
    // (Re)enable the UART transmit and receive interrupts.
    //
    UARTIntEnable(g_ui32UARTBase[ui32Port],
                  (UART_INT_RX | UART_INT_RT | UART_INT_TX));
    IntEnable(g_ui32UARTInterrupt[ui32Port]);
}

//*****************************************************************************
//
//! Configures the serial port to a default setup.
//!
//! \param ui32Port is the UART port number to be accessed.
//!
//! This function resets the serial port to a default configuration.
//!
//! \return None.
//
//*****************************************************************************
void
SerialSetDefault(uint32_t ui32Port)
{
    //
    // Check the arguments.
    //
    ASSERT(ui32Port < MAX_S2E_PORTS);

    //
    // Configure the serial port with default parameters.
    //
    _SerialSetConfig(ui32Port, &g_psDefaultParameters->sPort[ui32Port]);
}

//*****************************************************************************
//
//! Configures the serial port according to the current working parameter
//! values.
//!
//! \param ui32Port is the UART port number to be accessed.  Valid values are 0
//! and 1.
//!
//! This function configures the serial port according to the current working
//! parameters in g_sParameters.sPort for the specified port.  The actual
//! parameter set is then read back and g_sParameters.sPort updated to ensure
//! that the structure is correctly synchronized with the hardware.
//!
//! \return None.
//
//*****************************************************************************
void
SerialSetCurrent(uint32_t ui32Port)
{
    //
    // Check the arguments.
    //
    ASSERT(ui32Port < MAX_S2E_PORTS);

    //
    // Configure the serial port with current parameters.
    //
    _SerialSetConfig(ui32Port, &g_sParameters.sPort[ui32Port]);

    //
    // Get the current settings.
    //
    g_sParameters.sPort[ui32Port].ui32BaudRate = SerialGetBaudRate(ui32Port);
    g_sParameters.sPort[ui32Port].ui8DataSize = SerialGetDataSize(ui32Port);
    g_sParameters.sPort[ui32Port].ui8Parity = SerialGetParity(ui32Port);
    g_sParameters.sPort[ui32Port].ui8StopBits = SerialGetStopBits(ui32Port);
    g_sParameters.sPort[ui32Port].ui8FlowControl =
        SerialGetFlowControl(ui32Port);
}

//*****************************************************************************
//
//! Configures the serial port to the factory default setup.
//!
//! \param ui32Port is the UART port number to be accessed.
//!
//! This function resets the serial port to a default configuration.
//!
//! \return None.
//
//*****************************************************************************
void
SerialSetFactory(uint32_t ui32Port)
{
    //
    // Check the arguments.
    //
    ASSERT(ui32Port < MAX_S2E_PORTS);

    //
    // Configure the serial port with current parameters.
    //
    _SerialSetConfig(ui32Port, &g_psFactoryParameters->sPort[ui32Port]);
}

//*****************************************************************************
//
//! Initializes the serial port driver.
//!
//! This function initializes and configures the serial port driver.
//!
//! \return None.
//
//*****************************************************************************
void
SerialInit(void)
{
    //
    // Initialize the ring buffers used by the UART Drivers.
    //
    RingBufInit(&g_sRxBuf[0], g_pui8RX0Buffer, sizeof(g_pui8RX0Buffer));
    RingBufInit(&g_sTxBuf[0], g_pui8TX0Buffer, sizeof(g_pui8TX0Buffer));
    RingBufInit(&g_sRxBuf[1], g_pui8RX1Buffer, sizeof(g_pui8RX1Buffer));
    RingBufInit(&g_sTxBuf[1], g_pui8TX1Buffer, sizeof(g_pui8TX1Buffer));

    //
    // Enable and Configure Serial Port 0.
    //
    SysCtlPeripheralEnable(S2E_PORT0_RX_PERIPH);
    SysCtlPeripheralEnable(S2E_PORT0_TX_PERIPH);
    SysCtlPeripheralEnable(S2E_PORT0_RTS_PERIPH);
    SysCtlPeripheralEnable(S2E_PORT0_CTS_PERIPH);
    SysCtlPeripheralEnable(S2E_PORT0_UART_PERIPH);
    GPIOPinConfigure(S2E_PORT0_RX_CONFIG);
    GPIOPinConfigure(S2E_PORT0_TX_CONFIG);
    GPIOPinConfigure(S2E_PORT0_RTS_CONFIG);
    GPIOPinConfigure(S2E_PORT0_CTS_CONFIG);
    GPIOPinTypeUART(S2E_PORT0_RX_PORT, S2E_PORT0_RX_PIN);
    GPIOPinTypeUART(S2E_PORT0_TX_PORT, S2E_PORT0_TX_PIN);
    GPIOPinTypeUART(S2E_PORT0_RTS_PORT, S2E_PORT0_RTS_PIN);
    GPIOPinTypeUART(S2E_PORT0_CTS_PORT, S2E_PORT0_CTS_PIN);

    //
    // Configure the Port 1 pins appropriately.
    //
    SysCtlPeripheralEnable(S2E_PORT1_RX_PERIPH);
    SysCtlPeripheralEnable(S2E_PORT1_TX_PERIPH);
    SysCtlPeripheralEnable(S2E_PORT1_RTS_PERIPH);
    SysCtlPeripheralEnable(S2E_PORT1_CTS_PERIPH);
    SysCtlPeripheralEnable(S2E_PORT1_UART_PERIPH);
    GPIOPinConfigure(S2E_PORT1_RX_CONFIG);
    GPIOPinConfigure(S2E_PORT1_TX_CONFIG);
    GPIOPinConfigure(S2E_PORT1_RTS_CONFIG);
    GPIOPinConfigure(S2E_PORT1_CTS_CONFIG);
    GPIOPinTypeUART(S2E_PORT1_RX_PORT, S2E_PORT1_RX_PIN);
    GPIOPinTypeUART(S2E_PORT1_TX_PORT, S2E_PORT1_TX_PIN);
    GPIOPinTypeUART(S2E_PORT1_RTS_PORT, S2E_PORT1_RTS_PIN);
    GPIOPinTypeUART(S2E_PORT1_CTS_PORT, S2E_PORT1_CTS_PIN);

    //
    // Configure Port 0.
    //
    SerialSetDefault(0);

    //
    // Configure Port 1.
    //
    SerialSetDefault(1);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
