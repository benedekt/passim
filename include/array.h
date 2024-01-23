#ifndef ARRAY_H
#define ARRAY_H

#include <complex.h>
typedef complex float cf32;

enum array_geom { ULA, UCA, OCA };

struct array {
    int channel_count;
    int observe_count;

    double heading;
    double ant_geom;

    double frontend_gain;

    enum array_geom geom;
};

cf32*   array_steering(double azi, double elev);
int     array_blindspot(double azi);

#endif
