#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test.c"

#include "../algs.c"
#include "../config.c"

bool
cfg_eq (const struct config_t *c1, const struct config_t *c2)
{
    return memcmp (c1, c2, NCAP_CONFIG_SIZ) == 0
           && memcmp (c1->track_path, c2->track_path, c1->track_path_len) == 0;
}

int
main (void)
{
    const char *const cfgfile = "test.cfg";
    remove (cfgfile);
    assert_nonfatal (config_init (cfgfile) == CONFIG_INIT_CREAT,
                     "config file should have been initialized");

    ncap_config.aaudio_optimize = 2; // power saving
    ncap_config.cur_track       = 0;
    ncap_config.isrepeat        = 0; // false
    ncap_config.isshuffle       = 0; // false
    ncap_config.volume          = 80;
    ncap_config.track_path      = "foo/bar";
    ncap_config.track_path_len  = strlen (ncap_config.track_path) + 1;
    ncap_config.ntracks         = 2;
    ncap_config.track_vols      = malloc (ncap_config.ntracks);
    memset (ncap_config.track_vols, 100, ncap_config.ntracks);
    const struct config_t cfgcpy = ncap_config;

    // clang-format off
    assert_nonfatal (config_write () == CONFIG_OK, "config_write should work");
    assert_nonfatal (config_deinit () == CONFIG_OK, "error with config_deinit");
    assert_nonfatal (config_init (cfgfile) == CONFIG_INIT_EXISTS, "config file should exist after first init");
    assert_nonfatal (config_read () == CONFIG_OK, "config_read should work");
    assert_nonfatal (cfg_eq (&cfgcpy, &ncap_config), "written config should match read config");

    ncap_config.volume        = 100;
    ncap_config.track_vols[0] = 80;
    ncap_config.track_vols[1] = 60;
    assert_nonfatal (config_write () == CONFIG_OK, "config_write should work");
    assert_nonfatal (config_read () == CONFIG_OK, "config_read should work");
    assert_nonfatal (ncap_config.volume == 100, "master volume should match written volume");
    static uint8_t vols[] = { 80, 60, 100 };
    assert_nonfatal (memcmp (ncap_config.track_vols, vols, ncap_config.ntracks) == 0, "track volumes should match written volumes");

    assert_nonfatal (config_upd_vols(3, 100) == CONFIG_OK, "config_upd_vols should work");
    assert_nonfatal (ncap_config.ntracks == 3, "ntracks should update");
    assert_nonfatal (memcmp (ncap_config.track_vols, vols, ncap_config.ntracks) == 0, "track volumes should match after update");

    assert_nonfatal (config_upd_vols(1, 100) == CONFIG_OK, "config_upd_vols should work");
    assert_nonfatal (ncap_config.ntracks == 1, "ntracks should update");
    assert_nonfatal (memcmp (ncap_config.track_vols, vols, ncap_config.ntracks) == 0, "track volumes should match after update");

    assert_nonfatal (config_deinit () == CONFIG_OK, "error with config_deinit");
    // clang-format on

    // tord

    const size_t ntracks = 8;
    assert_nonfatal (config_tord_init (ntracks, 1314) == CONFIG_OK,
                     "tord_init should work");

    for (size_t i = 0; i < ntracks; ++i) {
        assert_nonfatal (config_tord_at (config_tord_r_at (i, NULL), NULL)
                             == i,
                         "tord and tord_r should be inverses");
    }

    assert_nonfatal (config_tord_deinit () == CONFIG_OK,
                     "tord_deinit should work");

    report ();

    return 0;
}
