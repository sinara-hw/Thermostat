/*
 * internal_adc.h
 *
 *
 *
 */

#ifndef INTERNAL_ADC_H_
#define INTERNAL_ADC_H_


#include <stdint.h>
#include <stdbool.h>

void internal_adc_config(void);
void internal_adc_getvalues0(uint32_t* Value);
void internal_adc_getvalues1(uint32_t* Value);


#endif /* INTERNAL_ADC_H_ */
