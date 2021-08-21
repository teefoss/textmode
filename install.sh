#!/bin/bash
make clean
make
cp -i *.a /usr/local/lib
cp -i *.h /usr/local/include

