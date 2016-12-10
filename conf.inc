PV=45.0
URL=https://github.com/tvraman/emacspeak/releases/download/$PV/emacspeak-$PV.tar.bz2
archivesDir=$PWD/archives
workDir=$PWD/build
installDir=$workDir/install/emacspeak-$PV
patchDir=$PWD/patch
SERVER=outloud
LOG=$workDir/emacspeak_voxin_install.$PV.log
TCLX_AUR=https://aur.archlinux.org/cgit/aur.git/snapshot/tclx.tar.gz

mkdir -p $workDir

###
check_distro()
{
    local status=1

    # Check if this is an arch linux based distro
    if [ -e "/etc/pacman.conf" ]; then
	check_emacs=arch_check_emacs
	check_libvoxin=arch_check_libvoxin
	check_espeak=arch_check_espeak
	install_dep=arch_install_dep
	status=0
    else
	# Check if this is a debian based distro
	type dpkg &> /dev/null
	if [ "$?" = "0" ]; then
	    check_emacs=debian_check_emacs
	    check_libvoxin=debian_check_libvoxin
	    check_espeak=debian_check_espeak
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

arch_check_espeak() {
    pacman -Q espeak &> /dev/null    
}

arch_install_dep() {
    pacman -Syu
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
    dpkg -l "libvoxin*" &> /dev/null    
}

debian_check_espeak() {
    dpkg -l "espeak*" &> /dev/null    
}

debian_install_dep() {
    apt-get update  &> $LOG
    apt-get -y install build-essential libasound2-plugins libasound2-dev texinfo tcl8.6-dev tclx8.4 &>> $LOG
}
