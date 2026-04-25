#!/bin/sh

set -e

mkdir -p ./vendor
cd ./vendor

# Use:
# - stdc++
# - glm
# - spdlog
# - gtest

mkdir -p .

if [ ! -d "./glm" ]; then
    git clone --depth 1 --branch 1.0.3 https://github.com/g-truc/glm.git ./glm
fi

if [ ! -d "./spdlog" ]; then
    git clone --depth 1 --branch v1.17.0 https://github.com/gabime/spdlog.git ./spdlog
fi

if [ ! -d "./gtest" ]; then
    git clone --depth 1 --branch v1.17.0 https://github.com/google/googletest.git ./gtest
fi
