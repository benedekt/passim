#include "config.h"
#include "gis.h"

int solve_path(struct path* path) {
    path->rx_dist = distance(config.rx_location, path->target->pos);
    path->tx_dist = distance(config.tx_location, path->target->pos);

    path->route_diff    = (path->tx_dist + path->rx_dist) - config.main_axel;
    path->time_diff     = path->route_diff / LIGHTSPEED_IN_AIR;
    path->delay         = round(path->time_diff / (1.0/config.sample_rate));

    if(path->delay > config.max_slowdelay) {
        //printf("\tinvalid slowtime delay\n");
        return 1;
    }


    struct ENU tx, rx, target;
    target = (struct ENU){0.0, 0.0, 0.0};
    lla_to_enu(&(config.rx_location), &(path->target->pos), &rx);
    lla_to_enu(&(config.tx_location), &(path->target->pos), &tx);

    //printf("TX  { e: %f n: %f u: %f } -> %.2f\n", tx.e, tx.n, tx.u, magnitudeENU(&tx));
    //printf("RX  { e: %f n: %f u: %f } -> %.2f\n", rx.e, rx.n, rx.u, magnitudeENU(&rx));

    path->bistatic_range = magnitudeENU(&tx) + magnitudeENU(&rx);

    struct ENU speed;
    speed.e = cos(M_PI/2.0 - deg2rad(path->target->heading)) * path->target->speed;
    speed.n = sin(M_PI/2.0 - deg2rad(path->target->heading)) * path->target->speed;
    speed.u = 0;

    double T = 10.0;

    struct ENU s = axpyENU(T, &speed, &target);

    //printf("V   { e: %f n: %f u: %f }\n", speed.e, speed.n, speed.u);
    //printf("S   { e: %f n: %f u: %f }\n", s.e, s.n, s.u);

    struct ENU rx_ = diffENU(&s, &rx);
    struct ENU tx_ = diffENU(&s, &tx);

    //printf("TX' { e: %f n: %f u: %f } -> %.2f\n", tx_.e, tx_.n, tx_.u, magnitudeENU(&tx_));
    //printf("RX' { e: %f n: %f u: %f } -> %.2f\n", rx_.e, rx_.n, rx_.u, magnitudeENU(&rx_));

    double bistatic_range_T = magnitudeENU(&tx_) + magnitudeENU(&rx_);
    double bistatic_range_dT= bistatic_range_T - path->bistatic_range;

    //printf("bistatic range: %.2f, bistatic_range_T: %.2f\n", path->bistatic_range, bistatic_range_T);
    //printf("bistatic range delta: %.2f\n", bistatic_range_dT);

    path->bistatic_speed = bistatic_range_dT / T;

    //printf("bistatic speed: %.2f\n", path->bistatic_speed);

    double lambda = LIGHTSPEED_IN_AIR / config.center_freq;

    path->doppler = path->bistatic_speed / (-1.0 * lambda);
    //path->doppler = 0;

    lla_to_enu(&(path->target->pos), &(config.rx_location), &target);

    //printf("T  { e: %f n: %f u: %f } -> %.2f\n",target.e,target.n,target.u, magnitudeENU(&target));

    path->azimuth = atan2(target.e, target.n);
    if(path->azimuth < 0) path->azimuth += 2.0*M_PI; // convert to heading

    if(array_blindspot(path->azimuth)) {
        return 2;
    }

    path->elevation = asin(target.u / magnitudeENU(&target));

#define PI4 4.0*M_PI

    double p_ref = 80.0 + 10*log10((lambda*lambda)/( (PI4)*(PI4)*config.main_axel*config.main_axel  ));
    double p_obs = 80.0 + 10*log10(path->rcs * (lambda*lambda)/( (PI4)*(PI4)*(PI4)*path->rx_dist*path->rx_dist*path->tx_dist*path->tx_dist ));

    double sn = p_obs - p_ref + config.rx_array.frontend_gain;

    printf("\tFYI Pref=%.2f dBm, Pobs=%.2f dBm Prf=%.2f dB -> attenuation = %.1f\n", p_ref, p_obs, config.rx_array.frontend_gain, sn);

    path->attenuation = pow(10.0, sn / 10.0);

    return 0;
}

void render_path(struct path* path) {
    clock_t begin = clock();

    if(path->observation == NULL)
        path->observation = calloc(config.sample_count, sizeof(cf32));

    dump_file("ref.cf32", __get_reference(), config.sample_count);

    //cnco(path->doppler, __get_reference(), path->observation);
    cnco(path->doppler, __get_reference(), path->observation);
    //printf("\tcnco: %.2f ms\n", (double)(clock() - begin) / CLOCKS_PER_SEC * 1000);

    //dump_file("cnco.cf32", path->observation, config.sample_count);

    delay_attenuate(path->observation, path->delay, path->attenuation);

    //dump_file("delay_attenuate.cf32", path->observation, config.sample_count);

    path->steering = array_steering(path->azimuth, path->elevation);

    clock_t end = clock();

    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    //printf("\tpath render took: %.2f ms\n", time_spent * 1000);
}

void __render_path(void* path) {
    //printf("rendering %p in %p\n", path, pthread_self());
    render_path((struct path*)path);
}
