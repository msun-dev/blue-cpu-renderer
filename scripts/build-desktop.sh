#!/bin/bash

[ ! -f "./bin/" ] && mkdir -v ./bin/ 2>/dev/null
[ ! -f "./build/" ] && mkdir -v ./build/ 2>/dev/null
rm -f ./build/*

flags_general="-std=gnu11 -Wall -Wpedantic -Wextra"
flags_debug="-ggdb -g3 -O0"
flags_ray="-lraylib -lGL -lm -lpthread -ldl -lrt -lX11"

debug() {
	cc -c ./include/blue-cpu/src/cpu.c \
	   -o ./build/cpu.o $flags_general $flags_debug
	cc -c ./src/main.c -o ./build/main.o $flags_general $flags_debug $flags_ray
	cc ./build/main.o ./build/cpu.o -o ./bin/main $flags_general $flags_ray
}

debug
