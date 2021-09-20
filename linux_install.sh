#!/bin/bash
make clean
make
cp -i *.a /usr/local/lib
cp -i textmode.h /usr/local/include

