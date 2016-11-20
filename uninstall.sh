#!/bin/bash

if [ "$(id -u)" != "0" ]; then
    echo "Please run this installer as root."
    exit 0
fi

case $(uname -m) in
    x86_64|ia64)
		for i in ia32-tcl8.4 ia32-tk8.4 ia32-tclx32-8.4; do
			apt-get remove --purge -y $i
		done
		rm /usr/lib32/libtcl8.4.so /usr/lib32/libtclx32-8.4.so
		;;
    *)
		;;
esac
