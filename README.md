------------------
BUILD instructions
------------------

1. go in NeuroProof directory and mkdir 'build'
2. cd build; cmake -DFLYEM_BUILD_DIR=/path/to/os-specific-build-dir ..; make
3. if you had not previously installed flyem-build, you'll need to rerun #2 above
4. set your environment variables to use python in /path/to/os-specific-build-dir/bin and possibly PYTHONPATH

The build directory specified above is where all the downloads, includes, and binaries are placed for FlyEM
software.  It should be specific to your OS, architecture, and compiler versions.  It should be used when
independently building software where "./configure --prefix=/path/to/os-specific-build-dir"

-----------
GPR Example
-----------

Call sample program:

    ./bin/calculate_GPR <graph> <num_threads> <num_paths>

An example graph is provided as graph.json.  This file acts as documentation on how to format an input graph.

Calling ./bin/calcualte_GPR graph.json 4 1 will use 4 threads to compute the GPR for the graph.json.  '1' refers
to using K=1 paths for finding node affinity.  Currently K>1 is not supported in NeuroProof.  Details
can be found in an upcoming paper.
