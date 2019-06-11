/*
 * pwm.c
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

#define IMAX 3000    //max peltier current in mA
#define ILIMMAX 3000 //max peltier current limit in mA
#define VMAX 5000 //max peltier voltage in mV

//*****************************************************************************
//
// Configure Timer1B as a 16-bit PWM with a duty cycle of 66%.
//
//*****************************************************************************
void pwm_config(void)
{

    int resolution = 65535;

    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER3);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER4);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER5);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);

    GPIOPinConfigure(GPIO_PM0_T2CCP0);
    GPIOPinConfigure(GPIO_PM1_T2CCP1);
    GPIOPinConfigure(GPIO_PM2_T3CCP0);
    GPIOPinConfigure(GPIO_PM3_T3CCP1);
    GPIOPinConfigure(GPIO_PM4_T4CCP0);
    GPIOPinConfigure(GPIO_PM5_T4CCP1);
    GPIOPinConfigure(GPIO_PM6_T5CCP0);
    GPIOPinConfigure(GPIO_PM7_T5CCP1);

    GPIOPinTypeTimer(GPIO_PORTM_BASE, GPIO_PIN_0);
    GPIOPinTypeTimer(GPIO_PORTM_BASE, GPIO_PIN_1);
    GPIOPinTypeTimer(GPIO_PORTM_BASE, GPIO_PIN_2);
    GPIOPinTypeTimer(GPIO_PORTM_BASE, GPIO_PIN_3);
    GPIOPinTypeTimer(GPIO_PORTM_BASE, GPIO_PIN_4);
    GPIOPinTypeTimer(GPIO_PORTM_BASE, GPIO_PIN_5);
    GPIOPinTypeTimer(GPIO_PORTM_BASE, GPIO_PIN_6);
    GPIOPinTypeTimer(GPIO_PORTM_BASE, GPIO_PIN_7);


    TimerConfigure(TIMER2_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PWM | TIMER_CFG_B_PWM);
    TimerConfigure(TIMER3_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PWM | TIMER_CFG_B_PWM);
    TimerConfigure(TIMER4_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PWM | TIMER_CFG_B_PWM);
    TimerConfigure(TIMER5_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PWM | TIMER_CFG_B_PWM);


    TimerLoadSet(TIMER2_BASE, TIMER_A, resolution);
    TimerLoadSet(TIMER2_BASE, TIMER_B, resolution);
    TimerLoadSet(TIMER3_BASE, TIMER_A, resolution);
    TimerLoadSet(TIMER3_BASE, TIMER_B, resolution);
    TimerLoadSet(TIMER4_BASE, TIMER_A, resolution);
    TimerLoadSet(TIMER4_BASE, TIMER_B, resolution);
    TimerLoadSet(TIMER5_BASE, TIMER_A, resolution);
    TimerLoadSet(TIMER5_BASE, TIMER_B, resolution);


    setMaxPosCurr0(g_sParameters.ui16MaxPosCurr0);
    setMaxNegCurr0(g_sParameters.ui16MaxNegCurr0);
    setCurr0(0);
    setMaxVolt0(g_sParameters.ui16MaxVolt0);
    setMaxPosCurr1(g_sParameters.ui16MaxPosCurr1);
    setMaxNegCurr1(g_sParameters.ui16MaxNegCurr1);
    setCurr1(0);
    setMaxVolt1(g_sParameters.ui16MaxVolt1);



    TimerEnable(TIMER2_BASE, TIMER_A | TIMER_B);
    TimerEnable(TIMER3_BASE, TIMER_A | TIMER_B);
    TimerEnable(TIMER4_BASE, TIMER_A | TIMER_B);
    TimerEnable(TIMER5_BASE, TIMER_A | TIMER_B);

}

void setMaxPosCurr0(uint16_t MaxCurr) {
    uint32_t Value = MaxCurr;
    if( Value > ILIMMAX ) {
        Value = ILIMMAX;
    }
    Value = Value*1500/3000;
    if(Value != 0) {
        TimerMatchSet(TIMER2_BASE, TIMER_A, TimerLoadGet(TIMER2_BASE, TIMER_A) * (3300 - Value) /3300);
    } else {
        TimerMatchSet(TIMER2_BASE, TIMER_A, TimerLoadGet(TIMER2_BASE, TIMER_A) - 1);
    }

}

void setMaxNegCurr0(uint16_t MaxCurr) {
    uint32_t Value = MaxCurr;
    if( Value > ILIMMAX ) {
        Value = ILIMMAX;
    }
    Value = Value*1500/3000;
    if(Value != 0) {
        TimerMatchSet(TIMER2_BASE, TIMER_B, TimerLoadGet(TIMER2_BASE, TIMER_B) * (3300 - Value) /3300);
    } else {
        TimerMatchSet(TIMER2_BASE, TIMER_B, TimerLoadGet(TIMER2_BASE, TIMER_B) - 1);
    }

}

void setCurr0(int32_t Curr) {
    if( Curr > IMAX ) {
        Curr = IMAX;
    }
    if( Curr < -IMAX ) {
        Curr = -IMAX;
    }

    Curr = (1500+Curr*1500/3000);
    if(Curr != 0) {
        TimerMatchSet(TIMER3_BASE, TIMER_A, TimerLoadGet(TIMER3_BASE, TIMER_A) * (3300 - Curr) /3300);
    } else {
        TimerMatchSet(TIMER3_BASE, TIMER_A, TimerLoadGet(TIMER3_BASE, TIMER_A) - 1);
    }

}

void setMaxVolt0(uint16_t MaxVolt) {
    uint32_t Value = MaxVolt;

    if( Value > VMAX ) {
        Value = VMAX;
    }

    Value = Value*1250/5000;
    if(Value != 0) {
        TimerMatchSet(TIMER3_BASE, TIMER_B, TimerLoadGet(TIMER3_BASE, TIMER_B) * (3300 - Value) /3300);
    } else {
        TimerMatchSet(TIMER3_BASE, TIMER_B, TimerLoadGet(TIMER3_BASE, TIMER_B) - 1);
    }

}

void setMaxPosCurr1(uint16_t MaxCurr) {
    uint32_t Value = MaxCurr;
    if( Value > ILIMMAX ) {
        Value = ILIMMAX;
    }
    Value = Value*1500/3000;
    if(Value != 0) {
        TimerMatchSet(TIMER4_BASE, TIMER_A, TimerLoadGet(TIMER4_BASE, TIMER_A) * (3300 - Value) /3300);
    } else {
        TimerMatchSet(TIMER4_BASE, TIMER_A, TimerLoadGet(TIMER4_BASE, TIMER_A) - 1);
    }

}

void setMaxNegCurr1(uint16_t MaxCurr) {
    uint32_t Value = MaxCurr;
    if( Value > ILIMMAX ) {
        Value = ILIMMAX;
    }
    Value = Value*1500/3000;
    if(Value != 0) {
        TimerMatchSet(TIMER4_BASE, TIMER_B, TimerLoadGet(TIMER4_BASE, TIMER_B) * (3300 - Value) /3300);
    } else {
        TimerMatchSet(TIMER4_BASE, TIMER_B, TimerLoadGet(TIMER4_BASE, TIMER_B) - 1);
    }

}

void setCurr1(int32_t Curr) {
    if( Curr > IMAX ) {
        Curr = IMAX;
    }
    if( Curr < -IMAX ) {
        Curr = -IMAX;
    }

    Curr = (1500+Curr*1500/3000);
    if(Curr != 0) {
        TimerMatchSet(TIMER5_BASE, TIMER_A, TimerLoadGet(TIMER5_BASE, TIMER_A) * (3300 - Curr) /3300);
    } else {
        TimerMatchSet(TIMER5_BASE, TIMER_A, TimerLoadGet(TIMER5_BASE, TIMER_A) - 1);
    }
}

void setMaxVolt1(uint16_t MaxVolt) {
    uint32_t Value = MaxVolt;

    if( Value > VMAX ) {
        Value = VMAX;
    }

    Value = Value*1250/5000;

    if(Value != 0) {
        TimerMatchSet(TIMER5_BASE, TIMER_B, TimerLoadGet(TIMER5_BASE, TIMER_B) * (3300 - Value) /3300);
    } else {
        TimerMatchSet(TIMER5_BASE, TIMER_B, TimerLoadGet(TIMER5_BASE, TIMER_B) - 1);
    }
}

