#!/bin/bash
# 2016, Gilles Casse <gcasse@oralux.org>
#

source conf.inc

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

set -e
trap "echo error: for more details check $LOG " ERR

echo "initialization; please wait... "
$install_dep

echo "downloading emacspeak $PV... "
mkdir -p $workDir
cd $workDir
if [ ! -e "emacspeak-${PV}.tar.bz2" ]; then
    wget $URL &>> $LOG
fi

if [ -e "emacspeak-${PV}" ]; then
    rm -rf emacspeak-${PV}
fi

tar -jxf emacspeak-${PV}.tar.bz2
cd emacspeak-${PV}

for i in $(ls $patchDir/*.patch); do
    patch -p1 < $i
done

echo "building emacspeak... "
make config &>> $LOG
if [ "$voxin_found" = "1" ]; then
    make outloud &>> $LOG
fi
if [ "$espeak_found" = "1" ]; then
    make espeak &>> $LOG
fi
make emacspeak &>> $LOG

if [ -e "$installDir" ]; then
  rm -rf "$installDir"
fi
mkdir -p $installDir
make prefix=$installDir install &>> $LOG

EMACSPEAK_DIR=$installDir/share/emacs/site-lisp/emacspeak/lisp

echo
echo "you may want to copy this line in your .bashrc file"
echo "alias emacspeak=\"$DTK emacs -q -l $EMACSPEAK_DIR/emacspeak-setup.el -l \$HOME/.emacs\""
echo
echo "then run emacspeak in a new shell terminal by typing emacspeak and press RETURN"
echo 
