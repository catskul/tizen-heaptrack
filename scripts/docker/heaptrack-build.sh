#!/bin/bash

DEVICE_ARCH=$1
SCRIPTS_PATH=$(dirname $BASH_SOURCE)

if [ ! -f "$SDB" ]; then
    source "$SCRIPTS_PATH/../common.sh"
fi

test_coreclr_devel() {
    coreclr_rpms=$(ls ${SCRIPTS_PATH}/../../coreclr-devel/coreclr-devel-*.rpm 2>/dev/null)
    if [ -z "$coreclr_rpms" ]; then
        echo "coreclr-devel rpms not found in ${SCRIPTS_PATH}/../../coreclr-devel ."
        read_consent "Do you want to download the latest coreclr-devel package?" DOWNLOAD_CORECLR_DEVEL
        if $DOWNLOAD_CORECLR_DEVEL; then
            coreclr_source="rpm"
            return
        fi

        read_dir "Enter the source location of a compiled coreclr repository. The binaries in the repository \
must match those on the device. [] " CORECLR_SRC_DIR

        read_one_of "Enter the CoreCLR build type" "Release Debug Checked" coreclr_build_type
        export CORECLR_BIN_DIR="$CORECLR_SRC_DIR/bin/Product/Linux.$DEVICE_ARCH.$coreclr_build_type"
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

test_coreclr_devel
build_container