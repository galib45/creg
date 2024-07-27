#! /bin/bash
set -x
if [ -f "build/tree.dot" ]; then rm build/tree.dot; fi
if [ -f "build/tree.png" ]; then rm build/tree.png; fi
./build/main $1 build/tree.dot
set -e
dot -Tpng build/tree.dot -o build/tree.png
qview build/tree.png
