#!/bin/bash

SDB=$(which sdb)
DEVICE_HEAPTRACK_PATH=/opt/heaptrack
DEVICE_ARCH=armel
HEAPTRACK_DATA_DIR=~/.heaptrack
SCRIPTS_PATH=$(dirname $BASH_SOURCE)
RES_FILE=~/.heaptrack/res.gz
APP_ID=$1
APP_PATH=$2

docker build -t heaptrack:latest $SCRIPTS_PATH/../..
DOCKER_CONTAINER_HASH=$(docker create heaptrack:latest)

$SDB root on
LAUNCH_GUI="true"

$SCRIPTS_PATH/heaptrack-run.sh "$SDB" \
                               "$DEVICE_HEAPTRACK_PATH" \
                               "$DEVICE_ARCH" \
                               "$HEAPTRACK_DATA_DIR" \
                               "$DOCKER_CONTAINER_HASH" \
                               "$SCRIPTS_PATH" \
                               "$RES_FILE" \
			       "$APP_ID" \
			       "$APP_PATH" \
                               "$LAUNCH_GUI"

