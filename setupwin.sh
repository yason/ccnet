#! /bin/bash

set -e

TARGET_DIR=/c/pack
if [[ $# != 1 ]] ; then
    echo
    echo "Usage: ./setupwin.sh <target_dir>"
    exit 1;
else
    TARGET_DIR=$1
fi

if [[ ! -d ${TARGET_DIR} ]]; then
    echo 
    echo "${TARGET_DIR} is not a valid directory"
    exit 1;
fi

echo "target directory is" ${TARGET_DIR}

BIN_DIR=${TARGET_DIR}/bin

mkdir -p "${BIN_DIR}"

cp net/daemon/.libs/ccnet.exe "${BIN_DIR}"
cp lib/.libs/libccnet-0.dll "${BIN_DIR}"

