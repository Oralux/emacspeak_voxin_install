#!/bin/bash

BASE="$(cd "$(dirname "$0")" && pwd)"

source $BASE/bin/conf.inc

if [ "$(id -u)" != "0" ]; then
    echo "please run this script as root."
    exit 0
fi  

cd $BASE

unset DTK
$check_libvoxin && DTK='(setenv "DTK_PROGRAM" "outloud")'

rm -rf build log

echo "
Remove please the following lines from your emacs init file (e.g. in  ~/.emacs ):
$DTK
(load-file \"$PWD/lisp/emacspeak-setup.el\")
"
