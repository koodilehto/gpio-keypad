#!/bin/sh -eu

. ./gpio_config.sh

# Register GPIO

for IO in $KEYPAD_COLS; do
    test -d $IODIR/gpio$IO || echo $IO >$IODIR/export
    echo out >$IODIR/gpio$IO/direction
    echo 1 >$IODIR/gpio$IO/value
done

for IO in $KEYPAD_ROWS; do
    echo $IO >$IODIR/export
    echo in >$IODIR/gpio$IO/direction
    echo both >$IODIR/gpio$IO/edge
done
