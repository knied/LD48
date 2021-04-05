#!/bin/bash
cd $1; shift
program=$1; shift
exec $program "$@"
