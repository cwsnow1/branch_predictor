#!/usr/bin/bash
clang -v
if [ $? -neq 0 ]
    then
        echo "clang is not installed, but can be installed with"
        echo "apt-get install clang"
        echo "Or using a package manager of your choice"
        exit 1
fi
mkdir build
WD=$(pwd)
PIN_PATH=~/pin
$PIN_PATH/pin -h
if [ $? -neq 0 ]
    then
        echo "Install pin and update PIN_PATH in setup.sh. See README.md for link."
        exit 1
fi
cp $WD/branch_tool.cpp $PIN_PATH/source/tools/ManualExamples
cd $PIN_PATH/source/tools/ManualExamples
make branch_tool.test TARGET=intel64
cd $WD
$PIN_PATH/pin -t $PIN_PATH/source/tools/ManualExamples/obj-intel64/branch_tool.so -- ./build.sh -c
mv branch.out build.txt
./predictor build.txt
