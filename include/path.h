#ifndef PATH_H
#define PATH_H

#include "pvt.h"
#include "rf.h"

typedef complex float cf32;

#define LIGHTSPEED_IN_AIR (2.99709e8)

struct path {
    struct PVT* target;
    double rcs;

    double rx_dist;
    double tx_dist;

    double bistatic_range;
    double bistatic_speed;

    double route_diff;
    double time_diff;

    int    delay;
    double doppler;

    double azimuth;
    double elevation;

    double attenuation;

    cf32*  observation;

    cf32*  steering;

};

int solve_path(struct path* path);
void render_path(struct path* path);

void __render_path(void* path);

double calculate_delay(double distance);





#endif
