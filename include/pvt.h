#ifndef PVT_H
#define PVT_H

#include <stdbool.h>
#include <time.h>



#define PVT_NO_FILE -1
#define PVT_LIBXML_ERROR -2
#define PVT_EMPTY -3
#define PVT_MEMERROR -4

#define KNOT_TO_MS 0.5144444

#include "gis.h"

struct PVT {
    struct GPSPOS pos;
    double speed;
    double heading;

    time_t time;
    bool   real;
    bool   valid;
};

struct PVTDB {
    struct PVT* list;
    int         size;
    float       rcs;
    char*       source;

    FILE*       dump_trackpoints;
};

int kml_to_pvt(const char* in, struct PVTDB* out);

int get_pvts_at(time_t timestamp, struct PVTDB** dbs, struct PVT** curs, int len);
int get_pvt_at(time_t timestamp, struct PVTDB* db, struct PVT* cur);
int interpolate_pvt(struct PVT pvt1, struct PVT pvt2, double timestamp, struct PVT* result);


void print_pvt(struct PVT* pvt);
void print_pvt_to_kml(struct PVT* pvt);


#endif
