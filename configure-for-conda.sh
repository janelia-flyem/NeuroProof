if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <install-environment-dir>"
    exit 1;
fi

# Start in the same directory as this script.
cd `dirname $0`

export PREFIX="$1"
export PYTHON="${PREFIX}/bin/python"
export CPU_COUNT=`python -c "import multiprocessing; print multiprocessing.cpu_count()"`
export PATH="${PREFIX}/bin":$PATH

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
