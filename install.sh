#!/bin/bash
# 2016-2018, Gilles Casse <gcasse@oralux.org>
#

BASE="$(cd "$(dirname "$0")" && pwd)"
source $BASE/bin/conf.inc

mkdir -p $workDir $logDir

check_distro

voxin_found=0
espeak_found=0
unset DTK

echo -n "Checking eSpeak : "
$check_espeak
if [ "$?" = "0" ]; then
    espeak_found=1
    echo "yes"
else    
    echo "no"
fi

echo -n "Checking Voxin >= 1.00 : "
$check_libvoxin
if [ "$?" = "0" ]; then
    voxin_found=1
    DTK="DTK_PROGRAM=outloud"
    echo "yes"
else    
    echo "no"
fi

if [ "$voxin_found" = "0" ] && [ "$espeak_found" = "0" ]; then
    echo "install voxin or espeak before running this script."
    exit 0        
fi

if [ "$(id -u)" != "0" ]; then
    echo "please run this script as root."
    exit 0
fi  

$check_emacs
if [ "$?" != "0" ]; then
    echo "install emacs before running this script."
    exit 0    
fi

trap error_msg ERR

echo "Initialization; please wait... "
$install_dep

echo "Downloading emacspeak $PV... "
mkdir -p $workDir
cd $workDir
if [ ! -e "emacspeak-${PV}.tar.bz2" ]; then
    wget $URL &>> $LOG
fi

if [ -e "emacspeak-${PV}" ]; then
    rm -rf emacspeak-${PV}
fi

tar --no-same-owner -jxf emacspeak-${PV}.tar.bz2
cd emacspeak-${PV}

echo "Building emacspeak... "
make config &>> $LOG
make &>> $LOG
if [ "$voxin_found" = "1" ]; then
    make outloud &>> $LOG
fi
if [ "$espeak_found" = "1" ]; then
    make espeak &>> $LOG
fi

chmod -R ugo+rX .

echo "
To run this Emacspeak build, add the following lines to the top of your emacs init file (e.g. in  ~/.emacs ); then, start emacs
" | tee -a $LOG

if [ -n "$DTK" ]; then
	echo '(setenv "DTK_PROGRAM" "outloud")' | tee -a $LOG
fi
echo "(load-file \"$PWD/lisp/emacspeak-setup.el\")
" | tee -a $LOG

echo "These instructions can be retrieved at the end of the log file: $LOG"
