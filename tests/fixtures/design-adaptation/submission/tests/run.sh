#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p out/tests
cc -std=c99 -Wall -Wextra -Werror -pedantic -Iinc src/desk.c tests/test_desk.c -o out/tests/test_desk
out/tests/test_desk
