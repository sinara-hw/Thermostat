/*
 * send_command.h
 *
 *
 */

#ifndef SEND_COMMAND_H_
#define SEND_COMMAND_H_

#include <stdint.h>
#include <stdbool.h>

extern uint32_t ui32RawTemp0;
extern uint32_t ui32RawTemp1;
extern int32_t i32ActTemp0;
extern int32_t i32ActTemp1;

void Send_Command(uint32_t ui32Port, const char *commandBuf, uint32_t ui32Len);
uint32_t General_Command(uint32_t ui32Port, uint32_t mode, char *answerBuf);
bool Set_TEC_limit(uint32_t ui32Port, int32_t limit, uint32_t PosNeg);
uint32_t Read_TEC(uint32_t ui32Port, uint32_t mode, char *answerBuf);
bool Set_Temp(uint32_t ui32Port, int32_t temp);
bool Set_Temp_Window(uint32_t ui32Port, uint32_t temp_window);
uint32_t Read_Temp(uint32_t ui32Port, uint32_t mode, char *answerBuf);
bool Set_PID(uint32_t ui32Port, uint32_t mode, int32_t pid);
uint32_t Read_PID(uint32_t ui32Port, uint32_t mode, char *answerBuf);
bool Set_Calib(uint32_t ui32Port, uint32_t mode, int32_t calib);
uint32_t Read_Calib(uint32_t ui32Port, uint32_t mode, char *answerBuf);
uint32_t Read_RawTemp(uint32_t ui32Port, char *answerBuf);
uint32_t Read_MaxVolt(uint32_t ui32Port, char *answerBuf);
bool OnOff(uint32_t ui32Port, int32_t onoff);
bool Set_MaxVolt(uint32_t ui32Port, int32_t voltage);

#endif /* SEND_COMMAND_H_ */
