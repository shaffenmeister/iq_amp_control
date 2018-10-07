#!/bin/sh
cmake -H. -Bbuild
cmake --build build --
cd build
make clean package
cd ..

