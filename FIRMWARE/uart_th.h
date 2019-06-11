/*
 * uart_th.h
 *
 *
 *
 */

#ifndef UART_TH_H_
#define UART_TH_H_


void UARTConfig(uint32_t g_ui32SysClock);
void UARTSend(char *pui8Buffer, uint32_t ui32Count);
void UARTIntHandler(void);


#endif /* UART_TH_H_ */
