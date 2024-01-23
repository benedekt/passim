#include "array.h"
#include "rf.h"

#include "config.h"

cf32 array_element_char_for_dir(double azi);

double calc_angle_to_rx(double azi, double elev) {
    double azi_deg = rad2deg(azi);

    char* azi_override = getenv("OVERRIDE_AZI");
    if(azi_override) {
        azi_deg = atof(azi_override);
        printf("OVERRIDE AZI TO %.2f\n", atof(azi_override));
    }

    double theta_deg = azi_deg - config.rx_array.heading;

    //printf("\tazi_deg: %.1f theta_deg: %.1f\n", azi_deg, theta_deg);

    if(theta_deg >= 3.0 * 90.0) {
        theta_deg -= 360;
    } else if(theta_deg > 90) {
        theta_deg -= 180;
        printf("target behind array\n");
    }

    return theta_deg;
}

cf32* array_steering(double azi, double elev) {
    double theta_deg = calc_angle_to_rx(azi, elev);

    cf32* a = calloc(config.rx_array.channel_count, sizeof(cf32));

    if(config.rx_array.geom == ULA) {
        float dphi = 2.0 * M_PI * config.rx_array.ant_geom * sin(deg2rad(theta_deg));
        for(int i=0; i<config.rx_array.channel_count; i++) {
            a[i] = cexp((float)(i) * dphi * I);
        }
    } else if(config.rx_array.geom == OCA) {
        double alpha = 2.0*M_PI / (config.rx_array.channel_count-1);
        double theta = deg2rad(theta_deg);

        cf32* ant = calloc(config.rx_array.channel_count, sizeof(cf32));

        ant[0] = 0 + 0*I; // reference channel in center
        for(int ch = 1 ; ch<config.rx_array.channel_count; ch++)
            ant[ch] = config.rx_array.ant_geom * cexp((ch-1) * alpha * I); // other elements on circle

        for(int ch = 0; ch<config.rx_array.channel_count; ch++) {
            a[ch] = cexp( I * 2.0 * M_PI * ( creal(ant[ch]) * cos(theta) + cimag(ant[ch]) * sin(theta) ) );

            double rel_azi = theta_deg - rad2deg(alpha) * ch;
            if(rel_azi < 0) rel_azi += 360;
            if(rel_azi < 0) rel_azi += 360;
            //printf("direction = %.2f deg, relative to channel %d -> %.2f\n", theta_deg, ch, rel_azi);
            a[ch] *= array_element_char_for_dir(rel_azi);
        }

        free(ant);
    }



    return a;
}

int array_blindspot(double azi) {
    if(config.rx_array.geom != ULA) return 0;

    double theta_deg = calc_angle_to_rx(azi, 0.0);

    if(theta_deg > 80 && theta_deg < 280) {
        return 1;
    }

    return 0;
}

cf32 array_element_char_for_dir(double azi) {
    if(azi < 0) azi += 360;
    if(azi < 0) azi += 360;

    //printf("azi: %.2f\n", azi);

    if(azi > 300 || azi < 60) {
        return 1.0;
    } else if(azi > 120 && azi < 240) {
        return 1.0;
    } else {
        return 0.0;
    }
}
