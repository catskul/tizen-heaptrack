#!/bin/bash -x

SDB=$1

DEVICE_HEAPTRACK_PATH=$2
DEVICE_ARCH=$3
HEAPTRACK_DATA_DIR=$4
DOCKER_CONTAINER_HASH=$5
SCRIPTS_PATH=$6
RES_FILE=$7
APP_ID=$8
APP_PATH=$9
LAUNCH_GUI=${10}

IS_FOUND_APP=$($SDB shell "app_launcher -l | cut -d \"'\" -f 4 | grep -q '^${APP_ID}$'; echo \$?" | tr -d "[:space:]")
if [ "$IS_FOUND_APP" != "0" ]; then
	echo "'${APP_ID}' application not found at device"
	exit 1
fi

IS_FOUND_APP_PATH=$($SDB shell "ls ${APP_PATH} >& /dev/null; echo \$?" | tr -d "[:space:]")
if [ "$IS_FOUND_APP_PATH" != "0" ]; then
	echo "'${APP_PATH}' application path not found at device"
	exit 1
fi

CORECLR_VERSION=$($SDB shell rpm -qa | grep -E coreclr-[0-9] | sort | tail -n1 | sed -E 's/.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+-[0-9]+\.[0-9]+.*)/\1/g' | tr -d '[:space:]')

rm -rf $HEAPTRACK_DATA_DIR
mkdir -p $HEAPTRACK_DATA_DIR
rm -f $RES_FILE

$SDB shell "mkdir -p $DEVICE_HEAPTRACK_PATH/build/bin;
            mkdir -p $DEVICE_HEAPTRACK_PATH/build/lib/heaptrack
            mkdir -p $DEVICE_HEAPTRACK_PATH/build/lib/heaptrack/libexec" &>/dev/null
docker cp $DOCKER_CONTAINER_HASH:/heaptrack-common/$DEVICE_ARCH $HEAPTRACK_DATA_DIR/$DEVICE_ARCH

if [ ! -d "$HEAPTRACK_DATA_DIR/$DEVICE_ARCH/$CORECLR_VERSION" ]; then
  echo "CoreCLR version on device $CORECLR_VERSION does not match any of the provided coreclr-devel package versions. Please \
update coreclr on device, or put the correct coreclr-devel rpm in coreclr-devel folder."
  exit 1
fi

$SDB push $HEAPTRACK_DATA_DIR/$DEVICE_ARCH/bin/* $DEVICE_HEAPTRACK_PATH/build/bin/ &>/dev/null
$SDB push $HEAPTRACK_DATA_DIR/$DEVICE_ARCH/lib/heaptrack/*.so $DEVICE_HEAPTRACK_PATH/build/lib/heaptrack/ &>/dev/null
$SDB push $HEAPTRACK_DATA_DIR/$DEVICE_ARCH/lib/heaptrack/libexec/* $DEVICE_HEAPTRACK_PATH/build/lib/heaptrack/libexec/ &>/dev/null
$SDB push $HEAPTRACK_DATA_DIR/$DEVICE_ARCH/libprofiler.so $DEVICE_HEAPTRACK_PATH/build/bin/ &>/dev/null
$SDB push $SCRIPTS_PATH/../* $DEVICE_HEAPTRACK_PATH/build/bin/ &>/dev/null
$SDB shell "cd $DEVICE_HEAPTRACK_PATH/build/bin
            ./heaptrack-pid.sh $DEVICE_HEAPTRACK_PATH/build/bin $DEVICE_HEAPTRACK_PATH/build/bin/res.gz ${APP_ID} ${APP_PATH}"

$SDB pull $DEVICE_HEAPTRACK_PATH/build/bin/res.gz $RES_FILE &>/dev/null

if [ ! -f $RES_FILE ]; then
  echo "error collecting the trace, see the log for details"
  exit 1
fi

if [ "$LAUNCH_GUI" == "true" ]; then

  echo "Showing malloc consumption"
  $SCRIPTS_PATH/heaptrack-gui.sh $RES_FILE --malloc
  echo

  echo "Showing managed consumption"
  $SCRIPTS_PATH/heaptrack-gui.sh $RES_FILE --managed --hide-unmanaged-stacks
  echo

  echo "Showing mmap (Private_Dirty part) consumption"
  $SCRIPTS_PATH/heaptrack-gui.sh $RES_FILE --private_dirty
  echo

  echo "Showing mmap (Private_Clean part) consumption"
  $SCRIPTS_PATH/heaptrack-gui.sh $RES_FILE --private_clean
  echo

  echo "Showing mmap (Shared_Clean + Shared_Dirty part) consumption"
  $SCRIPTS_PATH/heaptrack-gui.sh $RES_FILE --shared
  echo
fi
