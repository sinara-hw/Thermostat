/*
 * pwm.h
 *
 *
 */

#ifndef PWM_H_
#define PWM_H_

#include <stdint.h>
#include <stdbool.h>

void pwm_config(void);
void setMaxPosCurr0(uint16_t MaxCurr);
void setMaxNegCurr0(uint16_t MaxCurr);
void setCurr0(int32_t Curr);
void setMaxVolt0(uint16_t MaxVolt);
void setMaxPosCurr1(uint16_t MaxCurr);
void setMaxNegCurr1(uint16_t MaxCurr);
void setCurr1(int32_t Curr);
void setMaxVolt1(uint16_t MaxVolt);


#endif /* PWM_H_ */
