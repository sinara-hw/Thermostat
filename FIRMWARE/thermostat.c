
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/flash.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/watchdog.h"
//#include "utils/swupdate.h"
#include "utils/ustdlib.h"
#include "utils/uartstdio.h"
#include "drivers/pinout.h"
#include "config.h"
#include "lwip_task.h"
#include "serial_task.h"
#include "serial.h"
#include "send_command.h"
#include "mqtt_client.h"
#include "pwm.h"
#include "spi.h"
#include "internal_adc.h"
#include "leds.h"
#include "pid_task.h"
#include "watchdog.h"
#include "uart_th.h"
#include "lwip/mem.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "lwip/apps/mqtt.h"


//*****************************************************************************
//
// The current IP address.
//
//*****************************************************************************
uint32_t g_ui32IPAddress;


//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
// This hook is called by FreeRTOS when an stack overflow error is detected.
//
//*****************************************************************************
void
vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName)
{
    //
    // This function can not return, so loop forever.  Interrupts are disabled
    // on entry to this function, so no processor interrupts will interrupt
    // this loop.
    //
    while(1)
    {
    }
}

//*****************************************************************************
//
// The main entry function which configures the clock and hardware peripherals
// needed by the S2E application before calling the FreeRTOS scheduler.
//
//*****************************************************************************
int
main(void)
{
    //
    // Run from the PLL at configCPU_CLOCK_HZ MHz.
    //
    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_480),
                                            configCPU_CLOCK_HZ);

    //
    // Configure the device pins based on the selected board.
    //
#ifndef DK_TM4C129X
    PinoutSet(true, false);
#else
    PinoutSet();
#endif

    //
    // Configure Debug UART.
    //

    UARTConfig(g_ui32SysClock);

    //
    // Initialize the configuration parameter module.
    //
    ConfigInit();

    char msg[32];
    uint32_t msg_len;


    //
    // Clear the terminal and print banner.
    //

    msg_len = usprintf( msg, "\n\rSinara Thermostat\n\r");
    UARTSend(msg, msg_len);

    //
    // Tell the user what we are doing now.
    //
    msg_len = usprintf(msg, "Waiting for IP.\n\r");
    UARTSend(msg, msg_len);


    //
    // Initialize the Ethernet peripheral and create the lwIP tasks.
    //

    if(lwIPTaskInit() != 0)
    {
        msg_len = usprintf(msg, "Failed to create lwIP tasks!\n\r");
        UARTSend(msg, msg_len);
        while(1)
        {
        }
    }

    //
    // Initialize the serial peripherals and create the Serial task.
    //

    if(SerialTaskInit() != 0)
    {
        msg_len = usnprintf(msg, 32, "Failed to create Serial task!\n");
        UARTSend(msg, msg_len);
        while(1)
        {
        }
    }

    //
    // Initialize the PID task.
    //
    if(PIDTaskInit() != 0)
    {
        msg_len = usprintf(msg, "Failed to create PID task!\n\r");
        UARTSend(msg, msg_len);
        //UARTprintf("Failed to create PID task!\n");
        while(1)
        {
        }
    }



    internal_adc_config();
    pwm_config();
    spi_config(g_ui32SysClock);
    leds_config();

    //
    // Initialize watchdog.
    //

    Watchdog_config();

    //
    // Start the scheduler.  This should not return.
    //
    vTaskStartScheduler();

    //
    // In case the scheduler returns for some reason, print an error and loop
    // forever.
    //
    msg_len = usprintf(msg, "\nScheduler returned unexpectedly!\n\r");
    UARTSend(msg, msg_len);
    //UARTprintf("\nScheduler returned unexpectedly!\n");
    while(1)
    {
    }
}
