#!/bin/bash

gcc -o passim.out -I./include -I/usr/include/libxml2 -fopenacc src/main.c src/config.c src/array.c src/rf.c src/gis.c src/path.c src/pvt.c src/threadpool.c -lxml2 -lm -lpthread -lbsd -O3
