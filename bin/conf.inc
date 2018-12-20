# set $BASE before using this script
#
PN=emacspeak_voxin_install

# PV: default emacspeak version 
PV=49.0
REL=5
GIT_EMACSPEAK_URL=https://github.com/tvraman/emacspeak.git
GIT_EMACS_URL=https://github.com/mirrors/emacs.git
archivesDir=$BASE/archives
workDir=$BASE/build
installDir=$workDir/install
logDir=$BASE/log
SERVER=outloud
TCLX_AUR=https://aur.archlinux.org/cgit/aur.git/snapshot/tclx.tar.gz
LOG=$logDir/install.$(date -Iseconds)
DEP=$workDir/dep.txt
LIST_EMACSPEAK_MUST="alsa-utils libasound2-plugins libespeak-dev libsox-fmt-mp3 sox tcl8.6-dev tclx8.4"
LIST_EMACSPEAK_NICE="curl fil-plugins graphicsmagick-imagemagick-compat invada-studio-plugins-ladspa ladspa-foo-plugins ladspa-sdk  liquidsoap-plugin-ladspa mcp-plugins mplayer nodejs phantomjs pianobar rev-plugins sqlite3 swh-plugins tap-plugins unzip vco-plugins wah-plugins wget xbacklight youtube-dl zam-plugins"

###
gitDownloadDevelopperVersion() {
	# download only the last commit to reduce size
	local dir=$1
	local url=$2	
	git -C "$dir" clone --depth 1 "$url" | tee -a "$LOG"
}

gitUpdateLocalCopy() {
        local dir=$1
        if [ -d "$dir" ]; then
          git -C "$dir" fetch --depth 1 | tee -a "$LOG"
          git -C "$dir" reset --hard origin/master | tee -a "$LOG"
          git -C "$dir" clean -dfx | tee -a "$LOG"
        fi
}

gitCleanLocalCopy() {
	local dir=$1
	[ ! -d "$dir" ] || git -C "$dir" clean -xdf | tee -a "$LOG"
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
	# getDep=archInstallDep
	# status=0
    # else
	# Check if this is a debian based distro
	type dpkg &> /dev/null
	if [ "$?" = "0" ]; then
	    checkEmacs=debianCheckEmacs
	    checkLibvoxin=debianCheckLibvoxin
	    checkEspeak=debianCheckEspeak
	    getDep=debianGetDep
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

archGetDep() {
	local withEmacs=$1
	local with_X=$2

    pacman -Syu
    pacman --noconfirm -S base-devel wget texinfo alsa-lib alsa-plugins tcl tk  | tee -a "$LOG"
     
    #tclx
    # cd $workDir
    # wget $TCLX_AUR
    # tar -zxf tclx
    # makepkg -s
    # cd ..
    pacman --noconfirm -U $archivesDir/tclx-*-$(uname -m).pkg.tar.xz  | tee -a "$LOG"


	#TODO emacs dependencies
}

# debian based specific commands
debianCheckEmacs() {
    dpkg -l | grep " emacs" &> /dev/null    
}

debianCheckLibvoxin() {
    dpkg -l | grep libvoxin &> /dev/null    
}

debianCheckEspeak() {
    dpkg -l | grep libespeak &> /dev/null    
}

debianGetDep() {
	local withEmacs=$1
	local with_X=$2
	local emacspeakRelease=$3
	local espeakFound=$4
	local voxinFound=$5
	
	local list="$LIST_EMACSPEAK_DEB_MUST"
	local listmin
	local x

	if [ "$espeakFound" = "0" ] && [ "$voxinFound" = "0" ]; then
		listmin=espeak
		x=$(apt-cache search "^espeak-ng$" | wc -c)
		[ "$x" != 0 ] && listmin=espeak-ng
	fi

	list="$list $listmin"
	
	if [ "$emacspeakRelease" = "latest" ] || [ -n "$withEmacs" ]; then
		list="$list git"
	fi

	if [ -n "$withEmacs" ]; then
		[ "$with_X" = "0" ] && x=-nox

		# obtain the build dependencies required by the current emacs version
		local emacsVersion=$(apt-cache pkgnames | grep -E "^emacs[0-9]+$" | sed 's/emacs//' | sort -n | tail -1 )		
		
		list="$list git"
		apt-get -s build-dep emacs${emacsVersion}${x}
		if [ $? != 0 ]; then
			echo "Build dependencies of emacs${emacsVersion}${x} Package $i can't be found.
Possible cause:
- Your list of authorized repositories must be extended.
  You probably have to enable the src repositories.
  Check your /etc/apt/sources.list
  and run apt-get update after any modification
"  | tee -a "$LOG"
			exit 1
		fi		
		list="$list $(apt-get -s build-dep emacs${emacsVersion}${x} | grep "^  [^ ]")"
		list="$list $(apt-cache depends emacs${emacsVersion}${x} | awk '/Depends/{print $2}')"
	fi

	# check the presence of the "must have" dependencies
	unset listmin
	for i in $list; do
		dpkg -s "$i" &> /dev/null
		if [ $? != 0 ] && [ $i != libc6 ]; then
			# is "$i" a package name?
			x=$(echo "$i" | sed "s/\+/\\\+/g")
			x=$(apt-cache search "^$x$" | wc -c)
			if [ "$x" != 0 ]; then
				listmin="$listmin $i"
			else
				echo "Package $i can't be found.
Possible causes:
- Your list of authorized debian/ubuntu repositories must be extended.
  For example, for ubuntu server, you may have to enable the src directories.
  Check your /etc/apt/sources.list
  and run apt-get update after any modification

- if this package can't be found in your distribution then report this issue at contact at oralux.org
"  | tee -a "$LOG"
				exit 1
			fi
		fi
	done

	# add possibly the "nice to have" dependencies
	for i in $LIST_EMACSPEAK_DEB_MIGHT; do
		dpkg -s "$i" &>/dev/null
		if [ $? != 0 ]; then
			# is "$i" a package name?
			x=$(apt-cache search "^$i$" | wc -c)
			[ "$x" != 0 ] && listmin="$listmin $i"
		fi
	done

	# store list
	echo "list: $listmin" >> "$LOG"
	for i in $listmin; do
		echo -n " $i" >> "$DEP"
	done	
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
	
	./autogen.sh  | tee -a "$LOG"
	./configure $flags  | tee -a "$LOG"
	make bootstrap  | tee -a "$LOG"
	make  | tee -a "$LOG"
	make install | tee -a "$LOG"
	emacsAlias="alias emacs=\"$installDir/bin/emacs\""
	popd
}

downloadEmacspeakArchive() {
	local dir=$1
	local version=$2
	
	pushd "$dir" > /dev/null
	if [ ! -e "emacspeak-$version.tar.bz2" ]; then
		local url="https://github.com/tvraman/emacspeak/releases/download/$version/emacspeak-$version.tar.bz2"
		wget "$url" | tee -a "$LOG"
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
	make config  | tee -a "$LOG"
	make  | tee -a "$LOG"
	if [ "$voxinFound" = "1" ]; then
		make outloud | tee -a "$LOG"
	fi
	if [ "$espeakFound" = "1" ]; then
		make espeak | tee -a "$LOG"
	fi
	
	chmod -R ugo+rX .
	popd
}
