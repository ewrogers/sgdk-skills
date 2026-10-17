#!/bin/sh
# Native host architecture, SGDK toolchain stays inside Docker.
set -eu
cd "$(dirname "$0")"
docker run --rm -v "$PWD:/src" -w /src ewrogers/sgdk:latest make -f /opt/sgdk/makefile.gen release
