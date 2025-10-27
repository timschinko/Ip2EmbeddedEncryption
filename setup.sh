#!/bin/bash
set -e   

mkdir buildAndSdk

cd buildAndSdk/


SDK_DIR="$PROJECT_ROOT/buildAndSdk/pico-sdk"

sudo apt update
sudo apt install -y cmake gcc-arm-none-eabi build-essential libusb-1.0-0-dev git gcc

if [ ! -d "$SDK_DIR" ]; then
    git clone --recursive https://github.com/raspberrypi/pico-sdk 
    git clone --recursive https://github.com/raspberrypi/picotool 
    cd .. 
    export PICO_SDK_PATH=pico-sdk
    cp buildAndSdk/pico-sdk/external/pico_sdk_import.cmake .
else
    cd "$SDK_DIR"
    git pull
    git submodule update --init

    cd ../picotool
    git pull
    git submodule update --init
fi
