import neuroproof_test_compare

exe_string = '${BUILDEM_DIR}/bin/neuroproof_graph_analyze ${CMAKE_SOURCE_DIR}/integration_tests/outputs/test1_samp1_graph.json -g 1 -b 1 --node-size-threshold 1000'


outfile = "test3_samp1_fullanalyze.txt" 
file_comps = [] 

neuroproof_test_compare.compare_outputs(exe_string, outfile, file_comps)
