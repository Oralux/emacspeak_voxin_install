#!/bin/bash
# 2016, Gilles Casse <gcasse@oralux.org>
#

source conf.inc

check_distro

$check_libvoxin
if [ "$?" != "0" ]; then 
    echo "install Voxin >= 1.00 before running this script."
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

for i in $(ls $patchDir/*); do
    patch -p1 < $i
done

echo "building emacspeak... "
make config &>> $LOG
make emacspeak &>> $LOG

if [ -e "$installDir" ]; then
  rm -rf "$installDir"
fi
mkdir -p $installDir
make prefix=$installDir install &>> $LOG

EMACSPEAK_DIR=$installDir/share/emacs/site-lisp/emacspeak/lisp

echo
echo "you may want to copy this line in your .bashrc file"
echo "alias emacspeak=\"DTK_PROGRAM=outloud emacs -q -l $EMACSPEAK_DIR/emacspeak-setup.el -l \$HOME/.emacs\""
echo
echo "then run emacspeak in a new shell terminal by typing emacspeak and press RETURN"
echo 
