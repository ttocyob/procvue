#ifndef UPTIME_H
#define UPTIME_H

/*
 * uptime.h — Uptime formatter
 *
 * uptime_read() delegates to enigmatic_system_uptime_get() and
 * formats the result as "Nd HH:MM".
 */

#include <stddef.h>   /* size_t */

void uptime_read(char *buf, size_t len);

#endif /* UPTIME_H */
