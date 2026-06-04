/*
 * main.c
 *
 * Architecture:
 *   procvue is a fixed-panel ambient system monitor built on EFL without
 *   Elementary, using ecore, ecore-evas, and edje only.  It sits on the
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
 *   poll.c/h       — poll_cb: hostname clock, loginctl, disk, uptime
 *   disk.c/h       — enigmatic fs->usage.read / fs->usage.write reader
 *   uptime.c/h     — enigmatic_system_uptime_get() wrapper
 */

#include "config.h"

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Edje.h>
#include <Eio.h>
#include <Enigmatic_Client.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <getopt.h>

#include "main.h"
#include "enigmatic.h"
#include "poll.h"
#include "disk.h"

/* ------------------------------------------------------------------ */
/* Shared state definitions  (declared extern in procvue.h)           */
/* ------------------------------------------------------------------ */

Evas_Object      *edje         = NULL;
const char       *g_iface      = NULL;
Enigmatic_Client *enigmatic    = NULL;

Ecore_Timer      *rx_off_timer = NULL;
Ecore_Timer      *tx_off_timer = NULL;

uint64_t          g_disk_rd    = 0;
uint64_t          g_disk_wr    = 0;

Eina_Bool    g_enigmatic_ready = EINA_FALSE;
Eina_Bool       g_iface_locked = EINA_FALSE;

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

   /* Connect to enigmatic daemon (CPU, RAM, NET, PROCS, DISK)        */
   enigmatic_start();

   /* Start 1-second poll for clock, loginctl, disk display, uptime   */
   poll_start();

   return ECORE_CALLBACK_CANCEL;
}

/* ------------------------------------------------------------------ */
/* win_delete_cb                                                      */
/* ------------------------------------------------------------------ */

static void
win_delete_cb(Ecore_Evas *window)
{
   Procvue_Config cfg;
   ecore_evas_geometry_get(window, &cfg.x, &cfg.y, &cfg.w, &cfg.h);
   cfg.scale = edje_scale_get();

   if (!procvue_config_save(&cfg))
      fprintf(stderr, "procvue: failed to save geometry\n");

   ecore_main_loop_quit();
}

static void
usage(void)
{
   fprintf(stdout, "procvue [OPTIONS]\n");
   fprintf(stdout, "Where OPTIONS can be one of:\n");
   fprintf(stdout, "   -s SCALE       Scale factor (e.g. -s 1.5 = 150%%)\n");
   fprintf(stdout, "   -h | --help    This menu\n");
}

/* ------------------------------------------------------------------ */
/* main                                                               */
/* ------------------------------------------------------------------ */

int
main(int argc, char *argv[])
{
   double      scale   = 1.0;
   int         opt;

   if (argc > 1 && strcmp(argv[1], "--help") == 0)
     {
        usage();
        return 0;
     }

   while ((opt = getopt(argc, argv, "s:h")) != -1)
     {
        switch (opt)
          {
           case 's': scale = atof(optarg); break;
           case 'h': usage(); return 0;
           default:
             fprintf(stderr, "usage: procvue [-s scale]\n");
             return 1;
          }
     }

   ecore_evas_init();
   edje_init();
   eio_init();

   /* Load saved geometry — applies before CLI/env scale so those     */
   /* still take priority.                                            */
   Procvue_Config *cfg = procvue_config_load();

   /* Scale resolution: CLI -s > E_SCALE > saved > default           */
   if (scale == 1.0)
     {
        const char *e_scale = getenv("E_SCALE");
        if (e_scale)       scale = atof(e_scale);
        else if (cfg->scale > 0.0) scale = cfg->scale;
     }
   if (scale < 1.0 || scale > 2.0) scale = 1.0;

   edje_scale_set(scale);

   int win_w = (int)(WIN_W * scale);
   int win_h = (int)(WIN_H * scale);

   Ecore_Evas *window = ecore_evas_new(NULL, 0, 0, win_w, win_h, NULL);
   if (!window)
     {
        fprintf(stderr, "procvue: could not create window\n");
        free(cfg);
        return 1;
     }

   ecore_evas_title_set(window, "procvue");
   ecore_evas_borderless_set(window, EINA_FALSE);
   ecore_evas_callback_delete_request_set(window, win_delete_cb);

   /* Restore saved position before show()                           */
   ecore_evas_move(window, cfg->x, cfg->y);

   free(cfg);

   Evas *canvas = ecore_evas_get(window);
   edje = edje_object_add(canvas);

   char edj_path[PATH_MAX];
   snprintf(edj_path, sizeof(edj_path), PROCVUE_DATADIR "/procvue.edj");

   if (!edje_object_file_set(edje, edj_path, EDJ_GROUP))
     {
        int err = edje_object_load_error_get(edje);
        fprintf(stderr, "procvue: could not load '%s' from %s: %s\n",
                EDJ_GROUP, edj_path, edje_load_error_str(err));
        evas_object_del(edje);
        ecore_evas_free(window);
        eio_shutdown();
        edje_shutdown();
        ecore_evas_shutdown();
        return 1;
     }

   evas_object_move(edje, 0, 0);
   evas_object_resize(edje, win_w, win_h);
   evas_object_show(edje);
   ecore_evas_show(window);

   ecore_timer_add(0.1, procvue_init, NULL);

   ecore_main_loop_begin();

   /* Shutdown ---------------------------------------------------- */
   if (g_iface) eina_stringshare_del(g_iface);
   if (enigmatic)    enigmatic_client_del(enigmatic);
   if (rx_off_timer) { ecore_timer_del(rx_off_timer); rx_off_timer = NULL; }
   if (tx_off_timer) { ecore_timer_del(tx_off_timer); tx_off_timer = NULL; }
   procvue_config_shutdown(); /* free eet */
   evas_object_del(edje);
   ecore_evas_free(window);
   eio_shutdown();
   edje_shutdown();
   ecore_evas_shutdown();
   return 0;
}
