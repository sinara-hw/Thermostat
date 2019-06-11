//*****************************************************************************
//
// serial.h - Prototypes for the Serial Port Driver
//
//*****************************************************************************

#ifndef __SERIAL_H__
#define __SERIAL_H__

#include "utils/ringbuf.h"

//*****************************************************************************
//
// Values used to indicate various parity modes for SerialSetParity and
// SerialGetParity.
//
//*****************************************************************************
#define SERIAL_PARITY_NONE      ((uint8_t)1)
#define SERIAL_PARITY_ODD       ((uint8_t)2)
#define SERIAL_PARITY_EVEN      ((uint8_t)3)
#define SERIAL_PARITY_MARK      ((uint8_t)4)
#define SERIAL_PARITY_SPACE     ((uint8_t)5)

//*****************************************************************************
//
// Values used to indicate various flow control modes for SerialSetFlowControl
// and SerialGetFlowControl.
//
//*****************************************************************************
#define SERIAL_FLOW_CONTROL_NONE                                              \
                                ((uint8_t)1)
#define SERIAL_FLOW_CONTROL_HW  ((uint8_t)3)

//*****************************************************************************
//
// Values used to indicate various flow control modes for SerialSetFlowOut
// and SerialGetFlowOut.
//
//*****************************************************************************
#define SERIAL_FLOW_OUT_SET     ((uint8_t)11)
#define SERIAL_FLOW_OUT_CLEAR   ((uint8_t)12)

extern const uint32_t g_ui32UARTBase[];
extern tRingBufObject g_sRxBuf[];
extern tRingBufObject g_sTxBuf[];

//*****************************************************************************
//
// Prototypes for the Serial interface functions.
//
//*****************************************************************************
extern bool SerialSendFull(uint32_t ui32Port);
extern void SerialSend(uint32_t ui32Port, uint8_t ui8Char);
extern int32_t SerialReceive(uint32_t ui32Port);
extern uint32_t SerialReceiveAvailable(uint32_t ui32Port);
extern void SerialSetBaudRate(uint32_t ui32Port, uint32_t ui32BaudRate);
extern uint32_t SerialGetBaudRate(uint32_t ui32Port);
extern void SerialSetDataSize(uint32_t ui32Port, uint8_t ui8DataSize);
extern uint8_t SerialGetDataSize(uint32_t ui32Port);
extern void SerialSetParity(uint32_t ui32Port, uint8_t ui8Parity);
extern uint8_t SerialGetParity(uint32_t ui32Port);
extern void SerialSetStopBits(uint32_t ui32Port, uint8_t ui8StopBits);
extern uint8_t SerialGetStopBits(uint32_t ui32Port);
extern void SerialSetFlowControl(uint32_t ui32Port,
                                 uint8_t ui8FlowControl);
extern uint8_t SerialGetFlowControl(uint32_t ui32Port);
extern void SerialPurgeData(uint32_t ui32Port,
                            uint8_t ui8PurgeCommand);
extern void SerialSetDefault(uint32_t ui32Port);
extern void SerialSetFactory(uint32_t ui32Port);
extern void SerialSetCurrent(uint32_t ui32Port);
extern void SerialInit(void);

#endif // __SERIAL_H__
