rm -rf build/site/async_mqtt/example
mkdir -p build/site/async_mqtt/example
mkdir -p build/site/async_mqtt/example/separate_client
mkdir -p build/site/async_mqtt/example/separate_endpoint

cp -f ../example/*.cpp build/site/async_mqtt/example/
cp -f ../example/separate_client/*.cpp   build/site/async_mqtt/example/separate_client/
cp -f ../example/separate_client/CMakeLists.txt   build/site/async_mqtt/example/separate_client/
cp -f ../example/separate_endpoint/*.cpp build/site/async_mqtt/example/separate_endpoint/
cp -f ../example/separate_endpoint/CMakeLists.txt build/site/async_mqtt/example/separate_endpoint/
