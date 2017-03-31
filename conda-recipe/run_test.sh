# This script runs 'make test', which uses the build artifacts in the build directory, not the installed files.
# Therefore, they haven't been post-processed by conda to automatically locate their dependencies.
# We'll set LD_LIBRARY_PATH to avoid errors from ld
if [[ `uname` == 'Darwin' ]]; then
    export DYLD_FALLBACK_LIBRARY_PATH="$PREFIX/lib":"${DYLD_FALLBACK_LIBRARY_PATH}"
else
    export LD_LIBRARY_PATH="$PREFIX/lib":"${LD_LIBRARY_PATH}"
fi

export PYTHONPATH="$(pwd)/lib"

cd "build"

# Test executables in the build directory have internal links to .dylibs in the build prefix,
#  but conda deletes the build prefix before it runs this test script.
# As a quick fix, we symlink the test env prefix as the build env prefix.
BUILD_PREFIX=$(grep CMAKE_INSTALL_PREFIX CMakeCache.txt | python -c "import sys; print sys.stdin.readlines()[0].split('=')[1]")
if [ ! -d "${BUILD_PREFIX}" ]; then
    ln -s ${PREFIX} ${BUILD_PREFIX}
fi

# RUN THE TESTS
(
    set -e
    make test
)
TEST_RESULT=$?

# Cleanup the symlink
if [ ! -h "${BUILD_PREFIX}" ]; then
    rm ${BUILD_PREFIX}
fi

exit $TEST_RESULT
