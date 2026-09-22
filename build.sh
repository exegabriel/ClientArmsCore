#!/bin/bash
echo "Creating build folder..."
mkdir -p build
cd build
echo "Configuring..."
python ../configure.py --sdks csgo
echo "Building..."
ambuild