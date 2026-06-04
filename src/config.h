/* src/config.h */
#ifndef PROCVUE_CONFIG_H
#define PROCVUE_CONFIG_H

#include <Eina.h>

/* Window geometry + scale stored to ~/.config/procvue/procvue.eet */

typedef struct _Procvue_Config {
    int   x, y;
    int   w, h;
    double scale;
} Procvue_Config;

/* Defaults applied when no config file exists */
#define PROCVUE_DEFAULT_X      100
#define PROCVUE_DEFAULT_Y      100
#define PROCVUE_DEFAULT_W       72
#define PROCVUE_DEFAULT_H      418
#define PROCVUE_DEFAULT_SCALE  1.0

/**
 * Load geometry from ~/.config/procvue/procvue.eet.
 * Returns a heap-allocated Procvue_Config; caller must free().
 * Falls back to defaults silently on any error.
 */
Procvue_Config *procvue_config_load(void);

/**
 * Save geometry to ~/.config/procvue/procvue.eet.
 * Creates ~/.config/procvue/ if it does not exist.
 * Returns EINA_TRUE on success.
 */
Eina_Bool procvue_config_save(const Procvue_Config *cfg);

/* eet_data_descriptor_free() was never called */
void procvue_config_shutdown(void);

#endif /* PROCVUE_CONFIG_H */