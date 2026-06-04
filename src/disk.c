/*
 * disk.c — Disk I/O rate reader
 *
 * Reads g_disk_rd / g_disk_wr, which are populated each enigmatic
 * snapshot by enigmatic_update_cb() from fs->usage.read and
 * fs->usage.write (libenigmatic_client >= 2.0.9, pre-calculated
 * per-interval byte rates summed across all file_system entries).
 *
 * disk_init() is retained for API compatibility but is a no-op;
 * the enigmatic client handles its own initialisation.
 */

#include <stdio.h>
#include <stddef.h>

#include "main.h"
#include "disk.h"

void
disk_init(void)
{
   /* no-op: enigmatic client primes rates internally */
}

void
disk_read(unsigned long long *read_rate, unsigned long long *write_rate)
{
   *read_rate  = (unsigned long long)g_disk_rd;
   *write_rate = (unsigned long long)g_disk_wr;
}

void
disk_fmt_rate(unsigned long long bytes_per_sec, char *buf, size_t len)
{
   if (!g_enigmatic_ready)
     {
        snprintf(buf, len, "--");
        return;
     }
   if (bytes_per_sec >= 1024ULL * 1024ULL)
     snprintf(buf, len, "%.1f MB/s", (double)bytes_per_sec / (1024.0 * 1024.0));
   else
     snprintf(buf, len, "%llu KB/s", bytes_per_sec / 1024ULL);
}
