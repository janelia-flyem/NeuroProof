# Depending on our platform, shared libraries end with either .so or .dylib
if [[ `uname` == 'Darwin' ]]; then
    DYLIB_EXT=dylib
    CC=clang
    CXX=clang++
    CXXFLAGS="-stdlib=libc++"
else
    DYLIB_EXT=so
    CC=gcc
    CXX=g++
    CXXFLAGS=""
fi

CONFIGURE_ONLY=0
if [[ $1 != "" ]]; then
    if [[ $1 == "--configure-only" ]]; then
        CONFIGURE_ONLY=1
    else
        echo "Unknown argument: $1"
        exit 1
    fi
fi

PY_VER=$(python -c "import sys; print('{}.{}'.format(*sys.version_info[:2]))")
PY_ABIFLAGS=$(python -c "import sys; print('' if sys.version_info.major == 2 else sys.abiflags)")
PY_ABI=${PY_VER}${PY_ABIFLAGS}


# CONFIGURE
mkdir -p build # Using -p here is convenient for calling this script outside of conda.
cd build
cmake ..\
        -DCMAKE_C_COMPILER="${CC}" \
        -DCMAKE_CXX_COMPILER="${CXX}" \
        -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
        -DCMAKE_PREFIX_PATH="${PREFIX}" \
        -DCMAKE_CXX_FLAGS=-I"${PREFIX}/include -std=c++11 ${CXXFLAGS}" \
        -DCMAKE_SHARED_LINKER_FLAGS="-Wl,-rpath,${PREFIX}/lib -L${PREFIX}/lib" \
        -DCMAKE_EXE_LINKER_FLAGS="-Wl,-rpath,${PREFIX}/lib -L${PREFIX}/lib" \
        -DBOOST_ROOT=${PREFIX} \
        -DBoost_LIBRARY_DIR="${PREFIX}/lib" \
        -DBoost_INCLUDE_DIR="${PREFIX}/include" \
        -DPYTHON_EXECUTABLE="${PYTHON}" \
        -DPYTHON_LIBRARY="${PREFIX}/lib/libpython${PY_ABI}.${DYLIB_EXT}" \
        -DPYTHON_INCLUDE_DIR="${PREFIX}/include/python${PY_ABI}" \
        -DLIBDVIDCPP_INCLUDE_DIR="${PREFIX}/include" \
        -DLIBDVIDCPP_LIBRARY="${PREFIX}/lib/libdvidcpp.${DYLIB_EXT}" \
        -DENABLE_GUI=1 \
##

if [[ $CONFIGURE_ONLY == 0 ]]; then

    # BUILD
    make -j${CPU_COUNT}
    
    # TEST
    (
        set -e
    
        # (Since conda hasn't performed its link step yet, we must 
        #  help the tests locate their dependencies via LD_LIBRARY_PATH)
        if [[ $(uname) == Darwin ]]; then
            export DYLD_FALLBACK_LIBRARY_PATH="$PREFIX/lib":"${DYLD_FALLBACK_LIBRARY_PATH}"
        else
            export LD_LIBRARY_PATH="$PREFIX/lib":"${LD_LIBRARY_PATH}"
        fi
        
        # Run the tests
        make test
    )
    
    # INSTALL
    make install
fi
