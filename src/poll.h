/*
 * poll.h
 * 1-second ecore timer: hostname clock, loginctl user count,
 * disk I/O, and uptime.
 *
 * poll_start() fires poll_cb() once immediately (tick = 1) and then
 * arms the recurring POLL_INTERVAL timer.  Call it from procvue_init()
 * after disk_init() has been called.
 */

#ifndef POLL_H
#define POLL_H

#include <Eina.h>

/*
 * Fire the first poll tick and arm the recurring timer.
 * disk_init() must be called before poll_start().
 */
void poll_start(void);

/*
 * The timer callback; exposed so procvue_init() can call it once
 * directly before arming the timer if preferred — but poll_start()
 * handles that internally.
 */
Eina_Bool poll_cb(void *data);

#endif /* POLL_H */
