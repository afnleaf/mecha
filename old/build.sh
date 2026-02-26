#!/bin/bash
set -e

if [ "$1" = "n" ]; then
    gcc game.c -o game -I ./raylib/src -L ./lib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
elif [ "$1" = "o" ]; then
    gcc game.c -o game -O2 -march=native -flto -ffast-math -DNDEBUG -I ./raylib/src -L ./lib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    strip game
else
    emcc game.c -o game.js -Os -I ./raylib/src -L ./lib -l:libraylib.web.a -s USE_GLFW=3 -s SINGLE_FILE=1 -DPLATFORM_WEB
    histos config.yaml -o mecha.html
fi
