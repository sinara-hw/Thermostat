//*****************************************************************************
//
// idle_task.c - idle task monitors and manages changes to IP address.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "utils/lwiplib.h"
#include "utils/ustdlib.h"
#include "utils/uartstdio.h"
#include "lwip/stats.h"
#include "config.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "mqtt_client.h"
#include "uart_th.h"

//*****************************************************************************
//
// The current IP address.
//
//*****************************************************************************
static uint32_t g_ui32IPAddress;

//*****************************************************************************
//
// Displays the IP address in a human-readable format.
//
//*****************************************************************************
static void
DisplayIP(uint32_t ui32IP)
{
    char pcBuf[16];

    char msg[32];
    uint32_t msg_len;

    //
    // Has IP address been acquired?
    //
    if(ui32IP == 0)
    {
        //
        // If there is no IP address, indicate that one is being acquired.
        //
        msg_len = usprintf(msg, "Acquiring IP Address...\n\r");
        UARTSend(msg, msg_len);

        return;
    }
    else if(ui32IP == 0xffffffff)
    {
        //
        // If link is inactive, indicate that.
        //
        msg_len = usprintf(msg, "Waiting for link.\n\r");
        UARTSend(msg, msg_len);

        return;
    }


    //
    // Display IP address on Debug UART.
    //
    msg_len = usprintf(msg, "IP Address: ");
    UARTSend(msg, msg_len);

    //
    // Convert the IP Address into a string.
    //
    msg_len = usprintf(pcBuf, "%d.%d.%d.%d \n\r", ui32IP & 0xff, (ui32IP >> 8) & 0xff,
             (ui32IP >> 16) & 0xff, (ui32IP >> 24) & 0xff);
    UARTSend(pcBuf, msg_len);

    msg_len = usprintf(msg, "\nOpen a browser and enter");
    UARTSend(msg, msg_len);
    msg_len = usprintf(msg, " the IP address to access");
    UARTSend(msg, msg_len);
    msg_len = usprintf(msg, " the web server.\n\r");
    UARTSend(msg, msg_len);


    client = mqtt_client_new();
    if(client != NULL) {
       do_connect(client);
    }

}

//*****************************************************************************
//
// This hook is called by the FreeRTOS idle task when no other tasks are
// runnable.
//
//*****************************************************************************

bool bFirst = true;

void
vApplicationIdleHook(void)
{
    uint32_t ui32Temp;
    portTickType ui32CurrentTick, ui32InitialTick;

    //
    // Get the current IP address.
    //
    ui32Temp = lwIPLocalIPAddrGet();

    //
    // See if the IP address has changed.
    //
    if(ui32Temp != g_ui32IPAddress)
    {
        //
        // Save the current IP address.
        //
        g_ui32IPAddress = ui32Temp;

        //
        // Update the display of the IP address.
        //
        DisplayIP(ui32Temp);
    }

    //
    // Check for an IP update request.
    //
    if(g_ui8UpdateRequired)
    {


        //
        // Check if "ui32InitialTick" is to be initialized.
        //
        if(bFirst)
        {
            //
            // Get the initial tick count.  This is used to calculate a two
            // second delay to allow the response page to get back to the
            // browser before the IP address is changed.
            //
            ui32InitialTick = xTaskGetTickCount();

            //
            // Reset 'bFirst' so that 'ui32InitialTick' is not re-initialized.
            //
            bFirst = false;
        }

        //
        // Get the current Tick count.
        //
        ui32CurrentTick = xTaskGetTickCount();


        //
        // Check if 2 seconds have lapsed.
        //
        if((ui32CurrentTick - ui32InitialTick) > 2000 / portTICK_RATE_MS)
        {


            //
            // Are we updating only the IP address?
            //
            if(g_ui8UpdateRequired & UPDATE_IP_ADDR)
            {
                //
                // Actually update the IP address.
                //

                g_ui8UpdateRequired &= ~UPDATE_IP_ADDR;
                ConfigUpdateIPAddress();
            }

            //
            // Are we updating all parameters (including the IP address?)
            //
            if(g_ui8UpdateRequired & UPDATE_ALL)
            {
                //
                // Update everything.
                //

                g_ui8UpdateRequired &= ~UPDATE_ALL;
                ConfigUpdateAllParameters(true);
            }

            //
            // Set 'bFirst' to initialize 'ui32InitialTick'.
            //
            bFirst = true;
        }
    }
}
