/*
 * enigmatic.c
 * Enigmatic client callbacks — CPU, RAM, NET, DISK, PROCS count, THERMAL.
 *
 * Data flow:
 *   enigmatic daemon -> libenigmatic_client -> enigmatic_update_cb()
 *                                           -> edje colon-path API
 *
 * NET LED decay:
 *   Since libenigmatic_client 2.0.9 the daemon pre-calculates per-interval
 *   byte rates and delivers them as iface->in / iface->out.  The manual
 *   delta arithmetic (prev_in, prev_out, first) has been removed.
 *
 *   A 1.5 s decay timer keeps each LED lit across snapshots during
 *   sustained transfers, preventing flicker on zero-rate boundaries.
 *   The timers are shared state so that procvue.c can cancel them
 *   cleanly on shutdown.
 *
 * DISK:
 *   fs->usage.read and fs->usage.write are pre-calculated per-interval
 *   byte rates (libenigmatic_client >= 2.0.9).  All file_system entries
 *   are summed into g_disk_rd / g_disk_wr each snapshot; poll_cb reads
 *   those globals via disk_read() to update the display.
 */

#include <Ecore.h>
#include <Edje.h>
#include <Enigmatic_Client.h>
#include <enigmatic/system/file_systems.h>
#include <enigmatic/enigmatic_util.h>

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "main.h"
#include "enigmatic.h"

/* ------------------------------------------------------------------ */
/* NET LED decay timers                                               */
/* ------------------------------------------------------------------ */

static Eina_Bool
rx_led_off_cb(void *data EINA_UNUSED)
{
   edje_object_signal_emit(edje, "led,rx,off", "procvue");
   rx_off_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
tx_led_off_cb(void *data EINA_UNUSED)
{
   edje_object_signal_emit(edje, "led,tx,off", "procvue");
   tx_off_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

/* ------------------------------------------------------------------ */
/* enigmatic_update_cb                                                */
/* ------------------------------------------------------------------ */

void
enigmatic_update_cb(Enigmatic_Client *client EINA_UNUSED,
                    Snapshot *s, void *data EINA_UNUSED)
{
   char buf[128];

   /* --- CPU ------------------------------------------------------ */
   if (s->cores)
     {
        Eina_List *l;
        Cpu_Core  *core;
        int        total = 0, count = 0;

        EINA_LIST_FOREACH(s->cores, l, core)
          { total += core->percent; count++; }

        static double smooth = 0.0;
        double avg = count ? (double)total / count / 100.0 : 0.0;
        smooth = (smooth * 0.7) + (avg * 0.3);

        snprintf(buf, sizeof(buf), "%d%%", (int)round(smooth * 100.0));
        edje_object_part_text_set(edje,       "cpu:cpu_value",    buf);
        edje_object_part_drag_value_set(edje, "cpu:cpu_bar_drag", smooth, 0.0);
     }

   /* --- RAM ------------------------------------------------------ */
   if (s->meminfo.total > 0)
     {
        double ram_ratio = (double)s->meminfo.used / (double)s->meminfo.total;
        snprintf(buf, sizeof(buf), "%d%%", (int)round(ram_ratio * 100.0));
        edje_object_part_text_set(edje,       "ram:ram_value",    buf);
        edje_object_part_drag_value_set(edje, "ram:ram_bar_drag", ram_ratio, 0.0);
     }

   /* --- NETWORK -------------------------------------------------- */
   /*
    * iface->in and iface->out are pre-calculated per-interval byte
    * rates supplied by the daemon (libenigmatic_client >= 2.0.9).
    * No client-side delta arithmetic is needed.
    */
   if (s->network_interfaces)
     {
        Eina_List         *l;
        Network_Interface *iface;

        EINA_LIST_FOREACH(s->network_interfaces, l, iface)
          {
             if (strcmp(iface->name, "lo") == 0) continue;
             /* Auto-detect: first interface with cumulative traffic */
             if (!g_iface_locked)
               if (iface->total_in > 0 || iface->total_out > 0)
                 eina_stringshare_replace(&g_iface, iface->name);
             /* Drive LEDs for selected interface only */
             if (!g_iface || strcmp(iface->name, g_iface) != 0) continue;
             uint64_t rx = iface->in;
             uint64_t tx = iface->out;
             /* RX LED */
             if (rx > 50000)
               {
                  edje_object_signal_emit(edje, "led,rx,hi", "procvue");
                  if (rx_off_timer) ecore_timer_reset(rx_off_timer);
                  else rx_off_timer = ecore_timer_add(1.5, rx_led_off_cb, NULL);
               }
             else if (rx > 100)
               {
                  edje_object_signal_emit(edje, "led,rx,lo", "procvue");
                  if (rx_off_timer) ecore_timer_reset(rx_off_timer);
                  else rx_off_timer = ecore_timer_add(1.5, rx_led_off_cb, NULL);
               }
             /* TX LED */
             if (tx > 50000)
               {
                  edje_object_signal_emit(edje, "led,tx,hi", "procvue");
                  if (tx_off_timer) ecore_timer_reset(tx_off_timer);
                  else tx_off_timer = ecore_timer_add(1.5, tx_led_off_cb, NULL);
               }
             else if (tx > 100)
               {
                  edje_object_signal_emit(edje, "led,tx,lo", "procvue");
                  if (tx_off_timer) ecore_timer_reset(tx_off_timer);
                  else tx_off_timer = ecore_timer_add(1.5, tx_led_off_cb, NULL);
               }
             break;
          }
     }

   /* --- DISK ----------------------------------------------------- */
   /*
    * Sum fs->usage.read / fs->usage.write across all file_system
    * entries into g_disk_rd / g_disk_wr.  poll_cb reads these globals
    * via disk_read() once per second to update the display.
    */
   if (s->file_systems)
     {
        Eina_List   *l;
        File_System *fs;
        uint64_t     rd = 0, wr = 0;

        EINA_LIST_FOREACH(s->file_systems, l, fs)
          {
             rd += fs->usage.read;
             wr += fs->usage.write;
          }
        g_disk_rd = rd;
        g_disk_wr = wr;
     }

   /* --- PROCS count ---------------------------------------------- */
   if (s->processes)
     {
        int nprocs = eina_list_count(s->processes);
        snprintf(buf, sizeof(buf), "%d", nprocs);
        edje_object_part_text_set(edje, "procs:procs_value", buf);
     }

   /* --- THERMAL (freq + temp, mean across all cores) ------------- */
   if (s->cores)
     {
        Eina_List *l;
        Cpu_Core  *core;
        long       total_freq = 0;
        int        total_temp = 0, count = 0;

        EINA_LIST_FOREACH(s->cores, l, core)
          {
             total_freq += core->freq;
             total_temp += core->temp;
             count++;
          }
        if (count)
          {
             long avg_freq = total_freq / count / 1000;
             int  avg_temp = total_temp / count;
             if (avg_freq >= 1000)
               snprintf(buf, sizeof(buf), "%.1f GHz", avg_freq / 1000.0);
             else
               snprintf(buf, sizeof(buf), "%ld MHz", avg_freq);
             edje_object_part_text_set(edje, "thermal:thermal_freq_value", buf);
             snprintf(buf, sizeof(buf), "%d°C", avg_temp);
             edje_object_part_text_set(edje, "thermal:thermal_temp_value", buf);
             if (avg_temp >= THERMAL_HOT)
               edje_object_signal_emit(edje, "temp,hot",     "procvue");
             else if (avg_temp >= THERMAL_WARM)
               edje_object_signal_emit(edje, "temp,warm",    "procvue");
             else
               edje_object_signal_emit(edje, "temp,default", "procvue");
          }
     }
}

/* ------------------------------------------------------------------ */
/* enigmatic_init_cb — first snapshot; delegates to update            */
/* ------------------------------------------------------------------ */

void
enigmatic_init_cb(Enigmatic_Client *client, Snapshot *s, void *data)
{
   enigmatic_update_cb(client, s, data);
}

/* ------------------------------------------------------------------ */
/* enigmatic_start — open client, register callbacks                  */
/* ------------------------------------------------------------------ */

Eina_Bool
enigmatic_start(void)
{
   if (!enigmatic_running())
     {
        fprintf(stderr, "procvue: enigmatic daemon is not running - start it with: enigmatic\n");
        return EINA_FALSE;
     }

   enigmatic = enigmatic_client_open();
   if (!enigmatic)
     {
        fprintf(stderr, "procvue: enigmatic client open failed\n");
        return EINA_FALSE;
     }

   enigmatic_client_monitor_add(enigmatic,
                                enigmatic_init_cb,
                                enigmatic_update_cb,
                                NULL);
   g_enigmatic_ready = EINA_TRUE;
   return EINA_TRUE;
}
