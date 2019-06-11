//*****************************************************************************
//
// priorities.h - Priorities for the various FreeRTOS tasks.
//
//*****************************************************************************

#ifndef __PRIORITIES_H__
#define __PRIORITIES_H__

//*****************************************************************************
//
// The priorities of the various tasks.
//
//*****************************************************************************
#define UART3_INT_PRIORITY      0xC0
#define UART4_INT_PRIORITY      0xC0
#define ETHERNET_INT_PRIORITY   0xC0
#define WATCHDOG_INT_PRIORITY   0xE0

#define PRIORITY_SERIAL_TASK    2
#define PRIORITY_ETH_INT_TASK   3
#define PRIORITY_PID_TASK       4
#define PRIORITY_TCPIP_TASK     5

#endif // __PRIORITIES_H__
