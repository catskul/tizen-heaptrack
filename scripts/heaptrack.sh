#!/bin/bash

SDB=/home/ubuntu/tizen-studio/tools/sdb
DEVICE_HEAPTRACK_PATH=/opt/heaptrack
HEAPTRACK_FOR_DEVICE_HOST_PATH=/home/ubuntu/heaptrack-arm
HEAPTRACK_FOR_HOST_PATH=/home/ubuntu/heaptrack
SCRIPTS_PATH=/home/ubuntu/heaptrack-scripts
RES_FILE=/home/ubuntu/res.gz
APP_ID=$1
APP_PATH=$2

$SDB root on
LAUNCH_GUI="true"

$SCRIPTS_PATH/heaptrack-run.sh "$SDB" \
                               "$DEVICE_HEAPTRACK_PATH" \
                               "$HEAPTRACK_FOR_DEVICE_HOST_PATH" \
                               "$HEAPTRACK_FOR_HOST_PATH" \
                               "$SCRIPTS_PATH" \
                               "$RES_FILE" \
			       "$APP_ID" \
			       "$APP_PATH" \
                               "$LAUNCH_GUI"

