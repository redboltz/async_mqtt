#!/bin/sh
rm -rf .tmp_doc
mkdir .tmp_doc
cd .tmp_doc
git clone --branch doc https://github.com/redboltz/async_mqtt.git
cp ../CHANGELOG.adoc async_mqtt
bundle exec asciidoxy --base-dir async_mqtt async_mqtt/index.adoc --destination-dir ../doc --multipage -r asciidoctor-diagram --image-dir async_mqtt/img

rm -rf ../doc/api
cmake -DASYNC_MQTT_USE_TLS=ON -DASYNC_MQTT_USE_WS=ON ..
make doxygen
mv doc/html ../doc/api
cd ..
rm -rf .tmp_doc
