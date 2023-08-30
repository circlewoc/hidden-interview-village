#!/bin/bash
set -e

if [ ! -d build ]; then
    mkdir build
fi

rm -rf build/*
cd build/ && 
    cmake .. &&
    make

cd ..
if [ ! -d /usr/include/mymuduo ]; then
    mkdir /usr/include/mymuduo
fi
echo 1

for header in `ls *.h`
do 
    cp $header /usr/include/mymuduo
done
echo 2

cp lib/libmymuduo.so /usr/lib
ldconfig 
echo 3