#include "config.h"

#include <Eet.h>
#include <Eina.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>   /* mkdir */
#include <errno.h>

/* Eet descriptor */

static Eet_Data_Descriptor *_edd = NULL;

static void
_edd_init(void)
{
    if (_edd) return;

    Eet_Data_Descriptor_Class eddc;
    EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Procvue_Config);
    _edd = eet_data_descriptor_stream_new(&eddc);

    EET_DATA_DESCRIPTOR_ADD_BASIC(_edd, Procvue_Config, "x",     x,     EET_T_INT);
    EET_DATA_DESCRIPTOR_ADD_BASIC(_edd, Procvue_Config, "y",     y,     EET_T_INT);
}

/* Path helpers */

/* Returns a malloc'd path to ~/.config/procvue/procvue.eet */
static char *
_config_path(void)
{
    const char *home = getenv("HOME");
    if (!home || !home[0]) return NULL;

    /* $XDG_CONFIG_HOME overrides ~/.config if set */
    const char *xdg = getenv("XDG_CONFIG_HOME");
    char *path = NULL;

    if (xdg && xdg[0])
        asprintf(&path, "%s/procvue/procvue.eet", xdg);
    else
        asprintf(&path, "%s/.config/procvue/procvue.eet", home);

    return path;   /* caller free()s */
}

/* mkdir -p for a single intermediate directory level (dir of path) */
static void
_ensure_config_dir(const char *filepath)
{
    char *tmp = strdup(filepath);
    if (!tmp) return;

    /* Truncate at last '/' to get the directory */
    char *slash = strrchr(tmp, '/');
    if (slash) {
        *slash = '\0';
        if (mkdir(tmp, 0700) != 0 && errno != EEXIST)
            fprintf(stderr, "procvue: could not create config dir %s: %s\n",
                    tmp, strerror(errno));
    }
    free(tmp);
}

/* ── Public API ────────────────────────────────────────────────────────── */

Procvue_Config *
procvue_config_load(void)
{
    Procvue_Config *cfg = calloc(1, sizeof(Procvue_Config));
    if (!cfg) return NULL;

    /* Populate defaults unconditionally; overwrite below if file exists */
    cfg->x     = PROCVUE_DEFAULT_X;
    cfg->y     = PROCVUE_DEFAULT_Y;

    char *path = _config_path();
    if (!path) return cfg;

    eet_init();
    _edd_init();

    Eet_File *ef = eet_open(path, EET_FILE_MODE_READ);
    if (!ef) {
        /* No config yet — first run, defaults are fine */
        free(path);
        eet_shutdown();
        return cfg;
    }

    Procvue_Config *loaded = eet_data_read(ef, _edd, "geometry");
    eet_close(ef);
    free(path);
    eet_shutdown();

    if (loaded) {
        *cfg = *loaded;
        free(loaded);
    }

    return cfg;
}

Eina_Bool
procvue_config_save(const Procvue_Config *cfg)
{
    if (!cfg) return EINA_FALSE;

    char *path = _config_path();
    if (!path) return EINA_FALSE;

    _ensure_config_dir(path);

    eet_init();
    _edd_init();

    Eet_File *ef = eet_open(path, EET_FILE_MODE_WRITE);
    free(path);

    if (!ef) {
        eet_shutdown();
        return EINA_FALSE;
    }

    int ok = eet_data_write(ef, _edd, "geometry", cfg, EINA_TRUE);
    eet_close(ef);
    eet_shutdown();

    return (ok > 0) ? EINA_TRUE : EINA_FALSE;
}

/* eet_data_descriptor_free() missing from _edd_init teardown */
void
procvue_config_shutdown(void)
{
   if (!_edd) return;
   eet_data_descriptor_free(_edd);
   _edd = NULL;
}
