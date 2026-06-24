/*
 * poll.c
 * 1-second poll callback: hostname clock, loginctl user count,
 * DISK I/O rates, and UPTIME.
 *
 * These panels are driven from /proc directly (or loginctl for session
 * count) because libenigmatic_client has no equivalent API for them.
 *
 * Uptime is updated on tick 1 and then once per minute (tick % 60 == 0)
 * — it does not need per-second resolution.
 */

#include <Ecore.h>
#include <Edje.h>

#include <stdio.h>
#include <time.h>
#include <math.h>
#include <utmp.h>

#include "main.h"
#include "enigmatic.h"
#include "poll.h"
#include "disk.h"
#include "uptime.h"

/* ------------------------------------------------------------------ */
/* poll_cb                                                            */
/* ------------------------------------------------------------------ */

Eina_Bool
poll_cb(void *data EINA_UNUSED)
{
   char buf[128];
   static int tick = 0;
   tick++;

   /* --- HOSTNAME clock ------------------------------------------- */
   {
      time_t     now = time(NULL);
      struct tm *tm  = localtime(&now);
      strftime(buf, sizeof(buf), "%a %e %b", tm);
      edje_object_part_text_set(edje, "hostname:hostname_label_date", buf);
      strftime(buf, sizeof(buf), "%H:%M", tm);
      edje_object_part_text_set(edje, "hostname:hostname_label_time", buf);
   }

   /* --- PROCS user count (loginctl — no enigmatic equivalent) ---- */
   {
   int nusers = 0;
   FILE *f = popen(
      "loginctl list-sessions --no-legend 2>/dev/null"
      " | awk '$6==\"user\" {print $3}' | wc -l", "r");
   if (f) { fscanf(f, "%d", &nusers); pclose(f); }
   if (nusers == 0)
     {
        struct utmp *ut;
        setutent();
        while ((ut = getutent()) != NULL)
           if (ut->ut_type == USER_PROCESS) nusers++;
        endutent();
     }
   snprintf(buf, sizeof(buf), "%d", nusers);
   edje_object_part_text_set(edje, "procs:procs_users_value", buf);
   }

   /* --- DISK ----------------------------------------------------- */
   {
      unsigned long long disk_rd, disk_wr;
      disk_read(&disk_rd, &disk_wr);
      double dr_ratio = fmin((double)disk_rd / DISK_BAR_MAX, 1.0);
      double dw_ratio = fmin((double)disk_wr / DISK_BAR_MAX, 1.0);
      disk_fmt_rate(disk_rd, buf, sizeof(buf));
      edje_object_part_text_set(edje,       "disk:rd_value_text", buf);
      edje_object_part_drag_value_set(edje, "disk:rd_bar_drag",   dr_ratio, 0.0);
      disk_fmt_rate(disk_wr, buf, sizeof(buf));
      edje_object_part_text_set(edje,       "disk:wr_value_text", buf);
      edje_object_part_drag_value_set(edje, "disk:wr_bar_drag",   dw_ratio, 0.0);
   }

   /* --- UPTIME (once per minute) --------------------------------- */
   if (tick == 1 || tick % 60 == 0)
     {
        uptime_read(buf, sizeof(buf));
        edje_object_part_text_set(edje, "uptime:uptime_value", buf);
     }

   return ECORE_CALLBACK_RENEW;
}

/* ------------------------------------------------------------------ */
/* poll_start                                                         */
/* ------------------------------------------------------------------ */

void
poll_start(void)
{
   poll_cb(NULL);
   ecore_timer_add(POLL_INTERVAL, poll_cb, NULL);
}
