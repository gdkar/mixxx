#! /usr/bin/env sh
export CFLAGS='$CFLAGS -fPIC -O3 -Ofast -g -ggdb -march=native -std=gnu11 ' 
export CXXFLAGS='$CXXFLAGS -fPIC -Wno-error -O3 -Ofast -g  -ggdb -march=native -std=gnu++14 '
scons optimize=native qt5=1 qdebug=1 localecompare=0 opengles=0 $@ ffmpeg=0
