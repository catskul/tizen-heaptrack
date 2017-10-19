#!/bin/bash

ARTIFACT_DIR=$1

TIZEN_PACKAGES_PATTERN="-[0-9]+(\.[0-9]+)+-[0-9]+(\.[0-9]+)+\.armv7l\.rpm"
PROFILER_SRC_DIR="$(pwd)/../../profiler/profiler"
CORECLR_DEVEL_DIR="$(pwd)/../../coreclr-devel"

CORECLR_DEVEL_RPMS="$(ls $CORECLR_DEVEL_DIR/coreclr-devel-*.armv7l.rpm 2>/dev/null)"

mkdir -p $CORECLR_DEVEL_DIR

save_artifacts() {
    mkdir -p $ARTIFACT_DIR/$coreclr_version
    mv $PROFILER_SRC_DIR/build/src/libprofiler.so $ARTIFACT_DIR/$coreclr_version
}

cd $PROFILER_SRC_DIR

if [ "$CORECLR_SOURCE" == "manual" ]; then
    coreclr_version="unknown"
    ROOTFS_DIR=/rootfs CORECLR_SRC_DIR="/heaptrack/.coreclr" CORECLR_BIN_DIR="/heaptrack/.coreclr/bin/Product/Linux.$ARCH.$BUILD_TYPE" ARCH="$ARCH" ./build.sh
    save_artifacts
else
    if [ -z "$CORECLR_DEVEL_RPMS" ]; then
        echo Downloading coreclr-devel...
        wget -q -r -np -l 1 --accept-regex coreclr-devel$TIZEN_PACKAGES_PATTERN $REPO_UNIFIED/armv7l/
        find . -name "*.rpm" -exec mv {} $CORECLR_DEVEL_DIR \;
    fi

    CORECLR_DEVEL_RPMS="$(ls $CORECLR_DEVEL_DIR/coreclr-devel-*.armv7l.rpm 2>/dev/null)"

    for rpm in $CORECLR_DEVEL_RPMS; do
        coreclr_version=$(echo $rpm | sed -E 's/.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+-[0-9]+\.[0-9]+.*)\.rpm/\1/g')
        CORECLR_BIN_DIR= CORECLR_SRC_DIR= ROOTFS_DIR=/rootfs ./build.sh $rpm
        save_artifacts
    done
fi