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
    pacman -Q emacs &> /dev/null    
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
    dpkg -l emacs &> /dev/null    
}

debian_check_libvoxin() {
    dpkg -l libvoxin &> /dev/null    
}

debian_install_dep() {
    apt-get update  &> $LOG
    apt-get -y install build-essential libasound2-plugins libasound2-dev texinfo tcl8.6-dev tclx8.4 &>> $LOG
}
