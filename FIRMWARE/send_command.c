/*
 * send_command.c
 *
 *
 */


#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/ringbuf.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "config.h"
#include "serial.h"
#include "send_command.h"
#include "utils/ustdlib.h"
#include "internal_adc.h"
#include "pwm.h"
#include "leds.h"
#include "uart_th.h"

//*****************************************************************************
//
// External References.
//
//*****************************************************************************
extern uint32_t g_ui32SysClock;

//*****************************************************************************
//
//! \addtogroup serial_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//



//*****************************************************************************
//
//! Function to call general command to controller
//!
//! \param ui32Port holds port number
//!
//! \param mode is the identifier of command
//!
//! \param answerBuf pointer to answer
//!
//! \return returns length of the answer
//
//*****************************************************************************
uint32_t General_Command(uint32_t ui32Port, uint32_t mode, char *answerBuf)
{

    //serial number?
    uint32_t ans_len;

    ans_len = usnprintf(answerBuf, 32, "SNumber");  //reads SN


    return ans_len;
}

//*****************************************************************************
//
//! Function to set tec settings of controller
//!
//! \param ui32Port holds port number
//!
//! \param limit is tec current limit
//!
//!
//! \return true in case of success and false otherwise
//
//*****************************************************************************
bool Set_TEC_limit(uint32_t ui32Port, int32_t limit, uint32_t PosNeg)
{

    if(limit >= 0 && limit <= 3000)
    {
        if(ui32Port == 0 && PosNeg == 0) {
            g_sParameters.ui16MaxPosCurr0 = limit;
            setMaxPosCurr0((uint16_t) limit);
        }
        if(ui32Port == 0 && PosNeg == 1) {
            g_sParameters.ui16MaxNegCurr0 = limit;
            setMaxNegCurr0((uint16_t) limit);
        }
        if(ui32Port == 1 && PosNeg == 0) {
            g_sParameters.ui16MaxPosCurr1 = limit;
            setMaxPosCurr1((uint16_t) limit);
        }
        if(ui32Port == 1 && PosNeg == 1) {
            g_sParameters.ui16MaxNegCurr1 = limit;
            setMaxNegCurr1((uint16_t) limit);
        }
        return true;
    }  else
    {
        return false;
    }
}

//*****************************************************************************
//
//! Function to call read of tec paameters
//!
//! \param ui32Port holds port number
//!
//! \param mode is the identifier of wanted parameter
//!
//! \param answerBuf pointer to answer
//!
//! \return returns length of the answer
//
//*****************************************************************************
uint32_t Read_TEC(uint32_t ui32Port, uint32_t mode, char *answerBuf)
{
    ASSERT((mode == 0) || (mode == 1) || (mode == 2) || (mode == 3));

    uint32_t ans_len;
    uint32_t ADCValue[2];
    int32_t result[3];

    if (mode == 0) { //current pos limit
        switch(ui32Port)
        {
        case 0:
            ans_len = usnprintf(answerBuf, 32, "%d", g_sParameters.ui16MaxPosCurr0);  //reads tec limit
            break;
        case 1:
            ans_len = usnprintf(answerBuf, 32, "%d", g_sParameters.ui16MaxPosCurr1);  //reads tec limit
            break;
        }
    } else if (mode == 3) { //current neg limit
        switch(ui32Port)
        {
        case 0:
            ans_len = usnprintf(answerBuf, 32, "%d", g_sParameters.ui16MaxNegCurr0);  //reads tec limit
            break;
        case 1:
            ans_len = usnprintf(answerBuf, 32, "%d", g_sParameters.ui16MaxNegCurr1);  //reads tec limit
            break;
        }
    } else {
        switch(ui32Port)
        {
        case 0:
            internal_adc_getvalues0(ADCValue);
            result[1] = 3300*ADCValue[1]/4096; //voltage
            result[2] = 3300*((int32_t) ADCValue[0] - (int32_t) ADCValue[1])/1638; //1638 = 4096 * (8*0.05) = 4096 * 8 * Rsense
            break;
        case 1:
            internal_adc_getvalues1(ADCValue);
            result[1] = 3300*ADCValue[1]/4096; //voltage
            result[2] = 3300*((int32_t) ADCValue[0] - (int32_t) ADCValue[1])/1638; //1638 = 4096 * (8*0.05) = 4096 * 8 * Rsense
            break;
        }

        ans_len = usnprintf(answerBuf, 32, "%d", result[mode]);
    }

    return ans_len;
}


//*****************************************************************************
//
//! Function to set temperature
//!
//! \param ui32Port holds port number
//!
//! \param temp is wanted temperature
//!
//!
//! \return true in case of success and false otherwise
//
//*****************************************************************************
bool Set_Temp(uint32_t ui32Port, int32_t temp)
{
    if((temp < 0) || (temp >= 50000)) {
        return false;
    }

    if(ui32Port == 0) {
        g_sParameters.i32SetTemp0 = temp;
    } else {
        g_sParameters.i32SetTemp1 = temp;
    }
    return true;
}

//*****************************************************************************
//
//! Function to set temperature window
//!
//! \param ui32Port holds port number
//!
//! \param temp_window is wanted temperature window
//!
//!
//! \return true in case of success and false otherwise
//
//*****************************************************************************
bool Set_Temp_Window(uint32_t ui32Port, uint32_t temp_window)
{
    if(temp_window < 1 || temp_window > 10000) {
        return false;
    }

    if(ui32Port == 0) {
        g_sParameters.ui16TempWindow0 = temp_window;
    } else {
        g_sParameters.ui16TempWindow1 = temp_window;
    }

    return true;
}



//*****************************************************************************
//
//! Function to call read of temperature parameters
//!
//! \param ui32Port holds port number
//!
//! \param mode is the identifier of wanted parameter
//!
//! \param answerBuf pointer to answer
//!
//! \return returns length of the answer
//
//*****************************************************************************
uint32_t Read_Temp(uint32_t ui32Port, uint32_t mode, char *answerBuf)
{
    ASSERT((mode == 0) || (mode == 1) || (mode == 2));

    uint32_t ans_len;

    if(ui32Port == 0) {
        switch(mode)
        {
        case 0:
            ans_len = usnprintf(answerBuf, 32, "%d", g_sParameters.i32SetTemp0);  //reads the set temperature
            break;
        case 1:
            ans_len = usnprintf(answerBuf, 32, "%d", i32ActTemp0); //reads the actual temperature
            break;
        case 2:
            ans_len = usnprintf(answerBuf, 32, "%d", g_sParameters.ui16TempWindow0);
            break;
        }
    } else {
        switch(mode)
        {
        case 0:
            ans_len = usnprintf(answerBuf, 32, "%d", g_sParameters.i32SetTemp1);  //reads the set temperature
            break;
        case 1:
            ans_len = usnprintf(answerBuf, 32, "%d", i32ActTemp1); //reads the actual temperature
            break;
        case 2:
            ans_len = usnprintf(answerBuf, 32, "%d", g_sParameters.ui16TempWindow1);
            break;
        }
    }

    return ans_len;
}



//*****************************************************************************
//
//! Function to set PID parameters
//!
//! \param ui32Port holds port number
//!
//! \param mode identifies parameter to set
//!
//! \param pid is wanted value
//!
//!
//! \return true in case of success and false otherwise
//
//*****************************************************************************
bool Set_PID(uint32_t ui32Port, uint32_t mode, int32_t pid)
{

    ASSERT((mode == 0) || (mode == 1) || (mode == 2));

    if(pid < -10000 || pid > 10000) {
        return false;
    }

    if(ui32Port == 0) {
        switch(mode)
        {
        case 0:
            g_sParameters.i16clp0 = pid;
            break;
        case 1:
            g_sParameters.i16cli0 = pid;
            break;
        case 2:
            g_sParameters.i16cld0 = pid;
            break;
        }
    } else {
        switch(mode)
        {
        case 0:
            g_sParameters.i16clp1 = pid;
            break;
        case 1:
            g_sParameters.i16cli1 = pid;
            break;
        case 2:
            g_sParameters.i16cld1 = pid;
            break;
        }
    }

    return true;
}

//*****************************************************************************
//
//! Function to call read of control loop parameters
//!
//! \param ui32Port holds port number
//!
//! \param mode is the identifier of wanted parameter
//!
//! \param answerBuf pointer to answer
//!
//! \return returns length of the answer
//
//*****************************************************************************
uint32_t Read_PID(uint32_t ui32Port, uint32_t mode, char *answerBuf)
{
    ASSERT((mode == 0) || (mode == 1) || (mode == 2));

    uint32_t ans_len;

    if(ui32Port == 0) {
        switch(mode)
        {
        case 0:
            ans_len = usnprintf(answerBuf, 32, "%d", g_sParameters.i16clp0);  //reads p
            break;
        case 1:
            ans_len = usnprintf(answerBuf, 32, "%d", g_sParameters.i16cli0); //reads i
            break;
        case 2:
            ans_len = usnprintf(answerBuf, 32, "%d", g_sParameters.i16cld0); //reads d
            break;
        }
    } else {
        switch(mode)
        {
        case 0:
            ans_len = usnprintf(answerBuf, 32, "%d", g_sParameters.i16clp1);  //reads p
            break;
        case 1:
            ans_len = usnprintf(answerBuf, 32, "%d", g_sParameters.i16cli1); //reads i
            break;
        case 2:
            ans_len = usnprintf(answerBuf, 32, "%d", g_sParameters.i16cld1); //reads d
            break;
        }
    }

    return ans_len;
}

//*****************************************************************************
//
//! Function to set calib parameters
//!
//! \param ui32Port holds port number
//!
//! \param mode identifies parameter to set
//!
//! \param calib is wanted value
//!
//!
//! \return true in case of success and false otherwise
//
//*****************************************************************************
bool Set_Calib(uint32_t ui32Port, uint32_t mode, int32_t calib)
{

    ASSERT((mode == 0) || (mode == 1) || (mode == 2) || (mode == 3) || (mode == 4) || (mode == 5) || (mode == 6));

    switch(mode)
    {
    case 0: //t0
        if(calib < 0 || calib > 50000 ) {
            return false;
        }
        if(ui32Port == 0) {
            g_sParameters.i32T00 = calib;
        } else {
            g_sParameters.i32T01 = calib;
        }
        break;
    case 1: //beta
        if(calib < -20000000 || calib > 20000000 ) {
            return false;
        }
        if(ui32Port == 0) {
            g_sParameters.i32Beta0 = calib;
        } else {
            g_sParameters.i32Beta1 = calib;
        }
        break;
    case 2: //ratio
        if(calib < 0 || calib > 100000 ) {
            return false;
        }
        if(ui32Port == 0) {
            g_sParameters.i32Ratio0 = calib;
        } else {
            g_sParameters.i32Ratio1 = calib;
        }
        break;
    case 3: //swfreq
        if(calib != 500 && calib != 1000 ) {
            return false;
        }
        if(ui32Port == 0) {
            g_sParameters.ui16SwFreq0 = calib;
        } else {
            g_sParameters.ui16SwFreq1 = calib;
        }
        SetFreq(ui32Port, g_sParameters.ui16SwFreq0);
        break;
    case 4: //tempmode
        if(calib != 0 && calib != 1 ) {
            return false;
        }
        if(ui32Port == 0) {
            g_sParameters.ui16TempMode0 = calib;
        } else {
            g_sParameters.ui16TempMode1 = calib;
        }
        break;
    case 5: //ptA
        if(calib < -20000000 || calib > 20000000 ) {
            return false;
        }
        if(ui32Port == 0) {
            g_sParameters.i32ptA0 = calib;
        } else {
            g_sParameters.i32ptA1 = calib;
        }
        break;
    case 6: //ptB
        if(calib < -20000000 || calib > 20000000 ) {
            return false;
        }
        if(ui32Port == 0) {
            g_sParameters.i32ptB0 = calib;
        } else {
            g_sParameters.i32ptB1 = calib;
        }
        break;

    }
    return true;
}

//*****************************************************************************
//
//! Function to call read of control loop parameters
//!
//! \param ui32Port holds port number
//!
//! \param mode is the identifier of wanted parameter
//!
//! \param answerBuf pointer to answer
//!
//! \return returns length of the answer
//
//*****************************************************************************
uint32_t Read_Calib(uint32_t ui32Port, uint32_t mode, char *answerBuf)
{
    ASSERT((mode == 0) || (mode == 1) || (mode == 2) || (mode == 3) || (mode == 4) || (mode == 5) || (mode == 6));

    uint32_t ans_len;

    if(ui32Port == 0) {
        switch(mode)
        {
        case 0:
            ans_len = usnprintf(answerBuf, 32, "%d", g_sParameters.i32T00);  //reads t0
            break;
        case 1:
            ans_len = usnprintf(answerBuf, 32, "%d", g_sParameters.i32Beta0); //reads beta
            break;
        case 2:
            ans_len = usnprintf(answerBuf, 32, "%d", g_sParameters.i32Ratio0); //reads ratio
            break;
        case 3:
            ans_len = usnprintf(answerBuf, 32, "%d", g_sParameters.ui16SwFreq0); //reads swfreq
            break;
        case 4:
            if(g_sParameters.ui16TempMode0 == 0) {  //reads tempmode
                ans_len = usnprintf(answerBuf, 32, "NTC/PTC");
            } else {
                ans_len = usnprintf(answerBuf, 32, "Pt100/Pt1000");
            }
            break;
        case 5:
            ans_len = usnprintf(answerBuf, 32, "%d", g_sParameters.i32ptA0); //reads pta
            break;
        case 6:
            ans_len = usnprintf(answerBuf, 32, "%d", g_sParameters.i32ptB0); //reads ptb
            break;
        }
    } else {
        switch(mode)
        {
        case 0:
            ans_len = usnprintf(answerBuf, 32, "%d", g_sParameters.i32T01);  //reads t0
            break;
        case 1:
            ans_len = usnprintf(answerBuf, 32, "%d", g_sParameters.i32Beta1); //reads beta
            break;
        case 2:
            ans_len = usnprintf(answerBuf, 32, "%d", g_sParameters.i32Ratio1); //reads ratio
            break;
        case 3:
            ans_len = usnprintf(answerBuf, 32, "%d", g_sParameters.ui16SwFreq1); //reads swfreq
            break;
        case 4:
            if(g_sParameters.ui16TempMode1 == 0) {  //reads tempmode
                ans_len = usnprintf(answerBuf, 32, "NTC/PTC");
            } else {
                ans_len = usnprintf(answerBuf, 32, "Pt100/Pt1000");
            }
            break;
        case 5:
            ans_len = usnprintf(answerBuf, 32, "%d", g_sParameters.i32ptA1); //reads pta
            break;
        case 6:
            ans_len = usnprintf(answerBuf, 32, "%d", g_sParameters.i32ptB1); //reads ptb
            break;
        }
    }

    if(mode != 3 && mode != 4 && ans_len > 3) {
        answerBuf[ans_len] = answerBuf[ans_len-1];
        answerBuf[ans_len-1] = answerBuf[ans_len-2];
        answerBuf[ans_len-2] = answerBuf[ans_len-3];
        answerBuf[ans_len-3] = '.';
        answerBuf[ans_len+1] = '\0';
        ans_len = ans_len + 1;
    } else if(mode != 3 && mode != 4 && ans_len == 3) {
        answerBuf[ans_len+1] = answerBuf[ans_len-1];
        answerBuf[ans_len] = answerBuf[ans_len-2];
        answerBuf[ans_len-1] = answerBuf[ans_len-3];
        answerBuf[ans_len-2] = '.';
        answerBuf[ans_len-3] = '0';
        answerBuf[ans_len+2] = '\0';
        ans_len = ans_len + 2;
    } else if(mode != 3 && mode != 4 && ans_len == 2) {
        answerBuf[ans_len+2] = answerBuf[ans_len-1];
        answerBuf[ans_len+1] = answerBuf[ans_len-2];
        answerBuf[ans_len] = '0';
        answerBuf[ans_len-1] = '.';
        answerBuf[ans_len-2] = '0';
        answerBuf[ans_len+3] = '\0';
        ans_len = ans_len + 3;
    } else if(mode != 3 && mode != 4 && ans_len == 1) {
        answerBuf[ans_len+3] = answerBuf[ans_len-1];
        answerBuf[ans_len+2] = '0';
        answerBuf[ans_len+1] = '0';
        answerBuf[ans_len] = '.';
        answerBuf[ans_len-1] = '0';
        answerBuf[ans_len+4] = '\0';
        ans_len = ans_len + 3;
    }

    return ans_len;
}

//*****************************************************************************
//
//! Function to call read of raw temp
//!
//! \param ui32Port holds port number
//!
//! \param mode is the identifier of wanted parameter
//!
//! \param answerBuf pointer to answer
//!
//! \return returns length of the answer
//
//*****************************************************************************
uint32_t Read_RawTemp(uint32_t ui32Port, char *answerBuf)
{

    uint32_t ans_len;

    if(ui32Port == 0) {
        ans_len = usnprintf(answerBuf, 32, "%d", ui32RawTemp0);
    } else {
        ans_len = usnprintf(answerBuf, 32, "%d", ui32RawTemp1);
    }

    return ans_len;
}

//*****************************************************************************
//
//! Function to call read of max volt
//!
//! \param ui32Port holds port number
//!
//! \param mode is the identifier of wanted parameter
//!
//! \param answerBuf pointer to answer
//!
//! \return returns length of the answer
//
//*****************************************************************************
uint32_t Read_MaxVolt(uint32_t ui32Port, char *answerBuf)
{

    uint32_t ans_len;

    if(ui32Port == 0) {
        ans_len = usnprintf(answerBuf, 32, "%d", g_sParameters.ui16MaxVolt0);
    } else {
        ans_len = usnprintf(answerBuf, 32, "%d", g_sParameters.ui16MaxVolt1);
    }

    return ans_len;
}


//*****************************************************************************
//
//! Function to On/Off TEC driver
//!
//! \param ui32Port holds port number
//!
//! \param mode identifies parameter to set
//!
//! \param onoff is wanted behavior
//!
//!
//! \return true in case of success and false otherwise
//
//*****************************************************************************
bool OnOff(uint32_t ui32Port, int32_t onoff) {

    ASSERT((ui32Port == 0) || (ui32Port == 1));

    if(onoff == 0 || onoff == 1) {
        SetSHDN(ui32Port, onoff);
    } else {
        return false;
    }

    return true;
}


//*****************************************************************************
//
//! Function to set max volt
//!
//! \param ui32Port holds port number
//!
//! \param mode identifies parameter to set
//!
//! \param pid is wanted value
//!
//!
//! \return true in case of success and false otherwise
//
//*****************************************************************************
bool Set_MaxVolt(uint32_t ui32Port, int32_t voltage)
{

    if(voltage < 0 || voltage > 5000) {
        return false;
    }

    if(ui32Port == 0) {
        g_sParameters.ui16MaxVolt0 = voltage;
        setMaxVolt0((uint16_t) voltage);
    } else {
        g_sParameters.ui16MaxVolt1 = voltage;
        setMaxVolt1((uint16_t) voltage);
    }

    return true;
}
