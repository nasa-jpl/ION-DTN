#!/bin/bash

AL_BP_DIR="../"
ION_DIR="/sources/ion-3.7.0/"

gcc "-I${AL_BP_DIR}/src/bp_implementations" "-I${AL_BP_DIR}/src" "-I${ION_DIR}/include" "-I${ION_DIR}/library" -O2 -Wall -fmessage-length=0 -Werror -c main.c
g++ -L/usr/local/lib "-L${AL_BP_DIR}" -o main main.o "${AL_BP_DIR}/libal_bp_vION.a" -lbp -lici -limcfw -ldtn2fw -lipnfw -lpthread

