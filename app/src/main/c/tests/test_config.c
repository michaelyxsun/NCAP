#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "test.h"

#include "../config.h"

size_t passcnt = 0;
size_t failcnt = 0;

bool
cfg_eq (const struct config_t *c1, const struct config_t *c2)
{
    return memcmp (c1, c2, sizeof (struct config_t) - sizeof (char *)) == 0
           && memcmp (c1->track_path, c2->track_path, c1->track_path_len) == 0;
}

int
main (void)
{
    const char *const cfgfile = "test.cfg";
    remove (cfgfile);
    assert_nonfatal (config_init (cfgfile) == CONFIG_INIT_CREAT,
                     "config file should have been initialized");

    ncap_config.aaudio_optimize  = 2; // power saving
    ncap_config.cur_track        = 0;
    ncap_config.isrepeat         = 0; // false
    ncap_config.isshuffle        = 0; // false
    ncap_config.volume           = 80;
    ncap_config.track_path       = "foo/bar";
    ncap_config.track_path_len   = strlen (ncap_config.track_path) + 1;
    const struct config_t cfgcpy = ncap_config;

    config_write ();
    assert_nonfatal (config_deinit () == CONFIG_OK,
                     "error with config_deinit");
    assert_nonfatal (config_init (cfgfile) == CONFIG_INIT_EXISTS,
                     "config file should exist after first init");
    config_read ();
    assert_nonfatal (cfg_eq (&cfgcpy, &ncap_config),
                     "written config should match read config");
    ncap_config.volume = 100;
    config_write ();
    config_read ();
    assert_nonfatal (ncap_config.volume == 100,
                     "volume should match written volume");
    assert_nonfatal (config_deinit () == CONFIG_OK,
                     "error with config_deinit");

    report ();

    return 0;
}
