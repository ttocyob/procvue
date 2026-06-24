/*
 * main.c
 *
 * Architecture:
 *   procvue is a fixed-panel ambient system monitor built on EFL using
 *   Elementary, ecore, ecore-evas, and edje only.  It sits on the
 *   desktop and breathes with the machine: glanceable, not interactive.
 *
 *   System data comes entirely from libenigmatic_client (CPU, RAM, NET,
 *   PROCS count, DISK I/O rates, UPTIME).  The enigmatic daemon runs
 *   continuously in the background, logging system state.  procvue
 *   connects as a client and receives pushed snapshots.  The daemon must
 *   be running before procvue is launched — started automatically by
 *   Evisum, or manually via `enigmatic`.
 *   PROCS user count uses loginctl via popen as enigmatic does not expose
 *   session data.
 *
 * EDC and the colon-path API:
 *   type: GROUP parts in the root edje are composited directly into it —
 *   there are no separate child Edje objects.  All parts are driven through
 *   the root edje object using colon-separated paths:
 *     "panel_name:part_name"
 *   e.g. edje_object_part_text_set(edje, "cpu:cpu_value", buf)
 *
 * Source layout:
 *   main.c         — main, win_delete_cb, procvue_init (this file)
 *   main.h         — shared state: edje, g_iface, enigmatic, timers,
 *                    g_disk_rd, g_disk_wr
 *   enigmatic.c/h  — enigmatic_init_cb, enigmatic_update_cb, LED decay;
 *                    populates g_disk_rd / g_disk_wr each snapshot
 *   poll.c/h       — poll_cb: hostname clock, loginctl
 *   disk.c/h       — enigmatic fs->usage.read / fs->usage.write reader
 *   uptime.c/h     — enigmatic_system_uptime_get() wrapper
 */

#include "config.h"

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Edje.h>
#include <Elementary.h>
#include <Eio.h>
#include <Enigmatic_Client.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#include "main.h"
#include "enigmatic.h"
#include "poll.h"
#include "disk.h"

/* ------------------------------------------------------------------ */
/* Shared state definitions  (declared extern in main.h)           */
/* ------------------------------------------------------------------ */

Evas_Object      *edje                  = NULL;
Evas_Object      *layout                = NULL;

const char       *g_iface               = NULL;
Enigmatic_Client *enigmatic             = NULL;

Ecore_Timer      *rx_off_timer          = NULL;
Ecore_Timer      *tx_off_timer          = NULL;
Ecore_Timer      *enigmatic_retry_timer = NULL;

uint64_t          g_disk_rd             = 0;
uint64_t          g_disk_wr             = 0;

Eina_Bool         g_enigmatic_ready     = EINA_FALSE;
Eina_Bool         g_iface_locked        = EINA_FALSE;

/* ------------------------------------------------------------------ */
/* procvue_init — zeros bars, sets hostname, starts subsystems        */
/* ------------------------------------------------------------------ */

static Eina_Bool
procvue_init(void *data EINA_UNUSED)
{
   /* Zero all drag bars before first update                          */
   edje_object_part_drag_value_set(edje, "cpu:cpu_bar_drag",  0.0, 0.0);
   edje_object_part_drag_value_set(edje, "disk:rd_bar_drag",  0.0, 0.0);
   edje_object_part_drag_value_set(edje, "disk:wr_bar_drag",  0.0, 0.0);
   edje_object_part_drag_value_set(edje, "ram:ram_bar_drag",  0.0, 0.0);

   /* Initialise all enigmatic-dependent panels to "--".             */
   /* If the daemon is down these stay permanently; if it's up the   */
   /* first snapshot overwrites them naturally.                      */
   edje_object_part_text_set(edje, "cpu:cpu_value",              "--");
   edje_object_part_text_set(edje, "ram:ram_value",              "--");
   edje_object_part_text_set(edje, "disk:rd_value_text",         "--");
   edje_object_part_text_set(edje, "disk:wr_value_text",         "--");
   edje_object_part_text_set(edje, "procs:procs_value",          "--");
   edje_object_part_text_set(edje, "thermal:thermal_freq_value", "--");
   edje_object_part_text_set(edje, "thermal:thermal_temp_value", "--");

   /* disk_init() is now a no-op — enigmatic handles initialisation   */
   disk_init();

   /* Hostname is static — set once                                   */
   {
      char hostname[256] = {0};
      gethostname(hostname, sizeof(hostname));
      edje_object_part_text_set(edje, "hostname:hostname_label", hostname);
   }

   /* Connect to enigmatic daemon                                     */
   if (!enigmatic_start())
   fprintf(stderr, "procvue: retrying every 2s until enigmatic starts\n");

   /* Start 1-second poll for clock, loginctl, disk display, uptime   */
   poll_start();

   return ECORE_CALLBACK_CANCEL;
}

/* ------------------------------------------------------------------ */
/* win_delete_cb                                                      */
/* ------------------------------------------------------------------ */

static void
win_delete_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Ecore_Evas *ee = data;
   Procvue_Config cfg = {0};
   ecore_evas_geometry_get(ee, &cfg.x, &cfg.y, NULL, NULL);

   if (!procvue_config_save(&cfg))
      fprintf(stderr, "procvue: failed to save geometry\n");

   ecore_main_loop_quit();
}

/* ------------------------------------------------------------------ */
/* _config_changed_cb — live rescale when E changes scale at runtime  */
/* ------------------------------------------------------------------ */

static Eina_Bool
_config_changed_cb(void *data, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   static double last_scale = 0.0;

   Evas_Object *window = data;
   double        scale  = elm_config_scale_get();
   if (scale < 1.0 || scale > 2.0) scale = 1.0;

   if (scale == last_scale) return ECORE_CALLBACK_PASS_ON;
   last_scale = scale;

   int win_w = (int)(WIN_W * scale);
   int win_h = (int)(WIN_H * scale);

   evas_object_resize(window, win_w, win_h);
   evas_object_resize(layout, win_w, win_h);

   return ECORE_CALLBACK_PASS_ON;
}

/* ------------------------------------------------------------------ */
/* main                                                               */
/* ------------------------------------------------------------------ */

int
main(int argc, char *argv[])
{

   elm_init(argc, argv);
   edje_init();
   eio_init();

   /* Load saved positioning from .eet */
   Procvue_Config *cfg = procvue_config_load();

   double scale = elm_config_scale_get();
   if (scale < 1.0 || scale > 2.0) scale = 1.0;

   edje_scale_set(scale);

   int win_w = (int)(WIN_W * scale);
   int win_h = (int)(WIN_H * scale);

   Evas_Object *window = elm_win_add(NULL, "procvue", ELM_WIN_BASIC);
   if (!window)
     {
        fprintf(stderr, "procvue: could not create window\n");
        free(cfg);
        return 1;
     }

   elm_win_borderless_set(window, EINA_FALSE);
   
   /* Extract underlying Ecore_Evas handle for geometry and placement management */
   Ecore_Evas *ee = ecore_evas_ecore_evas_get(evas_object_evas_get(window));
   evas_object_smart_callback_add(window, "delete,request", win_delete_cb, ee);
   ecore_event_handler_add(ELM_EVENT_CONFIG_ALL_CHANGED, _config_changed_cb, window);

   /* Restore saved position and enforce starting bounds */
   evas_object_resize(window, win_w, win_h);
   ecore_evas_move(ee, cfg->x, cfg->y);

   free(cfg);

   char edj_path[PATH_MAX];
   snprintf(edj_path, sizeof(edj_path), PROCVUE_DATADIR "/procvue.edj");

/* error block */
   layout = elm_layout_add(window);
   if (!elm_layout_file_set(layout, edj_path, EDJ_GROUP))
     {
        Evas_Object *fail_edje = elm_layout_edje_get(layout);
        int err = edje_object_load_error_get(fail_edje);
        fprintf(stderr, "procvue: could not load '%s' from %s: %s\n", 
                EDJ_GROUP, edj_path, edje_load_error_str(err));
        evas_object_del(layout);
        evas_object_del(window);
        eio_shutdown();
        edje_shutdown();
        elm_shutdown();
        return 1;
     }

   edje = elm_layout_edje_get(layout);

   evas_object_move(layout, 0, 0);
   evas_object_resize(layout, win_w, win_h);

   evas_object_show(layout);
   evas_object_show(window);

   ecore_timer_add(0.1, procvue_init, NULL);

   ecore_main_loop_begin();

   /* Shutdown ---------------------------------------------------- */
   if (g_iface) eina_stringshare_del(g_iface);
   if (enigmatic)    enigmatic_client_del(enigmatic);
   if (rx_off_timer) { ecore_timer_del(rx_off_timer); rx_off_timer = NULL; }
   if (tx_off_timer) { ecore_timer_del(tx_off_timer); tx_off_timer = NULL; }
   if (enigmatic_retry_timer) { ecore_timer_del(enigmatic_retry_timer); enigmatic_retry_timer = NULL; }
   procvue_config_shutdown(); /* free eet */
   evas_object_del(layout);
   evas_object_del(window);
   eio_shutdown();
   edje_shutdown();
   elm_shutdown();
   return 0;
}
