/*
 * uptime.c
 * uptime_read() — format system uptime as "Nd HH:MM".
 *
 * Delegates to enigmatic_system_uptime_get() (libenigmatic_client,
 * installed at /usr/local/include/enigmatic/system/uptime.h) which
 * reads /proc/uptime internally and returns elapsed seconds as time_t.
 * On error it returns (time_t)-1; we emit "--" in that case.
 */

#include <enigmatic/system/uptime.h>

#include <stdio.h>
#include <time.h>

#include "uptime.h"

void
uptime_read(char *buf, size_t len)
{
   time_t secs = enigmatic_system_uptime_get();
   if (secs == (time_t)-1)
     {
        snprintf(buf, len, "--");
        return;
     }

   int days  =  secs / 86400;
   int hours = (secs % 86400) / 3600;
   int mins  = (secs % 3600)  / 60;

   if (days > 0)
     snprintf(buf, len, "%dd %02d:%02d", days, hours, mins);
   else
     snprintf(buf, len, "%02d:%02d", hours, mins);
}
