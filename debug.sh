#!/bin/bash

cd shaders

./compile.sh

cd ../

mkdir build

cd build

cmake ..

cd ../

cmake --build build

./bin/Nano
