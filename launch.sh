#!/bin/bash

DEVICE_HEAPTRACK_PATH=/opt/usr/heaptrack
DEVICE_ARCH=armel
HEAPTRACK_DATA_DIR=~/.heaptrack
SCRIPTS_PATH=$(dirname $BASH_SOURCE)/scripts/docker
RES_FILE=$HEAPTRACK_DATA_DIR/res.gz
APP_ID=$1
APP_PATH=$2

source "$SCRIPTS_PATH/../common.sh"
test_sdb_version

source $SCRIPTS_PATH/heaptrack-build.sh $DEVICE_ARCH

if [ "$?" != "0" ]; then
    echo "Errors; see the log for details"
    exit 1
fi

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
                               "$LAUNCH_GUI" \
                               "$CORECLR_BIN_DIR"

