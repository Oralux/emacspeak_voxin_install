PV=44.0
URL=https://github.com/tvraman/emacspeak/releases/download/$PV/emacspeak-$PV.tar.bz2
archivesDir=$PWD/archives
workDir=$PWD/build
installDir=$workDir/install/emacspeak-$PV
patchDir=$PWD/patch
SERVER=outloud
LOG=/tmp/emacspeak_voxin_install.$PV.log
TCLX_AUR=https://aur.archlinux.org/cgit/aur.git/snapshot/tclx.tar.gz


###
check_distro()
{
    local status=1

    # Check if this is an arch linux based distro
    if [ -e "/etc/arch-release" ]; then
	check_emacs=arch_check_emacs
	check_libvoxin=arch_check_libvoxin
	install_dep=arch_install_dep
	status=0
    else
	# Check if this is a debian based distro
	type dpkg &> /dev/null
	if [ "$?" = "0" ]; then
	    check_emacs=debian_check_emacs
	    check_libvoxin=debian_check_libvoxin
	    install_dep=debian_install_dep
	    status=0
	fi
    fi
    return $status
}

# arch linux based commands
arch_check_emacs() {
    (pacman -Q emacs || pacman -Q emacs-nox) &> /dev/null
}

arch_check_libvoxin() {
    pacman -Q libvoxin &> /dev/null    
}

arch_install_dep() {
    pacman --noconfirm -S base-devel wget texinfo alsa-lib alsa-plugins tcl tk &> $LOG
     
    #tclx
    # cd $workDir
    # wget $TCLX_AUR
    # tar -zxf tclx
    # makepkg -s
    # cd ..
    pacman --noconfirm -U $archivesDir/tclx-*-$(uname -m).pkg.tar.xz &>> $LOG
}

# debian based specific commands
debian_check_emacs() {
    (dpkg -l emacs || dpkg -l emacs-nox) &> /dev/null    
}

debian_check_libvoxin() {
    local status=1
    if [ -e /var/opt/IBM/ibmtts/cfg/eci.ini ]; then 
	dpkg -l "libvoxin*" &> /dev/null    
	if [ "$?" != "0" ]; then
	    status=0
	fi
    fi
    return $status
}

debian_install_dep() {
    apt-get update  &> $LOG

    apt-get -y install tcl8.4 tclx8.4 tk8.4 tcl8.4-dev libasound2-dev texinfo &>> $LOG
    case $(uname -m) in
	x86_64|ia64)
	    apt-get -y install g++-multilib libasound2:i386 libasound2-dev:i386 libx11-6:amd64 libx11-6:i386 &>> $LOG
	    dpkg --force-architecture -i ./archives/ia32-tcl8.4*_i386.deb &>> $LOG
	    dpkg --force-architecture -i ./archives/ia32-tclx32-8.4*_i386.deb &>> $LOG
	    dpkg --force-architecture -i ./archives/ia32-tk8.4*_i386.deb &>> $LOG
	    apt-get -y install libasound2-dev:i386 libasound2-plugins:i386 build-essential &>> $LOG
	    pushd /usr/lib32 &>> $LOG
	    ln -sf libtcl8.4.so.0 libtcl8.4.so &>> $LOG
	    ln -sf libtclx32-8.4.so.0 libtclx32-8.4.so &>> $LOG
	    popd
	    SERVER=32-outloud
	    ;;
	*)
	    apt-get -y install libasound2-plugins build-essential &>> $LOG
	    SERVER=outloud
	    ;;
    esac
}
