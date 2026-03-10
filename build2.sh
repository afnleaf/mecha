#!/bin/bash
set -e

if [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM_LIBS="-framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo"
else
    PLATFORM_LIBS="-lGL -lm -lpthread -ldl -lrt -lX11"
fi

SRCS="src/main.c src/init.c src/spawn.c src/collision.c src/update.c src/draw.c"

if [ "$1" = "n" ]; then
    gcc $SRCS -o mecha -I src -I ./raylib/src -L ./lib -lraylib $PLATFORM_LIBS
elif [ "$1" = "o" ]; then
    gcc $SRCS -o mecha -O2 -march=native -flto=auto -ffast-math -DNDEBUG -I src -I ./raylib/src -L ./lib -lraylib $PLATFORM_LIBS
    strip mecha
else
    emcc $SRCS -o web_pkg/mecha.js -Os -I src -I ./raylib/src -L ./lib -l:libraylib.web.a -s USE_GLFW=3 -s SINGLE_FILE=1 -DPLATFORM_WEB
    histos config.yaml -o mecha.html
fi
