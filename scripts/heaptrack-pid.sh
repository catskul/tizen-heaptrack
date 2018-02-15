#!/bin/bash -x

HEAPTRACK_DIR=$1
RES_FILE=$2
APP_ID=$3
APP_PATH=$4

CUR_DIR=`pwd`
cd $HEAPTRACK_DIR

rm -f $RES_FILE
rm -f heaptrack*.gz

export PATH=${PATH}:${HEAPTRACK_DIR}

export ELM_ENGINE=wayland_shm
export ECORE_EVAS_ENGINE=wayland_shm
export HOME=/opt/usr/home/owner
export XDG_RUNTIME_DIR=/run/user/5001
export DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/5001/dbus/user_bus_socket
export AUL_APPID=${APP_ID}
export CORECLR_PROFILER={C7BAD323-25F0-4C0B-B354-566390B215CA}
export CORECLR_PROFILER_PATH=${HEAPTRACK_DIR}/libprofiler.so
export CORECLR_ENABLE_PROFILING=1
export COMPlus_AltJit=*
export COMPlus_AltJitName='liblegacyjit.so'

nohup ./heaptrack /usr/bin/dotnet-launcher --standalone ${APP_PATH} &

sleep 1

PID=$(ps -o pid,cmd ax | grep dotnet-launcher | grep standalone | grep ${APP_PATH} | grep -v -e grep -e heaptrack | awk '{ print $1 }')

echo "Application PID: $PID. Press 'c' to stop and save results"

while true
do
  read -rsn1 input
  if [ "$input" = "c" ]; then
    echo "saving result"
    break
  fi
done

kill -15 $PID

for i in `seq 1 1 60`; do
  sleep 1;

  kill -0 $PID 2>/dev/null || break
  sleep 1
done

kill -0 $PID 2>/dev/null && kill -9 $PID 2>/dev/null

sleep 5

mv heaptrack*.gz $RES_FILE

cd $CUR_DIR
