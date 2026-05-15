#!/bin/sh

set -e

mkdir -p ./vendor
cd ./vendor

# Use:
# - stdc++
# - raylib
# - glm
# - spdlog
#
# - doctest

mkdir -p .

if [ ! -d "./raylib" ]; then
    git clone --depth 1 --branch 6.0 https://github.com/raysan5/raylib.git ./raylib
fi

if [ ! -d "./glm" ]; then
    git clone --depth 1 --branch 1.0.3 https://github.com/g-truc/glm.git ./glm
fi

if [ ! -d "./spdlog" ]; then
    git clone --depth 1 --branch v1.17.0 https://github.com/gabime/spdlog.git ./spdlog
fi

if [ ! -d "./doctest" ]; then
    git clone --depth 1 --branch v2.5.2 https://github.com/doctest/doctest.git ./doctest
fi
