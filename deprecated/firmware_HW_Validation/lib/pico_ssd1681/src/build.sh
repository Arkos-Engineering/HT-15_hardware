#!/bin/bash
set -e

MODE=${1:-4wire}

echo "=== SSD1681 Pico Build Script ==="

if [ -z "$PICO_SDK_PATH" ]; then
    echo "ERROR: PICO_SDK_PATH not set"
    echo "export PICO_SDK_PATH=/path/to/pico-sdk"
    exit 1
fi

echo "PICO_SDK_PATH: $PICO_SDK_PATH"
echo "Mode: $MODE"

rm -rf build
mkdir build
cd build

if [ "$MODE" = "3wire" ]; then
    echo "Configuring for 3-wire SPI..."
    cmake -DUSE_3WIRE_SPI=ON ..
else
    echo "Configuring for 4-wire SPI..."
    cmake ..
fi

make -j$(nproc)

echo ""
echo "=== Build Complete ==="
echo "Output: build/example.uf2"
echo ""
if [ "$MODE" = "3wire" ]; then
    echo "3-Wire Mode: 5 pins (MOSI, SCK, CS, RST, BUSY)"
    echo "No D/C pin needed!"
else
    echo "4-Wire Mode: 6 pins (MOSI, SCK, CS, D/C, RST, BUSY)"
fi
