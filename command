CC=clang CXX=clang++ CCFLAGS=' -std=gnu14 ' CXXFLAGS=' -std=gnu++14 -Wno-inconsistent-missing-override  ' scons optimize=native mad=1 qt5=1 color=1 verbose=1 qdebug=1 ffmpeg=1 build=debug mpg123=0 $@ 

