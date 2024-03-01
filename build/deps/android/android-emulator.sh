#!/bin/bash

unameOut="$(uname -s)"
case "${unameOut}" in
    Linux*)     export ANDROID_SDK=$HOME/Android/Sdk;;
    Darwin*)    export ANDROID_SDK=$HOME/Library/Android/sdk;;
    *)          echo "Unsupported Host OS"; exit 1;;
esac


export ANDROID_SDK_ROOT=$ANDROID_SDK
export PATH=$ANDROID_SDK/emulator:$ANDROID_SDK/tools:$ANDROID_SDK/platform-tools:$PATH

avd=`emulator -list-avds`

echo "Running emulator $avd on $unameOut"
emulator \@$avd &