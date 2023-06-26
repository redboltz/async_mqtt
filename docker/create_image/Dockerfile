FROM alpine:latest

RUN mkdir -p /var/async_mqtt
WORKDIR /var
ARG ASYNC_MQTT_VERSION

RUN apk update \
 && apk add --no-cache \
    gcc \
    libc-dev \
    clang16 \
    boost-dev \
    libssl3 \
    libcrypto3 \
 && apk add --no-cache --virtual .build-deps \
    git \
    openssl-dev \
    cmake \
    make \
 && git clone https://github.com/redboltz/async_mqtt.git -b ${ASYNC_MQTT_VERSION} \
    && mkdir async_mqtt_build \
    && cd async_mqtt_build \
    && cmake \
       -DCMAKE_CXX_COMPILER=clang++ \
       -DCMAKE_BUILD_TYPE=Release \
       -DCMAKE_CXX_FLAGS="-std=c++17 -O3" \
       -DASYNC_MQTT_USE_TLS=ON \
       -DASYNC_MQTT_USE_WS=ON \
       -DASYNC_MQTT_USE_STR_CHECK=OFF \
       -DASYNC_MQTT_USE_LOG=ON \
       -DASYNC_MQTT_BUILD_TOOLS=ON \
       /var/async_mqtt \
    && cd tool \
    && cmake --build . --parallel 3 \
    && cd .. \
    && rm -rf `find . -name "*.o"` \
    && apk del .build-deps

WORKDIR /var/async_mqtt_build/tool/conf
