#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/HTMLparser.h>

#include "config.h"
#include "pvt.h"
#include "gis.h"

int kml_to_pvt(const char* in, struct PVTDB* out) {
    int ret = 0;

    out->list = NULL;

    xmlDocPtr doc = NULL;
    xmlXPathContextPtr context = NULL;
    xmlXPathObjectPtr result = NULL;

    xmlInitParser();

    doc = xmlReadFile(in, NULL, 0);
    if(doc == NULL) {
        fprintf(stderr, "Failed to parse the KML file on path '%s'\n", in);
        ret = PVT_NO_FILE;
        goto cleanup;
    }

    context = xmlXPathNewContext(doc);
    if (context == NULL) {
        fprintf(stderr, "Failed to create XPath context.\n");
        ret = PVT_LIBXML_ERROR;
        goto cleanup;
    }

    xmlXPathRegisterNs(context, (xmlChar*)"kml", (xmlChar*)"http://www.opengis.net/kml/2.2");

    const xmlChar* xpathExpr = (xmlChar*)"//kml:Folder[1]/kml:Placemark";

    result = xmlXPathEvalExpression(xpathExpr, context);
    if (result == NULL) {
        ret = PVT_LIBXML_ERROR;
        goto cleanup;
    }

    if(xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        printf("No path found in KML file.\n");
        ret = PVT_EMPTY;
        goto cleanup;
    }

    struct PVT* table = calloc(result->nodesetval->nodeNr, sizeof(struct PVT));
    if(table == NULL) {
        printf("No memory for PVT points.\n");
        ret = PVT_MEMERROR;
        goto cleanup;
    }
    out->list = table;

    for (int i = 0; i < result->nodesetval->nodeNr; i++) {
        xmlNodePtr placemark    = result->nodesetval->nodeTab[i];
        xmlNodePtr name         = xmlFirstElementChild(placemark);
        xmlNodePtr description  = xmlNextElementSibling(name);
        xmlNodePtr timestamp    = xmlNextElementSibling(description);
        xmlNodePtr style        = xmlNextElementSibling(timestamp);
        xmlNodePtr coordinates  = xmlNextElementSibling(style);

        xmlChar* timestampText      = xmlNodeGetContent(xmlFirstElementChild(timestamp));
        xmlChar* coordinatesText    = xmlNodeGetContent(xmlNextElementSibling(xmlFirstElementChild(coordinates)));
        xmlChar* headingText        = xmlNodeGetContent(xmlFirstElementChild(xmlFirstElementChild(style)));

        xmlChar* descText = xmlNodeGetContent(description);

        //printf("Timestamp: %s\n", timestampText);
        //printf("Coordinates: %s\n", coordinatesText);
        //printf("Heading: %s\n", headingText);
        //printf("Description: %s\n", descText);

        struct tm tm;

        sscanf(timestampText, "%u-%u-%uT%u:%u:%u",
            &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
            &tm.tm_hour, &tm.tm_min, &tm.tm_sec
        );

        tm.tm_mon -= 1;
        tm.tm_year -= 1900;

        table[i].time = mktime(&tm);

        sscanf(coordinatesText, "%lf,%lf,%lf", &(table[i].pos.lon), &(table[i].pos.lat), &(table[i].pos.alt));

        table[i].heading = strtod(headingText, NULL);

        table[i].real = true;
        table[i].valid = true;

        if(descText){
            xmlDocPtr desc = htmlReadDoc((const xmlChar*)descText, NULL, NULL, HTML_PARSE_RECOVER);

            if(desc != NULL) {
                xmlXPathContextPtr dcontext = xmlXPathNewContext(desc);

               const xmlChar* dxpathExpr = (xmlChar*)"//div[span/b='Speed:']/span[2]";

               xmlXPathObjectPtr dresult = xmlXPathEvalExpression(dxpathExpr, dcontext);

               if (dresult != NULL && dresult->nodesetval != NULL && dresult->nodesetval->nodeNr > 0) {
                   xmlNodePtr speedNode = dresult->nodesetval->nodeTab[0];
                   xmlChar* speedText = xmlNodeGetContent(speedNode);

                   table[i].speed = strtod(speedText, NULL) * KNOT_TO_MS;

                   xmlFree(speedText);
               }

               if(dresult != NULL) xmlFree(dresult);
               xmlFree(dcontext);
               xmlFree(desc);
            }
        }

        // Clean up memory
        xmlFree(timestampText);
        xmlFree(coordinatesText);
        xmlFree(headingText);

        xmlFree(descText);
        ret++;
    }


    if(config.verbosity > 0) {
        char _start[20];
        char _end[20];
        strftime(_start, 20, "%Y-%m-%d %H:%M:%S", localtime(&(out->list[0].time)));
        strftime(_end, 20, "%Y-%m-%d %H:%M:%S", localtime(&(out->list[ret-1].time)));

        printf("Track loaded from %s - got %d points.\n", in, ret);
        printf("\tstarts at %u (%s)\n", out->list[0].time, _start);
        printf("\tends   at %u (%s)\n", out->list[ret-1].time, _end);
    }



    cleanup:
    if(result) xmlXPathFreeObject(result);
    if(context) xmlXPathFreeContext(context);
    if(doc) xmlFreeDoc(doc);
    xmlCleanupParser();

    out->size = ret;

    return ret;
}



int get_pvts_at(time_t timestamp, struct PVTDB** dbs, struct PVT** curs, int len) {
    int count = 0;
    for(int i=0; i<len; i++) {
        curs[i] = calloc(1, sizeof(struct PVT));
        if(get_pvt_at(timestamp, dbs[i], curs[i]) < 0) {
            //printf("WARN: no PVT found for track #%d\n", i);
            free(curs[i]);
            curs[i] = NULL;
        } else {
            count++;
        }
    }
    return count;
}
int get_pvt_at(time_t timestamp, struct PVTDB* db, struct PVT* cur) {
    if(db == NULL)      return -1;
    if(cur == NULL)     return -2;
    if(db->size <= 0)   return -3;

    if(timestamp < db->list[0].time) {
        return -4;
    } else if(timestamp > db->list[db->size-1].time) {
        return -5;
    }

    struct PVT* pre     = NULL;
    struct PVT* post    = NULL;
    for(int i=0; i < db->size-1; i++) {
        if(db->list[i].time >= timestamp && post == NULL) {
            post = &(db->list[i]);

            if(post->time != timestamp && i >= 1) pre = &(db->list[i-1]);
            else pre = post;
        }
    }
    if(post == NULL) {
        post = &(db->list[db->size-1]);
        pre = &(db->list[db->size-2]);
    }

    if(post->time == timestamp) {
        *cur = *post;
        return 0;
    }

    interpolate_pvt(*pre, *post, timestamp, cur);

    return 1;
}
int interpolate_pvt(struct PVT pvt1, struct PVT pvt2, double timestamp, struct PVT* result) {
    double t = (timestamp - pvt1.time) / (pvt2.time - pvt1.time);

    result->pos.lat = pvt1.pos.lat + t * (pvt2.pos.lat - pvt1.pos.lat);
    result->pos.lon = pvt1.pos.lon + t * (pvt2.pos.lon - pvt1.pos.lon);
    result->pos.alt = pvt1.pos.alt + t * (pvt2.pos.alt - pvt1.pos.alt);

    result->speed = pvt1.speed + t * (pvt2.speed - pvt1.speed);

    double heading1 = pvt1.heading;
    double heading2 = pvt2.heading;

    if (heading1 > heading2) {
        heading2 += 360.0;
    }

    double interpolated_heading = heading1 + t * (heading2 - heading1);
    if (interpolated_heading >= 360.0) {
        interpolated_heading -= 360.0;
    }
    result->heading = interpolated_heading;

    result->time = timestamp;

    result->real = false;

    return 0;
}

int filter_pvts(struct PVT** pvts, int len, double max_radius, double min_altitude, double min_speed) {
    int count = 0;

    for(int i=0; i<len; i++) {
        int drop = 0;
        if(pvts[i] == NULL) continue;
        double dist = distance(config.rx_location, pvts[i]->pos);
        if(dist > max_radius)               drop += 1;
        if(pvts[i]->pos.alt < min_altitude) drop += 2;
        if(pvts[i]->speed < min_speed)      drop += 4;

        if(drop) {
            free(pvts[i]);
            pvts[i] = NULL;
        } else {
            count++;
        }
    }
    return count;
}

void print_pvt_to_kml(struct PVT* pvt) {
    char iso8601[32];
    struct tm* timeinfo;
    timeinfo = gmtime(&pvt->time);

    strftime(iso8601, sizeof(iso8601), "%Y-%m-%dT%H:%M:%S%z", timeinfo);

    printf("<Placemark><name>%u</name><description></description><TimeStamp><when>%s</when></TimeStamp><Style><IconStyle><heading>%d</heading><Icon><href>https://maps.gstatic.com/intl/en_us/mapfiles/markers2/measle.png</href></Icon></IconStyle></Style><Point><altitudeMode>absolute</altitudeMode><coordinates>%f,%f,%.2f</coordinates></Point></Placemark>\n", pvt->time, iso8601, (int)pvt->heading, pvt->pos.lon, pvt->pos.lat, pvt->pos.alt);
}
void print_pvt(struct PVT* pvt) {
    if(pvt == NULL) {
        printf("{ no point }\n");
        return;
    }
    char _time[20];
    strftime(_time, 20, "%Y-%m-%d %H:%M:%S", localtime(&(pvt->time)));

    printf("{ time: %u (%s), lat: %f, lon: %f, alt: %.2f, spd: %.2f, head: %.2f, real: %d }\n",
        pvt->time, _time, pvt->pos.lat, pvt->pos.lon, pvt->pos.alt, pvt->speed, pvt->heading, pvt->real
    );
}

int pvt_test() {
    struct PVTDB pvtdb[3] = {0};

    kml_to_pvt("tracks/AF1294-327b7985.kml", &pvtdb[0]);
    kml_to_pvt("tracks/AY1251-327b6eff.kml", &pvtdb[1]);
    kml_to_pvt("tracks/FR604-327b7a56.kml", &pvtdb[2]);

    struct PVT not_up;
    get_pvt_at(1697606704, &pvtdb[0], &not_up);
    struct PVT takeoff;
    get_pvt_at(1697610384, &pvtdb[0], &takeoff);
    struct PVT exact;
    get_pvt_at(1697613645, &pvtdb[0], &exact);
    struct PVT interpolate;
    get_pvt_at(1697613647, &pvtdb[0], &interpolate);
    struct PVT landing;
    get_pvt_at(1697613807, &pvtdb[0], &landing);
    struct PVT landed;
    get_pvt_at(1697613860, &pvtdb[0], &landed);

    struct PVT** timepoint = calloc(3, sizeof(struct PVT*));
    //get_pvts_at(1697613807, (struct PVTDB*)pvtdb, timepoint, 3);

    for(int i=0; i<3; i++) print_pvt(timepoint[i]);
}
