/*
 * leds.c
 *
 *
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_gpio.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/adc.h"
#include "pwm.h"
#include "config.h"
#include "leds.h"

void leds_config(void)
{

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);

    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOK))
    {
    }

    GPIOPinTypeGPIOOutput(GPIO_PORTK_BASE, GPIO_PIN_1); //freq 0 - not led
    GPIOPinTypeGPIOOutput(GPIO_PORTK_BASE, GPIO_PIN_4); //stat1
    GPIOPinTypeGPIOOutput(GPIO_PORTK_BASE, GPIO_PIN_5); //stat2

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);

    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOP))
    {
    }

    GPIOPinTypeGPIOOutput(GPIO_PORTP_BASE, GPIO_PIN_2); //shdn0 - not led
    GPIOPinTypeGPIOOutput(GPIO_PORTP_BASE, GPIO_PIN_3); //shdn1 - not led

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);

    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOQ))
    {
    }

    GPIOPinTypeGPIOOutput(GPIO_PORTQ_BASE, GPIO_PIN_4); //freq 1 - not led

    SetFreq(0, g_sParameters.ui16SwFreq0);
    SetFreq(1, g_sParameters.ui16SwFreq1);
    SetStat(0, 0);
    SetStat(1, 0);
    SetSHDN(0, 0);
    SetSHDN(1, 0);

}

void SetFreq(uint32_t Port, uint16_t freq) {
    if(Port == 0) {
        if(freq == 500) {
            GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_1, 0);
        } else if(freq == 1000) {
            GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_1, GPIO_PIN_1);
        }
    } else {
        if(freq == 500) {
            GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_4, 0);
        } else if(freq == 1000) {
            GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_4, GPIO_PIN_4);
        }
    }
}

void SetSHDN(uint32_t Port, uint16_t set) {
    if(Port == 0) {
        if(set == 0) {
            GPIOPinWrite(GPIO_PORTP_BASE, GPIO_PIN_2, 0);
        } else if(set == 1) {
            GPIOPinWrite(GPIO_PORTP_BASE, GPIO_PIN_2, GPIO_PIN_2);
        }
    } else {
        if(set == 0) {
            GPIOPinWrite(GPIO_PORTP_BASE, GPIO_PIN_3, 0);
        } else if(set == 1) {
            GPIOPinWrite(GPIO_PORTP_BASE, GPIO_PIN_3, GPIO_PIN_3);
        }
    }
}

void SetStat(uint32_t Port, uint16_t set) {

    if(Port == 0) {
        if(set == 0) {
            GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_4, 0);
        } else if(set == 1) {
            GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_4, GPIO_PIN_4);
        }
    } else {
        if(set == 0) {
            GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_5, 0);
        } else if(set == 1) {
            GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_5, GPIO_PIN_5);
        }
    }
}
