/**
 * @file wdLib.h
 * @brief Watchdog timer API using tick-based delays.
 */
#ifndef __INCwdLibh
#define __INCwdLibh

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* WDOG_ID;
typedef void (*WDOG_HANDLER)(uintptr_t arg);

/* Create / delete */
WDOG_ID wdCreate(void);
int     wdDelete(WDOG_ID);
/* Start the watchdog: after delay ticks, call func(arg) on internal thread. */
int     wdStart(WDOG_ID, int delayTicks, WDOG_HANDLER func, uintptr_t arg);
/* Cancel a running watchdog */
int     wdCancel(WDOG_ID);

#ifdef __cplusplus
}
#endif
#endif /* __INCwdLibh */
