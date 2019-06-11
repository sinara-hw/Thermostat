//*****************************************************************************
//
// config.h - Configuration of the serial to Ethernet converter.
//
//*****************************************************************************

#ifndef __CONFIG_H__
#define __CONFIG_H__s

//*****************************************************************************
//
// The system clock frequency.
//
//*****************************************************************************
uint32_t g_ui32SysClock;

//*****************************************************************************
//
//! \addtogroup config_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// During debug, DEBUG_UART may be defined with values 0 or 1 to select which
// of the two UARTs are used to output debug messages.  Debug messages will be
// interleaved with any telnet data also being carried via that UART so great
// care must be exercised when enabling this debug option.  Typically, you
// should use only a single telnet connection when debugging with DEBUG_UART
// set to use the inactive UART.
//
//*****************************************************************************
//#define DEBUG_UART 0
//extern void UARTprintf(const char *pcString, ...);
#ifdef DEBUG_UART
#define DEBUG_MSG               UARTprintf
#else
#define DEBUG_MSG               while(0) ((int (*)(char *, ...))0)
#endif

//*****************************************************************************
//
//! The number of serial to Ethernet ports supported by this module.
//
//*****************************************************************************
#define MAX_S2E_PORTS           2

//*****************************************************************************
//
//! This structure defines the port parameters used to configure the UART and
//! telnet session associated with a single instance of a port on the S2E
//! module.
//
//*****************************************************************************
typedef struct
{
    //
    //! The baud rate to be used for the UART, specified in bits-per-second
    //! (bps).
    //
    uint32_t ui32BaudRate;

    //
    //! The data size to be use for the serial port, specified in bits.  Valid
    //! values are 5, 6, 7, and 8.
    //
    uint8_t ui8DataSize;

    //
    //! The parity to be use for the serial port, specified as an enumeration.
    //! Valid values are 1 for no parity, 2 for odd parity, 3 for even parity,
    //! 4 for mark parity, and 5 for space parity.
    //
    uint8_t ui8Parity;

    //
    //! The number of stop bits to be use for the serial port, specified as
    //! a number.  Valid values are 1 and 2.
    //
    uint8_t ui8StopBits;

    //
    //! The flow control to be use for the serial port, specified as an
    //! enumeration.  Valid values are 1 for no flow control and 3 for HW
    //! (RTS/CTS) flow control.
    //
    uint8_t ui8FlowControl;

    //
    //! The timeout for the TCP connection used for the telnet session,
    //! specified in seconds.  A value of 0 indicates no timeout is to
    //! be used.
    //
    uint32_t ui32TelnetTimeout;

    //
    //! The local TCP port number to be listened on when in server mode or
    //! from which the outgoing connection will be initiated in client mode.
    //
    uint16_t ui16TelnetLocalPort;

    //
    //! The TCP port number to which a connection will be established when in
    //! telnet client mode.
    //
    uint16_t ui16TelnetRemotePort;

    //
    //! The IP address which will be connected to when in telnet client mode.
    //
    uint32_t ui32TelnetIPAddr;

    //
    //! Miscellaneous flags associated with this connection.
    //
    uint8_t ui8Flags;

    //! Padding to ensure consistent parameter block alignment, and
    //! to allow for future expansion of port parameters.
    //
    uint8_t ui8Reserved0[19];
}
tPortParameters;

//*****************************************************************************
//
//! Bit 0 of field ucFlags in tPortParameters is used to indicate whether to
//! operate as a telnet server or a telnet client.
//
//*****************************************************************************
#define PORT_FLAG_TELNET_MODE   0x01

//*****************************************************************************
//
// Helpful labels used to determine if we are operating as a telnet client or
// server (based on the state of the PORT_FLAG_TELNET_MODE bit in the ucFlags
// field of tPortParameters).
//
//*****************************************************************************
#define PORT_TELNET_SERVER      0x00
#define PORT_TELNET_CLIENT      PORT_FLAG_TELNET_MODE

//*****************************************************************************
//
//! Bit 1 of field ucFlags in tPortParameters is used to indicate whether to
//! bypass the telnet protocol (raw data mode).
//
//*****************************************************************************
#define PORT_FLAG_PROTOCOL      0x02

//*****************************************************************************
//
// Helpful labels used to determine if we are operating in raw data mode, or
// in telnet protocol mode.
//
//*****************************************************************************
#define PORT_PROTOCOL_TELNET    0x00
#define PORT_PROTOCOL_RAW       PORT_FLAG_PROTOCOL

//*****************************************************************************
//
// The length of the ucModName array in the tConfigParameters structure.  NOTE:
// Be extremely careful if changing this since the structure packing relies
// upon this values!
//
//*****************************************************************************
#define MOD_NAME_LEN            40

//*****************************************************************************
//
//! This structure contains the S2E module parameters that are saved to flash.
//! A copy exists in RAM for use during the execution of the application, which
//! is loaded from flash at startup.  The modified parameter block can also be
//! written back to flash for use on the next power cycle.
//
//*****************************************************************************
typedef struct
{
    //
    //! The sequence number of this parameter block.  When in RAM, this value
    //! is not used.  When in flash, this value is used to determine the
    //! parameter block with the most recent information.
    //
    uint8_t ui8SequenceNum;

    //
    //! The CRC of the parameter block.  When in RAM, this value is not used.
    //! When in flash, this value is used to validate the contents of the
    //! parameter block (to avoid using a partially written parameter block).
    //
    uint8_t ui8CRC;

    //
    //! The version of this parameter block.  This can be used to distinguish
    //! saved parameters that correspond to an old version of the parameter
    //! block.
    //
    uint8_t ui8Version;

    //
    //! Character field used to store various bit flags.
    //
    uint8_t ui8Flags;

    //
    //! Padding to ensure consistent parameter block alignment.
    //
    uint8_t ui8Reserved1[4];

    //
    //! The configuration parameters for each port available on the S2E
    //! module.
    //
    tPortParameters sPort[MAX_S2E_PORTS];

    //
    //! An ASCII string used to identify the module to users via web
    //! configuration.
    //
    uint8_t ui8ModName[MOD_NAME_LEN];

    //
    //! The static IP address to use if DHCP is not in use.
    //
    uint32_t ui32StaticIP;

    //
    //! The default gateway IP address to use if DHCP is not in use.
    //
    uint32_t ui32GatewayIP;

    //
    //! The subnet mask to use if DHCP is not in use.
    //
    uint32_t ui32SubnetMask;

    //
    //! The deafult mqtt IP.
    //
    uint32_t ui32mqttip;

    //
    //!
    //
    int16_t i16clp0;
    int16_t i16clp1;
    int16_t i16cli0;
    int16_t i16cli1;
    int16_t i16cld0;
    int16_t i16cld1;

    int32_t i32SetTemp0;
    int32_t i32SetTemp1;

    int32_t i32T00;
    int32_t i32T01;

    int32_t i32Beta0;
    int32_t i32Beta1;

    int32_t i32Ratio0;
    int32_t i32Ratio1;

    int16_t ui16TempMode0;
    int16_t ui16TempMode1;

    int32_t i32ptA0;
    int32_t i32ptA1;
    int32_t i32ptB0;
    int32_t i32ptB1;

    uint16_t ui16SwFreq0;
    uint16_t ui16SwFreq1;

    uint16_t ui16MaxPosCurr0;
    uint16_t ui16MaxPosCurr1;
    uint16_t ui16MaxNegCurr0;
    uint16_t ui16MaxNegCurr1;

    uint16_t ui16MaxVolt0;
    uint16_t ui16MaxVolt1;

    uint16_t ui16TempWindow0;
    uint16_t ui16TempWindow1;


    //
    //! Padding to ensure the whole structure is 256 bytes long.
    //
    uint8_t ui8Reserved2[32];
}
tConfigParameters;

//*****************************************************************************
//
//! If this flag is set in the ucFlags field of tConfigParameters, the module
//! uses a static IP.  If not, DHCP and AutoIP are used to obtain an IP
//! address.
//
//*****************************************************************************
#define CONFIG_FLAG_STATICIP    0x80

//*****************************************************************************
//
//! The offset in EEPROM to be used for storing parameters.
//
//*****************************************************************************
#define EEPROM_PB_START         0x00000000

//*****************************************************************************
//
//! The size of the parameter block to save.  This must be a power of 2,
//! and should be large enough to contain the tConfigParameters structure.
//
//*****************************************************************************
#define EEPROM_PB_SIZE          256

//*****************************************************************************
//
//! The size of the ring buffers used for interface between the UART and
//! telnet session (RX).
//
//*****************************************************************************
#define RX_RING_BUF_SIZE        (256 * 4)

//*****************************************************************************
//
//! The size of the ring buffers used for interface between the UART and
//! telnet session (TX).
//
//*****************************************************************************
#define TX_RING_BUF_SIZE        (256 * 4)

//*****************************************************************************
//
//! Enable RFC-2217 (COM-PORT-OPTION) in the telnet server code.
//
//*****************************************************************************
#define CONFIG_RFC2217_ENABLED  1


//*****************************************************************************
//
//! The peripheral on which the Port 0 UART resides.
//
//*****************************************************************************
#define S2E_PORT0_UART_PERIPH   SYSCTL_PERIPH_UART4

//*****************************************************************************
//
//! The port on which the Port 0 UART resides.
//
//*****************************************************************************
#define S2E_PORT0_UART_PORT     UART4_BASE

//*****************************************************************************
//
//! The interrupt on which the Port 0 UART resides.
//
//*****************************************************************************
#define S2E_PORT0_UART_INT      INT_UART4

//*****************************************************************************
//
//! The peripheral on which the Port 0 RX pin resides.
//
//*****************************************************************************
#define S2E_PORT0_RX_PERIPH     SYSCTL_PERIPH_GPIOA

//*****************************************************************************
//
//! The GPIO port on which the Port 0 RX pin resides.
//
//*****************************************************************************
#define S2E_PORT0_RX_PORT       GPIO_PORTA_BASE

//*****************************************************************************
//
//! The GPIO pin on which the Port 0 RX pin resides.
//
//*****************************************************************************
#define S2E_PORT0_RX_PIN        GPIO_PIN_2

//*****************************************************************************
//
//! The GPIO pin configuration for Port 0 RX pin.
//
//*****************************************************************************
#define S2E_PORT0_RX_CONFIG     GPIO_PA2_U4RX

//*****************************************************************************
//
//! The peripheral on which the Port 0 TX pin resides.
//
//*****************************************************************************
#define S2E_PORT0_TX_PERIPH     SYSCTL_PERIPH_GPIOA

//*****************************************************************************
//
//! The GPIO port on which the Port 0 TX pin resides.
//
//*****************************************************************************
#define S2E_PORT0_TX_PORT       GPIO_PORTA_BASE

//*****************************************************************************
//
//! The GPIO pin on which the Port 0 TX pin resides.
//
//*****************************************************************************
#define S2E_PORT0_TX_PIN        GPIO_PIN_3

//*****************************************************************************
//
//! The GPIO pin configuration for Port 0 TX pin.
//
//*****************************************************************************
#define S2E_PORT0_TX_CONFIG     GPIO_PA3_U4TX

//*****************************************************************************
//
//! The peripheral on which the Port 0 RTS pin resides.
//
//*****************************************************************************
#define S2E_PORT0_RTS_PERIPH    SYSCTL_PERIPH_GPIOK

//*****************************************************************************
//
//! The GPIO port on which the Port 0 RTS pin resides.
//
//*****************************************************************************
#define S2E_PORT0_RTS_PORT      GPIO_PORTK_BASE

//*****************************************************************************
//
//! The GPIO pin on which the Port 0 RTS pin resides.
//
//*****************************************************************************
#define S2E_PORT0_RTS_PIN       GPIO_PIN_2

//*****************************************************************************
//
//! The GPIO pin configuration for Port 0 RTS pin.
//
//*****************************************************************************
#define S2E_PORT0_RTS_CONFIG    GPIO_PK2_U4RTS

//*****************************************************************************
//
//! The peripheral on which the Port 0 CTS pin resides.
//
//*****************************************************************************
#define S2E_PORT0_CTS_PERIPH    SYSCTL_PERIPH_GPIOK

//*****************************************************************************
//
//! The GPIO port on which the Port 0 CTS pin resides.
//
//*****************************************************************************
#define S2E_PORT0_CTS_PORT      GPIO_PORTK_BASE

//*****************************************************************************
//
//! The GPIO pin on which the Port 0 CTS pin resides.
//
//*****************************************************************************
#define S2E_PORT0_CTS_PIN       GPIO_PIN_3

//*****************************************************************************
//
//! The GPIO pin configuration for Port 0 CTS pin.
//
//*****************************************************************************
#define S2E_PORT0_CTS_CONFIG    GPIO_PK3_U4CTS



//*****************************************************************************
//
//! The peripheral on which the Port 1 UART resides.
//
//*****************************************************************************
#define S2E_PORT1_UART_PERIPH   SYSCTL_PERIPH_UART3

//*****************************************************************************
//
//! The port on which the Port 1 UART resides.
//
//*****************************************************************************
#define S2E_PORT1_UART_PORT     UART3_BASE

//*****************************************************************************
//
//! The interrupt on which the Port 1 UART resides.
//
//*****************************************************************************
#define S2E_PORT1_UART_INT      INT_UART3

//*****************************************************************************
//
//! The peripheral on which the Port 1 RX pin resides.
//
//*****************************************************************************
#define S2E_PORT1_RX_PERIPH     SYSCTL_PERIPH_GPIOJ

//*****************************************************************************
//
//! The GPIO port on which the Port 1 RX pin resides.
//
//*****************************************************************************
#define S2E_PORT1_RX_PORT       GPIO_PORTJ_BASE

//*****************************************************************************
//
//! The GPIO pin on which the Port 1 RX pin resides.
//
//*****************************************************************************
#define S2E_PORT1_RX_PIN        GPIO_PIN_0

//*****************************************************************************
//
//! The GPIO pin configuration for Port 1 RX pin.
//
//*****************************************************************************
#define S2E_PORT1_RX_CONFIG     GPIO_PJ0_U3RX

//*****************************************************************************
//
//! The peripheral on which the Port 1 TX pin resides.
//
//*****************************************************************************
#define S2E_PORT1_TX_PERIPH     SYSCTL_PERIPH_GPIOJ

//*****************************************************************************
//
//! The GPIO port on which the Port 1 TX pin resides.
//
//*****************************************************************************
#define S2E_PORT1_TX_PORT       GPIO_PORTJ_BASE

//*****************************************************************************
//
//! The GPIO pin on which the Port 1 TX pin resides.
//
//*****************************************************************************
#define S2E_PORT1_TX_PIN        GPIO_PIN_1

//*****************************************************************************
//
//! The GPIO pin configuration for Port 1 TX pin.
//
//*****************************************************************************
#define S2E_PORT1_TX_CONFIG     GPIO_PJ1_U3TX

//*****************************************************************************
//
//! The peripheral on which the Port 1 RTS pin resides.
//
//*****************************************************************************
#define S2E_PORT1_RTS_PERIPH    SYSCTL_PERIPH_GPION

//*****************************************************************************
//
//! The GPIO port on which the Port 1 RTS pin resides.
//
//*****************************************************************************
#define S2E_PORT1_RTS_PORT      GPIO_PORTN_BASE

//*****************************************************************************
//
//! The GPIO pin on which the Port 1 RTS pin resides.
//
//*****************************************************************************
#define S2E_PORT1_RTS_PIN       GPIO_PIN_4

//*****************************************************************************
//
//! The GPIO pin configuration for Port 1 RTS pin.
//
//*****************************************************************************
#define S2E_PORT1_RTS_CONFIG    GPIO_PN4_U3RTS

//*****************************************************************************
//
//! The peripheral on which the Port 1 CTS pin resides.
//
//*****************************************************************************
#define S2E_PORT1_CTS_PERIPH    SYSCTL_PERIPH_GPION

//*****************************************************************************
//
//! The GPIO port on which the Port 1 CTS pin resides.
//
//*****************************************************************************
#define S2E_PORT1_CTS_PORT      GPIO_PORTN_BASE

//*****************************************************************************
//
//! The GPIO pin on which the Port 1 CTS pin resides.
//
//*****************************************************************************
#define S2E_PORT1_CTS_PIN       GPIO_PIN_5

//*****************************************************************************
//
//! The GPIO pin configuration for Port 1 CTS pin.
//
//*****************************************************************************
#define S2E_PORT1_CTS_CONFIG    GPIO_PN5_U3CTS





//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

//*****************************************************************************
//
// Flags sent to the main loop indicating that it should update the IP address
// after a short delay (to allow us to send a suitable page back to the web
// browser telling it the address has changed).
//
//*****************************************************************************
extern uint8_t g_ui8UpdateRequired;

#define UPDATE_IP_ADDR          0x01
#define UPDATE_ALL              0x02

//*****************************************************************************
//
// Prototypes for the globals exported from the configuration module, along
// with public API function prototypes.
//
//*****************************************************************************
extern bool mqtt_conn;

extern uint32_t ui32RawTemp0;
extern uint32_t ui32RawTemp1;
extern int32_t i32ActTemp0;
extern int32_t i32ActTemp1;
extern int16_t onoff0;
extern int16_t onoff1;


extern tConfigParameters g_sParameters;
extern const tConfigParameters *g_psDefaultParameters;
extern const tConfigParameters *const g_psFactoryParameters;
extern void ConfigInit(void);
extern void ConfigLoadFactory(void);
extern void ConfigLoad(void);
extern void ConfigSave(void);
extern void ConfigWebInit(void);
extern void ConfigUpdateIPAddress(void);
extern void ConfigUpdateAllParameters(bool bUpdateIP);
extern bool ConfigCheckDecimalParam(const char *pcValue, int32_t *pi32Value);
extern void ConfigSaveAll(void);

#endif // __CONFIG_H__
