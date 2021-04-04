#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
pushd $DIR

source setup.sh
lldb install/server -- --dev --root install/web

popd
