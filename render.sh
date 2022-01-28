#!/bin/bash
if [ -z "$1" -o -z "$2" ]; then
    echo "Usage: ./render.sh <path to MIDI> <path to output>"
    exit
fi
if [ ! -e "$1" ]; then
    echo "Input file doesn't exist"
    exit
fi
build/midiplayer -o play "$1" | ffmpeg -f rawvideo -pix_fmt rgba -s:v 1920x1080 -r 60 -i pipe: "$2"
