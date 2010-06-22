
#WEB_USER=$ICUB_USER
WEB_USER=babybot
WEB_SERVER=eris.liralab.it
WEB_DIR=/var/www/html/iCub

WEB_DOC_DIR=$WEB_DIR/dox
WEB_DOWNLOAD_DIR=$WEB_DIR/downloads

WEB_DOC_SUFFIX=report-ubuntu

#COMPILE_FOR_MSVC8=true
COMPILE_DOXYGEN=true

SYSTEM_DESCRIPTION_FILE=get_sys_description.sh

CMAKE_OPTIONS="-DADD_IPOPT:BOOL=TRUE -DON_TEST_SERVER:BOOL=TRUE -DCREATE_GUIS_GTKMM:BOOL=TRUE -DCREATE_GUIS_QT:BOOL=TRUE -DCREATE_GUIS_GTK:BOOL=TRUE -DCOMPILE_NEW_FRAMEGRABBERGUI:BOOL=TRUE -DCOMPILE_FOR_TESTING:BOOL=TRUE"

function is_msvc8() {
    test ! "k$COMPILE_FOR_MSVC8" = "k"
}

function compile_dox() {
    test ! "k$COMPILE_DOXYGEN" = "k"
}

function make_clean () {
    echo "*** Cleaning project"
    if is_msvc8; then
	devenv *.sln /Clean /Out makelog.txt
	cat makelog.txt
    else
	make clean
    fi
}

function make_build () {
    echo "*** Building $*"
    export ICUB_TEST=true
    if is_msvc8; then
	rm -f makelog.txt
	if [ "k$1" = "k" ]; then
	    devenv *.sln /Build Debug /Out makelog.txt
	else
	    devenv *.sln /Project $1 /Build Debug /Out makelog.txt
	fi
	cat makelog.txt
	# devenv does not seem to give a reliable failure return value
	# so we parse the makelog.txt file
	grep -q "0 failed" makelog.txt || exit 1
    else
	make $1
    fi
}

function make_config () {
    common_flags=$CMAKE_OPTIONS
    echo "*** Configuring project using CMake with flags $common_flags"
    if is_msvc8; then
	"/cygdrive/c/Program Files/CMake 2.4/bin/cmake.exe" -DCMAKE_COLOR_MAKEFILE:BOOL=FALSE $common_flags -G "Visual Studio 8 2005" $*
    else
	cmake $common_flags $*
    fi
}

function normalize_path () {
    if is_msvc8; then
	cygpath -m $*
    else
	echo $*
    fi
}

function denormalize_path () {
    if is_msvc8; then
	cygpath -w $*
    else
	echo $*
    fi
}

function std_timeout () {
    sec=$1
    shift
    if is_msvc8; then
	# no timeout cmd
	"$@"
    else
	timeout $sec "$@"
    fi
}

# allow for subdirectory out-of-source builds, up to two levels of nesting
#srcdir=$PWD
#if [ ! -e ICUBConfig.cmake ]; then
#  if [ ! -e ../ICUBConfig.cmake ]; then
#    if [ -e ../../ICUBConfig.cmake ]; then
#	srcdir="$PWD/../.."
#    fi
#  else
#    srcdir="$PWD/.."
#  fi
#fi

if [ "k$ICUB_DIR" = "k" ]; then
    export ICUB_DIR=`denormalize_path $PWD`
else 
    export ICUB_DIR=`denormalize_path $ICUB_DIR`
fi
if [ "k$ICUB_ROOT" = "k" ]; then
    export ICUB_ROOT="$ICUB_DIR"
else
    export ICUB_ROOT=`denormalize_path $ICUB_ROOT`
fi
if [ "k$YARP_DIR" = "k" ]; then
    export YARP_DIR=`denormalize_path $PWD/../yarp2`
else
    export YARP_DIR=`denormalize_path $YARP_DIR`
fi
if [ "k$YARP_ROOT" = "k" ]; then
    export YARP_ROOT=$YARP_DIR
else
    export YARP_ROOT=`denormalize_path $YARP_ROOT`
fi

# report on all important environment variables
echo YARP_ROOT $YARP_ROOT
echo YARP_DIR $YARP_DIR
echo ICUB_ROOT $ICUB_ROOT
echo ICUB_DIR $ICUB_DIR
echo ODE_DIR $ODE_DIR " # just for ODE simulator"
echo IPOPT_DIR $IPOPT_DIR

