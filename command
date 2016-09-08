#! /usr/bin/env sh
export CFLAGS='$CFLAGS -fPIC -Ofast -g -ggdb -march=native -std=gnu11 ' 
export CXXFLAGS='$CXXFLAGS -fPIC -Wno-error -Ofast -g  -ggdb -march=native -std=gnu++14 '
scons optimize=native qt5=1 qdebug=1 localecompare=0 opengles=0 mpg123=1 ffmpeg=1 $@
