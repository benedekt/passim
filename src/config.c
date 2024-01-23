#include "config.h"

#include <libgen.h>

static struct argp_option options[] = {
    {"sampling-rate",   's', "FLOAT",   0, "Sampling rate of simulation.", 1},
    {"samples",         'n', "INT",     0, "Number of samples to be generated in each step.", 2},
    {"frequency",       'm', "FLOAT",   0, "Center frequency of transmission.", 3},

    {"start-time",      'b', "STR/INT", 0, "Start timestamp for simulation.", 4},
    {"timestamp",       'T', "STR/INT", OPTION_ALIAS, "Alias for --start-timestamp.", 3},
    {"end-time",        'e', "STR/INT", 0, "End timestamp for simulation.", 5},
    {"stride",          'd', "INT",     0, "Step time for simulation, 0 is single shot.", 6},

    {"max-distance",    'R', "FLOAT",   0, "Max distance to calculate reflection from RX.", 7},
    {"min-altitude",    'A', "FLOAT",   0, "Minimum altitude of object for calculation.", 8},
    {"max-slowdelay",   'D', "FLOAT",   0, "Maximum slow-time delay to calculate to.", 8},

    {"interactive",     'g', "INT",     OPTION_ARG_OPTIONAL, "Enable interactive mode.", 9},
    {"output",          'o', "FILE",    0, "Output file to dump samples.", 9},
    {"output-timing",   'p', "FILE",    0, "Output file to write timestap.", 9},
    {"verbose",         'v', "INT",     0, "Optional verbosity level for simulation.", 9},
    {"trackdump",       'u', "STR",     0, "Dump simulated tracks as PVT to this directory.", 9},

    {"tx-location",     't', "STR",     0, "Location of TX in GPS coordinates. (lat,lon,alt).", 10},
    {"tx-source",       'i', "FILE",    0, "Optional illumination signal file for TX, autogenerated GWN if not provided.", 11},
    {"rx-location",     'r', "STR",     0, "Location of RX in GPS coordinates (lat,lon,alt)", 12},
    {"rx-array",        'a', "FILE",    0, "Receiver antenna array definition.", 15},


    {"target-file",     'f', "FILE,FLOAT",  0, "File of KML target paths (multiple, excludes target coordinate option). RCS should be defined after comma.", 16},
    {"target-coord",    'c', "FLOATS",   0, "Coordinates of targets (multiple, excludes target files option). lat,lon,alt,speed,heading,rcs.", 17},
    {0}
};
struct argp argp = {
    options,
    parse_opt,
    NULL,
    "Program to simulate passive radar observation signals.\v(c) Benedek Tomka 2023, for MSc thesis",
    NULL,
    NULL,
    NULL
};

void init_config(int argc, char** argv) {
    config.tx_location = (struct GPSPOS){47.491691, 18.979128, 300}; // Széchenyi-hegy
    config.tx_source_file = NULL;

    config.rx_location = (struct GPSPOS){47.416431, 19.304206, 151}; // Ferihegy
    /*config.rx_array = (struct array){
        .geom           = ULA,
        .channel_count  = 8,
        .observe_count  = 7,
        .heading        = 0,
        .ant_geom       = 0.33,
        .frontend_gain  = 20
    };*/
    config.rx_array = (struct array){
        .geom           = OCA,
        .channel_count  = 8,
        .observe_count  = 7,
        .heading        = 0,
        .ant_geom       = 0.33,
        .frontend_gain  = 20
    };

    config.sample_rate      = 8333333.0;
    config.sample_count     = 1048576;
    config.center_freq      = 610e6;
    config.interactive      = 0;
    config.verbosity        = 0;
    config.max_distance     = 100e3;
    config.min_altitude     = 100;
    config.max_slowdelay    = 1024;

    config.output_file = NULL;
    config.output_timestamp = NULL;

    config.start_time = 0;
    config.end_time = 0;
    config.step = 0;

    config.targets = NULL;
    config.target_len = 0;

    config.dump_trackpoints = NULL;


    argp_parse(&argp, argc, argv, 0, 0, &config);
}

void prepare_config(struct config* config) {

    time_t min = time(NULL)*2, max = 0;
    for(int i=0; i<config->target_len; i++) {
        struct PVTDB* db = config->targets[i];
        if(db->list[0].time < min)
            min = db->list[0].time;
        if(db->list[db->size-1].time > max)
            max = db->list[db->size-1].time;
    }

    config->min_time = min;
    config->max_time = max;


    if(config->end_time == 0) {
        config->end_time = config->max_time;
    }
    if(config->start_time == 0) {
        config->start_time = config->min_time;
    }

    config->main_axel = distance(config->tx_location, config->rx_location);

}

void print_config(struct config* config) {
    printf("TX\n");
    printf("\tposition: %f %f %.2f\n", config->tx_location.lat, config->tx_location.lon, config->tx_location.alt);
    printf("\tsource:   %s\n", config->tx_source_file);
    printf("RX\n");
    printf("\tposition: %f %f %.2f\n", config->rx_location.lat, config->rx_location.lon, config->rx_location.alt);
    printf("\tarray:    ---\n");
    printf("SIMULATION\n");
    printf("\tsample_rate:     %.0f\n", config->sample_rate);
    printf("\tsample_count:    %d\n", config->sample_count);
    printf("\tcenter_freq:     %.0f\n", config->center_freq);
    printf("\tinteractive:     %d\n", config->interactive);
    printf("\toutput_file:     %s\n", config->output_file);

    printf("\tmax_distance:    %.0f m\n", config->max_distance);
    printf("\tmin_altitude:    %.0f m\n", config->min_altitude);

    char _start[20];
    char _end[20];
    strftime(_start, 20, "%Y-%m-%d %H:%M:%S", localtime(&(config->start_time)));
    strftime(_end, 20, "%Y-%m-%d %H:%M:%S", localtime(&(config->end_time)));

    printf("\tstart_time:      %s\n", _start);
    printf("\tend_time:        %s\n", _end);
    printf("\tstep:            %d sec\n", config->step);
    printf("TARGETS\n");
    printf("\tcount:    %d\n", config->target_len);
    for(int i=0; i<config->target_len; i++) {
        strftime(_start, 20, "%Y-%m-%d %H:%M:%S", localtime(&(config->targets[i]->list[0].time)));
        strftime(_end, 20, "%Y-%m-%d %H:%M:%S", localtime(&(config->targets[i]->list[config->targets[i]->size-1].time)));
        printf("\t[%d] (%s -> %s) %.1f %s %d \n",
            i, _start, _end, config->targets[i]->rcs, config->targets[i]->source, config->targets[i]->size
        );
    }

    time_t min = time(NULL)*2, max = 0, all;

    for(int i=0; i<config->target_len; i++) {
        struct PVTDB* db = config->targets[i];
        if(db->list[0].time < min)
            min = db->list[0].time;
        if(db->list[db->size-1].time > max)
            max = db->list[db->size-1].time;
    }

    strftime(_start, 20, "%Y-%m-%d %H:%M:%S", localtime(&(min)));
    strftime(_end, 20, "%Y-%m-%d %H:%M:%S", localtime(&(max)));
    printf("\tfrom: %s - to: %s\n", _start, _end);

}

int timestamp_parser(char* arg, time_t* t) {
    struct tm tm;
    if (strptime(arg, "%Y-%m-%dT%H:%M:%S", &tm) == NULL) {
        *t = strtoul(arg, NULL, 10);
        return 2;
    } else {
        *t = mktime(&tm);
        return 1;
    }
    return 0;
}
int location_parser(char* arg, struct GPSPOS* ret) {
    int r = sscanf(arg, "%lf,%lf,%lf", &ret->lat, &ret->lon, &ret->alt);
    return (r == 3);
}

struct array array_loader(char* arg) {

}

void target_file_loader(char* arg, struct config* config) {
    char* filename = strtok(arg, ",");
    char* rcs = strtok(NULL, ",");

    if(filename == NULL) return;
    if(rcs == NULL) rcs = "1";

    struct PVTDB* db = calloc(1, sizeof(struct PVTDB));

    db->rcs = atof(rcs);
    db->source = strdup(filename);
    db->dump_trackpoints = NULL;

    char path[64];
    snprintf(path, 64, "%s/%s", config->dump_trackpoints, db->source);
    db->dump_trackpoints = fopen(path, "w");

    if(kml_to_pvt(filename, db) >= 0) {
        config->targets = realloc(config->targets, sizeof(struct PVTDB*) * (config->target_len + 1));
        config->targets[config->target_len] = db;
        config->target_len++;
    }
}
void target_coord_loader(char* arg, struct config* config) {
    struct PVT pvt;
    double rcs = 0.0;
    int r = sscanf(arg, "%lf,%lf,%lf,%lf,%lf,%lf", &pvt.pos.lat, &pvt.pos.lon, &pvt.pos.alt, &pvt.speed, &pvt.heading, &rcs);
    pvt.time = 0;
    pvt.real = 1;

    if(r == 6) {
        struct PVTDB* db = calloc(1, sizeof(struct PVTDB));
        db->list = calloc(1, sizeof(struct PVT));
        db->list[0] = pvt;
        db->size = 1;
        db->rcs = 1;
        db->source = strdup("console");
        db->dump_trackpoints = NULL;

        char path[64];
        snprintf(path, 64, "%s/%s", config->dump_trackpoints, db->source);
        db->dump_trackpoints = fopen(path, "w");

        config->targets = realloc(config->targets, sizeof(struct PVTDB*) * (config->target_len + 1));
        config->targets[config->target_len] = db;
        config->target_len++;
    }
}

error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct config* config = state->input;

    switch (key) {
        case 's':
            config->sample_rate = atof(arg);
            if(config->sample_rate <= 1)
                argp_failure(state, 1, 0, "You must choose a valid sampling rate.");
            break;
        case 'n':
            config->sample_count = (int)atof(arg);
            break;
        case 'm':
            config->center_freq = (int)atof(arg);
            break;
        case 'R':
            config->max_distance = atof(arg);
            break;
        case 'A':
            config->min_altitude = atof(arg);
            break;
        case 'D':
            config->max_slowdelay = atoi(arg);
            break;
        case 'b':
        case 'T':
            if(!timestamp_parser(arg, &config->start_time))
                argp_failure(state, 1, 0, "Incorrect simulation start timestamp.");
            break;
        case 'e':
            if(!timestamp_parser(arg, &config->end_time))
                argp_failure(state, 1, 0, "Incorrect simulation end timestamp.");
            break;
        case 'd':
            config->step = atoi(arg);
            break;
        case 'g':
            config->interactive = 1;
            break;
        case 'o':
            config->output_file = strdup(arg);
            break;
        case 'p':
            config->output_timestamp = strdup(arg);
            break;
        case 'u':
            config->dump_trackpoints = strdup(arg);
            printf("opened path %s\n", config->dump_trackpoints);
            for(int i=0; i<config->target_len; i++) {
                char path[64];
                char alt[64];
                strlcpy(alt, config->targets[i]->source, 64);
                basename(alt);
                snprintf(path, 64, "%s/%s", config->dump_trackpoints, basename(alt));
                config->targets[i]->dump_trackpoints = fopen(path, "w");
                printf("opened path %s\n", path);
            }
            break;
        case 'v':
            config->verbosity = atoi(arg);
            break;
        case 't':
            if(!location_parser(arg, &config->tx_location))
                argp_failure(state, 1, 0, "Incorrect definition of TX location.");
            break;
        case 'i':
            config->tx_source_file = strdup(arg);
            break;
        case 'r':
            if(!location_parser(arg, &config->rx_location))
                argp_failure(state, 1, 0, "Incorrect definition of RX location.");
            break;
        case 'a':
            config->rx_array = array_loader(arg);
            break;
        case 'f':
            target_file_loader(arg, config);
            break;
        case 'c':
            target_coord_loader(arg, config);
            break;
        case ARGP_KEY_ARG:
            // Handle non-option arguments here if needed
            break;
        case ARGP_KEY_END:
            if(config->target_len <= 0) {
                argp_failure(state, 1, 0, "Some kind of target definition is needed.");
            }
            if(config->target_len <= 0) {
                argp_failure(state, 1, 0, "Some kind of target definition is needed.");
            }
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}
