#!/bin/bash -x

SDB=$1

DEVICE_HEAPTRACK_PATH=$2

HEAPTRACK_FOR_DEVICE_HOST_PATH=$3
HEAPTRACK_FOR_HOST_PATH=$4
SCRIPTS_PATH=$5
RES_FILE=$6
APP_ID=$7
APP_PATH=$8
LAUNCH_GUI=$9

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

rm -f $RES_FILE

$SDB shell "mkdir -p $DEVICE_HEAPTRACK_PATH/build/bin;
            mkdir -p $DEVICE_HEAPTRACK_PATH/build/lib/heaptrack
            mkdir -p $DEVICE_HEAPTRACK_PATH/build/lib/heaptrack/libexec" &>/dev/null
$SDB push $HEAPTRACK_FOR_DEVICE_HOST_PATH/build/bin/* $DEVICE_HEAPTRACK_PATH/build/bin/ &>/dev/null
$SDB push $HEAPTRACK_FOR_DEVICE_HOST_PATH/build/lib/heaptrack/*.so $DEVICE_HEAPTRACK_PATH/build/lib/heaptrack/ &>/dev/null
$SDB push $HEAPTRACK_FOR_DEVICE_HOST_PATH/build/lib/heaptrack/libexec/* $DEVICE_HEAPTRACK_PATH/build/lib/heaptrack/libexec/ &>/dev/null
$SDB push $HEAPTRACK_FOR_DEVICE_HOST_PATH/build/libprofiler.so $DEVICE_HEAPTRACK_PATH/build/bin/ &>/dev/null
$SDB push $SCRIPTS_PATH/* $DEVICE_HEAPTRACK_PATH/build/bin/ &>/dev/null
$SDB shell "cd $DEVICE_HEAPTRACK_PATH/build/bin
            ./heaptrack-pid.sh $DEVICE_HEAPTRACK_PATH/build/bin $DEVICE_HEAPTRACK_PATH/build/bin/res.gz ${APP_ID} ${APP_PATH}"

$SDB pull $DEVICE_HEAPTRACK_PATH/build/bin/res.gz $RES_FILE &>/dev/null

if [ "$LAUNCH_GUI" == "true" ]; then
  echo "Showing malloc consumption"
  $HEAPTRACK_FOR_HOST_PATH/build/bin/heaptrack_gui --malloc $RES_FILE 2>gui_malloc.stderr
  echo

  echo "Showing managed consumption"
  $HEAPTRACK_FOR_HOST_PATH/build/bin/heaptrack_gui --managed --hide-unmanaged-stacks $RES_FILE 2>gui_managed.stderr
  echo

  echo "Showing mmap (Private_Dirty part) consumption"
  $HEAPTRACK_FOR_HOST_PATH/build/bin/heaptrack_gui --private_dirty $RES_FILE 2>gui_private_dirty.stderr
  echo

  echo "Showing mmap (Private_Clean part) consumption"
  $HEAPTRACK_FOR_HOST_PATH/build/bin/heaptrack_gui --private_clean $RES_FILE 2>gui_private_clean.stderr
  echo

  echo "Showing mmap (Shared_Clean + Shared_Dirty part) consumption"
  $HEAPTRACK_FOR_HOST_PATH/build/bin/heaptrack_gui --shared $RES_FILE 2>gui_shared.stderr
  echo
fi
