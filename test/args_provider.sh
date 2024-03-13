#!/bin/sh

echo $1 $(echo ${CTEST_ARGS})
$1 $(echo ${CTEST_ARGS})
