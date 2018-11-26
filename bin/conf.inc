PN=emacspeak_voxin_install

# PV: default emacspeak version 
PV=49.0
GIT_EMACSPEAK_URL=https://github.com/tvraman/emacspeak.git
GIT_EMACS_URL=https://github.com/mirrors/emacs.git
archivesDir=$BASE/archives
workDir=$BASE/build
#installDir=$workDir/install/emacspeak-$PV
installDir=$workDir/install
logDir=$BASE/log
SERVER=outloud
TCLX_AUR=https://aur.archlinux.org/cgit/aur.git/snapshot/tclx.tar.gz
LOG=$logDir/install.$(date -Iseconds)

###
gitDownloadDevelopperVersion() {
	# download only the last commit to reduce size
	local dir=$1
	local url=$2	
	git -C "$dir" clone --depth 1 "$url" &>> "$LOG"
}

gitUpdateLocalCopy() {
	local dir=$1
	[ ! -d "$dir" ] || git -C "$dir" pull &>> "$LOG"
}

gitCleanLocalCopy() {
	local dir=$1
	[ ! -d "$dir" ] || git -C "$dir" clean -xdf &>> "$LOG"
}

checkDistro()
{
    local status=1

	# TODO ArchLinux support
    # # Check if this is an arch linux based distro
    # if [ -e "/etc/pacman.conf" ]; then
	# checkEmacs=archCheckEmacs
	# checkLibvoxin=archCheckLibvoxin
	# checkEspeak=archCheckEspeak
	# installDep=archInstallDep
	# status=0
    # else
	# Check if this is a debian based distro
	type dpkg &> /dev/null
	if [ "$?" = "0" ]; then
	    checkEmacs=debianCheckEmacs
	    checkLibvoxin=debianCheckLibvoxin
	    checkEspeak=debianCheckEspeak
	    installDep=debianInstallDep
	    status=0

		# TODO Arch??
		#		https://wiki.archlinux.org/index.php/Pacman/Tips_and_tricks#Getting_the_dependencies_list_of_several_packages

	fi
    # fi
    return $status
}

# arch linux based commands
archCheckEmacs() {
    (pacman -Q emacs || pacman -Q emacs-nox) &> /dev/null
}

archCheckLibvoxin() {
    pacman -Q libvoxin &> /dev/null    
}

archCheckEspeak() {
    pacman -Q espeak || pacman -Q espeak-ng &> /dev/null    
}

archInstallDep() {
	local withEmacs=$1
	local with_X=$2

    pacman -Syu
    pacman --noconfirm -S base-devel wget texinfo alsa-lib alsa-plugins tcl tk &>> "$LOG"
     
    #tclx
    # cd $workDir
    # wget $TCLX_AUR
    # tar -zxf tclx
    # makepkg -s
    # cd ..
    pacman --noconfirm -U $archivesDir/tclx-*-$(uname -m).pkg.tar.xz &>> "$LOG"


	#TODO emacs dependencies
	# if [ -n "$withEmacs" ]; then
	# 	local suffix
	# 	[ "$with_X" = "0" ] && suffix=-nox
	# 	apt-get build-dep emacs$suffix
	# 	apt-get install git  &>> "$LOG" || true
	# fi
	
}

# debian based specific commands
debianCheckEmacs() {
    dpkg -l emacs* &> /dev/null    
}

debianCheckLibvoxin() {
    dpkg -l "libvoxin*" &> /dev/null    
}

debianCheckEspeak() {
    dpkg -l "espeak*" &> /dev/null    
}

debianInstallDep() {
	local withEmacs=$1
	local with_X=$2
    sudo apt-get update  &>> "$LOG"
    sudo apt-get -y install build-essential libespeak-dev libasound2-plugins libasound2-dev texinfo tcl8.6-dev tclx8.4 &>> "$LOG"
	if [ -n "$withEmacs" ]; then
		local suffix
		[ "$with_X" = "0" ] && suffix=-nox
		sudo apt-get build-dep emacs$suffix &>> "$LOG"
	 	sudo apt-get install git &>> "$LOG" || true
	fi
}

usage() {
	echo "
Usage: 
 $NAME [options]

This installer builds emacspeak with espeak or voxin.
By default, the emacspeak $PV archive is downloaded. 
But another emacspeak release or even its latest sources from GitHub
can be selected.

Optionally the developer version of emacs (git HEAD) can be
downloaded and built with or without X support.

OPTIONS
-c, --clean            clean all: delete the build, install directories and log file
-r, --release=version  download the indicated version (e.g. '49.0' or 'latest' for the developer version).
-e, --emacs            download and build emacs (by default with X support for graphical environment)
-n, --nox              build emacs without X support (for console based environment)
-h, --help             display this help 

EXAMPLES
# build emacspeak $PV
 $0

# build emacspeak $PV and emacs (with X)
 $0 --emacs

# build emacspeak $PV and emacs (without X)
 $0 --nox

# build emacspeak 48.0
 $0 --release 48.0

# build emacspeak from the currently developed sources (git HEAD)
 $0 --release latest

"
	
}

buildEmacs() {
	local dir=$1
	local with_x=$2
	local flags

	pushd "$dir" > /dev/null
	unset emacsAlias
	flags="--without-gconf --prefix=$installDir"
	if [ "$with_x" = 1 ]; then
		flags="$flags --with-x=yes "
	else
		flags="$flags --with-x=no --without-gsettings"
	fi   
	
	./autogen.sh &>> "$LOG"
	./configure $flags &>> "$LOG"
	make bootstrap &>> "$LOG"
	make &>> "$LOG"
	make install &>> "$LOG"
	emacsAlias="alias emacs=\"$installDir/bin/emacs\""
	popd
}

downloadEmacspeakArchive() {
	local dir=$1
	local version=$2
	
	pushd "$dir" > /dev/null
	if [ ! -e "emacspeak-$version.tar.bz2" ]; then
		local url="https://github.com/tvraman/emacspeak/releases/download/$version/emacspeak-$version.tar.bz2"
		wget "$url" &>> "$LOG"
	fi
	
	if [ -e "emacspeak-$version" ]; then
		rm -rf "emacspeak-$version"
	fi
	
	tar --no-same-owner -jxf emacspeak-${version}.tar.bz2
	popd > /dev/null
}

downloadFromGit() {
	local dir=$1
	local url=$2
	if [ ! -d "$dir" ]; then
		p=$(dirname "$dir")
		[ ! -d "$p" ] && mkdir -p "$p"
		gitDownloadDevelopperVersion "$p" $url
	else
		gitCleanLocalCopy "$dir"
		gitUpdateLocalCopy "$dir"
	fi		
}

msg() {
	echo -e "$1" | tee -a "$LOG"
}

leave() {
	msg "$1"
	exit "$2"
}

quit() {
    leave "Error: for more details check log file: $LOG" 1
}

clean() {
	alias | grep -q "$workDir" && leave "Error: a shell alias is using the build directory. \n\
Please remove any alias referring to $workDir (check for example ~/.bashrc)" 1

	for i in ~/.emacs ~/.emacs.el ~/.emacs.d/init.el; do
		if [ -f $i ]; then
			strings "$i" | grep -Eq "^[^;].*$BASE/lisp/emacspeak-setup.el" && leave "Error: the emacs init file is using the build directory ($workDir) \n\
Please review file: $i \n\
and remove or comment any line referring to \n\
$BASE/lisp/emacspeak-setup.el" 1 
			strings "$i" | grep -Eiq "^[^;].*dtk_program.*outloud" && msg "Warning: dtk_program is set to outloud. \
Please review file: $i"
			break
		fi
	done	

	local GIT=$(which git) || true
	if [ -n "$GIT" ]; then
		gitCleanLocalCopy emacs
		gitCleanLocalCopy emacspeak
	fi
	rm -rf build log
	exit 0
}


buildEmacspeak() {
	local dir=$1

	pushd "$dir" > /dev/null
	emacspeakDir=$PWD
	make config &>> "$LOG"
	make &>> "$LOG"
	if [ "$voxinFound" = "1" ]; then
		make outloud &>> "$LOG"
	fi
	if [ "$espeakFound" = "1" ]; then
		make espeak &>> "$LOG"
	fi
	
	chmod -R ugo+rX .
	popd
}
