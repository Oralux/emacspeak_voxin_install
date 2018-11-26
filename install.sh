#!/bin/bash -e
# 2016-2018, Gilles Casse <gcasse@oralux.org>
#

BASE="$(cd "$(dirname "$0")" && pwd)"
NAME=$(basename "$0")
source "$BASE"/bin/conf.inc

mkdir -p "$installDir" "$logDir"
echo "$0 $@" > "$LOG"

unset CLEAN EMACS HELP emacspeakDir
WITH_X=1
RELEASE=$PV
OPTIONS=`getopt -o cehnr: --long clean,emacs,help,nox,release: \
             -n "$NAME" -- "$@"`
[ $? != 0 ] && usage && exit 1
eval set -- "$OPTIONS"

while true; do
  case "$1" in
    -c|--clean) CLEAN=1; shift;;
    -e|--emacs) EMACS=1; shift;;
    -h|--help) HELP=1; shift;;
    -n|--nox) EMACS=1; WITH_X=0; shift;;
    -r|--release) RELEASE=$2; shift 2;;
    --) shift; break;;
    *) break;;
  esac
done

[ -n "$HELP" ] && usage && exit 0

checkDistro

if [ -n "$EMACS" ] || [ "$RELEASE" = "latest" ]; then
	GIT=$(which git) || leave "git not found"
fi

voxinFound=0
espeakFound=0
unset DTK

echo -n "Checking eSpeak : "
$checkEspeak
if [ "$?" = "0" ]; then
    espeakFound=1
    echo "yes"
else    
    echo "no"
fi

echo -n "Checking Voxin >= 1.00 : "
$checkLibvoxin
if [ "$?" = "0" ]; then
    voxinFound=1
    DTK="DTK_PROGRAM=outloud"
    echo "yes"
else    
    echo "no"
fi

[ "$UID" = "0" ] && leave "Sorry, does not run this script as root." 0
[ -n "$CLEAN" ] && clean

[ "$voxinFound" = "0" ] && [ "$espeakFound" = "0" ] && leave "Install voxin or espeak before running this script." 0

trap quit ERR

[ -z "$EMACS" ] && ( $checkEmacs || leave "Install emacs before running this script. \n\ 
Or use this script to build the developper version of emacs ( $0 --help )." 0 )

msg "Initialization; please wait... "
$installDep $EMACS $WITH_X

if [ -n "$EMACS" ]; then
	msg "Downloading emacs... "
	downloadFromGit $workDir/emacs $GIT_EMACS_URL

	msg "Building emacs... "
	buildEmacs "$workDir"/emacs "$WITH_X"
	# clean the emacs build directory
	gitCleanLocalCopy "$workDir"/emacs
fi

msg "Downloading emacspeak ($RELEASE)... "
if [ "$RELEASE" = "latest" ]; then
	downloadFromGit $workDir/emacspeak $GIT_EMACSPEAK_URL
	[ ! -e "$workDir/emacspeak-$RELEASE" ] && ln -sf emacspeak "$workDir/emacspeak-$RELEASE"
else
	downloadEmacspeakArchive $workDir $RELEASE
fi

msg "Building emacspeak... "
emacspeakDir="$workDir/emacspeak-$RELEASE"
buildEmacspeak "$emacspeakDir"

if [ -n "$emacsAlias" ]; then
	msg "
To call your local emacs, add this alias to your shell initialization file (for example ~/.bashrc) 
"
	msg "$emacsAlias"
fi

msg "
To run this Emacspeak build, add the following lines to the top of your emacs init file (e.g. in  ~/.emacs ); then, start emacs
" 

if [ -n "$DTK" ]; then
	msg '(setenv "DTK_PROGRAM" "outloud")'
fi
msg "(load-file \"$emacspeakDir/lisp/emacspeak-setup.el\")"

msg "\nThese instructions can be retrieved at the end of the log file: $LOG"

