#!/bin/bash
set -e

if [ "$1" = "n" ]; then
    gcc mecha.c -o mecha -I ./raylib/src -L ./lib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
elif [ "$1" = "o" ]; then
    gcc mecha.c -o mecha -O2 -march=native -flto -ffast-math -DNDEBUG -I ./raylib/src -L ./lib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    strip game
else
    emcc mecha.c -o mecha.js -Os -I ./raylib/src -L ./lib -l:libraylib.web.a -s USE_GLFW=3 -s SINGLE_FILE=1 -DPLATFORM_WEB
    histos config.yaml -o mecha.html
fi
