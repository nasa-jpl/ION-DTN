#!/bin/bash
aclocal 
autoconf
automake
make clean 
./configure --enable-eclsa --with-upperProtLTP --with-lowerProtUDP --with-codecOpenFEC CFLAGS='-O0 -ggdb3' CPPFLAGS='-O0 -ggdb3' CXXFLAGS='-O0 -ggdb3'