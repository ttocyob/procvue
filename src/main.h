/*
 * procvue.h
 * Shared state and constants for procvue.
 *
 * All translation units that need the root edje object, the network
 * interface name, the enigmatic client handle, or the LED decay timers
 * include this header.  The definitions live in procvue.c; every other
 * file declares them extern.
 */

#ifndef PROCVUE_H
#define PROCVUE_H

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Edje.h>
#include <Elementary.h>
#include <Enigmatic_Client.h>

/* ------------------------------------------------------------------ */
/* Window / EDJ constants                                             */
/* ------------------------------------------------------------------ */

#define WIN_W          72
#define WIN_H          418
#define EDJ_GROUP      "procvue/main"
#define POLL_INTERVAL  1.0

#define DISK_BAR_MAX   (1.0 * 1024.0 * 1024.0)   /* 1 MB/s = full bar */
#define THERMAL_WARM   70   /* °C */
#define THERMAL_HOT    85   /* °C */

/* ------------------------------------------------------------------ */
/* Shared state  (defined in procvue.c)                               */
/* ------------------------------------------------------------------ */

extern Evas_Object      *edje;
extern Evas_Object      *layout;

extern const char       *g_iface;
extern Enigmatic_Client *enigmatic;

extern Ecore_Timer      *rx_off_timer;
extern Ecore_Timer      *tx_off_timer;

/* Disk I/O rates — populated each snapshot by enigmatic_update_cb(), */
/* consumed each poll tick by disk_read() in disk.c.                  */
extern uint64_t          g_disk_rd;
extern uint64_t          g_disk_wr;

extern Eina_Bool         g_enigmatic_ready;
extern Eina_Bool         g_iface_locked;

#endif /* PROCVUE_H */
