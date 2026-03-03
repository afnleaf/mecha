#!/bin/bash
set -e

if [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM_LIBS="-framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo"
else
    PLATFORM_LIBS="-lGL -lm -lpthread -ldl -lrt -lX11"
fi

if [ "$1" = "n" ]; then
    gcc mecha.c -o mecha -I ./raylib/src -L ./lib -lraylib $PLATFORM_LIBS
elif [ "$1" = "o" ]; then
    gcc mecha.c -o mecha -O2 -march=native -flto=auto -ffast-math -DNDEBUG -I ./raylib/src -L ./lib -lraylib $PLATFORM_LIBS
    strip mecha
else
    emcc mecha.c -o mecha.js -Os -I ./raylib/src -L ./lib -l:libraylib.web.a -s USE_GLFW=3 -s SINGLE_FILE=1 -DPLATFORM_WEB
    histos config.yaml -o mecha.html
fi
