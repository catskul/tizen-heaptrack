#!/bin/bash

SCRIPTS_PATH=$(dirname $BASH_SOURCE)

if [ ! -f "$SDB" ]; then
  source $SCRIPTS_PATH/common.sh
  test_sdb_version
  if [ $? != 0 ]; then
    exit 1
  fi
fi

if [ "$($SDB shell 'ls -d1 /opt/usr/rpms 2> /dev/null')" != "" ]; then
  echo "Device is already prepared to run the profiler."
  exit 0
fi

echo "This step will move things around on the device in order to get additional space"
echo "required to run the profiler. What will be moved:"
echo " - /usr/share/dotnet -> /opt/usr/dotnet"
echo " - /usr/share/dotnet-tizen -> /opt/usr/dotnet-tizen"
echo " - /usr/lib/debug -> /opt/usr/lib/debug"
echo " - /usr/src/debug -> /opt/usr/src/debug"
echo "Symlinks to new locations will be created in old locations."

read_consent "Do you want to proceed?" consent
if ! $consent; then
   echo "Can not proceed without preparing the device"
   exit 1
fi

read_dir "Please enter the location of debug RPMs [] " RPMS_DIR

$SDB root on

$SDB shell "mkdir /opt/usr/rpms"
$SDB push $RPMS_DIR/* /opt/usr/rpms

$SDB push $SCRIPTS_PATH/prepare-device-internal.sh /opt/usr/rpms
$SDB shell "/opt/usr/rpms/prepare-device-internal.sh"

$SDB shell "rm /usr/lib/debug/lib/libc-2.24.so.debug"

$SDB shell "cd /opt/usr/rpms; rpm -i *.rpm;"
