import neuroproof_test_compare

exe_string = '${INSTALL_PREFIX_PATH}/bin/neuroproof_graph_predict ${CMAKE_SOURCE_DIR}/integration_tests/inputs/samp1_labels.h5 ${CMAKE_SOURCE_DIR}/integration_tests/inputs/samp1_prediction.h5 ${CMAKE_SOURCE_DIR}/integration_tests/inputs/250-1_agglo_itr1_trial1_opencv_rf_tr255.xml --output-file ${CMAKE_SOURCE_DIR}/integration_tests/temp_data/test2_samp1_labels.h5 --graph-file ${CMAKE_SOURCE_DIR}/integration_tests/temp_data/test2_samp1_graph.json --threshold 0.0 --watershed-threshold 0 --synapse-file ${CMAKE_SOURCE_DIR}/integration_tests/inputs/samp1_synapses.json --postseg-classifier-file ${CMAKE_SOURCE_DIR}/integration_tests/inputs/250-1_agglo_itr5_trial1_vigra_rf_tr255.h5'


outfile = "test2_samp1_nooppredict.txt" 
file_comps = ["test2_samp1_graph.json"] 

neuroproof_test_compare.compare_outputs(exe_string, outfile, file_comps)
