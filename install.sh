#!/bin/bash
# 2016-2022, Gilles Casse <gcasse@oralux.org>
#

cd "$(dirname "$0")" && BASE=$PWD
NAME=$(basename "$0")

source bin/conf.inc

[ "$UID" = "0" ] && echo "Sorry, does not run this script as superuser." && exit 0

mkdir -p "$installDir" "$logDir"
rm -f $logDir/install.*
echo "$0 $@" > "$LOG"

CLEAN=0
EMACS=0
FULL=0
HELP=0
WITH_X=1
EMACSPEAK_RELEASE=$PV
OPTIONS=`getopt -o cefhnr: --long clean,emacs,full,help,nox,release: \
             -n "$NAME" -- "$@"`
[ $? != 0 ] && usage && exit 1
eval set -- "$OPTIONS"

# apply new pull request #67 (basic speech-dispatcher server) for
# release = 56.0
applyPatch() {
    local installDir=$1
    local dir=$PWD

    if [ "$EMACSPEAK_RELEASE" = "56.0" ]; then
	mkdir -p "$patchDir"
	cp "$BASE"/patch/56.0/*.patch "$patchDir"/

	cd "$patchDir"
	if [ ! -e "67.patch" ]; then
	    wget https://github.com/tvraman/emacspeak/pull/67.patch
	fi
	for p in *patch; do
	    patch --dry-run -d "$installDir" -fp1 < "$p" >/dev/null
	    [ $? = 0 ] && patch -d "$installDir" -fp1 < "$p"
	done
    fi

    cd "$dir"
}

while true; do
  case "$1" in
    -c|--clean) CLEAN=1; shift;;
    -e|--emacs) EMACS=1; shift;;
    -f|--full) FULL=1; shift;;
    -h|--help) HELP=1; shift;;
    -n|--nox) EMACS=1; WITH_X=0; shift;;
    -r|--release) EMACSPEAK_RELEASE=$2; shift 2;;
    --) shift; break;;
    *) break;;
  esac
done

[ "$HELP" = 1 ] && usage && exit 0

checkDistro || leave "Sorry, this distribution is not yet supported. For support, email to contact at oralux.org " 1

voxinFound=0
espeakFound=0
speechdFound=0
unset DTK

echo -n "Checking eSpeak : "
$checkEspeak
if [ "$?" = "0" ]; then
    espeakFound=1
    echo "yes"
else    
    echo "no"
fi

echo -n "Checking Voxin >= 2.0 : "
$checkVoxin
if [ "$?" = "0" ]; then
    voxinFound=1
    DTK="DTK_PROGRAM=outloud"
    echo "yes"
else    
    echo "no"
fi

echo -n "Checking Speech-Dispatcher: "
$checkSpeechd
if [ "$?" = "0" ]; then
    speechdFound=1
    DTK="DTK_PROGRAM=speechd"
    echo "yes"
else    
    echo "no"
fi

[ "$CLEAN" = 1 ] && clean

#[ "$voxinFound" = "0" ] && [ "$espeakFound" = "0" ] && leave "Install voxin or espeak before running this script." 0

trap quit ERR

$getDep $EMACS $WITH_X "$EMACSPEAK_RELEASE" "$espeakFound" "$voxinFound" "$speechdFound" "$FULL"
[ -e "$DEP" ] && leave "There are missing dependencies. Please run as superuser:\n bin/installDep.sh\nThe missing dependencies are listed in build/dep.txt" 0

msg "Initialization; please wait... "
msg "Log file: $LOG"

if [ "$EMACS" = 1 ]; then
	msg "Downloading emacs... "
	downloadFromGit $workDir/emacs $GIT_EMACS_URL

	msg "Building emacs... "
	buildEmacs "$workDir"/emacs "$WITH_X"
	# clean the emacs build directory
	gitCleanLocalCopy "$workDir"/emacs
fi

msg "Downloading emacspeak ($EMACSPEAK_RELEASE)... "
if [ "$EMACSPEAK_RELEASE" = "latest" ]; then
	downloadFromGit $workDir/emacspeak $GIT_EMACSPEAK_URL
	[ ! -e "$workDir/emacspeak-$EMACSPEAK_RELEASE" ] && ln -sf emacspeak "$workDir/emacspeak-$EMACSPEAK_RELEASE"
else
	downloadEmacspeakArchive "$workDir" "$EMACSPEAK_RELEASE"
fi

# emacs is needed to build emacspeak
export PATH=$BASE/build/install/bin:$PATH

msg "Building emacspeak... "
emacspeakDir="$workDir/emacspeak-$EMACSPEAK_RELEASE"

applyPatch "$emacspeakDir"

buildEmacspeak "$emacspeakDir"


msg "
# Configuration"

case $(getent group audio) in
        *:$USER|*:$USER:*) ;;
        *)
                msg "# To add user $USER to the audio group, type as superuser:"
                msg "usermod -aG audio $USER"
        ;;
esac

if [ -n "$emacsAlias" ]; then
        msg "# Add this alias to ~/.bashrc:
$emacsAlias"
fi

msg "# Add these lines to the top of your emacs init file (e.g. in  ~/.emacs )"

if [ -n "$DTK" ]; then
        msg '(setenv "DTK_PROGRAM" "outloud")'
fi
msg "(load-file \"$emacspeakDir/lisp/emacspeak-setup.el\")"

msg "# Now you may want to reboot your system, before launching emacs"

msg "# These instructions are copied at the end of log/install.*"
