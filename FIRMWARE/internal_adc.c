/*
 * internal_adc.c
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

void internal_adc_config(void)
{

    uint32_t ADC_SEQ = 1;

    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC1);
    ADCHardwareOversampleConfigure(ADC0_BASE, 16);
    ADCHardwareOversampleConfigure(ADC1_BASE, 16);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_2);
    GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_5);
    GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_4);


    ADCSequenceConfigure(ADC0_BASE, ADC_SEQ, ADC_TRIGGER_PROCESSOR, 0);

    ADCSequenceStepConfigure(ADC0_BASE, ADC_SEQ, 0, ADC_CTL_CH1);
    ADCSequenceStepConfigure(ADC0_BASE, ADC_SEQ, 1, ADC_CTL_CH6 | ADC_CTL_IE | ADC_CTL_END);

    ADCSequenceEnable(ADC0_BASE, ADC_SEQ);

    ADCIntClear(ADC0_BASE, ADC_SEQ);

    ADCSequenceConfigure(ADC1_BASE, ADC_SEQ, ADC_TRIGGER_PROCESSOR, 0);

    ADCSequenceStepConfigure(ADC1_BASE, ADC_SEQ, 0, ADC_CTL_CH0);
    ADCSequenceStepConfigure(ADC1_BASE, ADC_SEQ, 1, ADC_CTL_CH7 | ADC_CTL_IE | ADC_CTL_END);

    ADCSequenceEnable(ADC1_BASE, ADC_SEQ);

    ADCIntClear(ADC1_BASE, ADC_SEQ);

}

void internal_adc_getvalues0(uint32_t* Value) {

    uint32_t ADC_SEQ = 1;

    ADCProcessorTrigger(ADC0_BASE, ADC_SEQ);

    while(!ADCIntStatus(ADC0_BASE, ADC_SEQ, false))
    {
    }

    ADCIntClear(ADC0_BASE, ADC_SEQ);

    ADCSequenceDataGet(ADC0_BASE, ADC_SEQ, Value);

}

void internal_adc_getvalues1(uint32_t* Value) {

    uint32_t ADC_SEQ = 1;

    ADCProcessorTrigger(ADC1_BASE, ADC_SEQ);

    while(!ADCIntStatus(ADC1_BASE, ADC_SEQ, false))
    {
    }

    ADCIntClear(ADC1_BASE, ADC_SEQ);

    ADCSequenceDataGet(ADC1_BASE, ADC_SEQ, Value);

}
