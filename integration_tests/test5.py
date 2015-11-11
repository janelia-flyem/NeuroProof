import neuroproof_test_compare

exe_string = '${INSTALL_PREFIX_PATH}/bin/neuroproof_graph_learn ${CMAKE_SOURCE_DIR}/integration_tests/inputs/samp1_labels_processed.h5 ${CMAKE_SOURCE_DIR}/integration_tests/inputs/samp1_prediction.h5 ${CMAKE_SOURCE_DIR}/integration_tests/inputs/samp1_gt.h5 --classifier-name ${CMAKE_SOURCE_DIR}/integration_tests/temp_data/test5_samp1_classifier.xml --num-iterations 1 --strategy-type 1'

outfile = "test5_samp1_graphtrain.txt" 
file_comps = ["test5_samp1_classifier.xml"] 

neuroproof_test_compare.compare_outputs(exe_string, outfile, file_comps)
