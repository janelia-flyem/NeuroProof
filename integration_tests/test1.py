import neuroproof_test_compare

exe_string = '${BUILDEM_DIR}/bin/neuroproof_graph_predict ${CMAKE_SOURCE_DIR}/integration_tests/inputs/samp1_labels.h5 ${CMAKE_SOURCE_DIR}/integration_tests/inputs/samp1_prediction.h5 ${CMAKE_SOURCE_DIR}/integration_tests/inputs/250-1_agglo_itr1_trial1_opencv_rf_tr255.xml --output-file ${CMAKE_SOURCE_DIR}/integration_tests/temp_data/test1_samp1_labels.h5 --graph-file ${CMAKE_SOURCE_DIR}/integration_tests/temp_data/test1_samp1_graph.json --threshold 0.2 --watershed-threshold 50 --synapse-file ${CMAKE_SOURCE_DIR}/integration_tests/inputs/samp1_synapses.json --postseg-classifier-file ${CMAKE_SOURCE_DIR}/integration_tests/inputs/250-1_agglo_itr5_trial1_vigra_rf_tr255.h5 --post-synapse-threshold 0.1'


outfile = "test1_samp1_fullpredict.txt" 
file_comps = ["test1_samp1_graph.json"] 

neuroproof_test_compare.compare_outputs(exe_string, outfile, file_comps)
