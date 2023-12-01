#! /usr/bin/bash

if [ -d "src" ]; then
    source_files=$(find src -type f -name "*.cpp")
    if [ -n "$source_files" ]; then
        g++ $source_files \
            -o main \
            -L /usr/lib/libraylib.so \
            -lraylib \
            -g
    else 
        echo "No .cpp files in src dir."
    fi
else
    echo "No src dir!?"
fi
