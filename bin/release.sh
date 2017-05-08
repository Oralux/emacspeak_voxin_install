#!/bin/bash -vx


source conf.inc

REL=1
BASE="$(cd "$(dirname "$0")" && pwd)"
workDir=$(mktemp -d)
DEST=$workDir/$PN-$PV-$REL
SRC=..

mkdir $DEST
tar --exclude=.git --exclude=build --exclude="*~" -C .. -cf - . | tar -C $DEST -xf -
tar -C $workDir --owner=root --group=root -zcf $DEST.tgz $PN-$PV-$REL
rm -rf $DEST
echo $DEST.tgz

