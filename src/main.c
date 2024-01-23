#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "gis.h"
#include "threadpool.h"

struct config config;

int main(int argc, char *argv[]) {
    init_config(argc, argv);

    prepare_config(&config);

    print_config(&config);

    simulate();

    return 0;
}

void simulate() {
    printf("SIMULATION STARTING NOW... (%.2fm main)\n", config.main_axel);

    int target_cnt = 0, valid_cnt = 0;
    char _tick[20];

    FILE* output = NULL;
    if(config.output_file != NULL) {
        output = fopen(config.output_file, "wb");
        if(output == NULL) {
            printf("ERR: cannot open output file\n");
            exit(2);
        }
        printf("opened output path for writing '%s'\n", config.output_file);
    }

    FILE* output_timestamp = NULL;
    if(config.output_timestamp != NULL) {
        output_timestamp = fopen(config.output_timestamp, "w");
        if(output_timestamp == NULL) {
            printf("ERR: cannot open output timestamp file\n");
            exit(2);
        }
        printf("opened timestamp path for writing '%s'\n", config.output_timestamp);
    }

    struct PVT**  targets = calloc(config.target_len, sizeof(struct PVT*));
    struct path*  paths = calloc(config.target_len, sizeof(struct path));

    init_rf();

    tpool_t* workers = tpool_create(4);

    time_t tick = config.start_time;
    do {
        clock_t begin = clock();
        strftime(_tick, 20, "%Y-%m-%d %H:%M:%S", localtime(&tick));

        target_cnt  = get_pvts_at(tick, config.targets, targets, config.target_len);
        valid_cnt   = filter_pvts(targets, config.target_len, config.max_distance, config.min_altitude, 10.0);

        printf("[%s] %d in air, %d valid\n", _tick, target_cnt, valid_cnt);

        int c = 0;
        for(int i=0; i<config.target_len; i++) {
            if(targets[i] == NULL) continue;
            struct PVT* t = targets[i];

            paths[c].target = t;
            paths[c].rcs = config.targets[i]->rcs;

            if(solve_path(&paths[c])) {
                targets[i] = NULL;
                valid_cnt--;
                continue;
            }

            if(config.targets[i]->dump_trackpoints != NULL) {
                fprintf(config.targets[i]->dump_trackpoints, "\t{\"timestamp\": \"%s\", \"lat\": %.6f, \"lon\": %.7f}\n", _tick, t->pos.lat, t->pos.lon);
                fflush(config.targets[i]->dump_trackpoints);
            }

            printf("\t#%d -> %f,%f,%7.2f %5.1fm/s %5.1fdeg [%8.2f+%8.2f, %9.1f ds, %6.1f m/s, %4d dt, %7.1fHz df, az %5.1f, el %2.1f, %5.2fdB] \n", i,
                t->pos.lat, t->pos.lon, t->pos.alt, t->speed, t->heading,
                paths[c].tx_dist,
                paths[c].rx_dist,
                paths[c].route_diff,
                paths[c].bistatic_speed,
                paths[c].delay,
                paths[c].doppler,
                rad2deg(paths[c].azimuth),
                rad2deg(paths[c].elevation),
                10*log10(paths[c].attenuation)
            );

            c++;
        }

        if(valid_cnt == 0) goto fastforward;

        for(int i=0; i<config.target_len; i++) {
            if(targets[i] == NULL) continue;
            //tpool_add_work(workers, __render_path, &paths[i]);

            render_path(&paths[i]);
        }

        tpool_wait(workers);

        __clean_output();

        // create reference channel with strong signal
        addch_addnoise(__get_channel(0), __get_reference(), 0.0, -240.0);

        // count generated targets for normalization

        // create observation channels
        for(int ch=0; ch<config.rx_array.channel_count; ch++) {
            // reference and noise to each observation channel
            if(ch != 0) addch_addnoise(__get_channel(ch), __get_reference(), -50.0, -90.0);

            //if(ch == 1) continue;

            // add paths to channels
            for(int i=0; i<config.target_len; i++) {
                if(targets[i] == NULL) continue;
                addch_saaxpy(__get_channel(ch), paths[i].steering[ch], 1.0, paths[i].observation);
            }
        }

        //dump_file("all.cf32", __get_channel(0), config.sample_count * config.rx_array.channel_count);

        if(output != NULL) {
            fwrite(__get_channel(0), config.sample_count * config.rx_array.channel_count, sizeof(cf32), output);
            fflush(output);
        }

        if(output_timestamp != NULL) {
            fprintf(output_timestamp, "%u\n", tick);
            fflush(output_timestamp);
        }

        clock_t end = clock();
        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        //printf("\tstep render took: %.2f ms\n", time_spent * 1000);

        fastforward:
        if(config.step == 0)    break;
        else                    tick += config.step;
    } while (tick < config.end_time);

    free(paths);
    free(targets);

    if(output != NULL)
        fclose(output);
    if(output_timestamp)
        fclose(output_timestamp);


}
