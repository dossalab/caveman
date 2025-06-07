#!/bin/sh

CAMERA_NAME=$1
OUTPUT=$2

[ -z "$CAMERA_NAME" ] && echo "usage: $0 <camera name as seen by v4l2-ctl --list-devices>" && exit 1

DEVICE=$(v4l2-ctl --list-devices | grep -w -A1 "$CAMERA_NAME" | tail -n1 | xargs)

[ -z "$DEVICE" ] && echo "camera \"$CAMERA_NAME\" not found" && exit 1

echo "recording using device $DEVICE..."
ffmpeg -f v4l2 -video_size 640x480 -i "$DEVICE" -t 5 "$OUTPUT"
