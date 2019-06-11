/*
 * leds.h
 *
 *
 *
 */

#ifndef LEDS_H_
#define LEDS_H_

void leds_config(void);
void SetFreq(uint32_t Port, uint16_t freq);
void SetSHDN(uint32_t Port, uint16_t set);
void SetStat(uint32_t Port, uint16_t set);

#endif /* LEDS_H_ */
