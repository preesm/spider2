#!/bin/bash -eu

[ "$#" -ne "1" ] && echo "usage: $0 <new version>" && exit 1

DIR=$(cd `dirname $0` && echo `git rev-parse --show-toplevel`)

echo $1 > ${DIR}/VERSION
