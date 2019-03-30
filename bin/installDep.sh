#!/bin/bash

cd "$(dirname "$0")"
LOG=../log/installDep.$(date -Iseconds)
DEP=../build/dep.txt
archivesDir=../archives

[ ! -f "$DEP" ] && "Nothing to do" && exit 0
[ "$UID" != "0" ] && echo "Sorry, run this script as superuser. You may want to read this script before" && exit 0

rm -f ../log/installDep.*

archUpdateDistro() {
    pacman -Syu
    for i in $(cat "$DEP"); do
	pacman -S --noconfirm $i | tee -a "$LOG"
    done    
    pacman --noconfirm -U $archivesDir/tclx-*-$(uname -m).pkg.tar.xz  | tee -a "$LOG"
}

debianUpdateDistro() {
    DEBIAN_FRONTEND=noninteractive apt-get update | tee -a "$LOG"    
    for i in $(cat "$DEP"); do
	DEBIAN_FRONTEND=noninteractive apt-get -y install $i | tee -a "$LOG"
    done
}

checkDistro()
{
    local status=1
    unset updateDistro
    
    # Check if this is an arch linux based distro
    if [ -e "/etc/pacman.conf" ]; then
	updateDistro=archUpdateDistro
	status=0
    else
	# Check if this is a debian based distro
	type dpkg &> /dev/null
	if [ "$?" = "0" ]; then
	    updateDistro=debianUpdateDistro
	    status=0
	    # TODO Arch??
	    #		https://wiki.archlinux.org/index.php/Pacman/Tips_and_tricks#Getting_the_dependencies_list_of_several_packages	    
	fi
    fi
    return $status
}


checkDistro || leave "Sorry, this distribution is not yet supported. For support, email to contact at oralux.org " 1

echo "Updating packages. Please wait...
Log file: $LOG
"
$updateDistro

echo "Update completed.
You may now want to run as normal user install.sh to install emacspeak" | tee -a "$LOG"


