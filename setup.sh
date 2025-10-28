#!/bin/bash
set -e   



picoAndBuildFolderSetup(){

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
}



arduino_setup(){
    # Arduino Setup 
    if ! command -v brew >/dev/null 2>&1; then
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    brew update
    #Maybe have to manually add brew to path 
    if ! command -v arduino-cli >/dev/null 2>&1; then
        brew install arduino-cli
        arduino-cli core update-index
        arduino-cli config add board_manager.additional_urls http://arduino.esp8266.com/stable/package_esp8266com_index.json
        arduino-cli core search esp8266
        arduino-cli core install esp8266:esp8266
    fi
fi
}

case "$1" in
    arduino)
        arduino_setup
        ;;
    pico)
        picoAndBuildFolderSetup
        ;;
    *)    
      echo "Setup: $1 {arduino|pico}" 
      ;;
esac  
