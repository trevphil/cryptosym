#!/bin/bash

find . -iname *.hpp -o -iname *.cpp | xargs clang-format -i -style=file
