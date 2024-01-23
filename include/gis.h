#ifndef GIS_H
#define GIS_H

#include <math.h>
#include <stdio.h>

struct GPSPOS {
    double lat;
    double lon;
    double alt;
};

struct ECEF {
    double x;
    double y;
    double z;
};

struct ENU {
    double e;
    double n;
    double u;
};

#define EARTH_RADIUS 6378137.0 // Earth's semi-major axis (in meters)
#define EARTH_FLATTENING 1.0 / 298.257223563 // Earth's flattening (WGS-84)
#define EARTH_SEMI_MAJOR_AXIS 6378137.0
#define EARTH_ECCENTRICITY_SQUARED 0.00669437999014

double distance(struct GPSPOS pos1, struct GPSPOS pos2);

void lla_to_ecef(const struct GPSPOS* lla, struct ECEF* ecef);

void lla_to_enu(const struct GPSPOS* point_lla, const struct GPSPOS* center_lla, struct ENU* enu);

double      dotENU(const struct ENU* v1, const struct ENU* v2);
double      magnitudeENU(const struct ENU* v);
struct ENU  projectionENU(const struct ENU* v, const struct ENU* u);
struct ENU axpyENU(double a, const struct ENU* x, const struct ENU* y);
struct ENU diffENU(const struct ENU* a, const struct ENU* b);

double deg2rad(double degrees);
double rad2deg(double radians);

#endif
