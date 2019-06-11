/*
 * pid_task.c
 *
 *
 *
 */

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
#include "spi.h"
#include "pwm.h"
#include "pid_task.h"
#include "leds.h"
#include "watchdog.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"


#define STACKSIZE_PIDTASK    512
#define UPDATE_TIME 75 // time in ms beetween updates of temperature / peltier current

int32_t pid_t0, pid_t1;
int32_t pid_dt0, pid_dt1;               // ADC updates at 10Hz
int32_t pid_integrallimit0, pid_integrallimit1;
int32_t pid_integral0, pid_integral1;
int32_t pid_error0, pid_error1;

//*****************************************************************************
//
// Initialize the PID task.
//
//*****************************************************************************
uint32_t
PIDTaskInit(void)
{

    pid_t0 = xTaskGetTickCount() * portTICK_PERIOD_MS; //ms
    pid_dt0 = 100; //ms
    pid_integrallimit0 = 15000; // mK * ms
    pid_integral0 = 0; // mK * ms
    pid_error0 = 0; // mK

    pid_t1 = xTaskGetTickCount() * portTICK_PERIOD_MS; //ms
    pid_dt1 = 100; //ms
    pid_integrallimit1 = 15000; // mK * ms
    pid_integral1 = 0; // mK * ms
    pid_error1 = 0; // mK

    //
    // Create PID task.
    //
    if(xTaskCreate(PIDTask, (const portCHAR *)"PID", STACKSIZE_PIDTASK, NULL, tskIDLE_PRIORITY + PRIORITY_PID_TASK, NULL) != pdTRUE)
    {
        return(1);
    }
    //
    // Success.
    //
    return(0);
}

//*****************************************************************************
//
//! The PID Task handles temperature establishing
//
//*****************************************************************************

static void
PIDTask(void *pvParameters)
{

    uint32_t RawTemp;
    uint16_t channel;
    channel = 2;
    float integrator_max;
    float result = 0.0;
    float err, derivative;

    //
    // Loop forever.
    //

    while(1) {

        WatchdogClear();


        RawTemp = spi_getvalue(&channel);

        if(channel == 1) { // 1, not 0 due to schematic mistake (?)
            ui32RawTemp0 = RawTemp;
            i32ActTemp0 = getTemp(RawTemp, g_sParameters.i32T00, g_sParameters.i32Beta0, g_sParameters.i32Ratio0, g_sParameters.ui16TempMode0, g_sParameters.i32ptA0, g_sParameters.i32ptB0);
            pid_dt0 = xTaskGetTickCount() * portTICK_PERIOD_MS - pid_t0;

            pid_t0 = xTaskGetTickCount() * portTICK_PERIOD_MS;
            err = (float) g_sParameters.i32SetTemp0 - (float) i32ActTemp0;
            derivative = (err - (float) pid_error0) / ((float) pid_dt0);
            pid_error0 = g_sParameters.i32SetTemp0 - i32ActTemp0;
            pid_integral0 = pid_integral0 + pid_error0 * pid_dt0;
            if(g_sParameters.i16cli0 != 0) {
                integrator_max = fabs(pid_integrallimit0*g_sParameters.i16cli0);
                if(pid_integral0 > (int32_t) integrator_max) {
                    pid_integral0 = (int32_t) integrator_max;
                }
                if(pid_integral0 < - (int32_t) (integrator_max)) {
                    pid_integral0 = - (int32_t) integrator_max;
                }
            }

            result = ((float) g_sParameters.i16clp0)*pid_error0/1000.0 + ((float) g_sParameters.i16cli0)*pid_integral0/1000000.0 + ((float) g_sParameters.i16cld0)*derivative;
            setCurr0((int32_t) result);

            if(pid_error0 < g_sParameters.ui16TempWindow0 && pid_error0 > -g_sParameters.ui16TempWindow0) {
                SetStat(0, 1);
            } else {
                SetStat(0, 0);
            }

        } else if(channel == 0) { // 0, not 1 due to schematic mistake (?)
            ui32RawTemp1 = RawTemp;
            i32ActTemp1 = getTemp(RawTemp, g_sParameters.i32T01, g_sParameters.i32Beta1, g_sParameters.i32Ratio1, g_sParameters.ui16TempMode1, g_sParameters.i32ptA1, g_sParameters.i32ptB1);
            pid_dt1 = xTaskGetTickCount() * portTICK_PERIOD_MS - pid_t1;

            pid_t1 = xTaskGetTickCount() * portTICK_PERIOD_MS;
            err = (float) g_sParameters.i32SetTemp1 - (float) i32ActTemp1;
            derivative = (err - (float) pid_error1) / ((float) pid_dt1);
            pid_error1 = g_sParameters.i32SetTemp1 - i32ActTemp1;
            pid_integral1 = pid_integral1 + pid_error1 * pid_dt1;
            if(g_sParameters.i16cli1 != 0) {
                integrator_max = fabs(pid_integrallimit1*g_sParameters.i16cli1);
                if(pid_integral1 > (int32_t) integrator_max) {
                    pid_integral1 = (int32_t) integrator_max;
                }
                if(pid_integral1 < - (int32_t) (integrator_max)) {
                    pid_integral1 = - (int32_t) integrator_max;
                }
            }

            result = ((float) g_sParameters.i16clp1)*pid_error1/1000.0 + ((float) g_sParameters.i16cli1)*pid_integral1/1000000.0 + ((float) g_sParameters.i16cld1)*derivative;

            setCurr1((int32_t) result);

            if(pid_error1 < g_sParameters.ui16TempWindow1 && pid_error1 > -g_sParameters.ui16TempWindow1) {
                SetStat(1, 1);
            } else {
                SetStat(1, 0);
            }
        }


        vTaskDelay(UPDATE_TIME/portTICK_PERIOD_MS);

    }

}
