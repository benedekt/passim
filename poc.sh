#!/bin/bash


SAMPLESTREAM=../../clradar/clradar-dsp/io/samplein
TIMESTREAM=../../clradar/clradar-dsp/io/tsin
# R=2028
./passim -b "2023-10-18T08:54:25" -d 0 -f tracks/AY1251-327b6eff.kml,1000 -o $SAMPLESTREAM -p $TIMESTREAM

# R=12
./passim -b "2023-10-18T09:08:30" -d 0 -f tracks/AY1251-327b6eff.kml,1000 -o $SAMPLESTREAM -p $TIMESTREAM


./passim -f tracks/AF1294-327b7985.kml,10000 -f tracks/AY1251-327b6eff.kml,20000 -f tracks/FR604-327b7a56.kml,100 -f tracks/FR2278-327b6d5d.kml,100000 -f tracks/FR4230-327b6d59.kml,10000 -f tracks/FR4264-327b7dc8.kml,10000 -f tracks/W62272-327b6dca.kml,100000 -f tracks/W62430-327b7df3.kml,10000 -b "2023-10-18T09:00:30" -d 0 -o ../../clradar/clradar-dsp/io/samplein -p ../../clradar/clradar-dsp/io/tsin

#[2023-10-18 09:00:30] 7 in air, 5 valid
#1 -> 47.640127,19.124958,2068.07 116.3m/s 312.6deg [19901.11+28367.30,   22364.5 ds,  131.3 m/s,  622 dt,    60.0Hz df, az 331.6, el 3.7, -54.75dB] nothing on screen
#2 -> 47.613027,18.997988,1158.24  99.6m/s 247.8deg [13593.47+31795.80,   19485.4 ds,   -1.6 m/s,  542 dt,     3.2Hz df, az 313.6, el 1.7, -75.44dB] very bad noise level
#4 -> 47.517994,19.139968, 756.92  87.1m/s 132.0deg [12474.43+16769.71,    3340.3 ds,  -35.8 m/s,   93 dt,    72.8Hz df, az 312.4, el 2.0, -49.14dB] good

bash build.sh && ./passim -f tracks/AF1294-327b7985.kml,1000 -f tracks/AY1251-327b6eff.kml,20000 -f tracks/FR604-327b7a56.kml,100 -f tracks/FR2278-327b6d5d.kml,100000 -f tracks/FR4230-327b6d59.kml,10000 -f tracks/FR4264-327b7dc8.kml,10000 -f tracks/W62272-327b6dca.kml,100000 -f tracks/W62430-327b7df3.kml,10000 -b "2023-10-18T09:00:30" -d 0 -o ../../clradar/clradar-dsp/io/samplein -p ../../clradar/clradar-dsp/io/tsin
