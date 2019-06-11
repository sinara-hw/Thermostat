/*
 * spi.c
 *
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"
#include "spi.h"


#define READ_DATA_REG 0x44
#define WRITE_CH0_REG 0x10
#define WRITE_CH1_REG 0x11
#define WRITE_SETUPCON0_REG 0x20
#define WRITE_ADCMODE_REG 0x1
#define WRITE_IFMODE_REG 0x2
#define WRITE_GPIOCON_REG 0x6


void spi_config(uint32_t ui32SysClock)
{

    uint32_t command[5];
    uint32_t data[5];
    uint32_t i;

    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI1);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    GPIOPinConfigure(GPIO_PB5_SSI1CLK);
    GPIOPinConfigure(GPIO_PB4_SSI1FSS);
    GPIOPinConfigure(GPIO_PE4_SSI1XDAT0);
    GPIOPinConfigure(GPIO_PE5_SSI1XDAT1);

    GPIOPinTypeSSI(GPIO_PORTB_BASE, GPIO_PIN_5 | GPIO_PIN_4);
    GPIOPinTypeSSI(GPIO_PORTE_BASE, GPIO_PIN_4 | GPIO_PIN_5);


    SSIConfigSetExpClk(SSI1_BASE, ui32SysClock, SSI_FRF_MOTO_MODE_3, SSI_MODE_MASTER, 1000000, 8);

    SSIEnable(SSI1_BASE);

    while(SSIDataGetNonBlocking(SSI1_BASE, data))
    {
    }

    //Write channel 1 register

    command[0] = WRITE_CH1_REG;
    command[1] = 0x80;
    command[2] = 0x43;

    for(i = 0; i < 3; i = i + 1) {
        SSIDataPut(SSI1_BASE, command[i]);
    }

    while(SSIBusy(SSI1_BASE))
    {
    }

    for(i = 0; i < 3; i = i + 1) {
        SSIDataGet(SSI1_BASE, &data[i]);
    }

    //Write setup configuration register

    command[0] = WRITE_SETUPCON0_REG;
    command[1] = 0x1F;
    command[2] = 0x0;

    for(i = 0; i < 3; i = i + 1) {
        SSIDataPut(SSI1_BASE, command[i]);
    }

    while(SSIBusy(SSI1_BASE))
    {
    }

    for(i = 0; i < 3; i = i + 1) {
        SSIDataGet(SSI1_BASE, &data[i]);
    }

    //Write ADC mode register

    command[0] = WRITE_ADCMODE_REG;
    command[1] = 0x0;
    command[2] = 0x0;

    for(i = 0; i < 3; i = i + 1) {
        SSIDataPut(SSI1_BASE, command[i]);
    }

    while(SSIBusy(SSI1_BASE))
    {
    }

    for(i = 0; i < 3; i = i + 1) {
        SSIDataGet(SSI1_BASE, &data[i]);
    }

    //Write IF mode register

    command[0] = WRITE_IFMODE_REG;
    command[1] = 0x0;
    command[2] = 0x40;

    for(i = 0; i < 3; i = i + 1) {
        SSIDataPut(SSI1_BASE, command[i]);
    }

    while(SSIBusy(SSI1_BASE))
    {
    }

    for(i = 0; i < 3; i = i + 1) {
        SSIDataGet(SSI1_BASE, &data[i]);
    }

    //Write GPIO config register

    command[0] = WRITE_GPIOCON_REG;
    command[1] = 0x0;
    command[2] = 0x0;

    for(i = 0; i < 3; i = i + 1) {
        SSIDataPut(SSI1_BASE, command[i]);
    }

    while(SSIBusy(SSI1_BASE))
    {
    }

    for(i = 0; i < 3; i = i + 1) {
        SSIDataGet(SSI1_BASE, &data[i]);
    }

}

int32_t spi_getvalue(uint16_t *channel) {

    uint32_t command[5];
    uint32_t data[5];
    uint32_t i;
    int32_t result = 0;

    command[0] = READ_DATA_REG;
    command[1] = 0x0;
    command[2] = 0x0;
    command[3] = 0x0;
    command[4] = 0x0;

    for(i = 0; i < 5; i = i + 1) {
        SSIDataPut(SSI1_BASE, command[i]);
    }

    while(SSIBusy(SSI1_BASE))
    {
    }

    for(i = 0; i < 5; i = i + 1) {
        SSIDataGet(SSI1_BASE, &data[i]);
        data[i] &= 0x00FF;
    }

    data[4] &= 0x000F;

    result = result + (data[1] << 16);
    result = result + (data[2] << 8);
    result = result + (data[3]);

    result = result - 0x800000;

    *channel = data[4];

    if(data[4] == 0x0 || data[4] == 0x1) {
        return result;
    } else {
        return 0xffffffff;
    }

}

int32_t getTemp(int32_t RawTemp, int32_t T0, int32_t Beta, int16_t Ratio, int16_t tempmode, int32_t ptA, int32_t ptB)
{
    float alpha = ((float) RawTemp)/8388608.0; // raw/2^23
    float recepT;
    float T;
    float temp_adiv2b;
    float temp_1divb;
    int32_t result;

    if(tempmode == 0) {
        // 1/T = 1/T0 + 1/B*ln( alpha/(1-alpha)*Rref/R0
        recepT = 1.0 / (273.15 + ((float) T0)/1000.0) + (1000.0 / ((float) Beta))*logf( alpha/(1.0-alpha)/(((float) Ratio)/1000.0) );
        T = 1.0/recepT - 273.15;

    } else if(tempmode == 1 && ptB != 0) {
        temp_adiv2b = 1000.0*(((float) ptA)/((float) ptB) ) / 2.0 ;
        temp_1divb = 1000000000000.0 / ((float) ptB);
        if(ptB > 0) {
            T = - temp_adiv2b + sqrt(temp_adiv2b*temp_adiv2b - temp_1divb + temp_1divb * alpha/(1.0-alpha)/(((float) Ratio)/1000.0));
        } else {
            T = - temp_adiv2b - sqrt(temp_adiv2b*temp_adiv2b - temp_1divb + temp_1divb * alpha/(1.0-alpha)/(((float) Ratio)/1000.0));
        }
        T = T + (float) T0 / 1000.0;

    } else if(tempmode == 1 && ptA != 0) {
        T = ( 1000000000.0 / ((float) ptA) ) * ( alpha/(1.0-alpha)/(((float) Ratio)/1000.0) - 1.0);
        T = T + (float) T0 / 1000.0;
    } else {
        T = 666.666;

    }

    result = (int32_t) (T*1000.0); //result in mC
    return result;
}
