BUILD instructions:

1. make a link to libjsonlib.so in a default library directory
2. make a link to the 'json' directory in the default include directory
3. go in NeuroProof directory and mkdir 'build'
4. cd build; cmake ..; make
5. make link to build/libNeuroProofPriority.so to a default python directory to use the Python libraries

GPR Example:

Call sample program:

    ./bin/calculate_GPR <graph> <num_threads> <num_paths>

An example graph is provided as graph.json.  This file acts as documentation on how to format an input graph.

Calling ./bin/calcualte_GPR graph.json 4 1 will use 4 threads to compute the GPR for the graph.json.  '1' refers
to using K=1 paths for finding node affinity.  Currently K>1 is not supported in NeuroProof.  Details
can be found in an upcoming paper.
