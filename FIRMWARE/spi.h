/*
 * spi.h
 *
 *
 */

#ifndef SPI_H_
#define SPI_H_

#include <stdint.h>
#include <stdbool.h>

void spi_config(uint32_t ui32SysClock);
int32_t spi_getvalue(uint16_t *channel);
int32_t getTemp(int32_t RawTemp, int32_t T0, int32_t Beta, int16_t Ratio, int16_t tempmode, int32_t ptA, int32_t ptB);

#endif /* SPI_H_ */
