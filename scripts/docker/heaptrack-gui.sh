#!/bin/bash -x

XSOCK=/tmp/.X11-unix
XAUTH=/tmp/.docker.xauth
RES_FILE=$1
touch $XAUTH
xauth nlist $DISPLAY | sed -e 's/^..../ffff/' | xauth -f $XAUTH nmerge -

docker run -it \
    --volume=$XSOCK:$XSOCK:rw \
    --volume=$XAUTH:$XAUTH:rw \
    --volume=$RES_FILE:/res.gz:ro \
    --env="XAUTHORITY=${XAUTH}" \
    --env="DISPLAY" \
    --user="docker-user" \
    heaptrack:latest \
    /heaptrack/build-x64/bin/heaptrack_gui ${@:2:2} /res.gz 2>gui_malloc.srderr
