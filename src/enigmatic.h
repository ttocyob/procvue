/*
 * enigmatic.h
 * Enigmatic client callbacks and NET LED decay logic.
 *
 * enigmatic_start() opens the client connection and registers both
 * callbacks.  Call it once from procvue_init(), after the edje object
 * is live.
 */

#ifndef ENIGMATIC_H
#define ENIGMATIC_H

#include <Enigmatic_Client.h>

/*
 * Open the enigmatic client, register enigmatic_init_cb and
 * enigmatic_update_cb, and store the handle in the shared `enigmatic`
 * pointer.  Returns EINA_TRUE on success.
 */
Eina_Bool enigmatic_start(void);

/*
 * The two callbacks are also exposed so procvue.c can reference them
 * in the architecture comment; they are not normally called directly.
 */
void enigmatic_init_cb(Enigmatic_Client *client, Snapshot *s, void *data);
void enigmatic_update_cb(Enigmatic_Client *client, Snapshot *s, void *data);

#endif /* ENIGMATIC_H */
