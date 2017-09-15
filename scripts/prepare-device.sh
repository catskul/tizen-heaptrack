#!/bin/bash -x

SDB=/home/ubuntu/tizen-studio/tools/sdb
RPMS_DIR=/home/ubuntu/device-rpms
SCRIPTS_PATH=/home/ubuntu/heaptrack-scripts

$SDB root on

if [ "$($SDB shell 'ls -d1 /opt/usr/rpms 2> /dev/null')" != "" ]; then
  echo "The script already completed for the device. Please, don't use it second time until reset of the device to initial state"

  exit 1
fi

$SDB shell "mkdir /opt/usr/rpms"
$SDB push $RPMS_DIR/* /opt/usr/rpms

$SDB push $SCRIPTS_PATH/prepare-device-internal.sh /opt/usr/rpms
$SDB shell "/opt/usr/rpms/prepare-device-internal.sh"

$SDB shell "rm /usr/lib/debug/lib/libc-2.24.so.debug"

$SDB shell "cd /opt/usr/rpms; rpm -i *.rpm;"
