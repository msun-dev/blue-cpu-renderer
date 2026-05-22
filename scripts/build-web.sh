#!/bin/bash

[ ! -f "./bin/" ] && mkdir -v ./bin/ 2>/dev/null
[ ! -f "./build/" ] && mkdir -v ./build/ 2>/dev/null
rm -f ./build/*

flags_cc="--std=gnu11 -Wall -Wpedantic -Wextra -Os"
flags_emcc="$RAYLIB/libraylibweb.a \
            -I. -I$RAYLIB/ \
            -L. -L$RAYLIB/ \
            -s ASYNCIFY -s USE_GLFW=3 -DPLATFORM-WEB \
            --shell-file $RAYLIB/minshell.html "

build() {
	emcc -o ./bin/main.html \
        ./src/main.c \
        ./include/blue-cpu/src/cpu.c $flags_cc $flags_emcc
}

pack() {
	zip_name='BLUE-CPU-vis.zip'
	[ -e "./bin/$zip_name" ] && rm ./bin/$zip_name
	mv ./bin/main.html ./bin/index.html
	zip ./bin/$zip_name ./bin/index.html ./bin/main.js ./bin/main.wasm
}

build &&
pack
