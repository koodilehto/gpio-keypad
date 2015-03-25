<!-- -*- mode: markdown; coding: utf-8 -*- -->
# gpio-keypad

GPIO keypad driver. Allows simple GPIO connected numpads to be used as
real input devices on Linux. Any hardware with GPIO pins should work
and any key matrix size should work.

We have an example configuration for SparkFun 12 button keypad
https://www.sparkfun.com/products/8653

## Build dependencies

On Raspbian or other Debian based system all the dependencies present
in package manager. To install:

    sudo apt-get install scons libglib2.0-dev

## Usage

    sudo ./gpio-keypad CONFIG_FILE
