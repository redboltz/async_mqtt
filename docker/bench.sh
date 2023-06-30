#!/bin/bash
TAG=latest
if [[ "$#" -ne 0 && "$1" != -* ]] ;
then
    TAG=$1
    shift
fi
echo "tag $TAG is used"
docker pull redboltz/async_mqtt:$TAG
docker run -v $PWD/conf:/var/async_mqtt_build/tool/conf --add-host=host.docker.internal:host-gateway -it redboltz/async_mqtt:$TAG ../bench $@
