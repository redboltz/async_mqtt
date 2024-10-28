rm -rf build/site/async_mqtt/example
mkdir -p build/site/async_mqtt/example
mkdir -p build/site/async_mqtt/example/separate_client
mkdir -p build/site/async_mqtt/example/separate_client_manual
mkdir -p build/site/async_mqtt/example/separate_endpoint
mkdir -p build/site/async_mqtt/example/separate_endpoint_manual
mkdir -p build/site/async_mqtt/example/separate_protocol
mkdir -p build/site/async_mqtt/example/separate_protocol_manual

cp -f ../example/*.cpp build/site/async_mqtt/example/
cp -f ../example/separate_client/*.cpp   build/site/async_mqtt/example/separate_client/
cp -f ../example/separate_client/CMakeLists.txt   build/site/async_mqtt/example/separate_client/
cp -f ../example/separate_client_manual/*.cpp   build/site/async_mqtt/example/separate_client_manual/
cp -f ../example/separate_client_manual/CMakeLists.txt   build/site/async_mqtt/example/separate_client_manual/
cp -f ../example/separate_endpoint/*.cpp build/site/async_mqtt/example/separate_endpoint/
cp -f ../example/separate_endpoint/CMakeLists.txt build/site/async_mqtt/example/separate_endpoint/
cp -f ../example/separate_endpoint_manual/*.cpp build/site/async_mqtt/example/separate_endpoint_manual/
cp -f ../example/separate_endpoint_manual/CMakeLists.txt build/site/async_mqtt/example/separate_endpoint_manual/
cp -f ../example/separate_protocol/*.cpp build/site/async_mqtt/example/separate_protocol/
cp -f ../example/separate_protocol/CMakeLists.txt build/site/async_mqtt/example/separate_protocol/
cp -f ../example/separate_protocol_manual/*.cpp build/site/async_mqtt/example/separate_protocol_manual/
cp -f ../example/separate_protocol_manual/CMakeLists.txt build/site/async_mqtt/example/separate_protocol_manual/
