#!/usr/bin/bash
cd src
if [ $1 = "-c" ]; then
    echo "Clean build"
    rm ../build/*
else
    mv ../build/*.o .
fi
make
mv *.o ../build
mv predictor ..