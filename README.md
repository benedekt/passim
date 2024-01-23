# Passive radar signal Simulator
This is a program to simulate complex SDR (IQ) observable output based on real tracks for development and verification of passive radar signal processing chains.

## Usage
Consult `passim -h`:
```
$ passim -h
Program to simulate passive radar observation signals.

  -s, --sampling-rate=FLOAT  Sampling rate of simulation.
  -n, --samples=INT          Number of samples to be generated in each step.
  -m, --frequency=FLOAT      Center frequency of transmission.
  -b, -T, --start-time=STR/INT, --timestamp=STR/INT
                             Start timestamp for simulation.
  -e, --end-time=STR/INT     End timestamp for simulation.
  -d, --stride=INT           Step time for simulation, 0 is single shot.
  -R, --max-distance=FLOAT   Max distance to calculate reflection from RX.
  -A, --min-altitude=FLOAT   Minimum altitude of object for calculation.
  -D, --max-slowdelay=FLOAT  Maximum slow-time delay to calculate to.
  -g, --interactive[=INT]    Enable interactive mode.
  -o, --output=FILE          Output file to dump samples.
  -p, --output-timing=FILE   Output file to write timestap.
  -v, --verbose=INT          Optional verbosity level for simulation.
  -t, --tx-location=STR      Location of TX in GPS coordinates. (lat,lon,alt).
  -i, --tx-source=FILE       Optional illumination signal file for TX,
                             autogenerated GWN if not provided.
  -r, --rx-location=STR      Location of RX in GPS coordinates (lat,lon,alt)
  -a, --rx-array=FILE        Receiver chain and array definition as shared lib.
  -f, --target-file=FILE,FLOAT   File of KML target paths (multiple, excludes
                             target coordinate option). RCS should be defined
                             after comma.
  -c, --target-coord=FLOATS  Coordinates of targets (multiple, excludes target
                             files option). lat,lon,alt,speed,heading,rcs.
  -?, --help                 Give this help list
      --usage                Give a short usage message

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.

FLOAT accepts anything which can be parsed by atof() - ex. '1', '1.0', '10e6'
INT accepts inputs parseable with strtol() in base 10 - ex. '1', '10'
STR as timestamp accepts ISO8601 without timezone - ex. '2023-09-01T10:00:00'
INT as timestamp accepts UNIX timestamp
FILE accepts path strings - ex. '/dev/null', './sample.cf32'
FILE of rx-array accepts dynamic library filename, implementing array interf.
```

## Build from source
This tool only has one real dependency:
* libxml2 - to parse KML XML files

Other than that you may need libbsd for the *oh-so-obscure* strlcpy() function on Ubuntu. Seriously?

Simply run: `bash build.sh`
The output binary will be compiled to `passim.out`.

## License & version
This is a lite version of the generator code, with inbuilt antenna array definition and no coupling matrix.

Please contact me if you need the extended version. Permission will be granted, but it is nice to know that your project is used :).

The code is released under the attached MIT license.