//*****************************************************************************
//
// telnet.h - Definitions for the telnet command interface.
//
//*****************************************************************************

#ifndef __TELNET_H__
#define __TELNET_H__

//*****************************************************************************
//
// Telnet commands, as defined by RFC854.
//
//*****************************************************************************
#define TELNET_IAC              255
#define TELNET_WILL             251
#define TELNET_WONT             252
#define TELNET_DO               253
#define TELNET_DONT             254
#define TELNET_SE               240
#define TELNET_NOP              241
#define TELNET_DATA_MARK        242
#define TELNET_BREAK            243
#define TELNET_IP               244
#define TELNET_AO               245
#define TELNET_AYT              246
#define TELNET_EC               247
#define TELNET_EL               248
#define TELNET_GA               249
#define TELNET_SB               250

//*****************************************************************************
//
// Telnet options, as defined by RFC856-RFC861.
//
//*****************************************************************************
#define TELNET_OPT_BINARY       0
#define TELNET_OPT_ECHO         1
#define TELNET_OPT_SUPPRESS_GA  3
#define TELNET_OPT_STATUS       5
#define TELNET_OPT_TIMING_MARK  6
#define TELNET_OPT_EXOPL        255

#if CONFIG_RFC2217_ENABLED
//*****************************************************************************
//
// Telnet Com Port Control options, as defined by RFC2217.
//
//*****************************************************************************
#define TELNET_OPT_RFC2217      44

//
// Client to Server Option Definitions
//
#define TELNET_C2S_SIGNATURE    0
#define TELNET_C2S_SET_BAUDRATE 1
#define TELNET_C2S_SET_DATASIZE 2
#define TELNET_C2S_SET_PARITY   3
#define TELNET_C2S_SET_STOPSIZE 4
#define TELNET_C2S_SET_CONTROL  5
#define TELNET_C2S_NOTIFY_LINESTATE                                           \
                                6
#define TELNET_C2S_NOTIFY_MODEMSTATE                                          \
                                7
#define TELNET_C2S_FLOWCONTROL_SUSPEND                                        \
                                8
#define TELNET_C2S_FLOWCONTROL_RESUME                                         \
                                9
#define TELNET_C2S_SET_LINESTATE_MASK                                         \
                                10
#define TELNET_C2S_SET_MODEMSTATE_MASK                                        \
                                11
#define TELNET_C2S_PURGE_DATA                                                 \
                                12

//
// Server to Client Option Definitions
//
#define TELNET_S2C_SIGNATURE    (0 + 100)
#define TELNET_S2C_SET_BAUDRATE (1 + 100)
#define TELNET_S2C_SET_DATASIZE (2 + 100)
#define TELNET_S2C_SET_PARITY   (3 + 100)
#define TELNET_S2C_SET_STOPSIZE (4 + 100)
#define TELNET_S2C_SET_CONTROL  (5 + 100)
#define TELNET_S2C_NOTIFY_LINESTATE                                           \
                                (6 + 100)
#define TELNET_S2C_NOTIFY_MODEMSTATE                                          \
                                (7 + 100)
#define TELNET_S2C_FLOWCONTROL_SUSPEND                                        \
                                (8 + 100)
#define TELNET_S2C_FLOWCONTROL_RESUME                                         \
                                (9 + 100)
#define TELNET_S2C_SET_LINESTATE_MASK                                         \
                                (10 + 100)
#define TELNET_S2C_SET_MODEMSTATE_MASK                                        \
                                (11 + 100)
#define TELNET_S2C_PURGE_DATA                                                 \
                                (12 + 100)
#endif // CONFIG_RFC2217_ENABLED

//*****************************************************************************
//
// API Function prototypes
//
//*****************************************************************************
extern void TelnetInit(void);
extern void TelnetListen(uint16_t ui16TelnetPort,
                         uint32_t ui32SerialPort);
extern void TelnetOpen(uint32_t ui32IPAddr,
                       uint16_t ui16TelnetRemotePort,
                       uint16_t ui16TelnetLocalPort,
                       uint32_t ui32SerialPort);
extern void TelnetClose(uint32_t ui32SerialPort);
extern void TelnetHandler(void);
extern unsigned short TelnetGetLocalPort(uint32_t ui32SerialPort);
extern unsigned short TelnetGetRemotePort(uint32_t ui32SerialPort);
#if CONFIG_RFC2217_ENABLED
extern void TelnetNotifyModemState(uint32_t ui32Port,
                                   uint8_t ui8ModemState);
#endif
extern void TelnetNotifyLinkStatus(bool bLinkStatusUp);
#ifdef ENABLE_WEB_DIAGNOSTICS
extern void TelnetWriteDiagInfo(char *pcBuffer, int iLen,
                                uint8_t ui8Port);
#endif
#endif // __TELNET_H__
