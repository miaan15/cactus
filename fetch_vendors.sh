#!/bin/sh

set -e

mkdir -p ./vendor
cd ./vendor

# Use:
# - eastl
# - raylib
# - glm
# - spdlog
#
# - doctest

mkdir -p .

if [ ! -d "./eabase" ]; then
    mkdir eabase
    (
        cd eabase
        git init
        git remote add origin https://github.com/electronicarts/EABase.git
        git fetch --depth 1 origin 0699a15efdfd20b6cecf02153bfa5663decb653c
        git checkout FETCH_HEAD
    )
fi
if [ ! -d "./eastl" ]; then
    git clone --depth 1 --branch 3.27.01 https://github.com/electronicarts/EASTL.git ./eastl
fi

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
