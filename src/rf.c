#include "rf.h"

static cf32* sample_out_buf;
static cf32* sample_ref_buf;

static cf32* sample_obs_buf;

static cf32* noise_buffer;

static cf32* lo_p;
static cf32* lo_n;


void init_rf(void) {
    sample_out_buf = calloc(config.sample_count * config.rx_array.channel_count, sizeof(cf32));

    sample_ref_buf = calloc(config.sample_count, sizeof(cf32));
    noise_buffer = calloc(config.sample_count, sizeof(cf32));

    sample_obs_buf = calloc(config.sample_count * config.rx_array.observe_count, sizeof(cf32));


    if(config.tx_source_file != NULL) {
        FILE* fp = fopen(config.tx_source_file, "rb");
        if(fp == NULL) {
            fprintf(stderr, "WARN: cannot read TX sample file\n");
            exit(1);
        }

        size_t nr = fread(sample_ref_buf, config.sample_count, sizeof(cf32), fp);

        if(nr != config.sample_count) {
            fprintf(stderr, "WARN: cannot read enough TX samples from file\n");
            exit(1);
        }

        fclose(fp);
    } else {
        printf("filling reference channel with GWN\n");
        for(int i=0; i<config.sample_count; i++)
            sample_ref_buf[i] = 1.0*gwn();
    }

    for(int i=0; i<config.sample_count; i++)
        noise_buffer[i] = gwn();

    lo_p = calloc(config.sample_rate, sizeof(cf32));
    lo_n = calloc(config.sample_rate, sizeof(cf32));
    for(int i=0; i < config.sample_rate; i++) {
        float p = 2.0 * M_PI / config.sample_rate * i;
		lo_p[i]=cos(p)+sin(p)*I;
		lo_n[i]=cos(p)-sin(p)*I;
    }

}

static inline float rnd() {
    return rand()/(1.0+(float)RAND_MAX);
}

cf32 gwn() {
    float x,y,a,rr;
    do {
        x = rnd() * 2 - 1;
        y = rnd() * 2 - 1;
        rr=x*x+y*y;
    } while(rr >= 1.0 || rr == 0.0);

    a=sqrt(-log(rr)/rr);

    return a*x+a*y*I;
}

void cnco(double freq, cf32* source, cf32* target) {
#ifdef DDS_TYPE
    cf32* lo = NULL;
    if(freq > 0)    lo = lo_p;
    else            lo = lo_n;

    if(freq < 0) freq = -freq;

    long int loi = 0;
    for(int i = 0; i < config.sample_count; i++) {
        target[i] = source[i] * lo[loi];

        loi = (loi + (int)freq) % (int)config.sample_rate;
    }
#else
    double freq_abs = fabs(freq);
    double freq_sgn = freq < 0 ? -1.0 : 1.0;
    double T = (1.0 / config.sample_rate);

    for(int i = 0; i < config.sample_count; i++) {
        double phase = 2 * M_PI * freq_abs * ((double)i * T);
        cf32 oscval = cos(phase) + sin(phase)*I*freq_sgn;
        target[i] = source[i] * oscval;
    }
#endif
}

void delay_attenuate(cf32* target, int delay, double total_attenuation) {
    memcpy(&target[delay], target, (config.sample_count - delay) * sizeof(cf32));
    memset(target, 0x00, sizeof(cf32) * delay);

    for(int i=delay; i<config.sample_count; i++)
        target[i] *= total_attenuation;

}
void addch_addnoise(cf32* restrict target, cf32* restrict ref, double ref_att_db, double noise_att_db) {
    double ref_att_rel = pow(10.0, ref_att_db / 20.0);
    double noise_att_rel = pow(10.0, noise_att_db / 20.0);

    #pragma acc parallel loop
    for(int i=0; i<config.sample_count; i++) {
        cf32 noise;

        noise = (sample_ref_buf[i] * ref_att_rel);     // reference bleed

        if(noise_att_db > -120.0)
            noise += (noise_buffer[i] * noise_att_rel);    // uncorrelated noise

        target[i] += noise;
    }
}

void addch_saaxpy(cf32* restrict acc, cf32 a, cf32 aa, const cf32* restrict source) {
    #pragma acc parallel loop
    for (int i = 0; i < config.sample_count; ++i) {
        acc[i] = aa * a * source[i] + acc[i];
    }
}


cf32* __get_reference(void) {
    return sample_ref_buf;
}

void __clean_output(void) {
    memset(sample_out_buf, 0x00, sizeof(cf32)*config.rx_array.channel_count*config.sample_count);
}

cf32* __get_channel(int i) {
    return &sample_out_buf[i * config.sample_count];
}


void dump_file(const char* name, cf32* data, int len) {
    FILE* f = fopen(name, "wb");
    fwrite(data, len, sizeof(cf32), f);
    fclose(f);
}
