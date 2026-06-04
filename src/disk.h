#ifndef DISK_H
#define DISK_H

/*
 * disk.h — Disk I/O rate reader
 * Reads /proc/diskstats for whole-disk devices (SCSI/SATA major 8, NVMe major 259).
 * Reports byte rates since the previous call.
 * Call disk_init() once before the poll loop.
 */

#include <stddef.h>          /* size_t */

void disk_init(void);
void disk_read(unsigned long long *read_rate, unsigned long long *write_rate);

/* Format a byte/s rate as "N KB/s" or "N.N MB/s" into buf[len]. */
void disk_fmt_rate(unsigned long long bytes_per_sec, char *buf, size_t len);

#endif /* DISK_H */
