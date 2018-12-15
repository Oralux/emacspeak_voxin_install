#!/bin/bash

cd "$(dirname "$0")"
LOG=../log/installDep.$(date -Iseconds)
DEP=../build/dep.txt

[ ! -f "$DEP" ] && "Nothing to do" && exit 0
[ "$UID" != "0" ] && echo "Sorry, run this script as superuser. You may want to read this script before" && exit 0

echo "Updating system update. Please wait...
Log file: $LOG
"

apt-get update &>> "$LOG"

for i in $(cat "$DEP"); do
    apt-get -y install $i &>> "$LOG"
done

echo "Update completed.
You may now want to run as normal user install.sh to install emacspeak" | tee -a "$LOG"

