#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
pushd $DIR

source setup.sh
lldb install/server -- --root install/web --cert $SERVER_CERT --key $SERVER_KEY

popd
