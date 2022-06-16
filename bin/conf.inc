# set $BASE before using this script
#
PN=emacspeak_voxin_install

# PV: default emacspeak version 
PV=55.0
REL=1
GIT_EMACSPEAK_URL=https://github.com/tvraman/emacspeak.git
GIT_EMACS_URL=https://github.com/mirrors/emacs.git
archivesDir=$BASE/archives
workDir=$BASE/build
installDir=$workDir/install
logDir=$BASE/log
SERVER=outloud
LOG=$logDir/install.$(date -Iseconds)
DEP=$workDir/dep.txt
LIST_EMACSPEAK_MUST="alsa-utils libasound2-plugins libasound2-dev libespeak-dev libsox-fmt-mp3 sox tcl8.6-dev tclx8.4 libxml2-dev g++"
LIST_EMACSPEAK_NICE="curl fil-plugins graphicsmagick-imagemagick-compat invada-studio-plugins-ladspa ladspa-foo-plugins ladspa-sdk liquidsoap-plugin-ladspa mcp-plugins mplayer nodejs phantomjs pianobar rev-plugins sqlite3 swh-plugins tap-plugins unzip vco-plugins wah-plugins wget xbacklight youtube-dl zam-plugins"

LIST_EMACSPEAK_MUST_ARCH_LINUX="texinfo alsa-utils alsa-plugins alsa-lib espeak sox tcl tk libxml2"
LIST_EMACSPEAK_NICE_ARCH_LINUX="curl fil-plugins ladspa-plugins ladspa mcp-plugins mplayer nodejs pianobar rev-plugins sqlite3 swh-plugins tap-plugins unzip vco-plugins wah-plugins wget xbacklight youtube-dl zam-plugins"

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
    
    # Check if this is an arch linux based distro
    if [ -e "/etc/pacman.conf" ]; then
	checkEmacs=archCheckEmacs
	checkVoxin=archCheckVoxin
	checkEspeak=archCheckEspeak
	getDep=archGetDep
	status=0
    else
	# Check if this is a debian based distro
	type dpkg &> /dev/null
	if [ "$?" = "0" ]; then
	    checkEmacs=debianCheckEmacs
	    checkVoxin=debianCheckVoxin
	    checkEspeak=debianCheckEspeak
	    getDep=debianGetDep
	    status=0
	    # TODO Arch??
	    #		https://wiki.archlinux.org/index.php/Pacman/Tips_and_tricks#Getting_the_dependencies_list_of_several_packages	    
	fi
    fi
    return $status
}

# arch linux based commands
archCheckEmacs() {
    (pacman -Q emacs || pacman -Q emacs-nox) &> /dev/null
}

archCheckVoxin() {
    pacman -Q voxin &> /dev/null
}

archCheckEspeak() {
    pacman -Q espeak &> /dev/null    
}

archGetDep() {
    local withEmacs=$1
    local with_X=$2
    local emacspeakRelease=$3
    local espeakFound=$4
    local voxinFound=$5
    local full=$6    
    local list="$LIST_EMACSPEAK_MUST_ARCH_LINUX"
    local x
    local y

    unlink "$DEP" &> /dev/null

    # add possibly the "nice to have" dependencies
    if [ "$full" = 1 ]; then
	list="$list $LIST_EMACSPEAK_NICE_ARCH_LINUX"
    fi
    
    if [ "$espeakFound" = "0" ] && [ "$voxinFound" = "0" ]; then
	list="$list espeak"
    fi

    list="$list make gcc binutils wget texinfo"    

    if [ "$emacspeakRelease" = "latest" ]; then
	list="$list git"
    fi

    for i in $list; do
	archIsPackageInstalled $i || x="$x $i"
    done

    y=$(echo -n "$x" | wc -w)
    if [ "$y" != "0" ]; then
	echo "$y Packages to install: $x" >> "$LOG"
	echo "$x" > "$DEP"
    fi        
}

archIsPackageInstalled() {
	[ $# != 1 ] || [ -z "$1" ] && return 1
	local package=$1
	pacman -Q "$package" &>/dev/null || pacman -Qg "$package" &>/dev/null	
}

# debian based specific commands
debianCheckEmacs() {
    dpkg -l | grep " emacs" &> /dev/null    
}

debianCheckVoxin() {
    dpkg -l | grep voxin &> /dev/null    
}

debianCheckEspeak() {
    dpkg -l | grep libespeak &> /dev/null    
}

debianIsPackageInstalled() {
	local deb=$1
	grep -A1 "Package: $deb$" /var/lib/dpkg/status | grep "Status: install ok installed" &> /dev/null
}

debianDoesPackageExist() {
	local deb=$1
	grep "Package: $deb$" /var/lib/apt/lists/*_Packages &> /dev/null
}

debianGetDep() {
	local withEmacs=$1
	local with_X=$2
	local emacspeakRelease=$3
	local espeakFound=$4
	local voxinFound=$5
	local full=$6
	
	local list="$LIST_EMACSPEAK_MUST"
	local x
	local y
	local emacsVersion=$(apt-cache pkgnames | grep -E "^emacs[0-9]+$" | sed 's/emacs//' | sort -n | tail -1 )		

	unlink "$DEP" &> /dev/null
	
	if [ "$espeakFound" = "0" ] && [ "$voxinFound" = "0" ]; then
		x=espeak
		debianDoesPackageExist espeak && x=espeak
		list="$list $x"
	fi

	if [ "$emacspeakRelease" = "latest" ] || [ "$withEmacs" = 1 ]; then
		list="$list git"
	fi

	if [ "$withEmacs" = 0 ]; then
		[ -z "$DISPLAY" ] && x=-nox || unset x		
		debianIsPackageInstalled emacs${emacsVersion}${x} || list="$list emacs${emacsVersion}${x}"
	else		
		[ "$with_X" = "0" ] && x=-nox || unset x

		# obtain the build dependencies required by the current emacs version
		apt-get -s build-dep emacs${emacsVersion}${x} &> /dev/null
		if [ $? != 0 ]; then
			echo "The build dependencies of emacs${emacsVersion}${x} can't be found.
Your list of authorized repositories should probably be extended.
You would have to uncomment the deb-src lines in your
/etc/apt/sources.list and run apt-get update after any modification
" | tee -a "$LOG"
			exit 1
		fi		
		list="$list $(apt-get -s build-dep emacs${emacsVersion}${x} | grep "^  [^ ]")"
		debianIsPackageInstalled emacs${emacsVersion}${x} || list="$list emacs${emacsVersion}${x}"
	fi

	# find which "must have" dependency is not yet installed
	unset x
	for i in $list; do
		debianIsPackageInstalled "$i" || x="$x $i"
	done

	# add possibly the "nice to have" dependencies
	if [ "$full" = 1 ]; then
		for i in $LIST_EMACSPEAK_NICE; do
			debianIsPackageInstalled "$i"
			if [ $? != 0 ]; then
				debianDoesPackageExist "$i" && x="$x $i"
			fi
		done
	fi

	y=$(echo -n "$x" | wc -w)
	if [ "$y" != "0" ]; then
		echo "$y Packages to install: $x" >> "$LOG"
		echo "$x" > "$DEP"
	fi
}

usage() {
	echo "
Usage: 
 $NAME [options]

This installer builds emacspeak with espeak or voxin.
By default, the emacspeak $PV archive is downloaded. 
Another emacspeak release or even its latest sources from GitHub can
be selected.

On Debian based distributions, the developer version of emacs (git
HEAD) can be downloaded and built with or without X support.

OPTIONS
-c, --clean            clean all: delete the build, install directories and log file
-r, --release=version  download the indicated version (e.g. '53.0' or 'latest' for the developer version).
-f, --full             install the full dependencies (ladspa, youtube-dl, nodejs,...)
-h, --help             display this help 

Additional options for Debian based distributions
-e, --emacs            download and build emacs (by default with X support for graphical environment)
-n, --nox              build emacs without X support (for console based environment)

EXAMPLES
# build emacspeak $PV
 $0

# build emacspeak $PV and emacs (with X)
 $0 --emacs

# build emacspeak $PV and emacs (without X)
 $0 --nox

# build emacspeak 53.0
 $0 --release 53.0

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
