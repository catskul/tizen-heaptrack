#!/bin/bash

SDB=$(which sdb)
DEVICE_HEAPTRACK_PATH=/opt/heaptrack
DEVICE_ARCH=armel
HEAPTRACK_DATA_DIR=~/.heaptrack
SCRIPTS_PATH=$(dirname $BASH_SOURCE)
RES_FILE=$HEAPTRACK_DATA_DIR/res.gz
APP_ID=$1
APP_PATH=$2

test_sdb_version() {
    if [ ! -f $SDB ]; then
        echo "sdb not found. Please make sure that sdb is in your PATH."
        exit 1
    fi
    ver=( $($SDB version | sed -r 's/.*([0-9]+)\.([0-9]+)\.([0-9]+).*/\1 \2 \3/g') )
    if [[ "${ver[0]}" < "2" ]] || [[ "${ver[1]}" < "3" ]]; then
        echo "Unsupported sdb version:  ${ver[0]}.${ver[1]}.${ver[2]}. Please update sdb to at least 2.3.0"
        exit 1
    fi
    echo "Found sdb version ${ver[0]}.${ver[1]}.${ver[2]}"
}

test_coreclr_devel() {
    coreclr_rpms=$(ls ${SCRIPTS_PATH}/../../coreclr-devel-*.rpm 2>/dev/null)
    if [ -z "$coreclr_rpms" ]; then
        echo "coreclr-devel rpms not found in ${SCRIPTS_PATH}/../.. . Please copy the rpms matching the CoreCLR version on your device there."
        exit 1
    fi

    for coreclr_rpm in $coreclr_rpms; do
        echo "Found $(basename $coreclr_rpm)"
    done
}

test_sdb_version
test_coreclr_devel

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

