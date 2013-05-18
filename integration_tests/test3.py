import neuroproof_test_compare

exe_string = '${BUILDEM_DIR}/bin/neuroproof_graph_analyze ${CMAKE_SOURCE_DIR}/integration_tests/inputs/samp1_graph_processed.json -g 1 -b 1 --node-size-threshold 1000'


outfile = "test3_samp1_fullanalyze.txt" 
file_comps = [] 

neuroproof_test_compare.compare_outputs(exe_string, outfile, file_comps)
