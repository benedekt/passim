#include "gis.h"


// Define WGS84 ellipsoid parameters
const double a = 6378137.0;  // Semi-major axis (meters)
const double e = 0.0818191908426;  // Eccentricity

double distance(struct GPSPOS pos1_, struct GPSPOS pos2_) {
    struct ECEF pos1;
    lla_to_ecef(&pos1_, &pos1);
    struct ECEF pos2;
    lla_to_ecef(&pos2_, &pos2);

    double dx = pos2.x - pos1.x;
    double dy = pos2.y - pos1.y;
    double dz = pos2.z - pos1.z;

    double dist = sqrt(dx * dx + dy * dy + dz * dz);

    //printf("dist[(%f,%f,%f), (%f,%f,%f)] = %f\n", pos1_.lat, pos1_.lon, pos1_.alt, pos2_.lat, pos2_.lon, pos2_.alt, dist);

    return dist;
}



// Convert degrees to radians
double deg2rad(double degrees) {
    return degrees * M_PI / 180.0;
}

double rad2deg(double radians) {
    return radians / (M_PI / 180.0);
}

// Convert LLA coordinates to ECEF coordinates
void lla_to_ecef(const struct GPSPOS* lla, struct ECEF* ecef) {
    double lat_rad = deg2rad(lla->lat);
    double lon_rad = deg2rad(lla->lon);

    double N = a / sqrt(1 - e * e * sin(lat_rad) * sin(lat_rad));
    ecef->x = (N + lla->alt) * cos(lat_rad) * cos(lon_rad);
    ecef->y = (N + lla->alt) * cos(lat_rad) * sin(lon_rad);
    ecef->z = (N * (1 - e * e) + lla->alt) * sin(lat_rad);
}

// Convert ECEF coordinates to ENU coordinates around a center point
// Convert ECEF coordinates to ENU coordinates around a center point specified in LLA
void lla_to_enu(const struct GPSPOS* point_lla, const struct GPSPOS* center_lla, struct ENU* enu) {
    struct ECEF point_ecef;
    lla_to_ecef(point_lla, &point_ecef);

    struct ECEF center_ecef;
    lla_to_ecef(center_lla, &center_ecef);

    double center_lat_rad = deg2rad(center_lla->lat);
    double center_lon_rad = deg2rad(center_lla->lon);

    double dx = point_ecef.x - center_ecef.x;
    double dy = point_ecef.y - center_ecef.y;
    double dz = point_ecef.z - center_ecef.z;

    enu->e = -sin(center_lon_rad) * dx + cos(center_lon_rad) * dy;
    enu->n = -sin(center_lat_rad) * cos(center_lon_rad) * dx - sin(center_lat_rad) * sin(center_lon_rad) * dy + cos(center_lat_rad) * dz;
    enu->u = cos(center_lat_rad) * cos(center_lon_rad) * dx + cos(center_lat_rad) * sin(center_lon_rad) * dy + sin(center_lat_rad) * dz;
}

double dotENU(const struct ENU* v1, const struct ENU* v2) {
    return v1->e * v2->e + v1->n * v2->n + v1->u * v2->u;
}

double magnitudeENU(const struct ENU* v) {
    return sqrt(v->e * v->e + v->n * v->n + v->u * v->u);
}

struct ENU projectionENU(const struct ENU* v, const struct ENU* u) {
    double dot = dotENU(v, u);
    double mag_squared = magnitudeENU(u) * magnitudeENU(u);

    struct ENU result;
    result.e = (dot / mag_squared) * u->e;
    result.n = (dot / mag_squared) * u->n;
    result.u = (dot / mag_squared) * u->u;

    return result;
}

struct ENU axpyENU(double a, const struct ENU* x, const struct ENU* y) {
    struct ENU result;
    result.e = a*x->e + y->e;
    result.n = a*x->n + y->n;
    result.u = a*x->u + y->u;

    return result;
}

struct ENU diffENU(const struct ENU* a, const struct ENU* b) {
    struct ENU result;
    result.e = a->e - b->e;
    result.n = a->n - b->n;
    result.u = a->u - b->u;

    return result;
}
