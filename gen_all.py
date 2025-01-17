import glob
import os
import re

files = [p for p in glob.glob('**/*', root_dir='include', recursive=True)
       if re.search('\.(h|hpp)$', p)]

print(
    r'''// Copyright Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_ALL_HPP)
#define ASYNC_MQTT_ALL_HPP
'''
)

for file in sorted(files, key=lambda file: (file.count('/'), file)):
    if file.find("/all.hpp") != -1:
        continue
    if file.find("/separate/") != -1:
        continue
    if file.find("/broker/") != -1:
        continue
    if file.find("/impl/") != -1:
        continue
    if file.find("/detail/") != -1:
        continue
    if file.find("/predefined_layer/") != -1 and file.find("/predefined_layer/mqtt.hpp") == -1:
        continue
    if file.find("/picosha2.h") != -1:
        continue
    print("#include <{}>".format(file))
print(
    r'''
#endif // ASYNC_MQTT_ALL_HPP'''
)
