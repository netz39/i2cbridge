#!/bin/sh

autoreconf -vi || exit 1

./configure "$@"
