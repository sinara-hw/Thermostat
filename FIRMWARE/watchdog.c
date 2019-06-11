/*
 * watchdog.c
 *
 *
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_gpio.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "inc/hw_types.h"
#include "driverlib/rom.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/watchdog.h"
#include "config.h"
#include "priorities.h"

void WatchdogInt(void)
{
    //
    // Do nothing.
    //

}

void WatchdogClear(void)
{
    //
    // Clear the watchdog interrupt.
    //
    MAP_WatchdogIntClear(WATCHDOG0_BASE);

}

void Watchdog_config(void) {


MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_WDOG0);
MAP_IntMasterEnable();

MAP_WatchdogIntEnable(WATCHDOG0_BASE);
MAP_IntPrioritySet(INT_WATCHDOG, WATCHDOG_INT_PRIORITY);

//
// Set the period of the watchdog timer to 3 seconds.
//
MAP_WatchdogReloadSet(WATCHDOG0_BASE, 3*g_ui32SysClock);

//
// Enable reset generation from the watchdog timer.
//
MAP_WatchdogResetEnable(WATCHDOG0_BASE);

//WatchdogIntRegister(WATCHDOG0_BASE, &WatchdogClear);

//
// Enable the watchdog timer.
//
MAP_WatchdogEnable(WATCHDOG0_BASE);


}
