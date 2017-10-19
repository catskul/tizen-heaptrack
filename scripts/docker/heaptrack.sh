#!/bin/bash

SDB=$(which sdb)
DEVICE_HEAPTRACK_PATH=/opt/heaptrack
DEVICE_ARCH=armel
HEAPTRACK_DATA_DIR=~/.heaptrack
SCRIPTS_PATH=$(dirname $BASH_SOURCE)
RES_FILE=$HEAPTRACK_DATA_DIR/res.gz
APP_ID=$1
APP_PATH=$2
DOWNLOAD_CORECLR_DEVEL=""
CORECLR_SRC_DIR=""
CORECLR_BIN_DIR=""

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
    coreclr_rpms=$(ls ${SCRIPTS_PATH}/../../coreclr-devel/coreclr-devel-*.rpm 2>/dev/null)
    if [ -z "$coreclr_rpms" ]; then
        echo "coreclr-devel rpms not found in ${SCRIPTS_PATH}/../../coreclr-devel ."
        while [ "$DOWNLOAD_CORECLR_DEVEL" != "y" ] && [ "$DOWNLOAD_CORECLR_DEVEL" != "n" ]; do
            read -p "Do you want to download the latest coreclr-devel package? [Y/n] " DOWNLOAD_CORECLR_DEVEL
            if [ -z "$DOWNLOAD_CORECLR_DEVEL" ]; then
                DOWNLOAD_CORECLR_DEVEL="y"
            fi
            DOWNLOAD_CORECLR_DEVEL=$(echo $DOWNLOAD_CORECLR_DEVEL | awk '{ print(tolower($0)) }')
        done

        if [ "$DOWNLOAD_CORECLR_DEVEL" == "y" ]; then
            coreclr_source="rpm"
            return
        fi

        read -p "Enter the source location of a compiled coreclr repository. The binaries in the repository \
must match those on the device. [] " CORECLR_SRC_DIR
        if [ ! -d "$CORECLR_SRC_DIR" ] ||  [ ! -d "$CORECLR_SRC_DIR/src" ]; then
            echo "Can't find CoreCLR source in '$CORECLR_SRC_DIR'"
            exit 1
        fi

        coreclr_build_type=""
        read -p "Enter the CoreCLR build type [Release/Debug/Checked] " coreclr_build_type
        CORECLR_BIN_DIR="$CORECLR_SRC_DIR/bin/Product/Linux.$DEVICE_ARCH.$coreclr_build_type"
        if [ ! -d "$CORECLR_BIN_DIR" ]; then
            echo "Can't find CoreCLR binaries in '$CORECLR_BIN_DIR'"
            exit 1
        fi
        coreclr_source="manual"
    else
        for coreclr_rpm in $coreclr_rpms; do
            echo "Found $(basename $coreclr_rpm)"
        done
        coreclr_source="rpm"
    fi
}

build_container() {
    temp_dir="$SCRIPTS_PATH/../../.coreclr"
    if [ "$coreclr_source" == "manual" ]; then
        base_dir="$(basename $CORECLR_BIN_DIR)"
        mkdir -p $temp_dir/bin/Product/$base_dir/lib
        mkdir -p $temp_dir/src
        rsync -a --exclude=".nuget" --exclude="test*" $CORECLR_SRC_DIR/src/ $temp_dir/src
        rsync -a $CORECLR_BIN_DIR/lib/* \
                  $temp_dir/bin/Product/$base_dir/lib
        rsync -a $CORECLR_BIN_DIR/inc \
                  $temp_dir/bin/Product/$base_dir/inc

    fi

    docker build --build-arg CORECLR_SOURCE="$coreclr_source" \
                 --build-arg BUILD_TYPE="$coreclr_build_type" \
                 --build-arg ARCH="$DEVICE_ARCH" \
                 -t heaptrack:latest $SCRIPTS_PATH/../..
    if [ ! "$?" -eq "0" ]; then
        echo "build errors; see the log for details"
        exit 1
    fi
}

test_sdb_version
test_coreclr_devel
build_container

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

