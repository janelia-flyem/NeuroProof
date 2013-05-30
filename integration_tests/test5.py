import neuroproof_test_compare

exe_string = '${BUILDEM_DIR}/bin/neuroproof_graph_learn -watershed ${CMAKE_SOURCE_DIR}/integration_tests/inputs/samp1_labels_processed.h5 stack -prediction ${CMAKE_SOURCE_DIR}/integration_tests/inputs/samp1_prediction.h5 volume/predictions -groundtruth ${CMAKE_SOURCE_DIR}/integration_tests/inputs/samp1_gt.h5 stack -classifier ${CMAKE_SOURCE_DIR}/integration_tests/temp_data/test5_samp1_classifier.xml -iteration 1 -strategy 1'

outfile = "test5_samp1_graphtrain.txt" 
file_comps = ["test5_samp1_classifier.xml"] 

neuroproof_test_compare.compare_outputs(exe_string, outfile, file_comps)
