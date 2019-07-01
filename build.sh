#!/bin/bash

mkdir build
cd build
cmake ..
make install

# 编译成功
echo -e "build done"
exit 0
