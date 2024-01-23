#ifndef CONFIG_H
#define CONFIG_H
#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <argp.h>
#include <pthread.h>

#include "pvt.h"
#include "sim.h"
#include "path.h"
#include "gis.h"
#include "array.h"

extern struct config config;

struct config {
    struct GPSPOS   tx_location;
    char*           tx_source_file;

    struct GPSPOS   rx_location;
    struct array    rx_array;

    double          center_freq;
    double          sample_rate;
    int             sample_count;
    int             interactive;
    int             verbosity;

    char*           dump_trackpoints;

    double          max_distance;
    double          min_altitude;
    int             max_slowdelay;

    char*           output_file;
    char*           output_timestamp;

    time_t          start_time;
    time_t          end_time;
    time_t          step;

    struct PVTDB**  targets;
    int             target_len;

    time_t          min_time;
    time_t          max_time;

    double          main_axel;
};

void init_config();
void prepare_config(struct config* config);
void print_config(struct config* config);

extern struct argp argp;
error_t parse_opt(int key, char *arg, struct argp_state *state);


#endif
