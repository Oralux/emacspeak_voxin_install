#!/bin/bash

cd "$(dirname "$0")"
LOG=../log/installDep.$(date -Iseconds)
DEP=../build/dep.txt

[ ! -f "$DEP" ] && "Nothing to do" && exit 0
[ "$UID" != "0" ] && echo "Sorry, run this script as superuser. You may want to read this script before" && exit 0

rm -f ../log/installDep.*

echo "Updating system. Please wait...
Log file: $LOG
"

apt-get update | tee -a "$LOG"

for i in $(cat "$DEP"); do
    apt-get -y install $i | tee -a "$LOG"
done

echo "Update completed.
You may now want to run as normal user install.sh to install emacspeak" | tee -a "$LOG"

