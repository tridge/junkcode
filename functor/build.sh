#!/bin/bash

arm-none-eabi-g++ -Wall -Os -mthumb -march=armv7e-m -mfloat-abi=hard -std=gnu++11 ftest.cpp -c
