import neuroproof_test_compare

exe_string = '${BUILDEM_DIR}/bin/neuroproof_graph_analyze_gt ${CMAKE_SOURCE_DIR}/integration_tests/outputs/test1_samp1_labels.h5 ${CMAKE_SOURCE_DIR}/integration_tests/inputs/samp1_gt.h5 --dump-split-merge-bodies 1 --synapse-file ${CMAKE_SOURCE_DIR}/integration_tests/inputs/samp1_synapses.json --body-error-size 1000 --graph-file ${CMAKE_SOURCE_DIR}/integration_tests/outputs/test1_samp1_graph.json --recipe-file ${CMAKE_SOURCE_DIR}/integration_tests/inputs/samp1_recipe.json'


outfile = "test4_samp1_fullanalyzegt.txt" 
file_comps = [] 

neuroproof_test_compare.compare_outputs(exe_string, outfile, file_comps)
