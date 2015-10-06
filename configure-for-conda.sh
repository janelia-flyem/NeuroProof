if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <install-environment-dir>"
    exit 1;
fi

export PREFIX="${1%/}"
export PYTHON="${PREFIX}/bin/python"
export CPU_COUNT=`python -c "import multiprocessing; print multiprocessing.cpu_count()"`
export PATH="${PREFIX}/bin":$PATH

# Start in the same directory as this script.
cd `dirname $0`

# If the build dir already exists and CMAKE_INSTALL_PREFIX doesn't
# match the new destination, we need to start from scratch.
if [[ -e build/CMakeCache.txt ]]; then

    grep "CMAKE_INSTALL_PREFIX:PATH=$PREFIX" build/CMakeCache.txt > /dev/null 2> /dev/null
    GREP_RESULT=$?
    if [[ $GREP_RESULT == 1 ]]; then
        echo "*** Removing old build directory: $(pwd)/build" 2>&1
        rm -r build
    fi
fi

# Install gcc first if necessary.
if [ ! -e "${PREFIX}/bin/gcc" ]; then
    conda install -p "${PREFIX}" -c flyem gcc
fi

# Do you already have curl?
curl --help > /dev/null
if [[ $? == 0 ]]; then
    CURL=curl
else
    # Install to conda prefix
    conda install -p "${PREFIX}" curl
    CURL="${PREFIX}/bin/curl"
fi

BUILD_SCRIPT_URL=https://raw.githubusercontent.com/janelia-flyem/flyem-build-conda/master/neuroproof/build.sh
"$CURL" "$BUILD_SCRIPT_URL" | bash -x -e -s - --configure-only
