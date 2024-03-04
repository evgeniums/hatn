#!/bin/bash

export CC=clang
export CXX=clang++

if [ -z "$gettext_path" ]
then
    export gettext_path=/usr/local/opt/gettext/bin
fi
export PATH=$gettext_path:$PATH:/usr/local/bin

