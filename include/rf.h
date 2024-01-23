#ifndef RF_H
#define RF_H

#include <math.h>
#include <complex.h>


#include "config.h"

typedef complex float cf32;

void init_rf(void);

cf32 gwn(void);
void cnco(double freq, cf32* source, cf32* target);
void delay_attenuate(cf32* target, int delay, double total_attenuation);
void addch_addnoise(cf32* target, cf32* ref, double ref_att_db, double noise_att_db);
void addch_saaxpy(cf32* restrict acc, cf32 a, cf32 aa, const cf32* restrict source);

void __clean_output(void);
cf32* __get_reference(void);
cf32* __get_channel(int i);

void dump_file(const char* name, cf32* data, int len);

#endif
