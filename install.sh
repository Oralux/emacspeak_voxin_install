#!/bin/bash
# 2016-2018, Gilles Casse <gcasse@oralux.org>
#

cd "$(dirname "$0")" && BASE=$PWD
NAME=$(basename "$0")

source bin/conf.inc

[ "$UID" = "0" ] && echo "Sorry, does not run this script as superuser." && exit 0

mkdir -p "$installDir" "$logDir"
rm -f $logDir/install.*
echo "$0 $@" > "$LOG"

unset CLEAN EMACS HELP emacspeakDir
WITH_X=1
EMACSPEAK_RELEASE=$PV
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
    -r|--release) EMACSPEAK_RELEASE=$2; shift 2;;
    --) shift; break;;
    *) break;;
  esac
done

[ -n "$HELP" ] && usage && exit 0

checkDistro

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

[ -n "$CLEAN" ] && clean

#[ "$voxinFound" = "0" ] && [ "$espeakFound" = "0" ] && leave "Install voxin or espeak before running this script." 0

trap quit ERR

[ -z "$EMACS" ] && ( $checkEmacs || leave "Install emacs before running this script. \n\ 
Or use this script to build the developper version of emacs ( $0 --help )." 0 )

rm -f "$DEP"
$getDep $EMACS $WITH_X "$EMACSPEAK_RELEASE" "$espeakFound" "$voxinFound"
[ -e "$DEP" ] && leave "Some dependencies are lacking. Please run as superuser:\n bin/installDep.sh\nThe missing dependencies are listed in build/dep.txt" 0  

msg "Initialization; please wait... "
msg "Log file: $LOG"

if [ -n "$EMACS" ]; then
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
	downloadEmacspeakArchive $workDir $EMACSPEAK_RELEASE
fi

# emacs is needed to build emacspeak
export PATH=$BASE/build/install/bin:$PATH

msg "Building emacspeak... "
emacspeakDir="$workDir/emacspeak-$EMACSPEAK_RELEASE"
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

