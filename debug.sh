#!/bin/bash

cd shaders

./compile.sh

cd ../

mkdir build

cmake --build build

./bin/Nano
