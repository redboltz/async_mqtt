#!/bin/bash
TAG=latest
if [[ "$#" -ne 0 && "$1" != -* ]] ;
then
    TAG=$1
    shift
fi
echo "tag $TAG is used"
docker pull redboltz/async_mqtt:$TAG
docker run -v $PWD/conf:/var/async_mqtt_build/tool/conf -p 1883:1883 -p 8883:8883 -p80:80 -p 443:443 -it redboltz/async_mqtt:$TAG ../broker $@
