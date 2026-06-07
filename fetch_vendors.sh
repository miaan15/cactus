#!/bin/sh

set -e

mkdir -p ./vendor
cd ./vendor

mkdir -p .

if [ ! -d "./sdl" ]; then
    git clone --depth 1 --branch release-3.4.10 https://github.com/libsdl-org/SDL.git ./sdl
fi

if [ ! -d "./glm" ]; then
    git clone --depth 1 --branch 1.0.3 https://github.com/g-truc/glm.git ./glm
fi
