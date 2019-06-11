//*****************************************************************************
//
// config.c - Configuration of the serial to Ethernet converter.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/uart.h"
#include "driverlib/sysctl.h"
#include "utils/eeprom_pb.h"
#include "driverlib/eeprom.h"
#include "utils/locator.h"
#include "utils/lwiplib.h"
#include "utils/ustdlib.h"
//#include "httpserver_raw/httpd.h"
#include "utils/uartstdio.h"
#include "lwip/apps/httpd.h"
#include "lwip/ip_addr.h"
#include "http/httpd.c"
#include "config.h"
#include "serial.h"
#include "telnet.h"
#include "send_command.h"
#include "mqtt_client.h"
#include "uart_th.h"
#include "leds.h"

#include "driverlib/rom.h"
#include "inc/hw_memmap.h"

//*****************************************************************************
//
//! \addtogroup config_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! Flags sent to the main loop indicating that it should update the IP address
//! after a short delay (to allow us to send a suitable page back to the web
//! browser telling it the address has changed).
//
//*****************************************************************************
uint8_t g_ui8UpdateRequired;

//*****************************************************************************
//
//! The maximum length of any HTML form variable name used in this application.
//
//*****************************************************************************
#define MAX_VARIABLE_NAME_LEN   16

//*****************************************************************************
//
// SSI tag indices for each entry in the g_pcSSITags array.
//
//*****************************************************************************
#define SSI_INDEX_IPADDR        0
#define SSI_INDEX_MACADDR       1
#define SSI_INDEX_P0BR          2
#define SSI_INDEX_P0SB          3
#define SSI_INDEX_P0P           4
#define SSI_INDEX_P0BC          5
#define SSI_INDEX_P0FC          6
#define SSI_INDEX_P0TT          7
#define SSI_INDEX_P0TLP         8
#define SSI_INDEX_P0TRP         9
#define SSI_INDEX_P0TIP         10
#define SSI_INDEX_P0TIP1        11
#define SSI_INDEX_P0TIP2        12
#define SSI_INDEX_P0TIP3        13
#define SSI_INDEX_P0TIP4        14
#define SSI_INDEX_P0TNM         15
#define SSI_INDEX_P1BR          16
#define SSI_INDEX_P1SB          17
#define SSI_INDEX_P1P           18
#define SSI_INDEX_P1BC          19
#define SSI_INDEX_P1FC          20
#define SSI_INDEX_P1TT          21
#define SSI_INDEX_P1TLP         22
#define SSI_INDEX_P1TRP         23
#define SSI_INDEX_P1TIP         24
#define SSI_INDEX_P1TIP1        25
#define SSI_INDEX_P1TIP2        26
#define SSI_INDEX_P1TIP3        27
#define SSI_INDEX_P1TIP4        28
#define SSI_INDEX_P1TNM         29
#define SSI_INDEX_MODNAME       30
#define SSI_INDEX_PNPPORT       31 //
#define SSI_INDEX_DISABLE       32 //
#define SSI_INDEX_DVARS         33
#define SSI_INDEX_P0VARS        34
#define SSI_INDEX_P1VARS        35
#define SSI_INDEX_MODNINP       36
#define SSI_INDEX_PNPINP        37 //
#define SSI_INDEX_P0TVARS       38
#define SSI_INDEX_P1TVARS       39
#define SSI_INDEX_P0IPVAR       40
#define SSI_INDEX_P1IPVAR       41
#define SSI_INDEX_IPVARS        42
#define SSI_INDEX_SNVARS        43
#define SSI_INDEX_GWVARS        44
#define SSI_INDEX_REVISION      45 //
#define SSI_INDEX_P0PROT        46
#define SSI_INDEX_P1PROT        47

#define SSI_INDEX_TECLIMP0      48//
#define SSI_INDEX_TECLIMP1      49

#define SSI_INDEX_TECLIMN0      50//
#define SSI_INDEX_TECLIMN1      51

#define SSI_INDEX_TECCUR0       52
#define SSI_INDEX_TECCUR1       53

#define SSI_INDEX_TECVOL0       54
#define SSI_INDEX_TECVOL1       55
#define SSI_INDEX_TEMPSET0		56
#define SSI_INDEX_TEMPSET1		57
#define SSI_INDEX_TEMPACT0		58
#define SSI_INDEX_TEMPACT1		59
#define SSI_INDEX_TEMPWIN0 		60
#define SSI_INDEX_TEMPWIN1  	61

#define SSI_INDEX_CLP0			62
#define SSI_INDEX_CLP1			63
#define SSI_INDEX_CLI0			64
#define SSI_INDEX_CLI1			65
#define SSI_INDEX_CLD0			66
#define SSI_INDEX_CLD1			67

#define SSI_INDEX_T00           68
#define SSI_INDEX_T01           69
#define SSI_INDEX_BETA0         70
#define SSI_INDEX_BETA1         71
#define SSI_INDEX_RATIO0        72
#define SSI_INDEX_RATIO1        73

#define SSI_INDEX_TEMPMOD0      74
#define SSI_INDEX_TEMPMOD1      75
#define SSI_INDEX_PTA0          76
#define SSI_INDEX_PTA1          77
#define SSI_INDEX_PTB0          78
#define SSI_INDEX_PTB1          79

#define SSI_INDEX_SWFREQ0       80
#define SSI_INDEX_SWFREQ1       81
#define SSI_INDEX_RAWTEMP0      82
#define SSI_INDEX_RAWTEMP1      83

#define SSI_INDEX_MAXVOLT0      84
#define SSI_INDEX_MAXVOLT1      85

#define SSI_INDEX_ONOFF0        86
#define SSI_INDEX_ONOFF1        87

#define SSI_INDEX_ONOFF0R       88
#define SSI_INDEX_ONOFF1R       89


#define SSI_INDEX_MQTTIP        90
#define SSI_INDEX_MQTTCONN      91

//*****************************************************************************
//
//! This array holds all the strings that are to be recognized as SSI tag
//! names by the HTTPD server.  The server will call ConfigSSIHandler to
//! request a replacement string whenever the pattern <!--#tagname--> (where
//! tagname appears in the following array) is found in ``.ssi'' or ``.shtm''
//! files that it serves.
//
//*****************************************************************************
static const char *g_pcConfigSSITags[] =
{
    "ipaddr",     // SSI_INDEX_IPADDR
    "macaddr",    // SSI_INDEX_MACADDR
    "p0br",       // SSI_INDEX_P0BR
    "p0sb",       // SSI_INDEX_P0SB
    "p0p",        // SSI_INDEX_P0P
    "p0bc",       // SSI_INDEX_P0BC
    "p0fc",       // SSI_INDEX_P0FC
    "p0tt",       // SSI_INDEX_P0TT
    "p0tlp",      // SSI_INDEX_P0TLP
    "p0trp",      // SSI_INDEX_P0TRP
    "p0tip",      // SSI_INDEX_P0TIP
    "p0tip1",     // SSI_INDEX_P0TIP1
    "p0tip2",     // SSI_INDEX_P0TIP2
    "p0tip3",     // SSI_INDEX_P0TIP3
    "p0tip4",     // SSI_INDEX_P0TIP4
    "p0tnm",      // SSI_INDEX_P0TNM
    "p1br",       // SSI_INDEX_P1BR
    "p1sb",       // SSI_INDEX_P1SB
    "p1p",        // SSI_INDEX_P1P
    "p1bc",       // SSI_INDEX_P1BC
    "p1fc",       // SSI_INDEX_P1FC
    "p1tt",       // SSI_INDEX_P1TT
    "p1tlp",      // SSI_INDEX_P1TLP
    "p1trp",      // SSI_INDEX_P1TRP
    "p1tip",      // SSI_INDEX_P1TIP
    "p1tip1",     // SSI_INDEX_P1TIP1
    "p1tip2",     // SSI_INDEX_P1TIP2
    "p1tip3",     // SSI_INDEX_P1TIP3
    "p1tip4",     // SSI_INDEX_P1TIP4
    "p1tnm",      // SSI_INDEX_P1TNM
    "modname",    // SSI_INDEX_MODNAME
    "pnpport",    // SSI_INDEX_PNPPORT
    "disable",    // SSI_INDEX_DISABLE
    "dvars",      // SSI_INDEX_DVARS
    "p0vars",     // SSI_INDEX_P0VARS
    "p1vars",     // SSI_INDEX_P1VARS
    "modninp",    // SSI_INDEX_MODNINP
    "pnpinp",     // SSI_INDEX_PNPINP
    "p0tvars",    // SSI_INDEX_P0TVARS
    "p1tvars",    // SSI_INDEX_P1TVARS
    "p0ipvar",    // SSI_INDEX_P0IPVAR
    "p1ipvar",    // SSI_INDEX_P1IPVAR
    "ipvars",     // SSI_INDEX_IPVARS
    "snvars",     // SSI_INDEX_SNVARS
    "gwvars",     // SSI_INDEX_GWVARS
    "revision",   // SSI_INDEX_REVISION
    "p0prot",     // SSI_INDEX_P0PROT
    "p1prot",     // SSI_INDEX_P1PROT
    "teclimp0",    // SSI_INDEX_TECLIMP0
    "teclimp1",    // SSI_INDEX_TECLIMP1
    "teclimn0",    // SSI_INDEX_TECLIMN0
    "teclimn1",    // SSI_INDEX_TECLIMN1
    "teccur0",    // SSI_INDEX_TECCUR0
    "teccur1",    // SSI_INDEX_TECCUR1
    "tecvol0",    // SSI_INDEX_TECVOL0
    "tecvol1",    // SSI_INDEX_TECVOL1
    "tempset0",   // SSI_INDEX_TEMPSET0
    "tempset1",   // SSI_INDEX_TEMPSET1
    "tempact0",   // SSI_INDEX_TEMPACT0
    "tempact1",   // SSI_INDEX_TEMPACT1
    "tempwin0",   // SSI_INDEX_TEMPWIN0
    "tempwin1",   // SSI_INDEX_TEMPWIN1
	"clp0",       // SSI_INDEX_CLP0
	"clp1",       // SSI_INDEX_CLP1
	"cli0",       // SSI_INDEX_CLI0
	"cli1",       // SSI_INDEX_CLI1
	"cld0",       // SSI_INDEX_CLD0
	"cld1",       // SSI_INDEX_CLD1
    "t00",        // SSI_INDEX_T00
    "t01",        // SSI_INDEX_T01
    "beta0",      // SSI_INDEX_BETA0
    "beta1",      // SSI_INDEX_BETA1
    "ratio0",     // SSI_INDEX_RATIO0
    "ratio1",     // SSI_INDEX_RATIO1
    "tempmod0",   // SSI_INDEX_TEMPMOD0
    "tempmod1",   // SSI_INDEX_TEMPMOD1
    "pta0",       // SSI_INDEX_PTA0
    "pta1",       // SSI_INDEX_PTA1
    "ptb0",       // SSI_INDEX_PTB0
    "ptb1",       // SSI_INDEX_PTB1
    "swfreq0",    // SSI_INDEX_SWFREQ0
    "swfreq1",    // SSI_INDEX_SWFREQ1
    "rawtemp0",   // SSI_INDEX_RAWTEMP0
    "rawtemp1",   // SSI_INDEX_RAWTEMP1
    "maxvolt0",   // SSI_INDEX_MAXVOLT0
    "maxvolt1",   // SSI_INDEX_MAXVOLT1
    "onoff0",     // SSI_INDEX_ONOFF0
    "onoff1",     // SSI_INDEX_ONOFF1
    "onoff0R",    // SSI_INDEX_ONOFF0
    "onoff1R",    // SSI_INDEX_ONOFF1
	"mqttip",     // SSI_INDEX_MQTTIP
    "mqttconn"    // SSI_INDEX_MQTTCONN

};

//*****************************************************************************
//
//! The number of individual SSI tags that the HTTPD server can expect to
//! find in our configuration pages.
//
//*****************************************************************************
#define NUM_CONFIG_SSI_TAGS     (sizeof(g_pcConfigSSITags) / sizeof (char *))

//*****************************************************************************
//
//! Prototype for the function which handles requests for config.cgi.
//
//*****************************************************************************
static const char *ConfigCGIHandler(int iIndex, int iNumParams,
                                    char *pcParam[], char *pcValue[]);

//*****************************************************************************
//
//! Prototype for the function which handles requests for misc.cgi.
//
//*****************************************************************************
static const char *ConfigMiscCGIHandler(int iIndex, int iNumParams,
                                        char *pcParam[], char *pcValue[]);

//*****************************************************************************
//
//! Prototype for the function which handles requests for ip.cgi.
//
//*****************************************************************************
static const char *ConfigIPCGIHandler(int iIndex, int iNumParams,
                                      char *pcParam[], char *pcValue[]);


//*****************************************************************************
//
//! Prototype for the function which handles requests for teclim.cgi.
//
//*****************************************************************************

static const char *ConfigTecLimCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);

//*****************************************************************************
//
//! Prototype for the function which handles requests for tempset.cgi.
//
//*****************************************************************************

static const char *ConfigTempSetCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);

//*****************************************************************************
//
//! Prototype for the function which handles requests for tempwin.cgi.
//
//*****************************************************************************

static const char *ConfigTempWinCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
//*****************************************************************************
//
//! Prototype for the function which handles requests for clp.cgi.
//
//*****************************************************************************

static const char *ConfigCLPCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
//*****************************************************************************
//
//! Prototype for the function which handles requests for cli.cgi.
//
//*****************************************************************************

static const char *ConfigCLICGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
//*****************************************************************************
//
//! Prototype for the function which handles requests for cld.cgi.
//
//*****************************************************************************

static const char *ConfigCLDCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);

//*****************************************************************************
//
//! Prototype for the function which handles requests for soft_reset.cgi.
//
//*****************************************************************************

static const char *ConfigSoftResetCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);

//*****************************************************************************
//
//! Prototype for the function which handles requests for setmqtt.cgi.
//
//*****************************************************************************

static const char *ConfigSetMQTTCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);

//*****************************************************************************
//
//! Prototype for the function which handles requests for calibt0.cgi.
//
//*****************************************************************************

static const char *ConfigSetT0CGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);

//*****************************************************************************
//
//! Prototype for the function which handles requests for calibbeta.cgi.
//
//*****************************************************************************

static const char *ConfigSetBetaCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);

//*****************************************************************************
//
//! Prototype for the function which handles requests for calibratio.cgi.
//
//*****************************************************************************

static const char *ConfigSetRatioCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);

//*****************************************************************************
//
//! Prototype for the function which handles requests for calibmode.cgi.
//
//*****************************************************************************

static const char *ConfigSetTempModeCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);

//*****************************************************************************
//
//! Prototype for the function which handles requests for calibpta.cgi.
//
//*****************************************************************************

static const char *ConfigSetptACGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);

//*****************************************************************************
//
//! Prototype for the function which handles requests for calibptb.cgi.
//
//*****************************************************************************

static const char *ConfigSetptBCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);


//*****************************************************************************
//
//! Prototype for the function which handles requests for calibswfreq.cgi.
//
//*****************************************************************************

static const char *ConfigSetSwFreqCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);

//*****************************************************************************
//
//! Prototype for the function which handles requests for calibswfreq.cgi.
//
//*****************************************************************************

static const char *ConfigSetMaxVoltCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);

//*****************************************************************************
//
//! Prototype for the function which handles requests for turnonoff.cgi.
//
//*****************************************************************************

static const char *ConfigTurnOnOffCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);

//*****************************************************************************
//
//! Prototype for the function which handles requests for soft_reset.cgi.
//
//*****************************************************************************

static const char *ConfigSaveAllCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);

//*****************************************************************************
//
//! Prototype for the function which handles requests for defaults.cgi.
//
//*****************************************************************************
static const char *ConfigDefaultsCGIHandler(int iIndex, int iNumParams,
                                            char *pcParam[], char *pcValue[]);

//*****************************************************************************
//
//! Prototype for the main handler used to process server-side-includes for the
//! application's web-based configuration screens.
//
//*****************************************************************************
static uint16_t ConfigSSIHandler(int iIndex, char *pcInsert, int iInsertLen);

//*****************************************************************************
//
// CGI URI indices for each entry in the g_psConfigCGIURIs array.
//
//*****************************************************************************
#define CGI_INDEX_CONFIG        0
#define CGI_INDEX_MISC          1
#define CGI_INDEX_DEFAULTS      2
#define CGI_INDEX_IP            3
#define CGI_INDEX_TECLIM	    4
#define CGI_INDEX_TEMPSET       5
#define CGI_INDEX_TEMPWIN       6
#define CGI_INDEX_CLP           7
#define CGI_INDEX_CLI           8
#define CGI_INDEX_CLD           9
#define CGI_INDEX_SOFT_RESET    10
#define CGI_INDEX_SETMQTT       11
#define CGI_INDEX_SETT0         12
#define CGI_INDEX_SETBETA       13
#define CGI_INDEX_SETRATIO      14
#define CGI_INDEX_SETTEMPMODE   15
#define CGI_INDEX_SETPTA        16
#define CGI_INDEX_SETPTB        17
#define CGI_INDEX_SETSWFREQ     18
#define CGI_INDEX_SETMAXVOLT    19
#define CGI_INDEX_SAVEALL       20
#define CGI_INDEX_TURNONOFF     21

//*****************************************************************************
//
//! This array is passed to the HTTPD server to inform it of special URIs
//! that are treated as common gateway interface (CGI) scripts.  Each URI name
//! is defined along with a pointer to the function which is to be called to
//! process it.
//
//*****************************************************************************
static const tCGI g_psConfigCGIURIs[] =
{
    { "/config.cgi", ConfigCGIHandler },                // CGI_INDEX_CONFIG
    { "/misc.cgi", ConfigMiscCGIHandler },              // CGI_INDEX_MISC
    { "/defaults.cgi", ConfigDefaultsCGIHandler },      // CGI_INDEX_DEFAULTS
    { "/ip.cgi", ConfigIPCGIHandler },                  // CGI_INDEX_IP
    { "/teclim.cgi", ConfigTecLimCGIHandler },          // CGI_INDEX_TECLIM
    { "/tempset.cgi", ConfigTempSetCGIHandler },        // CGI_INDEX_TEMPSET
    { "/tempwin.cgi", ConfigTempWinCGIHandler },        // CGI_INDEX_TEMPWIN
    { "/clp.cgi", ConfigCLPCGIHandler },                // CGI_INDEX_CLP
    { "/cli.cgi", ConfigCLICGIHandler },                // CGI_INDEX_CLI
    { "/cld.cgi", ConfigCLDCGIHandler },			    // CGI_INDEX_CLD
    { "/soft_reset.cgi", ConfigSoftResetCGIHandler },   // CGI_INDEX_SOFT_RESET
    { "/setmqtt.cgi", ConfigSetMQTTCGIHandler },        // CGI_INDEX_SETMQTT
    { "/calibt0.cgi", ConfigSetT0CGIHandler },          // CGI_INDEX_SETT0
    { "/calibbeta.cgi", ConfigSetBetaCGIHandler },      // CGI_INDEX_SETBETA
    { "/calibratio.cgi", ConfigSetRatioCGIHandler },    // CGI_INDEX_SETRATIO
    { "/calibmode.cgi", ConfigSetTempModeCGIHandler },  // CGI_INDEX_SETTEMPMODE
    { "/calibpta.cgi", ConfigSetptACGIHandler },        // CGI_INDEX_SETPTTA
    { "/calibptb.cgi", ConfigSetptBCGIHandler },        // CGI_INDEX_SETPTB
    { "/calibswfreq.cgi", ConfigSetSwFreqCGIHandler},   // CGI_INDEX_SETSWFREQ
    { "/maxvolt.cgi", ConfigSetMaxVoltCGIHandler },     // CGI_INDEX_SETMAXVOLT
    { "/saveall.cgi", ConfigSaveAllCGIHandler },        // CGI_INDEX_SAVEALL
    { "/turnonoff.cgi", ConfigTurnOnOffCGIHandler }     // CGI_INDEX_TUNRONOFF
	
};

//*****************************************************************************
//
//! The number of individual CGI URIs that are used by our configuration
//! web pages.
//
//*****************************************************************************
#define NUM_CONFIG_CGI_URIS     (sizeof(g_psConfigCGIURIs) / sizeof(tCGI))

//*****************************************************************************
//
//! The file sent back to the browser by default following completion of any
//! of our CGI handlers.
//
//*****************************************************************************
#define DEFAULT_CGI_RESPONSE    "/index.htm"

//*****************************************************************************
//
//! The file sent back to the browser in cases where a parameter error is
//! detected by one of the CGI handlers.  This should only happen if someone
//! tries to access the CGI directly via the browser command line and doesn't
//! enter all the required parameters alongside the URI.
//
//*****************************************************************************
#define PARAM_ERROR_RESPONSE    "/perror.shtm"

//*****************************************************************************
//
//! The file sent back to the browser to signal that the IP address of the
//! device is about to change and that the web server is no longer operating.
//
//*****************************************************************************
#define IP_UPDATE_RESPONSE      "/ipchg.shtm"

//*****************************************************************************
//
//! The URI of the ``General Info'' page offered by the web server.
//
//*****************************************************************************
#define GEN_PAGE_URI           "/gen_info.shtm"

//*****************************************************************************
//
//! The URI of the ``Tec0'' page offered by the web server.
//
//*****************************************************************************
#define TEC0_PAGE_URI           "/tec0.shtm"

//*****************************************************************************
//
//! The URI of the ``Tec1'' page offered by the web server.
//
//*****************************************************************************
#define TEC1_PAGE_URI           "/tec1.shtm"

//*****************************************************************************
//
//! The URI of the ``Temp0'' page offered by the web server.
//
//*****************************************************************************
#define TEMP0_PAGE_URI           "/temp0.shtm"

//*****************************************************************************
//
//! The URI of the ``Temp1'' page offered by the web server.
//
//*****************************************************************************
#define TEMP1_PAGE_URI           "/temp1.shtm"

//*****************************************************************************
//
//! The URI of the ``CL0'' page offered by the web server.
//
//*****************************************************************************
#define CL0_PAGE_URI           "/cl0.shtm"

//*****************************************************************************
//
//! The URI of the ``CL1'' page offered by the web server.
//
//*****************************************************************************
#define CL1_PAGE_URI           "/cl1.shtm"

//*****************************************************************************
//
//! The URI of the ``CL1'' page offered by the web server.
//
//*****************************************************************************
#define STATUS_PAGE_URI           "/status.shtm"

//*****************************************************************************
//
//! The URI of the ``MQTT'' page offered by the web server.
//
//*****************************************************************************
#define MQTT_PAGE_URI           "/mqttipchg.shtm"

//*****************************************************************************
//
//! The URI of the ``CALIB0'' page offered by the web server.
//
//*****************************************************************************
#define CALIB0_PAGE_URI           "/calib0.shtm"

//*****************************************************************************
//
//! The URI of the ``CALIB1'' page offered by the web server.
//
//*****************************************************************************
#define CALIB1_PAGE_URI           "/calib1.shtm"

//*****************************************************************************
//
//! The URI of the ``CALIB1'' page offered by the web server.
//
//*****************************************************************************
#define SAVED_PAGE_URI           "/saved.shtm"

//*****************************************************************************
//
// Strings used for format JavaScript parameters for use by the configuration
// web pages.
//
//*****************************************************************************
#define JAVASCRIPT_HEADER                                                     \
    "<script type='text/javascript' language='JavaScript'><!--\n"
#define SER_JAVASCRIPT_VARS                                                   \
    "var br = %d;\n"                                                          \
    "var sb = %d;\n"                                                          \
    "var bc = %d;\n"                                                          \
    "var fc = %d;\n"                                                          \
    "var par = %d;\n"
#define TN_JAVASCRIPT_VARS                                                    \
    "var tt = %d;\n"                                                          \
    "var tlp = %d;\n"                                                         \
    "var trp = %d;\n"                                                         \
    "var tnm = %d;\n"                                                         \
    "var tnp = %d;\n"
#define TIP_JAVASCRIPT_VARS                                                   \
    "var tip1 = %d;\n"                                                        \
    "var tip2 = %d;\n"                                                        \
    "var tip3 = %d;\n"                                                        \
    "var tip4 = %d;\n"
#define IP_JAVASCRIPT_VARS                                                    \
    "var staticip = %d;\n"                                                    \
    "var sip1 = %d;\n"                                                        \
    "var sip2 = %d;\n"                                                        \
    "var sip3 = %d;\n"                                                        \
    "var sip4 = %d;\n"
#define SUBNET_JAVASCRIPT_VARS                                                \
    "var mip1 = %d;\n"                                                        \
    "var mip2 = %d;\n"                                                        \
    "var mip3 = %d;\n"                                                        \
    "var mip4 = %d;\n"
#define GW_JAVASCRIPT_VARS                                                    \
    "var gip1 = %d;\n"                                                        \
    "var gip2 = %d;\n"                                                        \
    "var gip3 = %d;\n"                                                        \
    "var gip4 = %d;\n"
#define JAVASCRIPT_FOOTER                                                     \
    "//--></script>\n"

//*****************************************************************************
//
//! Structure used in mapping numeric IDs to human-readable strings.
//
//*****************************************************************************
typedef struct
{
    //
    //! A human readable string related to the identifier found in the ui8Id
    //! field.
    //
    const char *pcString;

    //
    //! An identifier value associated with the string held in the pcString
    //! field.
    //
    uint8_t ui8Id;
}
tStringMap;

//*****************************************************************************
//
//! The array used to map between parity identifiers and their human-readable
//! equivalents.
//
//*****************************************************************************
static const tStringMap g_psParityMap[] =
{
   { "None", SERIAL_PARITY_NONE },
   { "Odd", SERIAL_PARITY_ODD },
   { "Even", SERIAL_PARITY_EVEN },
   { "Mark", SERIAL_PARITY_MARK },
   { "Space", SERIAL_PARITY_SPACE }
};

#define NUM_PARITY_MAPS         (sizeof(g_psParityMap) / sizeof(tStringMap))

//*****************************************************************************
//
//! The array used to map between flow control identifiers and their
//! human-readable equivalents.
//
//*****************************************************************************
static const tStringMap g_psFlowControlMap[] =
{
   { "None", SERIAL_FLOW_CONTROL_NONE },
   { "Hardware", SERIAL_FLOW_CONTROL_HW }
};

#define NUM_FLOW_CONTROL_MAPS   (sizeof(g_psFlowControlMap) / \
                                 sizeof(tStringMap))

//*****************************************************************************
//
//! This structure instance contains the factory-default set of configuration
//! parameters for S2E module.
//
//*****************************************************************************
static const tConfigParameters g_sParametersFactory =
{
    //
    // The sequence number (ui8SequenceNum); this value is not important for
    // the copy in SRAM.
    //
    0,

    //
    // The CRC (ui8CRC); this value is not important for the copy in SRAM.
    //
    0,

    //
    // The parameter block version number (ui8Version).
    //
    0,

    //
    // Flags (ui8Flags)
    //
    0,

    //
    // Reserved (ui8Reserved1).
    //
    {
        0, 0, 0, 0
    },

    //
    // Port Parameter Array.
    //
    {
        //
        // Parameters for Port 0 (sPort[0]).
        //
        {
            //
            // The baud rate (ui32BaudRate).
            //
            115200,

            //
            // The number of data bits.
            //
            8,

            //
            // The parity (NONE).
            //
            SERIAL_PARITY_NONE,

            //
            // The number of stop bits.
            //
            1,

            //
            // The flow control (NONE).
            //
            SERIAL_FLOW_CONTROL_NONE,

            //
            // The telnet session timeout (ui32TelnetTimeout).
            //
            0,

            //
            // The telnet session listen or local port number
            // (ui16TelnetLocalPort).
            //
            23,

            //
            // The telnet session remote port number (when in client mode).
            //
            23,

            //
            // The IP address to which a connection will be made when in telnet
            // client mode.  This defaults to an invalid address since it's not
            // sensible to have a default value here.
            //
            0x00000000,

            //
            // Flags indicating the operating mode for this port.
            //
            PORT_TELNET_SERVER,

            //
            // Reserved (ui8Reserved0).
            //
            {
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
            },
        },

        //
        // Parameters for Port 1 (sPort[1]).
        //
        {
            //
            // The baud rate (ui32BaudRate).
            //
            115200,

            //
            // The number of data bits.
            //
            8,

            //
            // The parity (NONE).
            //
            SERIAL_PARITY_NONE,

            //
            // The number of stop bits.
            //
            1,

            //
            // The flow control (NONE).
            //
            SERIAL_FLOW_CONTROL_NONE,

            //
            // The telnet session timeout (ui32TelnetTimeout).
            //
            0,

            //
            // The telnet session listen or local port number
            // (ui16TelnetLocalPort).
            //
            26,

            //
            // The telnet session remote port number (when in client mode).
            //
            23,

            //
            // The IP address to which a connection will be made when in telnet
            // client mode.  This defaults to an invalid address since it's not
            // sensible to have a default value here.
            //
            0x00000000,

            //
            // Flags indicating the operating mode for this port.
            //
            PORT_TELNET_SERVER,

            //
            // Reserved (ui8Reserved0).
            //
            {
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
            },
        },
    },

    //
    // Module Name (ui8ModName).
    //
    {
        'S','i','n','a','r','a',' ','T','h','e',
        'r','m','o','s','t','a','t', 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    },

    //
    // Static IP address (used only if indicated in ui8Flags).
    //
    0x00000000,

    //
    // Default gateway IP address (used only if static IP is in use).
    //
    0x00000000,

    //
    // Subnet mask (used only if static IP is in use).
    //
    0xFFFFFF00,

    //
    // MQTT IP
    //
    0xC317FEA9, //IP in hex

    //pid
    0,
    0,
    -20,
    -20,
    0,
    0,

    //settemp
    23000, //div 1000 = 23
    23000, //div 1000 = 23

    //t0
    25000, //div 1000 = 25
    25000, //div 1000 = 25

    //beta
    -696000, //div 1000 = -696
    -696000, //div 1000 = -696

    //ratio
    100, //div 1000 = 0.1
    100, //div 1000 = 0.1

    //tempmode
    0,
    0,

    //ptA0, ptA1
    3908300, //div 1000 = 3908.3
    3908300, //div 1000 = 3908.3

    //ptB0, ptB1
    -577500, //div 1000 = -577.5
    -577500, //div 1000 = -577.5

    //swwfreq
    500,
    500,

    //maxcurr
    3000,
    3000,
    3000,
    3000,

    //maxvolt
    5000,
    5000,

    //tempwindow
    100,
    100,

    //
    // ui8Reserved2 (compiler will pad to the full length)
    //
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }
};

//*****************************************************************************
//
//! This structure instance contains the run-time set of configuration
//! parameters for S2E module.  This is the active parameter set and may
//! contain changes that are not to be committed to flash.
//
//*****************************************************************************
tConfigParameters g_sParameters;

//*****************************************************************************
//
//! This structure instance points to the most recently saved parameter block
//! in flash.  It can be considered the default set of parameters.
//
//*****************************************************************************
const tConfigParameters *g_psDefaultParameters;

//*****************************************************************************
//
//! This structure contains the latest set of parameter committed to flash
//! and is used by the configuration pages to store changes that are to be
//! written back to flash.  Note that g_sParameters may contain other changes
//! which are not to be written so we can't merely save the contents of the
//! active parameter block if the user requests some change to the defaults.
//
//*****************************************************************************
static tConfigParameters g_sWorkingDefaultParameters;

//*****************************************************************************
//
//! This structure instance points to the factory default set of parameters in
//! flash memory.
//
//*****************************************************************************
const tConfigParameters *const g_psFactoryParameters = &g_sParametersFactory;

//*****************************************************************************
//
//! Variables holding values to read
//
//*****************************************************************************
uint32_t ui32RawTemp0;
uint32_t ui32RawTemp1;
int32_t i32ActTemp0;
int32_t i32ActTemp1;
int16_t onoff0;
int16_t onoff1;

//*****************************************************************************
//
//! Loads the S2E parameter block from factory-default table.
//!
//! This function is called to load the factory default parameter block.
//!
//! \return None.
//
//*****************************************************************************
void
ConfigLoadFactory(void)
{
    //
    // Copy the factory default parameter set to the active and working
    // parameter blocks.
    //
    g_sParameters = g_sParametersFactory;
    g_sWorkingDefaultParameters = g_sParametersFactory;
}

//*****************************************************************************
//
//! Save all parameters
//!
//!
//! \return None.
//
//*****************************************************************************
void ConfigSaveAll(void)
{
    g_sWorkingDefaultParameters = g_sParameters;
    ConfigSave();
}

//*****************************************************************************
//
//! Loads the S2E parameter block from flash.
//!
//! This function is called to load the most recently saved parameter block
//! from flash.
//!
//! \return None.
//
//*****************************************************************************
void
ConfigLoad(void)
{
    uint8_t *pui8Buffer;

    //EEPROMMassErase();
    //
    // Get a pointer to the latest parameter block in flash.
    //
    pui8Buffer = EEPROMPBGet();

    //
    // See if a parameter block was found in flash.
    //
    if(pui8Buffer)
    {
        //
        // A parameter block was found so copy the contents to both our
        // active parameter set and the working default set.
        //
        g_sParameters = *(tConfigParameters *)pui8Buffer;
        g_sWorkingDefaultParameters = g_sParameters;
    }
}

//*****************************************************************************
//
//! Saves the S2E parameter block to flash.
//!
//! This function is called to save the current S2E configuration parameter
//! block to flash memory.
//!
//! \return None.
//
//*****************************************************************************
void
ConfigSave(void)
{
    uint8_t *pui8Buffer;

    //
    // Save the working defaults parameter block to flash.
    //
    EEPROMPBSave((uint8_t *)&g_sWorkingDefaultParameters);

    //
    // Get the pointer to the most recenly saved buffer.
    // (should be the one we just saved).
    //
    pui8Buffer = EEPROMPBGet();

    //
    // Update the default parameter pointer.
    //
    if(pui8Buffer)
    {
        g_psDefaultParameters = (tConfigParameters *)pui8Buffer;
    }
    else
    {
        g_psDefaultParameters = (tConfigParameters *)g_psFactoryParameters;
    }
}

//*****************************************************************************
//
//! Initializes the configuration parameter block.
//!
//! This function initializes the configuration parameter block.  If the
//! version number of the parameter block stored in flash is older than
//! the current revision, new parameters will be set to default values as
//! needed.
//!
//! \return None.
//
//*****************************************************************************
void
ConfigInit(void)
{
    uint8_t *pui8Buffer;
    char msg[32];
    uint32_t msg_len;

    onoff0 = 0;
    onoff1 = 0;

    //
    // Verify that the parameter block structure matches the FLASH parameter
    // block size.
    //
    ASSERT(sizeof(tConfigParameters) == FLASH_PB_SIZE)

    //
    // Initialize the flash parameter block driver.
    //
    if(EEPROMPBInit(EEPROM_PB_START, EEPROM_PB_SIZE))
    {
        DEBUG_MSG("EEPROM Parameter Block Initalization Failed!");
        while(true)
        {
        }
    }

    //
    // First, load the factory default values.
    //
    ConfigLoadFactory();

    //Watchdog handling
    if(SysCtlResetCauseGet() & SYSCTL_CAUSE_WDOG0) {
      msg_len = usprintf(msg, "WatchDog Reset\n\r");
      UARTSend(msg, msg_len);

    } else {
        //
        // If available, load the latest non-volatile set of values.
        //
        ConfigLoad();
    }

    SysCtlResetCauseClear(SYSCTL_CAUSE_WDOG0 | SYSCTL_CAUSE_EXT);


    //
    // Get the pointer to the most recently saved buffer.
    //
    pui8Buffer = EEPROMPBGet();

    //
    // Update the default parameter pointer.
    //
    if(pui8Buffer)
    {
        g_psDefaultParameters = (tConfigParameters *)pui8Buffer;
    }
    else
    {
        g_psDefaultParameters = (tConfigParameters *)g_psFactoryParameters;
    }

}

//*****************************************************************************
//
//! Configures HTTPD server SSI and CGI capabilities for our configuration
//! forms.
//!
//! This function informs the HTTPD server of the server-side-include tags
//! that we will be processing and the special URLs that are used for
//! CGI processing for the web-based configuration forms.
//!
//! \return None.
//
//*****************************************************************************
void
ConfigWebInit(void)
{
    //
    // Pass our tag information to the HTTP server.
    //
    http_set_ssi_handler(ConfigSSIHandler, g_pcConfigSSITags,
                         NUM_CONFIG_SSI_TAGS);

    //
    // Pass our CGI handlers to the HTTP server.
    //
    http_set_cgi_handlers(g_psConfigCGIURIs, NUM_CONFIG_CGI_URIS);
}

//*****************************************************************************
//
//! \internal
//!
//! Searches a mapping array to find a human-readable description for a
//! given identifier.
//!
//! \param psMap points to an array of \e tStringMap structures which contain
//! the mappings to be searched for the provided identifier.
//! \param ui32Entries contains the number of map entries in the \e psMap
//! array.
//! \param ui8Id is the identifier whose description is to be returned.
//!
//! This function scans the given array of ID/string maps and returns a pointer
//! to the string associated with the /e ui8Id parameter passed.  If the
//! identifier is not found in the map array, a pointer to ``**UNKNOWN**'' is
//! returned.
//!
//! \return Returns a pointer to an ASCII string describing the identifier
//! passed, if found, or ``**UNKNOWN**'' if not found.
//
//*****************************************************************************
static const char *
ConfigMapIdToString(const tStringMap *psMap, uint32_t ui32Entries,
                    uint8_t ui8Id)
{
    uint32_t ui32Loop;

    //
    // Check each entry in the map array looking for the ID number we were
    // passed.
    //
    for(ui32Loop = 0; ui32Loop < ui32Entries; ui32Loop++)
    {
        //
        // Does this map entry match?
        //
        if(psMap[ui32Loop].ui8Id == ui8Id)
        {
            //
            // Yes - return the IDs description string.
            //
            return(psMap[ui32Loop].pcString);
        }
    }

    //
    // If we drop out, the ID passed was not found in the map array so return
    // a default "**UNKNOWN**" string.
    //
    return("**UNKNOWN**");
}

//*****************************************************************************
//
//! \internal
//!
//! Updates all parameters associated with a single port.
//!
//! \param ui32Port specifies which of the two supported ports to update.
//! Valid values are 0 and 1.
//!
//! This function changes the serial and telnet configuration to match the
//! values stored in g_sParameters.sPort for the supplied port.  On exit, the
//! new parameters will be in effect and g_sParameters.sPort will have been
//! updated to show the actual parameters in effect (in case any of the
//! supplied parameters are not valid or the actual hardware values differ
//! slightly from the requested value).
//!
//! \return None.
//
//*****************************************************************************
void
ConfigUpdatePortParameters(uint32_t ui32Port, bool bSerial, bool bTelnet)
{
    //
    // Do we have to update the telnet settings?  Note that we need to do this
    // first since the act of initiating a telnet connection resets the serial
    // port settings to defaults.
    //
    if(bTelnet)
    {
        //
        // Close any existing connection and shut down the server if required.
        //
        TelnetClose(ui32Port);

        //
        // Are we to operate as a telnet server?
        //
        if((g_sParameters.sPort[ui32Port].ui8Flags & PORT_FLAG_TELNET_MODE) ==
           PORT_TELNET_SERVER)
        {
            //
            // Yes - start listening on the required port.
            //
            TelnetListen(g_sParameters.sPort[ui32Port].ui16TelnetLocalPort,
                         ui32Port);
        }
        else
        {
            //
            // No - we are a client so initiate a connection to the desired
            // IP address using the configured ports.
            //
            TelnetOpen(g_sParameters.sPort[ui32Port].ui32TelnetIPAddr,
                       g_sParameters.sPort[ui32Port].ui16TelnetRemotePort,
                       g_sParameters.sPort[ui32Port].ui16TelnetLocalPort,
                       ui32Port);
        }
    }

    //
    // Do we need to update the serial port settings?  We do this if we are
    // told that the serial settings changed or if we just reconfigured the
    // telnet settings (which resets the serial port parameters to defaults as
    // a side effect).
    //
    if(bSerial || bTelnet)
    {
        SerialSetCurrent(ui32Port);
    }
}

//*****************************************************************************
//
//! \internal
//!
//! Performs any actions necessary in preparation for a change of IP address.
//!
//! This function is called before ConfigUpdateIPAddress in preparation for a
//! change of IP address or switch between DHCP and StaticIP.
//!
//! \return None.
//
//*****************************************************************************
void
ConfigPreUpdateIPAddress(void)
{
}

//*****************************************************************************
//
//! \internal
//!
//! Sets the IP address selection mode and associated parameters.
//!
//! This function ensures that the IP address selection mode (static IP or
//! DHCP/AutoIP) is set according to the parameters stored in g_sParameters.
//!
//! \return None.
//
//*****************************************************************************
void
ConfigUpdateIPAddress(void)
{
    //
    // Change to static/dynamic based on the current settings in the
    // global parameter block.
    //
    if((g_sParameters.ui8Flags & CONFIG_FLAG_STATICIP) == CONFIG_FLAG_STATICIP)
    {
        lwIPNetworkConfigChange(g_sParameters.ui32StaticIP,
                                g_sParameters.ui32SubnetMask,
                                g_sParameters.ui32GatewayIP,
                                IPADDR_USE_STATIC);
    }
    else
    {
        lwIPNetworkConfigChange(0, 0, 0, IPADDR_USE_DHCP);
    }
}

//*****************************************************************************
//
//! \internal
//!
//! Performs changes as required to apply all active parameters to the system.
//!
//! \param bUpdateIP is set to \e true to update parameters related to the
//! S2E board's IP address. If \e false, the IP address will remain unchanged.
//!
//! This function ensures that the system configuration matches the values in
//! the current, active parameter block.  It is called after the parameter
//! block has been reset to factory defaults.
//!
//! \return None.
//
//*****************************************************************************
void
ConfigUpdateAllParameters(bool bUpdateIP)
{
    //
    // Have we been asked to update the IP address along with the other
    // parameters?
    //
    if(bUpdateIP)
    {
        //
        // Yes - update the IP address selection parameters.
        //
        ConfigPreUpdateIPAddress();
        ConfigUpdateIPAddress();
    }

    //
    // Update the parameters for each of the individual ports.
    //
    ConfigUpdatePortParameters(0, true, true);
    ConfigUpdatePortParameters(1, true, true);

    //
    // Update the name that the board publishes to the locator/finder app.
    //
    LocatorAppTitleSet((char *)g_sParameters.ui8ModName);
}

//*****************************************************************************
//
//! \internal
//!
//! Searches the list of parameters passed to a CGI handler and returns the
//! index of a given parameter within that list.
//!
//! \param pcToFind is a pointer to a string containing the name of the
//! parameter that is to be found.
//! \param pcParam is an array of character pointers, each containing the name
//! of a single parameter as encoded in the URI requesting the CGI.
//! \param iNumParams is the number of elements in the pcParam array.
//!
//! This function searches an array of parameters to find the string passed in
//! \e pcToFind.  If the string is found, the index of that string within the
//! \e pcParam array is returned, otherwise -1 is returned.
//!
//! \return Returns the index of string \e pcToFind within array \e pcParam
//! or -1 if the string does not exist in the array.
//
//*****************************************************************************
static int
ConfigFindCGIParameter(const char *pcToFind, char *pcParam[], int iNumParams)
{
    int iLoop;

    //
    // Scan through all the parameters in the array.
    //
    for(iLoop = 0; iLoop < iNumParams; iLoop++)
    {
        //
        // Does the parameter name match the provided string?
        //
        if(strcmp(pcToFind, pcParam[iLoop]) == 0)
        {
            //
            // We found a match - return the index.
            //
            return(iLoop);
        }
    }

    //
    // If we drop out, the parameter was not found.
    //
    return(-1);
}

static bool
ConfigIsValidHexDigit(const char cDigit)
{
    if(((cDigit >= '0') && (cDigit <= '9')) ||
       ((cDigit >= 'a') && (cDigit <= 'f')) ||
       ((cDigit >= 'A') && (cDigit <= 'F')))
    {
        return(true);
    }
    else
    {
        return(false);
    }
}

static uint8_t
ConfigHexDigit(const char cDigit)
{
    if((cDigit >= '0') && (cDigit <= '9'))
    {
        return(cDigit - '0');
    }
    else
    {
        if((cDigit >= 'a') && (cDigit <= 'f'))
        {
            return((cDigit - 'a') + 10);
        }
        else
        {
            if((cDigit >= 'A') && (cDigit <= 'F'))
            {
                return((cDigit - 'A') + 10);
            }
        }
    }

    //
    // If we get here, we were passed an invalid hex digit so return 0xFF.
    //
    return(0xFF);
}

//*****************************************************************************
//
//! \internal
//!
//! Decodes a single %xx escape sequence as an ASCII character.
//!
//! \param pcEncoded points to the ``%'' character at the start of a three
//! character escape sequence which represents a single ASCII character.
//! \param pcDecoded points to a byte which will be written with the decoded
//! character assuming the escape sequence is valid.
//!
//! This function decodes a single escape sequence of the form ``%xy'' where
//! x and y represent hexadecimal digits.  If each digit is a valid hex digit,
//! the function writes the decoded character to the pcDecoded buffer and
//! returns true, else it returns false.
//!
//! \return Returns \b true on success or \b false if pcEncoded does not point
//! to a valid escape sequence.
//
//*****************************************************************************
static bool
ConfigDecodeHexEscape(const char *pcEncoded, char *pcDecoded)
{
    if((pcEncoded[0] != '%') || !ConfigIsValidHexDigit(pcEncoded[1]) ||
       !ConfigIsValidHexDigit(pcEncoded[2]))
    {
        return(false);
    }
    else
    {
        *pcDecoded = ((ConfigHexDigit(pcEncoded[1]) * 16) +
                      ConfigHexDigit(pcEncoded[2]));
        return(true);
    }
}

//*****************************************************************************
//
//! \internal
//!
//! Encodes a string for use within an HTML tag, escaping non alphanumeric
//! characters.
//!
//! \param pcDecoded is a pointer to a null terminated ASCII string.
//! \param pcEncoded is a pointer to a storage for the encoded string.
//! \param ui32Len is the number of bytes of storage pointed to by pcEncoded.
//!
//! This function encodes a string, adding escapes in place of any special,
//! non-alphanumeric characters.  If the encoded string is too long for the
//! provided output buffer, the output will be truncated.
//!
//! \return Returns the number of characters written to the output buffer
//! not including the terminating NULL.
//
//*****************************************************************************
static uint32_t
ConfigEncodeFormString(const char *pcDecoded, char *pcEncoded,
                       uint32_t ui32Len)
{
    uint32_t ui32Loop;
    uint32_t ui32Count;

    //
    // Make sure we were not passed a tiny buffer.
    //
    if(ui32Len <= 1)
    {
        return(0);
    }

    //
    // Initialize our output character counter.
    //
    ui32Count = 0;

    //
    // Step through each character of the input until we run out of data or
    // space to put our output in.
    //
    for(ui32Loop = 0;
        pcDecoded[ui32Loop] && (ui32Count < (ui32Len - 1));
        ui32Loop++)
    {
        switch(pcDecoded[ui32Loop])
        {
            //
            // Pass most characters without modification.
            //
            default:
            {
                pcEncoded[ui32Count++] = pcDecoded[ui32Loop];
                break;
            }

            case '\'':
            {
                ui32Count += usnprintf(&pcEncoded[ui32Count],
                                       (ui32Len - ui32Count),
                                       "&#39;");
                break;
            }
        }
    }

    return(ui32Count);
}

//*****************************************************************************
//
//! \internal
//!
//! Decodes a string encoded as part of an HTTP URI.
//!
//! \param pcEncoded is a pointer to a null terminated string encoded as per
//! RFC1738, section 2.2.
//! \param pcDecoded is a pointer to storage for the decoded, null terminated
//! string.
//! \param ui32Len is the number of bytes of storage pointed to by pcDecoded.
//!
//! This function decodes a string which has been encoded using the method
//! described in RFC1738, section 2.2 for URLs.  If the decoded string is too
//! long for the provided output buffer, the output will be truncated.
//!
//! \return Returns the number of character written to the output buffer, not
//! including the terminating NULL.
//
//*****************************************************************************
static uint32_t
ConfigDecodeFormString(const  char *pcEncoded, char *pcDecoded,
                       uint32_t ui32Len)
{
    uint32_t ui32Loop;
    uint32_t ui32Count;
    bool bValid;

    ui32Count = 0;
    ui32Loop = 0;

    //
    // Keep going until we run out of input or fill the output buffer.
    //
    while(pcEncoded[ui32Loop] && (ui32Count < (ui32Len - 1)))
    {
        switch(pcEncoded[ui32Loop])
        {
            //
            // '+' in the encoded data is decoded as a space.
            //
            case '+':
            {
                pcDecoded[ui32Count++] = ' ';
                ui32Loop++;
                break;
            }

            //
            // '%' in the encoded data indicates that the following 2
            // characters indicate the hex ASCII code of the decoded character.
            //
            case '%':
            {
                if(pcEncoded[ui32Loop + 1] && pcEncoded[ui32Loop + 2])
                {
                    //
                    // Decode the escape sequence.
                    //
                    bValid = ConfigDecodeHexEscape(&pcEncoded[ui32Loop],
                                                   &pcDecoded[ui32Count]);

                    //
                    // If the escape sequence was valid, move to the next
                    // output character.
                    //
                    if(bValid)
                    {
                        ui32Count++;
                    }

                    //
                    // Skip past the escape sequence in the encoded string.
                    //
                    ui32Loop += 3;
                }
                else
                {
                    //
                    // We reached the end of the string partway through an
                    // escape sequence so just ignore it and return the number
                    // of decoded characters found so far.
                    //
                    pcDecoded[ui32Count] = '\0';
                    return(ui32Count);
                }
                break;
            }

            //
            // For all other characters, just copy the input to the output.
            //
            default:
            {
                pcDecoded[ui32Count++] = pcEncoded[ui32Loop++];
                break;
            }
        }
    }

    //
    // Terminate the string and return the number of characters we decoded.
    //
    pcDecoded[ui32Count] = '\0';
    return(ui32Count);
}

//*****************************************************************************
//
//! \internal
//!
//! Ensures that a string passed represents a valid decimal number and,
//! if so, converts that number to a long.
//!
//! \param pcValue points to a null terminated string which should contain an
//! ASCII representation of a decimal number.
//! \param pi32Value points to storage which will receive the number
//! represented by pcValue assuming the string is a valid decimal number.
//!
//! This function determines whether or not a given string represents a valid
//! decimal number and, if it does, converts the string into a decimal number
//! which is returned to the caller.
//!
//! \return Returns \b true if the string is a valid representation of a
//! decimal number or \b false if not.

//*****************************************************************************
bool
ConfigCheckDecimalParam(const char *pcValue, int32_t *pi32Value)
{
    uint32_t ui32Loop;
    bool bStarted;
    bool bFinished;
    bool bNeg;
    int32_t i32Accum;

    //
    // Check that the string is a valid decimal number.
    //
    bStarted = false;
    bFinished = false;
    bNeg = false;
    ui32Loop = 0;
    i32Accum = 0;

    while(pcValue[ui32Loop])
    {
        //
        // Ignore whitespace before the string.
        //
        if(!bStarted)
        {
            if((pcValue[ui32Loop] == ' ') || (pcValue[ui32Loop] == '\t'))
            {
                ui32Loop++;
                continue;
            }

            //
            // Ignore a + or - character as long as we have not started.
            //
            if(pcValue[ui32Loop] == '-')
            {
                //
                // If the string starts with a '-', remember to negate the
                // result.
                //
                bNeg = true;
                bStarted = true;
                ui32Loop++;
            } else if(pcValue[ui32Loop] == '%' && pcValue[ui32Loop+1] == '2' && pcValue[ui32Loop+2] == 'B')
            {
                //
                // If the string starts with a '-', remember to negate the
                // result.
                //
                bStarted = true;
                ui32Loop = ui32Loop + 3;
            }
            else
            {
                //
                // We found something other than whitespace or a sign character
                // so we start looking for numerals now.
                //
                bStarted = true;
            }
        }

        if(bStarted)
        {
            if(!bFinished)
            {
                //
                // We expect to see nothing other than valid digit characters
                // here.
                //
                if((pcValue[ui32Loop] >= '0') && (pcValue[ui32Loop] <= '9'))
                {
                    i32Accum = (i32Accum * 10) + (pcValue[ui32Loop] - '0');
                }
                else
                {
                    //
                    // Have we hit whitespace?  If so, check for no more
                    // characters until the end of the string.
                    //
                    if((pcValue[ui32Loop] == ' ') ||
                       (pcValue[ui32Loop] == '\t'))
                    {
                        bFinished = true;
                    }
                    else
                    {
                        //
                        // We got something other than a digit or whitespace so
                        // this makes the string invalid as a decimal number.
                        //
                        return(false);
                    }
                }
            }
            else
            {
                //
                // We are scanning for whitespace until the end of the string.
                //
                if((pcValue[ui32Loop] != ' ') && (pcValue[ui32Loop] != '\t'))
                {
                    //
                    // We found something other than whitespace so the string
                    // is not valid.
                    //
                    return(false);
                }
            }

            //
            // Move on to the next character in the string.
            //
            ui32Loop++;
        }
    }

    //
    // If we drop out of the loop, the string must be valid.  All we need to do
    // now is negate the accumulated value if the string started with a '-'.
    //
    *pi32Value = bNeg ? -i32Accum : i32Accum;
    return(true);
}

//*****************************************************************************
//
//! \internal
//!
//! Searches the list of parameters passed to a CGI handler for a parameter
//! with the given name and, if found, reads the parameter value as a decimal
//! number.
//!
//! \param pcName is a pointer to a string containing the name of the
//! parameter that is to be found.
//! \param pcParam is an array of character pointers, each containing the name
//! of a single parameter as encoded in the URI requesting the CGI.
//! \param iNumParams is the number of elements in the pcParam array.
//! \param pcValues is an array of values associated with each parameter from
//! the pcParam array.
//! \param pbError is a pointer that will be written to \b true if there is any
//! error during the parameter parsing process (parameter not found, value is
//! not a valid decimal number).
//!
//! This function searches an array of parameters to find the string passed in
//! \e pcName.  If the string is found, the corresponding parameter value is
//! read from array pcValues and checked to make sure that it is a valid
//! decimal number.  If so, the number is returned.  If any error is detected,
//! parameter \e pbError is written to \b true.  Note that \e pbError is NOT
//! written if the parameter is successfully found and validated.  This is to
//! allow multiple parameters to be parsed without the caller needing to check
//! return codes after each individual call.
//!
//! \return Returns the value of the parameter or 0 if an error is detected (in
//! which case \e *pbError will be \b true).
//
//*****************************************************************************
static int32_t
ConfigGetCGIParam(const char *pcName, char *pcParams[], char *pcValue[],
                  int iNumParams, bool *pbError)
{
    int iParam;
    int32_t i32Value;
    bool bRetcode;

    //
    // Is the parameter we are looking for in the list?
    //
    i32Value = 0;
    iParam = ConfigFindCGIParameter(pcName, pcParams, iNumParams);
    if(iParam != -1)
    {
        //
        // We found the parameter so now get its value.
        //
        bRetcode = ConfigCheckDecimalParam(pcValue[iParam], &i32Value);
        if(bRetcode)
        {
            //
            // All is well - return the parameter value.
            //
            return(i32Value);
        }
    }

    //
    // If we reach here, there was a problem so return 0 and set the error
    // flag.
    //
    *pbError = true;
    return(0);
}

//*****************************************************************************
//
//! \internal
//!
//! Searches the list of parameters passed to a CGI handler for 4 parameters
//! representing an IP address and extracts the IP address defined by them.
//!
//! \param pcName is a pointer to a string containing the base name of the IP
//! address parameters.
//! \param pcParam is an array of character pointers, each containing the name
//! of a single parameter as encoded in the URI requesting the CGI.
//! \param iNumParams is the number of elements in the pcParam array.
//! \param pcValues is an array of values associated with each parameter from
//! the pcParam array.
//! \param pbError is a pointer that will be written to \b true if there is any
//! error during the parameter parsing process (parameter not found, value is
//! not a valid decimal number).
//!
//! This function searches an array of parameters to find four parameters
//! whose names are \e pcName appended with digits 1 - 4.  Each of these
//! parameters is expected to have a value which is a decimal number between
//! 0 and 255.  The parameter values are read and concatenated into an unsigned
//! long representing an IP address with parameter 1 in the leftmost postion.
//!
//! For example, if \e pcName points to string ``ip'', the function will look
//! for 4 CGI parameters named ``ip1'', ``ip2'', ``ip3'' and ``ip4'' and read
//! their values to generate an IP address of the form 0xAABBCCDD where ``AA''
//! is the value of parameter ``ip1'', ``BB'' is the value of ``p2'', ``CC''
//! is the value of ``ip3'' and ``DD'' is the value of ``ip4''.
//!
//! \return Returns the IP address read or 0 if an error is detected (in
//! which case \e *pbError will be \b true).
//
//*****************************************************************************
uint32_t
ConfigGetCGIIPAddr(const char *pcName, char *pcParam[], char *pcValue[],
                   int iNumParams, bool *pbError)
{
    uint32_t ui32IPAddr;
    uint32_t ui32Loop;
    long i32Value;
    char pcVariable[MAX_VARIABLE_NAME_LEN];
    bool bError;

    //
    // Set up for the loop which reads each address element.
    //
    ui32IPAddr = 0;
    bError = false;

    //
    // Look for each of the four variables in turn.
    //
    for(ui32Loop = 1; ui32Loop <= 4; ui32Loop++)
    {
        //
        // Generate the name of the variable we are looking for next.
        //
        usnprintf(pcVariable, MAX_VARIABLE_NAME_LEN, "%s%d", pcName, ui32Loop);

        //
        // Shift our existing IP address to the left prior to reading the next
        // byte.
        //
        ui32IPAddr <<= 8;

        //
        // Get the next variable and mask it into the IP address.
        //
        i32Value = ConfigGetCGIParam(pcVariable, pcParam, pcValue, iNumParams,
                                   &bError);
        ui32IPAddr |= ((uint32_t)i32Value & 0xFF);


    }

    //
    // Did we encounter any error while reading the parameters?
    //
    if(bError)
    {
        //
        // Yes - mark the clients error flag and return 0.
        //
        *pbError = true;
        return(0);
    }
    else
    {
        //
        // No - all is well so return the IP address.
        //
        return(ui32IPAddr);
    }
}

//*****************************************************************************
//
//! \internal
//!
//! Performs processing for the URI ``/config.cgi''.
//!
//! \param iIndex is an index into the g_psConfigCGIURIs array indicating which
//! CGI URI has been requested.
//! \param uNumParams is the number of entries in the pcParam and pcValue
//! arrays.
//! \param pcParam is an array of character pointers, each containing the name
//! of a single parameter as encoded in the URI requesting this CGI.
//! \param pcValue is an array of character pointers, each containing the value
//! of a parameter as encoded in the URI requesting this CGI.
//!
//! This function is called whenever the HTTPD server receives a request for
//! URI ``/config.cgi''.  Parameters from the request are parsed into the
//! \e pcParam and \e pcValue arrays such that the parameter name and value
//! are contained in elements with the same index.  The strings contained in
//! \e pcParam and \e pcValue contain all replacements and encodings performed
//! by the browser so the CGI function is responsible for reversing these if
//! required.
//!
//! After processing the parameters, the function returns a fully qualified
//! filename to the HTTPD server which will then open this file and send the
//! contents back to the client in response to the CGI.
//!
//! This specific CGI expects the following parameters:
//!
//! - ``port'' indicates which connection's settings to update.  Valid
//!   values are ``0'' or ``1''.
//! - ``br'' supplies the baud rate.
//! - ``bc'' supplies the number of bits per character.
//! - ``parity'' supplies the parity.  Valid values are ``0'', ``1'', ``2'',
//!   ``3'' or ``4'' with meanings as defined by \b SERIAL_PARITY_xxx in
//!   serial.h.
//! - ``stop'' supplies the number of stop bits.
//! - ``flow'' supplies the flow control setting.  Valid values are ``1'' or
//!   ``3'' with meanings as defined by the \b SERIAL_FLOW_CONTROL_xxx in
//!   serial.h.
//! - ``telnetlp'' supplies the local port number for use by the telnet server.
//! - ``telnetrp'' supplies the remote port number for use by the telnet
//!   client.
//! - ``telnett'' supplies the telnet timeout in seconds.
//! - ``telnetip1'' supplies the first digit of the telnet server IP address.
//! - ``telnetip2'' supplies the second digit of the telnet server IP address.
//! - ``telnetip3'' supplies the third digit of the telnet server IP address.
//! - ``telnetip4'' supplies the fourth digit of the telnet server IP address.
//! - ``tnmode'' supplies the telnet mode, ``0'' for server, ``1'' for client.
//! - ``tnprot'' supplies the telnet protocol, ``0'' for telnet, ``1'' for raw.
//! - ``default'' will be defined with value ``1'' if the settings supplied are
//!   to be saved to flash as the defaults for this port.
//!
//! \return Returns a pointer to a string containing the file which is to be
//! sent back to the HTTPD client in response to this request.
//
//*****************************************************************************
static const char *
ConfigCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    int32_t i32Port;
    int32_t i32Value;
    int32_t i32TelnetMode;
    int32_t i32TelnetProtocol;
    bool bParamError;
    bool bSerialChanged;
    bool bTelnetChanged;
    tPortParameters sPortParams;

    //
    // We have not encountered any parameter errors yet.
    //
    bParamError = false;

    //
    // Get the port number.
    //
    i32Port = ConfigGetCGIParam("port", pcParam, pcValue, iNumParams,
                              &bParamError);

    if(bParamError || ((i32Port != 0) && (i32Port != 1)))
    {
        //
        // The connection parameter was invalid.
        //
        return(PARAM_ERROR_RESPONSE);
    }

    //
    // Take a local copy of the current parameter set for this connection
    //
    sPortParams = g_sParameters.sPort[i32Port];

    //
    // Baud rate
    //
    sPortParams.ui32BaudRate = (uint32_t)ConfigGetCGIParam("br", pcParam,
                                                              pcValue,
                                                              iNumParams,
                                                              &bParamError);

    //
    // Parity
    //
    sPortParams.ui8Parity = (uint8_t)ConfigGetCGIParam("parity", pcParam,
                                                            pcValue,
                                                            iNumParams,
                                                            &bParamError);

    //
    // Stop bits
    //
    sPortParams.ui8StopBits = (uint8_t)ConfigGetCGIParam("stop", pcParam,
                                                              pcValue,
                                                              iNumParams,
                                                              &bParamError);

    //
    // Data Size
    //
    sPortParams.ui8DataSize = (uint8_t)ConfigGetCGIParam("bc", pcParam,
                                                              pcValue,
                                                              iNumParams,
                                                              &bParamError);

    //
    // Flow control
    //
    sPortParams.ui8FlowControl = (uint8_t)ConfigGetCGIParam("flow",
                                                                 pcParam,
                                                                 pcValue,
                                                                 iNumParams,
                                                                 &bParamError);

    //
    // Telnet mode
    //
    i32TelnetMode = ConfigGetCGIParam("tnmode", pcParam, pcValue, iNumParams,
                                      &bParamError);

    //
    // Telnet protocol
    //
    i32TelnetProtocol = ConfigGetCGIParam("tnprot", pcParam, pcValue,
                                          iNumParams, &bParamError);

    //
    // Telnet local port
    //
    sPortParams.ui16TelnetLocalPort =
        (uint16_t)ConfigGetCGIParam("telnetlp", pcParam, pcValue,
                                          iNumParams, &bParamError);

    //
    // Telnet timeout
    //
    sPortParams.ui32TelnetTimeout =
        (uint8_t)ConfigGetCGIParam("telnett", pcParam, pcValue,
                                         iNumParams, &bParamError);

    //
    // If we are in telnet client mode, get the additional parameters required.
    //
    if(i32TelnetMode == PORT_TELNET_CLIENT)
    {
        //
        // Telnet remote port
        //
        sPortParams.ui16TelnetRemotePort =
            (uint16_t)ConfigGetCGIParam("telnetrp", pcParam, pcValue,
                                             iNumParams, &bParamError);

        //
        // Telnet IP address
        //
        sPortParams.ui32TelnetIPAddr = ConfigGetCGIIPAddr("telnetip", pcParam,
                                                        pcValue, iNumParams,
                                                        &bParamError);
    }

    //
    // We have now read all the parameters and made sure that they are valid
    // decimal numbers.  Did we see any errors during this process?
    //
    if(bParamError)
    {
        //
        // Yes - tell the user there was an error.
        //
        return(PARAM_ERROR_RESPONSE);
    }

    //
    // Update the telnet mode from the parameter we read.
    //
    sPortParams.ui8Flags &= ~PORT_FLAG_TELNET_MODE;
    sPortParams.ui8Flags |= (i32TelnetMode ? PORT_TELNET_CLIENT :
                            PORT_TELNET_SERVER);

    //
    // Update the telnet protocol from the parameter we read.
    //
    sPortParams.ui8Flags &= ~PORT_FLAG_PROTOCOL;
    sPortParams.ui8Flags |= (i32TelnetProtocol ? PORT_PROTOCOL_RAW :
                            PORT_PROTOCOL_TELNET);

    //
    // Did any of the serial parameters change?
    //
    if((g_sParameters.sPort[i32Port].ui8DataSize != sPortParams.ui8DataSize) ||
       (g_sParameters.sPort[i32Port].ui8FlowControl !=
        sPortParams.ui8FlowControl) ||
       (g_sParameters.sPort[i32Port].ui8Parity != sPortParams.ui8Parity) ||
       (g_sParameters.sPort[i32Port].ui8StopBits != sPortParams.ui8StopBits) ||
       (g_sParameters.sPort[i32Port].ui32BaudRate != sPortParams.ui32BaudRate))
    {
        bSerialChanged = true;
    }
    else
    {
        bSerialChanged = false;
    }

    //
    // Did any of the telnet parameters change?
    //
    if((g_sParameters.sPort[i32Port].ui32TelnetIPAddr !=
        sPortParams.ui32TelnetIPAddr) ||
       (g_sParameters.sPort[i32Port].ui32TelnetTimeout !=
        sPortParams.ui32TelnetTimeout) ||
       (g_sParameters.sPort[i32Port].ui16TelnetLocalPort !=
        sPortParams.ui16TelnetLocalPort) ||
       (g_sParameters.sPort[i32Port].ui16TelnetRemotePort !=
        sPortParams.ui16TelnetRemotePort) ||
       ((g_sParameters.sPort[i32Port].ui8Flags & PORT_FLAG_TELNET_MODE) !=
        (sPortParams.ui8Flags & PORT_FLAG_TELNET_MODE)) ||
       ((g_sParameters.sPort[i32Port].ui8Flags & PORT_FLAG_PROTOCOL) !=
        (sPortParams.ui8Flags & PORT_FLAG_PROTOCOL)))
    {
        bTelnetChanged = true;
    }
    else
    {
        bTelnetChanged = false;
    }

    //
    // Update the current parameters with the new settings.
    //
    g_sParameters.sPort[i32Port] = sPortParams;

    //
    // Were we asked to save this parameter set as the new default?
    //
    i32Value = (uint8_t)ConfigGetCGIParam("default", pcParam, pcValue,
                                                iNumParams, &bParamError);
    if(!bParamError && (i32Value == 1))
    {
        //
        // Yes - save these settings as the defaults.
        //
        g_sWorkingDefaultParameters.sPort[i32Port] =
            g_sParameters.sPort[i32Port];
        ConfigSave();
    }

    //
    // Apply all the changes to the working parameter set.
    //
    ConfigUpdatePortParameters(i32Port, bSerialChanged, bTelnetChanged);

    //
    // Send the user back to the main status page.
    //
    return(STATUS_PAGE_URI);
}

//*****************************************************************************
//
//! \internal
//!
//! Performs processing for the URI ``/ip.cgi''.
//!
//! \param iIndex is an index into the g_psConfigCGIURIs array indicating which
//! CGI URI has been requested.
//! \param uNumParams is the number of entries in the pcParam and pcValue
//! arrays.
//! \param pcParam is an array of character pointers, each containing the name
//! of a single parameter as encoded in the URI requesting this CGI.
//! \param pcValue is an array of character pointers, each containing the value
//! of a parameter as encoded in the URI requesting this CGI.
//!
//! This function is called whenever the HTTPD server receives a request for
//! URI ``/ip.cgi''.  Parameters from the request are parsed into the
//! \e pcParam and \e pcValue arrays such that the parameter name and value
//! are contained in elements with the same index.  The strings contained in
//! \e pcParam and \e pcValue contain all replacements and encodings performed
//! by the browser so the CGI function is responsible for reversing these if
//! required.
//!
//! After processing the parameters, the function returns a fully qualified
//! filename to the HTTPD server which will then open this file and send the
//! contents back to the client in response to the CGI.
//!
//! This specific CGI expects the following parameters:
//!
//! - ``staticip'' contains ``1'' to use a static IP address or ``0'' to use
//!   DHCP/AutoIP.
//! - ``sip1'' contains the first digit of the static IP address.
//! - ``sip2'' contains the second digit of the static IP address.
//! - ``sip3'' contains the third digit of the static IP address.
//! - ``sip4'' contains the fourth digit of the static IP address.
//! - ``gip1'' contains the first digit of the gateway IP address.
//! - ``gip2'' contains the second digit of the gateway IP address.
//! - ``gip3'' contains the third digit of the gateway IP address.
//! - ``gip4'' contains the fourth digit of the gateway IP address.
//! - ``mip1'' contains the first digit of the subnet mask.
//! - ``mip2'' contains the second digit of the subnet mask.
//! - ``mip3'' contains the third digit of the subnet mask.
//! - ``mip4'' contains the fourth digit of the subnet mask.
//!
//! \return Returns a pointer to a string containing the file which is to be
//! sent back to the HTTPD client in response to this request.
//
//*****************************************************************************
static const char *
ConfigIPCGIHandler(int iIndex, int iNumParams, char *pcParam[],
                   char *pcValue[])
{
    bool bChanged;
    bool bParamError;
    int32_t i32Mode;
    uint32_t ui32IPAddr;
    uint32_t ui32GatewayAddr;
    uint32_t ui32SubnetMask;

    //
    // Nothing has changed and we have seen no errors so far.
    //
    bChanged = false;
    bParamError = false;
    ui32IPAddr = 0;
    ui32GatewayAddr = 0;
    ui32SubnetMask = 0;

    //
    // Get the IP selection mode.
    //
    i32Mode = ConfigGetCGIParam("staticip", pcParam, pcValue, iNumParams,
                                &bParamError);

    //
    // This parameter is required so tell the user there has been a problem if
    // it is not found or is invalid.
    //
    if(bParamError)
    {
        return(PARAM_ERROR_RESPONSE);
    }

    //
    // If we are being told to use a static IP, read the remaining information.
    //
    if(i32Mode)
    {
        //
        // Get the static IP address to use.
        //
        ui32IPAddr = ConfigGetCGIIPAddr("sip", pcParam, pcValue, iNumParams,
                                      &bParamError);
        //
        // Get the gateway IP address to use.
        //
        ui32GatewayAddr = ConfigGetCGIIPAddr("gip", pcParam, pcValue,
                                             iNumParams, &bParamError);

        ui32SubnetMask = ConfigGetCGIIPAddr("mip", pcParam, pcValue,
                                            iNumParams, &bParamError);
    }

    //
    // Make sure we read all the required parameters correctly.
    //
    if(bParamError)
    {
        //
        // Oops - some parameter was invalid.
        //
        return(PARAM_ERROR_RESPONSE);
    }

    //
    // We have all the parameters so determine if anything changed.
    //

    //
    // Did the basic mode change?
    //
    if((i32Mode && !(g_sParameters.ui8Flags & CONFIG_FLAG_STATICIP)) ||
       (!i32Mode && (g_sParameters.ui8Flags & CONFIG_FLAG_STATICIP)))
    {
        //
        // The mode changed so set the new mode in the parameter block.
        //
        if(!i32Mode)
        {
            g_sParameters.ui8Flags &= ~CONFIG_FLAG_STATICIP;
        }
        else
        {
            g_sParameters.ui8Flags |= CONFIG_FLAG_STATICIP;
        }

        //
        // Remember that something changed.
        //
        bChanged = true;
    }

    //
    // If we are now using static IP, check for modifications to the IP
    // addresses and mask.
    //
    if(i32Mode)
    {
        if((g_sParameters.ui32StaticIP != ui32IPAddr) ||
           (g_sParameters.ui32GatewayIP != ui32GatewayAddr) ||
           (g_sParameters.ui32SubnetMask != ui32SubnetMask))
        {
            //
            // Something changed so update the parameter block.
            //
            bChanged = true;
            g_sParameters.ui32StaticIP = ui32IPAddr;
            g_sParameters.ui32GatewayIP = ui32GatewayAddr;
            g_sParameters.ui32SubnetMask = ui32SubnetMask;
        }

    }

    //
    // If anything changed, we need to resave the parameter block.
    //
    if(bChanged)
    {
        //
        // Shut down connections in preparation for the IP address change.
        //
        ConfigPreUpdateIPAddress();

        //
        // Update the working default set and save the parameter block.
        //
        g_sWorkingDefaultParameters = g_sParameters;
        ConfigSave();

        //
        // Tell the main loop that a IP address update has been requested.
        //
        g_ui8UpdateRequired |= UPDATE_IP_ADDR;


        //
        // Direct the browser to a page warning about the impending IP
        // address change.
        //
        return(IP_UPDATE_RESPONSE);
    }

    //
    // Direct the user back to our miscellaneous settings page.
    //
    return(STATUS_PAGE_URI);
}


static const char *
ConfigTecLimCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    int32_t i32Port;
    int32_t i32Value;
    int32_t i32PosNeg;
    bool bParamError = false;
	bool bCommandError;

    //
    // Get the port number.
    //
    i32Port = ConfigGetCGIParam("port", pcParam, pcValue, iNumParams, &bParamError);
	
	i32Value = (uint32_t)ConfigGetCGIParam("teclim", pcParam, pcValue, iNumParams, &bParamError);

	i32PosNeg = (uint32_t)ConfigGetCGIParam("posneg", pcParam, pcValue, iNumParams, &bParamError);

    if(bParamError || ((i32Port != 0) && (i32Port != 1)))
    {
        return(PARAM_ERROR_RESPONSE);
    }
	
	bCommandError = Set_TEC_limit(i32Port, i32Value, i32PosNeg);

	if(bCommandError == 0)
	{
		return(PARAM_ERROR_RESPONSE);
	} else
	{
		if(i32Port == 0)
		{
			return(TEC0_PAGE_URI);
		} else
		{
			return(TEC1_PAGE_URI);
		}
	}	
}
static const char *
ConfigTempSetCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    int32_t i32Port;
    int32_t i32Value = 0;
    bool bParamError = false;
    bool bCommandError;
    int iParam;
    int i = 0;
    int j = 0;
    int digits1 = 0;
    int digits2 = 0;
    char tempval[11] = {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',NULL};

    //
    // Get the port number.
    //
    i32Port = ConfigGetCGIParam("port", pcParam, pcValue, iNumParams, &bParamError);

    iParam = ConfigFindCGIParameter("tempset", pcParam, iNumParams);
    if(iParam != -1)
    {
        //
        // We found the parameter so now get its value.
        //


        while(pcValue[iParam][i] && (pcValue[iParam][i]<'0' || pcValue[iParam][i]>'9') && (i < 11))
        {
            if(pcValue[iParam][i] == '-')
            {
                tempval[j] = pcValue[iParam][i];
                j = j + 1;
            }
            if(pcValue[iParam][i] == '%' && pcValue[iParam][i+1] == '2' && pcValue[iParam][i+2] == 'B')
            {
                i = i + 2;
            }
            i = i + 1;
        }

        while(pcValue[iParam][i] && pcValue[iParam][i]>='0' && pcValue[iParam][i]<='9')
        {
            tempval[j] = pcValue[iParam][i];
            j = j + 1;
            digits1 = digits1 + 1;
            i = i + 1;
        }

        if(pcValue[iParam][i]) {
            if(pcValue[iParam][i]=='.')
            {
                 i = i+1;

                 while(pcValue[iParam][i] >= '0' && pcValue[iParam][i] <= '9') {
                     tempval[j] = pcValue[iParam][i];
                     j = j + 1;
                     digits2 = digits2 + 1;
                     i = i + 1;
                 }

            } else if(pcValue[iParam][i]=='%' && pcValue[iParam][i+1]=='2' && pcValue[iParam][i+2]=='C')
            {
                 i = i+3;

                 while(pcValue[iParam][i] >= '0' && pcValue[iParam][i] <= '9') {
                     tempval[j] = pcValue[iParam][i];
                     j = j + 1;
                     digits2 = digits2 + 1;
                     i = i + 1;
                 }

            } else
            {
                tempval[j] = pcValue[iParam][i];
                j = j + 1;
            }
        }

        if(digits2 == 0)
        {
            tempval[j] = '0';
            tempval[j+1] = '0';
            tempval[j+2] = '0';
            tempval[j+3] = ' ';
        } else if(digits2 == 1)
        {
            tempval[j] = '0';
            tempval[j+1] = '0';
            tempval[j+2] = ' ';
        } else if(digits2 == 2)
        {
            tempval[j] = '0';
            tempval[j+1] = ' ';
        } else if(digits2 == 3)
        {
            tempval[j] = ' ';
        } else
        {
            tempval[j-1] = ' ';
        }
    }

    bParamError = !(ConfigCheckDecimalParam(tempval, &i32Value));

    if(bParamError || ((i32Port != 0) && (i32Port != 1)))
    {
        return(PARAM_ERROR_RESPONSE);
    }

    bCommandError = Set_Temp(i32Port, i32Value);

    if(bCommandError == 0)
    {
        return(PARAM_ERROR_RESPONSE);
    } else
    {
        if(i32Port == 0)
        {
            return(TEMP0_PAGE_URI);
        } else
        {
            return(TEMP1_PAGE_URI);
        }
    }

}

static const char *
ConfigTempWinCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    int32_t i32Port;
    int32_t i32Value;
    bool bParamError = false;
    bool bCommandError;

    //
    // Get the port number.
    //
    i32Port = ConfigGetCGIParam("port", pcParam, pcValue, iNumParams, &bParamError);
	
	i32Value = (uint32_t)ConfigGetCGIParam("tempwin", pcParam, pcValue, iNumParams, &bParamError);

    if(bParamError || ((i32Port != 0) && (i32Port != 1)))
    {
        return(PARAM_ERROR_RESPONSE);
    }

    bCommandError = Set_Temp_Window(i32Port, i32Value);

     if(bCommandError == 0)
     {
         return(PARAM_ERROR_RESPONSE);
     } else
     {
         if(i32Port == 0)
         {
             return(TEMP0_PAGE_URI);
         } else
         {
             return(TEMP1_PAGE_URI);
         }
     }
}

static const char *
ConfigCLPCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    int32_t i32Port;
    int32_t i32Value;
    bool bParamError = false;
    bool bCommandError;

    //
    // Get the port number.
    //
    i32Port = ConfigGetCGIParam("port", pcParam, pcValue, iNumParams, &bParamError);
	
	i32Value = (uint32_t)ConfigGetCGIParam("clp", pcParam, pcValue, iNumParams, &bParamError);

    if(bParamError || ((i32Port != 0) && (i32Port != 1)))
    {
        return(PARAM_ERROR_RESPONSE);
    }
	
    bCommandError = Set_PID(i32Port, 0, i32Value);

    if(bCommandError == 0)
    {
        return(PARAM_ERROR_RESPONSE);
    } else
    {
        if(i32Port == 0)
        {
            return(CL0_PAGE_URI);
        } else
        {
            return(CL1_PAGE_URI);
        }
    }
}
static const char *
ConfigCLICGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    int32_t i32Port;
    int32_t i32Value;
    bool bParamError = false;
    bool bCommandError;

    //
    // Get the port number.
    //
    i32Port = ConfigGetCGIParam("port", pcParam, pcValue, iNumParams, &bParamError);
	
	i32Value = (uint32_t)ConfigGetCGIParam("cli", pcParam, pcValue, iNumParams, &bParamError);

    if(bParamError || ((i32Port != 0) && (i32Port != 1)))
    {
        return(PARAM_ERROR_RESPONSE);
    }

    bCommandError = Set_PID(i32Port, 1, i32Value);

    if(bCommandError == 0)
    {
        return(PARAM_ERROR_RESPONSE);
    } else
    {
        if(i32Port == 0)
        {
            return(CL0_PAGE_URI);
        } else
        {
            return(CL1_PAGE_URI);
        }
    }
}
static const char *
ConfigCLDCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    int32_t i32Port;
    int32_t i32Value;
    bool bParamError = false;
    bool bCommandError;

    //
    // Get the port number.
    //
    i32Port = ConfigGetCGIParam("port", pcParam, pcValue, iNumParams, &bParamError);
	
	i32Value = (uint32_t)ConfigGetCGIParam("cld", pcParam, pcValue, iNumParams, &bParamError);

    if(bParamError || ((i32Port != 0) && (i32Port != 1)))
    {
        return(PARAM_ERROR_RESPONSE);
    }

    bCommandError = Set_PID(i32Port, 2, i32Value);

    if(bCommandError == 0)
    {
        return(PARAM_ERROR_RESPONSE);
    } else
    {
        if(i32Port == 0)
        {
            return(CL0_PAGE_URI);
        } else
        {
            return(CL1_PAGE_URI);
        }
    }
}


static const char *
ConfigTurnOnOffCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{

    int32_t i32Port;
    int32_t i32Value;
    bool bParamError = false;
    bool bCommandError;

    //
    // Get the port number.
    //
    i32Port = ConfigGetCGIParam("port", pcParam, pcValue, iNumParams, &bParamError);

    i32Value = (uint32_t)ConfigGetCGIParam("onoff", pcParam, pcValue, iNumParams, &bParamError);

    if(bParamError || ((i32Port != 0) && (i32Port != 1)))
    {
        return(PARAM_ERROR_RESPONSE);
    }

    if(i32Value == 1) {
            if(i32Port == 0) {
                if(onoff0 == 0) {
                    onoff0 = 1;
                } else if (onoff0 == 1) {
                    onoff0 = 0;
                }
                bCommandError = OnOff(i32Port, onoff0);
            } else {
                if(onoff1 == 0) {
                    onoff1 = 1;
                } else if (onoff1 == 1) {
                    onoff1 = 0;
                }
                bCommandError = OnOff(i32Port, onoff1);
            }
        } else {
            bCommandError = false;
        }

    if(bCommandError == 0)
    {
        return(PARAM_ERROR_RESPONSE);
    } else
    {
        if(i32Port == 0)
        {
            return(TEC0_PAGE_URI);
        } else
        {
            return(TEC1_PAGE_URI);
        }
    }
}


static const char *
ConfigSoftResetCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    int32_t i32Value;
    bool bParamError = false;

    i32Value = (uint32_t)ConfigGetCGIParam("soft_reset", pcParam, pcValue, iNumParams, &bParamError);

    if(bParamError)
    {
        return(PARAM_ERROR_RESPONSE);
    }

    if(i32Value == 1)
    {
        SysCtlReset();
        return(DEFAULT_CGI_RESPONSE);
    } else
    {
        return(DEFAULT_CGI_RESPONSE);
    }
}

static const char *
ConfigSaveAllCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{

    int32_t i32Value;
    bool bParamError = false;

    i32Value = (uint32_t)ConfigGetCGIParam("saveall", pcParam, pcValue, iNumParams, &bParamError);

    if(bParamError)
    {
        return(PARAM_ERROR_RESPONSE);
    }

    if(i32Value == 1)
    {
        ConfigSaveAll();
        return(SAVED_PAGE_URI);
    } else
    {
        return(DEFAULT_CGI_RESPONSE);
    }
}

static const char *
ConfigSetMQTTCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    uint32_t i32Value, i32ipval;
    bool bParamError;
    i32ipval = 0;

    i32Value = (uint8_t)ConfigGetCGIParam("mqttip1", pcParam, pcValue,
                                                iNumParams, &bParamError);
    if(!bParamError)
     {
        i32ipval = (i32ipval | i32Value);
     }

    i32Value = (uint8_t)ConfigGetCGIParam("mqttip2", pcParam, pcValue,
                                                iNumParams, &bParamError);
    if(!bParamError)
     {
        i32ipval = (i32ipval | i32Value << 8);
     }

    i32Value = (uint8_t)ConfigGetCGIParam("mqttip3", pcParam, pcValue,
                                                iNumParams, &bParamError);
    if(!bParamError)
     {
        i32ipval = (i32ipval | i32Value << 16);
     }

    i32Value = (uint8_t)ConfigGetCGIParam("mqttip4", pcParam, pcValue,
                                                iNumParams, &bParamError);
    if(!bParamError)
     {
        i32ipval = (i32ipval | i32Value << 24);
     }

    g_sParameters.ui32mqttip = i32ipval;

    //
    // Were we asked to save this parameter set as the new default?
    //
    i32Value = (uint8_t)ConfigGetCGIParam("default", pcParam, pcValue,
                                                iNumParams, &bParamError);
    if(!bParamError && (i32Value == 1))
    {
        //
        // Yes - save these settings as the defaults.
        //
        g_sWorkingDefaultParameters.ui32mqttip =
            g_sParameters.ui32mqttip;
        ConfigSave();
    }

    mqtt_disconnect(client);
    mqtt_conn = false;
    do_connect();

    //
    // Send the user back to the main status page.
    //
    return(MQTT_PAGE_URI);
}


static const char *
ConfigSetT0CGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    int32_t i32Port;
    int32_t i32Value = 0;
    bool bParamError = false;
    bool bCommandError;
    int iParam;
    int i = 0;
    int j = 0;
    int digits1 = 0;
    int digits2 = 0;
    char tempval[11] = {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',NULL};

    //
    // Get the port number.
    //
    i32Port = ConfigGetCGIParam("port", pcParam, pcValue, iNumParams, &bParamError);

    iParam = ConfigFindCGIParameter("t0", pcParam, iNumParams);
    if(iParam != -1)
    {
        //
        // We found the parameter so now get its value.
        //


        while(pcValue[iParam][i] && (pcValue[iParam][i]<'0' || pcValue[iParam][i]>'9') && (i < 11))
        {
            if(pcValue[iParam][i] == '-')
            {
                tempval[j] = pcValue[iParam][i];
                j = j + 1;
            }
            if(pcValue[iParam][i] == '%' && pcValue[iParam][i+1] == '2' && pcValue[iParam][i+2] == 'B')
            {
                i = i + 2;
            }
            i = i + 1;
        }

        while(pcValue[iParam][i] && pcValue[iParam][i]>='0' && pcValue[iParam][i]<='9')
        {
            tempval[j] = pcValue[iParam][i];
            j = j + 1;
            digits1 = digits1 + 1;
            i = i + 1;
        }

        if(pcValue[iParam][i]) {
            if(pcValue[iParam][i]=='.')
            {
                 i = i+1;

                 while(pcValue[iParam][i] >= '0' && pcValue[iParam][i] <= '9') {
                     tempval[j] = pcValue[iParam][i];
                     j = j + 1;
                     digits2 = digits2 + 1;
                     i = i + 1;
                 }

            } else if(pcValue[iParam][i]=='%' && pcValue[iParam][i+1]=='2' && pcValue[iParam][i+2]=='C')
            {
                 i = i+3;

                 while(pcValue[iParam][i] >= '0' && pcValue[iParam][i] <= '9') {
                     tempval[j] = pcValue[iParam][i];
                     j = j + 1;
                     digits2 = digits2 + 1;
                     i = i + 1;
                 }

            } else
            {
                tempval[j] = pcValue[iParam][i];
                j = j + 1;
            }
        }

        if(digits2 == 0)
        {
            tempval[j] = '0';
            tempval[j+1] = '0';
            tempval[j+2] = '0';
            tempval[j+3] = ' ';
        } else if(digits2 == 1)
        {
            tempval[j] = '0';
            tempval[j+1] = '0';
            tempval[j+2] = ' ';
        } else if(digits2 == 2)
        {
            tempval[j] = '0';
            tempval[j+1] = ' ';
        } else if(digits2 == 3)
        {
            tempval[j] = ' ';
        } else
        {
            tempval[j-1] = ' ';
        }
    }

    bParamError = !(ConfigCheckDecimalParam(tempval, &i32Value));

    if(bParamError || ((i32Port != 0) && (i32Port != 1)))
    {
        return(PARAM_ERROR_RESPONSE);
    }

    bCommandError = Set_Calib(i32Port, 0, i32Value);

    if(bCommandError == 0)
    {
        return(PARAM_ERROR_RESPONSE);
    } else
    {
        if(i32Port == 0)
        {
            return(CALIB0_PAGE_URI);
        } else
        {
            return(CALIB1_PAGE_URI);
        }
    }

}

static const char *
ConfigSetBetaCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    int32_t i32Port;
    int32_t i32Value = 0;
    bool bParamError = false;
    bool bCommandError;
    int iParam;
    int i = 0;
    int j = 0;
    int digits1 = 0;
    int digits2 = 0;
    char tempval[11] = {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',NULL};

    //
    // Get the port number.
    //
    i32Port = ConfigGetCGIParam("port", pcParam, pcValue, iNumParams, &bParamError);

    iParam = ConfigFindCGIParameter("beta", pcParam, iNumParams);
    if(iParam != -1)
    {
        //
        // We found the parameter so now get its value.
        //


        while(pcValue[iParam][i] && (pcValue[iParam][i]<'0' || pcValue[iParam][i]>'9') && (i < 11))
        {
            if(pcValue[iParam][i] == '-')
            {
                tempval[j] = pcValue[iParam][i];
                j = j + 1;
            }
            if(pcValue[iParam][i] == '%' && pcValue[iParam][i+1] == '2' && pcValue[iParam][i+2] == 'B')
            {
                i = i + 2;
            }
            i = i + 1;
        }

        while(pcValue[iParam][i] && pcValue[iParam][i]>='0' && pcValue[iParam][i]<='9')
        {
            tempval[j] = pcValue[iParam][i];
            j = j + 1;
            digits1 = digits1 + 1;
            i = i + 1;
        }

        if(pcValue[iParam][i]) {
            if(pcValue[iParam][i]=='.')
            {
                 i = i+1;

                 while(pcValue[iParam][i] >= '0' && pcValue[iParam][i] <= '9') {
                     tempval[j] = pcValue[iParam][i];
                     j = j + 1;
                     digits2 = digits2 + 1;
                     i = i + 1;
                 }

            } else if(pcValue[iParam][i]=='%' && pcValue[iParam][i+1]=='2' && pcValue[iParam][i+2]=='C')
            {
                 i = i+3;

                 while(pcValue[iParam][i] >= '0' && pcValue[iParam][i] <= '9') {
                     tempval[j] = pcValue[iParam][i];
                     j = j + 1;
                     digits2 = digits2 + 1;
                     i = i + 1;
                 }

            } else
            {
                tempval[j] = pcValue[iParam][i];
                j = j + 1;
            }
        }

        if(digits2 == 0)
        {
            tempval[j] = '0';
            tempval[j+1] = '0';
            tempval[j+2] = '0';
            tempval[j+3] = ' ';
        } else if(digits2 == 1)
        {
            tempval[j] = '0';
            tempval[j+1] = '0';
            tempval[j+2] = ' ';
        } else if(digits2 == 2)
        {
            tempval[j] = '0';
            tempval[j+1] = ' ';
        } else if(digits2 == 3)
        {
            tempval[j] = ' ';
        } else
        {
            tempval[j-1] = ' ';
        }
    }

    bParamError = !(ConfigCheckDecimalParam(tempval, &i32Value));

    if(bParamError || ((i32Port != 0) && (i32Port != 1)))
    {
        return(PARAM_ERROR_RESPONSE);
    }

    bCommandError = Set_Calib(i32Port, 1, i32Value);

    if(bCommandError == 0)
    {
        return(PARAM_ERROR_RESPONSE);
    } else
    {
        if(i32Port == 0)
        {
            return(CALIB0_PAGE_URI);
        } else
        {
            return(CALIB1_PAGE_URI);
        }
    }

}


static const char *
ConfigSetRatioCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    int32_t i32Port;
    int32_t i32Value = 0;
    bool bParamError = false;
    bool bCommandError;
    int iParam;
    int i = 0;
    int j = 0;
    int digits1 = 0;
    int digits2 = 0;
    char tempval[11] = {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',NULL};

    //
    // Get the port number.
    //
    i32Port = ConfigGetCGIParam("port", pcParam, pcValue, iNumParams, &bParamError);

    iParam = ConfigFindCGIParameter("ratio", pcParam, iNumParams);
    if(iParam != -1)
    {
        //
        // We found the parameter so now get its value.
        //


        while(pcValue[iParam][i] && (pcValue[iParam][i]<'0' || pcValue[iParam][i]>'9') && (i < 11))
        {
            if(pcValue[iParam][i] == '-')
            {
                tempval[j] = pcValue[iParam][i];
                j = j + 1;
            }
            if(pcValue[iParam][i] == '%' && pcValue[iParam][i+1] == '2' && pcValue[iParam][i+2] == 'B')
            {
                i = i + 2;
            }
            i = i + 1;
        }

        while(pcValue[iParam][i] && pcValue[iParam][i]>='0' && pcValue[iParam][i]<='9')
        {
            tempval[j] = pcValue[iParam][i];
            j = j + 1;
            digits1 = digits1 + 1;
            i = i + 1;
        }

        if(pcValue[iParam][i]) {
            if(pcValue[iParam][i]=='.')
            {
                 i = i+1;

                 while(pcValue[iParam][i] >= '0' && pcValue[iParam][i] <= '9') {
                     tempval[j] = pcValue[iParam][i];
                     j = j + 1;
                     digits2 = digits2 + 1;
                     i = i + 1;
                 }

            } else if(pcValue[iParam][i]=='%' && pcValue[iParam][i+1]=='2' && pcValue[iParam][i+2]=='C')
            {
                 i = i+3;

                 while(pcValue[iParam][i] >= '0' && pcValue[iParam][i] <= '9') {
                     tempval[j] = pcValue[iParam][i];
                     j = j + 1;
                     digits2 = digits2 + 1;
                     i = i + 1;
                 }

            } else
            {
                tempval[j] = pcValue[iParam][i];
                j = j + 1;
            }
        }

        if(digits2 == 0)
        {
            tempval[j] = '0';
            tempval[j+1] = '0';
            tempval[j+2] = '0';
            tempval[j+3] = ' ';
        } else if(digits2 == 1)
        {
            tempval[j] = '0';
            tempval[j+1] = '0';
            tempval[j+2] = ' ';
        } else if(digits2 == 2)
        {
            tempval[j] = '0';
            tempval[j+1] = ' ';
        } else if(digits2 == 3)
        {
            tempval[j] = ' ';
        } else
        {
            tempval[j-1] = ' ';
        }
    }

    bParamError = !(ConfigCheckDecimalParam(tempval, &i32Value));

    if(bParamError || ((i32Port != 0) && (i32Port != 1)))
    {
        return(PARAM_ERROR_RESPONSE);
    }

    bCommandError = Set_Calib(i32Port, 2, i32Value);

    if(bCommandError == 0)
    {
        return(PARAM_ERROR_RESPONSE);
    } else
    {
        if(i32Port == 0)
        {
            return(CALIB0_PAGE_URI);
        } else
        {
            return(CALIB1_PAGE_URI);
        }
    }

}

static const char *
ConfigSetTempModeCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{

    int32_t i32Port;
    int32_t i32Value;
    bool bParamError = false;
    bool bCommandError;

    //
    // Get the port number.
    //
    i32Port = ConfigGetCGIParam("port", pcParam, pcValue, iNumParams, &bParamError);

    i32Value = (uint32_t)ConfigGetCGIParam("tempmode", pcParam, pcValue, iNumParams, &bParamError);

    if(bParamError || ((i32Port != 0) && (i32Port != 1)))
    {
        return(PARAM_ERROR_RESPONSE);
    }

    bCommandError = Set_Calib(i32Port, 4, i32Value);

    if(bCommandError == 0)
    {
        return(PARAM_ERROR_RESPONSE);
    } else
    {
        if(i32Port == 0)
        {
            return(CALIB0_PAGE_URI);
        } else
        {
            return(CALIB1_PAGE_URI);
        }
    }

}


static const char *
ConfigSetptACGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    int32_t i32Port;
    int32_t i32Value = 0;
    bool bParamError = false;
    bool bCommandError;
    int iParam;
    int i = 0;
    int j = 0;
    int digits1 = 0;
    int digits2 = 0;
    char tempval[11] = {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',NULL};

    //
    // Get the port number.
    //
    i32Port = ConfigGetCGIParam("port", pcParam, pcValue, iNumParams, &bParamError);

    iParam = ConfigFindCGIParameter("pta", pcParam, iNumParams);
    if(iParam != -1)
    {
        //
        // We found the parameter so now get its value.
        //


        while(pcValue[iParam][i] && (pcValue[iParam][i]<'0' || pcValue[iParam][i]>'9') && (i < 11))
        {
            if(pcValue[iParam][i] == '-')
            {
                tempval[j] = pcValue[iParam][i];
                j = j + 1;
            }
            if(pcValue[iParam][i] == '%' && pcValue[iParam][i+1] == '2' && pcValue[iParam][i+2] == 'B')
            {
                i = i + 2;
            }
            i = i + 1;
        }

        while(pcValue[iParam][i] && pcValue[iParam][i]>='0' && pcValue[iParam][i]<='9')
        {
            tempval[j] = pcValue[iParam][i];
            j = j + 1;
            digits1 = digits1 + 1;
            i = i + 1;
        }

        if(pcValue[iParam][i]) {
            if(pcValue[iParam][i]=='.')
            {
                 i = i+1;

                 while(pcValue[iParam][i] >= '0' && pcValue[iParam][i] <= '9') {
                     tempval[j] = pcValue[iParam][i];
                     j = j + 1;
                     digits2 = digits2 + 1;
                     i = i + 1;
                 }

            } else if(pcValue[iParam][i]=='%' && pcValue[iParam][i+1]=='2' && pcValue[iParam][i+2]=='C')
            {
                 i = i+3;

                 while(pcValue[iParam][i] >= '0' && pcValue[iParam][i] <= '9') {
                     tempval[j] = pcValue[iParam][i];
                     j = j + 1;
                     digits2 = digits2 + 1;
                     i = i + 1;
                 }

            } else
            {
                tempval[j] = pcValue[iParam][i];
                j = j + 1;
            }
        }

        if(digits2 == 0)
        {
            tempval[j] = '0';
            tempval[j+1] = '0';
            tempval[j+2] = '0';
            tempval[j+3] = ' ';
        } else if(digits2 == 1)
        {
            tempval[j] = '0';
            tempval[j+1] = '0';
            tempval[j+2] = ' ';
        } else if(digits2 == 2)
        {
            tempval[j] = '0';
            tempval[j+1] = ' ';
        } else if(digits2 == 3)
        {
            tempval[j] = ' ';
        } else
        {
            tempval[j-1] = ' ';
        }
    }

    bParamError = !(ConfigCheckDecimalParam(tempval, &i32Value));

    if(bParamError || ((i32Port != 0) && (i32Port != 1)))
    {
        return(PARAM_ERROR_RESPONSE);
    }

    bCommandError = Set_Calib(i32Port, 5, i32Value);

    if(bCommandError == 0)
    {
        return(PARAM_ERROR_RESPONSE);
    } else
    {
        if(i32Port == 0)
        {
            return(CALIB0_PAGE_URI);
        } else
        {
            return(CALIB1_PAGE_URI);
        }
    }

}


static const char *
ConfigSetptBCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    int32_t i32Port;
    int32_t i32Value = 0;
    bool bParamError = false;
    bool bCommandError;
    int iParam;
    int i = 0;
    int j = 0;
    int digits1 = 0;
    int digits2 = 0;
    char tempval[11] = {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',NULL};

    //
    // Get the port number.
    //
    i32Port = ConfigGetCGIParam("port", pcParam, pcValue, iNumParams, &bParamError);

    iParam = ConfigFindCGIParameter("ptb", pcParam, iNumParams);
    if(iParam != -1)
    {
        //
        // We found the parameter so now get its value.
        //


        while(pcValue[iParam][i] && (pcValue[iParam][i]<'0' || pcValue[iParam][i]>'9') && (i < 11))
        {
            if(pcValue[iParam][i] == '-')
            {
                tempval[j] = pcValue[iParam][i];
                j = j + 1;
            }
            if(pcValue[iParam][i] == '%' && pcValue[iParam][i+1] == '2' && pcValue[iParam][i+2] == 'B')
            {
                i = i + 2;
            }
            i = i + 1;
        }

        while(pcValue[iParam][i] && pcValue[iParam][i]>='0' && pcValue[iParam][i]<='9')
        {
            tempval[j] = pcValue[iParam][i];
            j = j + 1;
            digits1 = digits1 + 1;
            i = i + 1;
        }

        if(pcValue[iParam][i]) {
            if(pcValue[iParam][i]=='.')
            {
                 i = i+1;

                 while(pcValue[iParam][i] >= '0' && pcValue[iParam][i] <= '9') {
                     tempval[j] = pcValue[iParam][i];
                     j = j + 1;
                     digits2 = digits2 + 1;
                     i = i + 1;
                 }

            } else if(pcValue[iParam][i]=='%' && pcValue[iParam][i+1]=='2' && pcValue[iParam][i+2]=='C')
            {
                 i = i+3;

                 while(pcValue[iParam][i] >= '0' && pcValue[iParam][i] <= '9') {
                     tempval[j] = pcValue[iParam][i];
                     j = j + 1;
                     digits2 = digits2 + 1;
                     i = i + 1;
                 }

            } else
            {
                tempval[j] = pcValue[iParam][i];
                j = j + 1;
            }
        }

        if(digits2 == 0)
        {
            tempval[j] = '0';
            tempval[j+1] = '0';
            tempval[j+2] = '0';
            tempval[j+3] = ' ';
        } else if(digits2 == 1)
        {
            tempval[j] = '0';
            tempval[j+1] = '0';
            tempval[j+2] = ' ';
        } else if(digits2 == 2)
        {
            tempval[j] = '0';
            tempval[j+1] = ' ';
        } else if(digits2 == 3)
        {
            tempval[j] = ' ';
        } else
        {
            tempval[j-1] = ' ';
        }
    }

    bParamError = !(ConfigCheckDecimalParam(tempval, &i32Value));

    if(bParamError || ((i32Port != 0) && (i32Port != 1)))
    {
        return(PARAM_ERROR_RESPONSE);
    }

    bCommandError = Set_Calib(i32Port, 6, i32Value);

    if(bCommandError == 0)
    {
        return(PARAM_ERROR_RESPONSE);
    } else
    {
        if(i32Port == 0)
        {
            return(CALIB0_PAGE_URI);
        } else
        {
            return(CALIB1_PAGE_URI);
        }
    }

}


static const char *
ConfigSetSwFreqCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{

    int32_t i32Port;
    int32_t i32Value;
    bool bParamError = false;
    bool bCommandError;

    //
    // Get the port number.
    //
    i32Port = ConfigGetCGIParam("port", pcParam, pcValue, iNumParams, &bParamError);

    i32Value = (uint32_t)ConfigGetCGIParam("swfreq", pcParam, pcValue, iNumParams, &bParamError);

    if(bParamError || ((i32Port != 0) && (i32Port != 1)))
    {
        return(PARAM_ERROR_RESPONSE);
    }

    bCommandError = Set_Calib(i32Port, 3, i32Value);

    if(bCommandError == 0)
    {
        return(PARAM_ERROR_RESPONSE);
    } else
    {
        if(i32Port == 0)
        {
            return(CALIB0_PAGE_URI);
        } else
        {
            return(CALIB1_PAGE_URI);
        }
    }

}

static const char *
ConfigSetMaxVoltCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{

    int32_t i32Port;
    int32_t i32Value;
    bool bParamError = false;
    bool bCommandError;

    //
    // Get the port number.
    //
    i32Port = ConfigGetCGIParam("port", pcParam, pcValue, iNumParams, &bParamError);

    i32Value = (uint32_t)ConfigGetCGIParam("maxvolt", pcParam, pcValue, iNumParams, &bParamError);

    if(bParamError || ((i32Port != 0) && (i32Port != 1)))
    {
        return(PARAM_ERROR_RESPONSE);
    }

    bCommandError = Set_MaxVolt(i32Port, i32Value);

    if(bCommandError == 0)
    {
        return(PARAM_ERROR_RESPONSE);
    } else
    {
        if(i32Port == 0)
        {
            return(TEC0_PAGE_URI);
        } else
        {
            return(TEC1_PAGE_URI);
        }
    }

}




//*****************************************************************************
//
//! \internal
//!
//! Performs processing for the URI ``/misc.cgi''.
//!
//! \param iIndex is an index into the g_psConfigCGIURIs array indicating which
//! CGI URI has been requested.
//! \param uNumParams is the number of entries in the pcParam and pcValue
//! arrays.
//! \param pcParam is an array of character pointers, each containing the name
//! of a single parameter as encoded in the URI requesting this CGI.
//! \param pcValue is an array of character pointers, each containing the value
//! of a parameter as encoded in the URI requesting this CGI.
//!
//! This function is called whenever the HTTPD server receives a request for
//! URI ``/misc.cgi''.  Parameters from the request are parsed into the
//! \e pcParam and \e pcValue arrays such that the parameter name and value
//! are contained in elements with the same index.  The strings contained in
//! \e pcParam and \e pcValue contain all replacements and encodings performed
//! by the browser so the CGI function is responsible for reversing these if
//! required.
//!
//! After processing the parameters, the function returns a fully qualified
//! filename to the HTTPD server which will then open this file and send the
//! contents back to the client in response to the CGI.
//!
//! This specific CGI expects the following parameters:
//!
//! - ``modname'' provides a string to be used as the friendly name for the
//!   module.  This is encoded by the browser and must be decoded here.
//!
//! \return Returns a pointer to a string containing the file which is to be
//! sent back to the HTTPD client in response to this request.
//
//*****************************************************************************
static const char *
ConfigMiscCGIHandler(int iIndex, int iNumParams, char *pcParam[],
                     char *pcValue[])
{
    int iParam;
    bool bChanged;

    //
    // We have not made any changes that need written to flash yet.
    //
    bChanged = false;

    //
    // Find the "modname" parameter.
    //
    iParam = ConfigFindCGIParameter("modname", pcParam, iNumParams);
    if(iParam != -1)
    {
        ConfigDecodeFormString(pcValue[iParam],
                               (char *)g_sWorkingDefaultParameters.ui8ModName,
                               MOD_NAME_LEN);
        strncpy((char *)g_sParameters.ui8ModName,
                (char *)g_sWorkingDefaultParameters.ui8ModName, MOD_NAME_LEN);
        LocatorAppTitleSet((char *)g_sParameters.ui8ModName);
        bChanged = true;
    }

    //
    // Did anything change?
    //
    if(bChanged)
    {
        //
        // Yes - save the latest parameters to flash.
        //
        ConfigSave();
    }

    return(STATUS_PAGE_URI);
}

//*****************************************************************************
//
//! \internal
//!
//! Determines whether the supplied configuration parameter structure indicates
//! that changes are required which are likely to change the board IP address.
//!
//! \param psNow is a pointer to the currently active configuration parameter
//!        structure.
//! \param psNew is a pointer to the configuration parameter structure
//!        containing the latest changes, as yet unapplied.
//!
//! This function is called to determine whether applying a set of
//! configuration parameter changes will required (or likely result in) a
//! change in the local board's IP address.  The function is used to determine
//! whether changes can be made immediately or whether they should be deferred
//! until later, giving the system a chance to send a warning to the user's web
//! browser.
//!
//! \return Returns \e true if an IP address change is likely to occur when
//! the parameters are applied or \e false if the address will not change.
//
//*****************************************************************************
static bool
ConfigWillIPAddrChange(tConfigParameters const *psNow,
                       tConfigParameters const *psNew)
{
    //
    // Did we switch between DHCP/AUTOIP and static IP address?
    //
    if((psNow->ui8Flags & CONFIG_FLAG_STATICIP) !=
       (psNew->ui8Flags & CONFIG_FLAG_STATICIP))
    {
        //
        // Mode change will almost certainly result in an IP change.
        //
        return(true);
    }

    //
    // If we are using a static IP, check the IP address, subnet mask and
    // gateway address for changes.
    //
    if(psNew->ui8Flags & CONFIG_FLAG_STATICIP)
    {
        //
        // Have any of the addresses changed?
        //
        if((psNew->ui32StaticIP != psNow->ui32StaticIP) ||
           (psNew->ui32GatewayIP != psNow->ui32GatewayIP) ||
           (psNew->ui32SubnetMask != psNow->ui32SubnetMask))
        {
            //
            // Yes - either the local IP address or one of the other important
            // IP parameters changed.
            //
            return(true);
        }
    }

    //
    // If we get this far, the IP address, subnet mask or gateway address are
    // not going to change so return false to tell the caller this.
    //
    return(false);
}

//*****************************************************************************
//
//! \internal
//!
//! Performs processing for the URI ``/defaults.cgi''.
//!
//! \param iIndex is an index into the g_psConfigCGIURIs array indicating which
//! CGI URI has been requested.
//! \param uNumParams is the number of entries in the pcParam and pcValue
//! arrays.
//! \param pcParam is an array of character pointers, each containing the name
//! of a single parameter as encoded in the URI requesting this CGI.
//! \param pcValue is an array of character pointers, each containing the value
//! of a parameter as encoded in the URI requesting this CGI.
//!
//! This function is called whenever the HTTPD server receives a request for
//! URI ``/defaults.cgi''.  Parameters from the request are parsed into the
//! \e pcParam and \e pcValue arrays such that the parameter name and value
//! are contained in elements with the same index.  The strings contained in
//! \e pcParam and \e pcValue contain all replacements and encodings performed
//! by the browser so the CGI function is responsible for reversing these if
//! required.
//!
//! After processing the parameters, the function returns a fully qualified
//! filename to the HTTPD server which will then open this file and send the
//! contents back to the client in response to the CGI.
//!
//! This specific CGI expects no specific parameters and any passed are
//! ignored.
//!
//! \return Returns a pointer to a string containing the file which is to be
//! sent back to the HTTPD client in response to this request.
//
//*****************************************************************************
static const char *
ConfigDefaultsCGIHandler(int iIndex, int iNumParams, char *pcParam[],
                         char *pcValue[])
{
    bool bAddrChange;

    //
    // Will this update cause an IP address change?
    //
    bAddrChange = ConfigWillIPAddrChange(&g_sParameters,
                                         g_psFactoryParameters);

    //
    // Update the working parameter set with the factory defaults.
    //
    ConfigLoadFactory();

    //
    // Save the new defaults to flash.
    //
    ConfigSave();

    //
    // If the IP address won't change, we can apply the other changes
    // immediately.
    //
    if(!bAddrChange)
    {
        //
        // Apply the various changes required as a result of changing back to
        // the default settings.
        //
        ConfigUpdateAllParameters(false);

        //
        // In this case,we send the usual page back to the browser.
        //
        return(DEFAULT_CGI_RESPONSE);
    }
    else
    {
        //
        // The IP address is likely to change so send the browser a warning
        // message and defer the actual update for a couple of seconds by
        // sending a message to the main loop.
        //
        g_ui8UpdateRequired |= UPDATE_ALL;

        //
        // Send back the warning page.
        //
        return(IP_UPDATE_RESPONSE);
    }
}

//*****************************************************************************
//
//! \internal
//!
//! Provides replacement text for each of our configured SSI tags.
//!
//! \param iIndex is an index into the g_pcConfigSSITags array and indicates
//! which tag we are being passed
//! \param pcInsert points to a buffer into which this function should write
//! the replacement text for the tag.  This should be plain text or valid HTML
//! markup.
//! \param iInsertLen is the number of bytes available in the pcInsert buffer.
//! This function must ensure that it does not write more than this or memory
//! corruption will occur.
//!
//! This function is called by the HTTPD server whenever it is serving a page
//! with a ``.ssi'', ``.shtml'' or ``.shtm'' file extension and encounters a
//! tag of the form <!--#tagname--> where ``tagname'' is found in the
//! g_pcConfigSSITags array.  The function writes suitable replacement text to
//! the \e pcInsert buffer.
//!
//! \return Returns the number of bytes written to the pcInsert buffer, not
//! including any terminating NULL.
//
//*****************************************************************************
static uint16_t
ConfigSSIHandler(int iIndex, char *pcInsert, int iInsertLen)
{
    uint32_t ui32Port;
    int iCount;
    const char *pcString;
    char answer[32];
    uint32_t ans_len = 0;
    ip_addr_t *addr_temp;

    //
    // Which SSI tag are we being asked to provide content for?
    //
    switch(iIndex)
    {
        //
        // The local IP address tag "ipaddr".
        //
        case SSI_INDEX_IPADDR:
        {
            uint32_t ui32IPAddr;

            ui32IPAddr = lwIPLocalIPAddrGet();
            return(usnprintf(pcInsert, iInsertLen, "%d.%d.%d.%d",
                             ((ui32IPAddr >>  0) & 0xFF),
                             ((ui32IPAddr >>  8) & 0xFF),
                             ((ui32IPAddr >> 16) & 0xFF),
                             ((ui32IPAddr >> 24) & 0xFF)));
        }

        //
        // The local MAC address tag "macaddr".
        //
        case SSI_INDEX_MACADDR:
        {
            uint8_t pucMACAddr[6];

            lwIPLocalMACGet(pucMACAddr);
            return(usnprintf(pcInsert, iInsertLen,
                             "%02X-%02X-%02X-%02X-%02X-%02X", pucMACAddr[0],
                             pucMACAddr[1], pucMACAddr[2], pucMACAddr[3],
                             pucMACAddr[4], pucMACAddr[5]));
        }

        //
        // These tag are replaced with the current serial port baud rate for
        // their respective ports.
        //
        case SSI_INDEX_P0BR:
        case SSI_INDEX_P1BR:
        {
            ui32Port = (iIndex == SSI_INDEX_P0BR) ? 0 : 1;
            return(usnprintf(pcInsert, iInsertLen, "%d",
                             SerialGetBaudRate(ui32Port)));
        }

        //
        // These tag are replaced with the current number of stop bits for
        // the appropriate serial port.
        //
        case SSI_INDEX_P0SB:
        case SSI_INDEX_P1SB:
        {
            ui32Port = (iIndex == SSI_INDEX_P0SB) ? 0 : 1;
            return(usnprintf(pcInsert, iInsertLen, "%d",
                             SerialGetStopBits(ui32Port)));
        }

        //
        // These tag are replaced with the current parity mode for the
        // appropriate serial port.
        //
        case SSI_INDEX_P0P:
        case SSI_INDEX_P1P:
        {
            ui32Port = (iIndex == SSI_INDEX_P0P) ? 0 : 1;
            pcString = ConfigMapIdToString(g_psParityMap, NUM_PARITY_MAPS,
                                           SerialGetParity(ui32Port));
            return(usnprintf(pcInsert, iInsertLen, "%s", pcString));
        }

        //
        // These tag are replaced with the current number of bits per character
        // for the appropriate serial port.
        //
        case SSI_INDEX_P0BC:
        case SSI_INDEX_P1BC:
        {
            ui32Port = (iIndex == SSI_INDEX_P0BC) ? 0 : 1;
            return(usnprintf(pcInsert, iInsertLen, "%d",
                             SerialGetDataSize(ui32Port)));
        }

        //
        // These tag are replaced with the current flow control settings for
        // the appropriate serial port.
        //
        case SSI_INDEX_P0FC:
        case SSI_INDEX_P1FC:
        {
            ui32Port = (iIndex == SSI_INDEX_P0FC) ? 0 : 1;
            pcString = ConfigMapIdToString(g_psFlowControlMap,
                                           NUM_FLOW_CONTROL_MAPS,
                                           SerialGetFlowControl(ui32Port));
            return(usnprintf(pcInsert, iInsertLen, "%s", pcString));
        }

        //
        // These tag are replaced with the timeout for the appropriate
        // port's telnet session.
        //
        case SSI_INDEX_P0TT:
        case SSI_INDEX_P1TT:
        {
            ui32Port = (iIndex == SSI_INDEX_P0TT) ? 0 : 1;
            return(usnprintf(pcInsert, iInsertLen, "%d",
                             g_sParameters.sPort[ui32Port].ui32TelnetTimeout));
        }

        //
        // These tag are replaced with the local TCP port number in use by the
        // appropriate port's telnet session.
        //
        case SSI_INDEX_P0TLP:
        case SSI_INDEX_P1TLP:
        {
            ui32Port = (iIndex == SSI_INDEX_P0TLP) ? 0 : 1;
            return( usnprintf(pcInsert, iInsertLen, "%d",
                g_sParameters.sPort[ui32Port].ui16TelnetLocalPort));
        }

        //
        // These tag are replaced with the remote TCP port number in use by
        // the appropriate port's telnet session when in client mode.
        //
        case SSI_INDEX_P0TRP:
        case SSI_INDEX_P1TRP:
        {
            ui32Port = (iIndex == SSI_INDEX_P0TRP) ? 0 : 1;
            if((g_sParameters.sPort[ui32Port].ui8Flags & PORT_FLAG_TELNET_MODE) ==
               PORT_TELNET_SERVER)
            {
                return(usnprintf(pcInsert, iInsertLen, "N/A"));
            }
            else
            {
                return(usnprintf(pcInsert, iInsertLen, "%d",
                    g_sParameters.sPort[ui32Port].ui16TelnetRemotePort));
            }
        }

        //
        // These tag are replaced with a string describing the port's current
        // telnet mode, client or server.
        //
        case SSI_INDEX_P0TNM:
        case SSI_INDEX_P1TNM:
        {
            ui32Port = (iIndex == SSI_INDEX_P0TNM) ? 0 : 1;
            return(usnprintf(pcInsert, iInsertLen, "%s",
                             ((g_sParameters.sPort[ui32Port].ui8Flags &
                               PORT_FLAG_TELNET_MODE) == PORT_TELNET_SERVER) ?
                             "Server" : "Client"));
        }

        //
        // These tag are replaced with a string describing the port's current
        // telnet mode, client or server.
        //
        case SSI_INDEX_P0PROT:
        case SSI_INDEX_P1PROT:
        {
            ui32Port = (iIndex == SSI_INDEX_P0PROT) ? 0 : 1;
            return(usnprintf(pcInsert, iInsertLen, "%s",
                             ((g_sParameters.sPort[ui32Port].ui8Flags &
                               PORT_FLAG_PROTOCOL) == PORT_PROTOCOL_TELNET) ?
                             "Telnet" : "Raw"));
        }

        //
        // These tags are replaced with the full destination IP address for
        // the relevant port's telnet connection (which is only valid
        // when operating as a telnet client).
        //
        case SSI_INDEX_P0TIP:
        case SSI_INDEX_P1TIP:
        {
            ui32Port = (iIndex == SSI_INDEX_P0TIP) ? 0 : 1;
            if((g_sParameters.sPort[ui32Port].ui8Flags &
                PORT_FLAG_TELNET_MODE) == PORT_TELNET_SERVER)
            {
                return(usnprintf(pcInsert, iInsertLen, "N/A"));
            }
            else
            {
                return(usnprintf(pcInsert, iInsertLen, "%d.%d.%d.%d",
                    (g_sParameters.sPort[ui32Port].ui32TelnetIPAddr >> 24) & 0xFF,
                    (g_sParameters.sPort[ui32Port].ui32TelnetIPAddr >> 16) & 0xFF,
                    (g_sParameters.sPort[ui32Port].ui32TelnetIPAddr >> 8) & 0xFF,
                    (g_sParameters.sPort[ui32Port].ui32TelnetIPAddr >> 0) & 0xFF));
            }
        }

        //
        // These tags are replaced with the first (most significant) number
        // in an aa.bb.cc.dd IP address (aa in this case).
        //
        case SSI_INDEX_P0TIP1:
        case SSI_INDEX_P1TIP1:
        {
            ui32Port = (iIndex == SSI_INDEX_P0TIP1) ? 0 : 1;
            return(usnprintf(pcInsert, iInsertLen, "%d",
                             (g_sParameters.sPort[ui32Port].ui32TelnetIPAddr >>
                              24) & 0xFF));
        }

        //
        // These tags are replaced with the first (most significant) number
        // in an aa.bb.cc.dd IP address (aa in this case).
        //
        case SSI_INDEX_P0TIP2:
        case SSI_INDEX_P1TIP2:
        {
            ui32Port = (iIndex == SSI_INDEX_P0TIP2) ? 0 : 1;
            return(usnprintf(pcInsert, iInsertLen, "%d",
                             (g_sParameters.sPort[ui32Port].ui32TelnetIPAddr >>
                              16) & 0xFF));
        }

        //
        // These tags are replaced with the first (most significant) number
        // in an aa.bb.cc.dd IP address (aa in this case).
        //
        case SSI_INDEX_P0TIP3:
        case SSI_INDEX_P1TIP3:
        {
            ui32Port = (iIndex == SSI_INDEX_P0TIP3) ? 0 : 1;
            return(usnprintf(pcInsert, iInsertLen, "%d",
                             (g_sParameters.sPort[ui32Port].ui32TelnetIPAddr >>
                              8) & 0xFF));
        }

        //
        // These tags are replaced with the first (most significant) number
        // in an aa.bb.cc.dd IP address (aa in this case).
        //
        case SSI_INDEX_P0TIP4:
        case SSI_INDEX_P1TIP4:
        {
            ui32Port = (iIndex == SSI_INDEX_P0TIP4) ? 0 : 1;
            return(usnprintf(pcInsert, iInsertLen, "%d",
                             g_sParameters.sPort[ui32Port].ui32TelnetIPAddr &
                             0xFF));
        }

        //
        // Generate a block of JavaScript declaring variables which hold the
        // current settings for one of the ports.
        //
        case SSI_INDEX_P0VARS:
        case SSI_INDEX_P1VARS:
        {
            ui32Port = (iIndex == SSI_INDEX_P0VARS) ? 0 : 1;
            iCount = usnprintf(pcInsert, iInsertLen, "%s", JAVASCRIPT_HEADER);
            if(iCount < iInsertLen)
            {
                iCount += usnprintf(pcInsert + iCount, iInsertLen - iCount,
                                    SER_JAVASCRIPT_VARS,
                                    SerialGetBaudRate(ui32Port),
                                    SerialGetStopBits(ui32Port),
                                    SerialGetDataSize(ui32Port),
                                    SerialGetFlowControl(ui32Port),
                                    SerialGetParity(ui32Port));
            }
            if(iCount < iInsertLen)
            {
                iCount += usnprintf(pcInsert + iCount, iInsertLen - iCount,
                                    "%s", JAVASCRIPT_FOOTER);
            }

            return(iCount);
        }

        //
        // Return the user-editable friendly name for the module.
        //
        case SSI_INDEX_MODNAME:
        {
            return(usnprintf(pcInsert, iInsertLen, "%s",
                             g_sParameters.ui8ModName));
        }

        //
        // Initialise JavaScript variables containing the information related
        // to the telnet settings for a given port.
        //
        case SSI_INDEX_P0TVARS:
        case SSI_INDEX_P1TVARS:
        {
            ui32Port = (iIndex == SSI_INDEX_P0TVARS) ? 0 : 1;
            iCount = usnprintf(pcInsert, iInsertLen, "%s", JAVASCRIPT_HEADER);
            if(iCount < iInsertLen)
            {
                iCount +=
                    usnprintf(pcInsert + iCount, iInsertLen - iCount,
                        TN_JAVASCRIPT_VARS,
                        g_sParameters.sPort[ui32Port].ui32TelnetTimeout,
                        g_sParameters.sPort[ui32Port].ui16TelnetLocalPort,
                        g_sParameters.sPort[ui32Port].ui16TelnetRemotePort,
                        (((g_sParameters.sPort[ui32Port].ui8Flags &
                           PORT_FLAG_TELNET_MODE) == PORT_TELNET_SERVER) ?
                         0 : 1),
                        (((g_sParameters.sPort[ui32Port].ui8Flags &
                           PORT_FLAG_PROTOCOL) == PORT_PROTOCOL_TELNET) ?
                         0 : 1));
            }

            if(iCount < iInsertLen)
            {
                iCount += usnprintf(pcInsert + iCount, iInsertLen - iCount,
                                    "%s", JAVASCRIPT_FOOTER);
            }
            return(iCount);
        }

        //
        // Initialise JavaScript variables containing the information related
        // to the telnet settings for a given port.
        //
        case SSI_INDEX_P0IPVAR:
        case SSI_INDEX_P1IPVAR:
        {
            ui32Port = (iIndex == SSI_INDEX_P0IPVAR) ? 0 : 1;
            iCount = usnprintf(pcInsert, iInsertLen, "%s", JAVASCRIPT_HEADER);
            if(iCount < iInsertLen)
            {
                iCount +=
                    usnprintf(pcInsert + iCount, iInsertLen - iCount,
                        TIP_JAVASCRIPT_VARS,
                        (g_sParameters.sPort[ui32Port].ui32TelnetIPAddr >> 24) & 0xFF,
                        (g_sParameters.sPort[ui32Port].ui32TelnetIPAddr >> 16) & 0xFF,
                        (g_sParameters.sPort[ui32Port].ui32TelnetIPAddr >> 8) & 0xFF,
                        (g_sParameters.sPort[ui32Port].ui32TelnetIPAddr >> 0) & 0xFF);
            }

            if(iCount < iInsertLen)
            {
                iCount += usnprintf(pcInsert + iCount, iInsertLen - iCount,
                                    "%s", JAVASCRIPT_FOOTER);
            }
            return(iCount);
        }

        //
        // Generate a block of JavaScript variables containing the current
        // static UP address and static/DHCP setting.
        //
        case SSI_INDEX_IPVARS:
        {
            iCount = usnprintf(pcInsert, iInsertLen, "%s", JAVASCRIPT_HEADER);
            if(iCount < iInsertLen)
            {
                iCount += usnprintf(pcInsert + iCount, iInsertLen - iCount,
                                    IP_JAVASCRIPT_VARS,
                                    (g_sParameters.ui8Flags &
                                     CONFIG_FLAG_STATICIP) ? 1 : 0,
                                    (g_sParameters.ui32StaticIP >> 24) & 0xFF,
                                    (g_sParameters.ui32StaticIP >> 16) & 0xFF,
                                    (g_sParameters.ui32StaticIP >> 8) & 0xFF,
                                    (g_sParameters.ui32StaticIP >> 0) & 0xFF);
            }

            if(iCount < iInsertLen)
            {
                iCount += usnprintf(pcInsert + iCount, iInsertLen - iCount,
                                    "%s", JAVASCRIPT_FOOTER);
            }
            return(iCount);
        }

        //
        // Generate a block of JavaScript variables containing the current
        // subnet mask.
        //
        case SSI_INDEX_SNVARS:
        {
            iCount = usnprintf(pcInsert, iInsertLen, "%s", JAVASCRIPT_HEADER);
            if(iCount < iInsertLen)
            {
                iCount += usnprintf(pcInsert + iCount, iInsertLen - iCount,
                    SUBNET_JAVASCRIPT_VARS,
                    (g_sParameters.ui32SubnetMask >> 24) & 0xFF,
                    (g_sParameters.ui32SubnetMask >> 16) & 0xFF,
                    (g_sParameters.ui32SubnetMask >> 8) & 0xFF,
                    (g_sParameters.ui32SubnetMask >> 0) & 0xFF);
            }

            if(iCount < iInsertLen)
            {
                iCount += usnprintf(pcInsert + iCount, iInsertLen - iCount,
                                    "%s", JAVASCRIPT_FOOTER);
            }
            return(iCount);
        }

        //
        // Generate a block of JavaScript variables containing the current
        // subnet mask.
        //
        case SSI_INDEX_GWVARS:
        {
            iCount = usnprintf(pcInsert, iInsertLen, "%s", JAVASCRIPT_HEADER);
            if(iCount < iInsertLen)
            {
                iCount += usnprintf(pcInsert + iCount, iInsertLen - iCount,
                                    GW_JAVASCRIPT_VARS,
                                    (g_sParameters.ui32GatewayIP >> 24) & 0xFF,
                                    (g_sParameters.ui32GatewayIP >> 16) & 0xFF,
                                    (g_sParameters.ui32GatewayIP >> 8) & 0xFF,
                                    (g_sParameters.ui32GatewayIP >> 0) & 0xFF);
            }

            if(iCount < iInsertLen)
            {
                iCount += usnprintf(pcInsert + iCount, iInsertLen - iCount,
                                    "%s", JAVASCRIPT_FOOTER);
            }
            return(iCount);
        }

        //
        // Generate an HTML text input field containing the current module
        // name.
        //
        case SSI_INDEX_MODNINP:
        {
            iCount = usnprintf(pcInsert, iInsertLen, "<input value='");

            if(iCount < iInsertLen)
            {
                iCount +=
                    ConfigEncodeFormString((char *)g_sParameters.ui8ModName,
                                           pcInsert + iCount,
                                           iInsertLen - iCount);
            }

            if(iCount < iInsertLen)
            {
                iCount +=
                    usnprintf(pcInsert + iCount, iInsertLen - iCount,
                              "' maxlength='%d' size='%d' name='modname'>",
                              (MOD_NAME_LEN - 1), MOD_NAME_LEN);
            }
            return(iCount);
        }
        case SSI_INDEX_TECLIMP0:
        case SSI_INDEX_TECLIMP1:
        {
            ui32Port = (iIndex == SSI_INDEX_TECLIMP0) ? 0 : 1;

            ans_len = Read_TEC(ui32Port, 0, answer);
            return(usnprintf(pcInsert, iInsertLen, "%s", answer));
        }
        case SSI_INDEX_TECLIMN0:
        case SSI_INDEX_TECLIMN1:
        {
            ui32Port = (iIndex == SSI_INDEX_TECLIMN0) ? 0 : 1;

            ans_len = Read_TEC(ui32Port, 3, answer);
            return(usnprintf(pcInsert, iInsertLen, "%s", answer));
        }
        case SSI_INDEX_TECCUR0:
        case SSI_INDEX_TECVOL0:
        {
            ui32Port = (iIndex == SSI_INDEX_TECCUR0) ? 2 : 1; //!this is not port id, but curr/vol id
            ans_len = Read_TEC(0, ui32Port, answer);
            return(usnprintf(pcInsert, iInsertLen, "%s", answer));
        }
        case SSI_INDEX_TECCUR1:
        case SSI_INDEX_TECVOL1:
        {
            ui32Port = (iIndex == SSI_INDEX_TECCUR1) ? 2 : 1; //!this is not port id, but curr/vol id
            ans_len = Read_TEC(1, ui32Port, answer);
            return(usnprintf(pcInsert, iInsertLen, "%s", answer));
        }	
        case SSI_INDEX_TEMPSET0:
        case SSI_INDEX_TEMPSET1:
        {
            ui32Port = (iIndex == SSI_INDEX_TEMPSET0) ? 0 : 1;

            ans_len = Read_Temp(ui32Port, 0, answer);

            if(answer[0] == '-')
            {
                if(ans_len == 2) {
                    answer[6] = '\0';
                    answer[5] = answer[1];
                    answer[4] = '0';
                    answer[3] = '0';
                    answer[2] = '.';
                    answer[1] = '0';
                } else if(ans_len == 3)
                {
                    answer[6] = '\0';
                    answer[5] = answer[2];
                    answer[4] = answer[1];
                    answer[3] = '0';
                    answer[2] = '.';
                    answer[1] = '0';
                } else if(ans_len == 4)
                {
                    answer[6] = '\0';
                    answer[5] = answer[3];
                    answer[4] = answer[2];
                    answer[3] = answer[1];
                    answer[2] = '.';
                    answer[1] = '0';
                } else if(ans_len == 5)
                {
                    answer[6] = '\0';
                    answer[5] = answer[4];
                    answer[4] = answer[3];
                    answer[3] = answer[2];
                    answer[2] = '.';
                } else if(ans_len == 6)
                {
                    answer[7] = '\0';
                    answer[6] = answer[5];
                    answer[5] = answer[4];
                    answer[4] = answer[3];
                    answer[3] = '.';
                } else if(ans_len == 7)
                {
                    answer[8] = '\0';
                    answer[7] = answer[5];
                    answer[6] = answer[4];
                    answer[5] = answer[3];
                    answer[4] = '.';
                }
            } else
            {
                if(ans_len == 1) {
                    answer[5] = '\0';
                    answer[4] = answer[0];
                    answer[3] = '0';
                    answer[2] = '0';
                    answer[1] = '.';
                    answer[0] = '0';
                } else if(ans_len == 2)
                {
                    answer[5] = '\0';
                    answer[4] = answer[1];
                    answer[3] = answer[0];
                    answer[2] = '0';
                    answer[1] = '.';
                    answer[0] = '0';
                } else if(ans_len == 3)
                {
                    answer[5] = '\0';
                    answer[4] = answer[2];
                    answer[3] = answer[1];
                    answer[2] = answer[0];
                    answer[1] = '.';
                    answer[0] = '0';
                } else if(ans_len == 4)
                {
                    answer[5] = '\0';
                    answer[4] = answer[3];
                    answer[3] = answer[2];
                    answer[2] = answer[1];
                    answer[1] = '.';
                } else if(ans_len == 5)
                {
                    answer[6] = '\0';
                    answer[5] = answer[4];
                    answer[4] = answer[3];
                    answer[3] = answer[2];
                    answer[2] = '.';
                } else if(ans_len == 6)
                {
                    answer[7] = '\0';
                    answer[6] = answer[4];
                    answer[5] = answer[3];
                    answer[4] = answer[2];
                    answer[3] = '.';
                }
            }

            return(usnprintf(pcInsert, iInsertLen, "%s", answer));
        }
        case SSI_INDEX_TEMPACT0:
        case SSI_INDEX_TEMPACT1:
        {
            ui32Port = (iIndex == SSI_INDEX_TEMPACT0) ? 0 : 1;

            ans_len = Read_Temp(ui32Port, 1, answer);

            if(answer[0] == '-')
            {
                if(ans_len == 2) {
                    answer[6] = '\0';
                    answer[5] = answer[1];
                    answer[4] = '0';
                    answer[3] = '0';
                    answer[2] = '.';
                    answer[1] = '0';
                } else if(ans_len == 3)
                {
                    answer[6] = '\0';
                    answer[5] = answer[2];
                    answer[4] = answer[1];
                    answer[3] = '0';
                    answer[2] = '.';
                    answer[1] = '0';
                } else if(ans_len == 4)
                {
                    answer[6] = '\0';
                    answer[5] = answer[3];
                    answer[4] = answer[2];
                    answer[3] = answer[1];
                    answer[2] = '.';
                    answer[1] = '0';
                } else if(ans_len == 5)
                {
                    answer[6] = '\0';
                    answer[5] = answer[4];
                    answer[4] = answer[3];
                    answer[3] = answer[2];
                    answer[2] = '.';
                } else if(ans_len == 6)
                {
                    answer[7] = '\0';
                    answer[6] = answer[5];
                    answer[5] = answer[4];
                    answer[4] = answer[3];
                    answer[3] = '.';
                } else if(ans_len == 7)
                {
                    answer[8] = '\0';
                    answer[7] = answer[5];
                    answer[6] = answer[4];
                    answer[5] = answer[3];
                    answer[4] = '.';
                }
            } else
            {
                if(ans_len == 1) {
                    answer[5] = '\0';
                    answer[4] = answer[0];
                    answer[3] = '0';
                    answer[2] = '0';
                    answer[1] = '.';
                    answer[0] = '0';
                } else if(ans_len == 2)
                {
                    answer[5] = '\0';
                    answer[4] = answer[1];
                    answer[3] = answer[0];
                    answer[2] = '0';
                    answer[1] = '.';
                    answer[0] = '0';
                } else if(ans_len == 3)
                {
                    answer[5] = '\0';
                    answer[4] = answer[2];
                    answer[3] = answer[1];
                    answer[2] = answer[0];
                    answer[1] = '.';
                    answer[0] = '0';
                } else if(ans_len == 4)
                {
                    answer[5] = '\0';
                    answer[4] = answer[3];
                    answer[3] = answer[2];
                    answer[2] = answer[1];
                    answer[1] = '.';
                } else if(ans_len == 5)
                {
                    answer[6] = '\0';
                    answer[5] = answer[4];
                    answer[4] = answer[3];
                    answer[3] = answer[2];
                    answer[2] = '.';
                } else if(ans_len == 6)
                {
                    answer[7] = '\0';
                    answer[6] = answer[4];
                    answer[5] = answer[3];
                    answer[4] = answer[2];
                    answer[3] = '.';
                }
            }

            return(usnprintf(pcInsert, iInsertLen, "%s", answer));
        }
        case SSI_INDEX_TEMPWIN0:
        case SSI_INDEX_TEMPWIN1:
        {
            ui32Port = (iIndex == SSI_INDEX_TEMPWIN0) ? 0 : 1;

            ans_len = Read_Temp(ui32Port, 2, answer);

            return(usnprintf(pcInsert, iInsertLen, "%s", answer));
        }		
		case SSI_INDEX_CLP0:
		case SSI_INDEX_CLP1:
        {
            ui32Port = (iIndex == SSI_INDEX_CLP0) ? 0 : 1;
            ans_len = Read_PID(ui32Port, 0, answer);
            return(usnprintf(pcInsert, iInsertLen, "%s", answer));
        }
		case SSI_INDEX_CLI0:
		case SSI_INDEX_CLI1:
        {
            ui32Port = (iIndex == SSI_INDEX_CLI0) ? 0 : 1;
            ans_len = Read_PID(ui32Port, 1, answer);
            return(usnprintf(pcInsert, iInsertLen, "%s", answer));
        }
		case SSI_INDEX_CLD0:
		case SSI_INDEX_CLD1:
        {
            ui32Port = (iIndex == SSI_INDEX_CLD0) ? 0 : 1;
            ans_len = Read_PID(ui32Port, 2, answer);
            return(usnprintf(pcInsert, iInsertLen, "%s", answer));
        }

        case SSI_INDEX_T00:
        case SSI_INDEX_T01:
        {
            ui32Port = (iIndex == SSI_INDEX_T00) ? 0 : 1;

            ans_len = Read_Calib(ui32Port, 0, answer);
            return(usnprintf(pcInsert, iInsertLen, "%s", answer));
        }
        case SSI_INDEX_BETA0:
        case SSI_INDEX_BETA1:
        {
            ui32Port = (iIndex == SSI_INDEX_BETA0) ? 0 : 1;

            ans_len = Read_Calib(ui32Port, 1, answer);
            return(usnprintf(pcInsert, iInsertLen, "%s", answer));
        }
        case SSI_INDEX_RATIO0:
        case SSI_INDEX_RATIO1:
        {
            ui32Port = (iIndex == SSI_INDEX_RATIO0) ? 0 : 1;

            ans_len = Read_Calib(ui32Port, 2, answer);
            return(usnprintf(pcInsert, iInsertLen, "%s", answer));
        }
        case SSI_INDEX_TEMPMOD0:
        case SSI_INDEX_TEMPMOD1:
        {
            ui32Port = (iIndex == SSI_INDEX_TEMPMOD0) ? 0 : 1;

            ans_len = Read_Calib(ui32Port, 4, answer);
            return(usnprintf(pcInsert, iInsertLen, "%s", answer));
        }
        case SSI_INDEX_PTA0:
        case SSI_INDEX_PTA1:
        {
            ui32Port = (iIndex == SSI_INDEX_PTA0) ? 0 : 1;

            ans_len = Read_Calib(ui32Port, 5, answer);
            return(usnprintf(pcInsert, iInsertLen, "%s", answer));
        }
        case SSI_INDEX_PTB0:
        case SSI_INDEX_PTB1:
        {
            ui32Port = (iIndex == SSI_INDEX_PTB0) ? 0 : 1;

            ans_len = Read_Calib(ui32Port, 6, answer);
            return(usnprintf(pcInsert, iInsertLen, "%s", answer));
        }
        case SSI_INDEX_SWFREQ0:
        case SSI_INDEX_SWFREQ1:
        {
            ui32Port = (iIndex == SSI_INDEX_SWFREQ0) ? 0 : 1;

            ans_len = Read_Calib(ui32Port, 3, answer);
            return(usnprintf(pcInsert, iInsertLen, "%s", answer));
        }
        case SSI_INDEX_RAWTEMP0:
        case SSI_INDEX_RAWTEMP1:
        {
            ui32Port = (iIndex == SSI_INDEX_RAWTEMP0) ? 0 : 1;

            ans_len = Read_RawTemp(ui32Port, answer);
            return(usnprintf(pcInsert, iInsertLen, "%s", answer));
        }

        case SSI_INDEX_MAXVOLT0:
        case SSI_INDEX_MAXVOLT1:
        {
            ui32Port = (iIndex == SSI_INDEX_MAXVOLT0) ? 0 : 1;

            ans_len = Read_MaxVolt(ui32Port, answer);
            return(usnprintf(pcInsert, iInsertLen, "%s", answer));
        }

        case SSI_INDEX_ONOFF0:
        case SSI_INDEX_ONOFF1:
        {
            ui32Port = (iIndex == SSI_INDEX_ONOFF0) ? 0 : 1;

            if(ui32Port == 0) {
                if(onoff0 == 0) {
                    strcpy(answer,"OFF");
                } else {
                    strcpy(answer,"ON");
                }
            } else {
                if(onoff1 == 0) {
                    strcpy(answer,"OFF");
                } else {
                    strcpy(answer,"ON");
                }
            }
            return(usnprintf(pcInsert, iInsertLen, "%s", answer));
        }

        case SSI_INDEX_ONOFF0R:
        case SSI_INDEX_ONOFF1R:
        {
            ui32Port = (iIndex == SSI_INDEX_ONOFF0R) ? 0 : 1;

            if(ui32Port == 0) {
                if(onoff0 == 0) {
                    strcpy(answer,"ON");
                } else {
                    strcpy(answer,"OFF");
                }
            } else {
                if(onoff1 == 0) {
                    strcpy(answer,"ON");
                } else {
                    strcpy(answer,"OFF");
                }
            }
            return(usnprintf(pcInsert, iInsertLen, "%s", answer));
        }

        case SSI_INDEX_MQTTIP:
        {
            addr_temp = (ip_addr_t*) mem_malloc(sizeof(ip_addr_t));
            addr_temp->addr = g_sParameters.ui32mqttip;
            strcpy(answer,ipaddr_ntoa(addr_temp));
            mem_free(addr_temp);
            return(usnprintf(pcInsert, iInsertLen, "%s", answer));
        }

        case SSI_INDEX_MQTTCONN:
        {
            if(mqtt_conn) {
                strcpy(answer,"MQTT connected");
            } else {
                strcpy(answer,"MQTT not connected");
            }
            return(usnprintf(pcInsert, iInsertLen, "%s", answer));
        }

        //
        // All other tags are unknown.
        //
        default:
        {
            return(usnprintf(pcInsert, iInsertLen,
                             "<b><i>Tag %d unknown!</i></b>", iIndex));
        }
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
