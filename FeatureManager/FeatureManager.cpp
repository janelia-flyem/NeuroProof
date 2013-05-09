#include "FeatureManager.h"

using std::vector;
using namespace NeuroProof;
#ifdef SETPYTHON
using namespace boost::python;
#endif 

// ?! assume every feature is on every channel -- for now

void FeatureMgr::add_channel()
{
    if (specified_features) {
        throw ErrMsg("Cannot add a channel after adding features");
    }

    vector<FeatureCompute*> features_temp;
    vector<vector<bool> > features_modes_temp;   
 
    channels_features.push_back(features_temp);
    channels_features_equal.push_back(features_temp);
    channels_features_modes.push_back(features_modes_temp);

    ++num_channels;
}

void FeatureMgr::compute_diff_features(std::vector<void*>* caches1, std::vector<void*>* caches2, std::vector<double>& feature_results, RagEdge<Label>* edge)
{
    vector<vector<bool> > examine_equal(num_channels);
    vector<vector<unsigned int> > spot_equal(num_channels);

    // !! stupid hack to put features in the 'correct' order
    unsigned int pos = 0;
    for (int j = 0; j < num_channels; ++j) {
        unsigned int count = 0;
        for (int i = 0; i < channels_features_equal[0].size(); ++i) {
            spot_equal[j].push_back(pos);
            if (channels_features_equal[j][i]) {
                ++pos;
                if (channels_features_modes[j][count][2]) {
                    examine_equal[j].push_back(true);
                } else {
                    examine_equal[j].push_back(false);
                } 
                ++count;
            } else {
                examine_equal[j].push_back(false);
            }
        }
    }

    for (int i = 0; i < channels_features_equal[0].size(); ++i) {
        for (int j = 0; j < num_channels; ++j) {
            if (examine_equal[j][i]) {
                unsigned int id = spot_equal[j][i];
                channels_features_equal[j][i]->get_diff_feature_array((*caches2)[id], (*caches1)[id], feature_results, edge);
            }
        }
    }
} 

void FeatureMgr::compute_diff_features2(std::vector<void*>* caches1, std::vector<void*>* caches2, std::vector<double>& feature_results, RagEdge<Label>* edge)
{
    unsigned int pos = 0;
    for (unsigned int i = 0; i < num_channels; i++) {
        std::vector<FeatureCompute*>& features = channels_features[i];
        for (int j = 0; j < features.size(); j++) {
            features[j]->get_diff_feature_array((*caches2)[pos],(*caches1)[pos],feature_results, edge);
            pos++;
        } 
    }
}


void FeatureMgr::compute_features2(unsigned int prediction_type, std::vector<void*>* caches, std::vector<double>& feature_results, RagEdge<Label>* edge, unsigned int node_number)
{

    unsigned int pos = 0;
    for (unsigned int i = 0; i < num_channels; i++) {
        std::vector<FeatureCompute*>& features = channels_features[i];
        for (int j = 0; j < features.size(); j++) {
            features[j]->get_feature_array((*caches)[pos],feature_results, edge, node_number);
            pos++;
        } 
    }

}

void FeatureMgr::compute_features(unsigned int prediction_type, std::vector<void*>* caches, std::vector<double>& feature_results, RagEdge<Label>* edge, unsigned int node_number)
{
    vector<vector<bool> > examine_equal(num_channels);
    vector<vector<unsigned int> > spot_equal(num_channels);

    // !! stupid hack to put features in the 'correct' order
    unsigned int pos = 0;
    for (int j = 0; j < num_channels; ++j) {
        unsigned int count = 0;
        for (int i = 0; i < channels_features_equal[0].size(); ++i) {
            spot_equal[j].push_back(pos);
            if (channels_features_equal[j][i]) {
                ++pos;
                if (channels_features_modes[j][count][prediction_type]) {
                    examine_equal[j].push_back(true);
                } else {
                    examine_equal[j].push_back(false);
                } 
                ++count;
            } else {
                examine_equal[j].push_back(false);
            }
        }
    }

    for (int i = 0; i < channels_features_equal[0].size(); ++i) {
        for (int j = 0; j < num_channels; ++j) {
            if (examine_equal[j][i]) {
                unsigned int id = spot_equal[j][i];
                channels_features_equal[j][i]->get_feature_array((*caches)[id],
                        feature_results, edge, node_number);
            }
        }
    }
} 


void FeatureMgr::add_median_feature()
{
    if (num_channels != 1) {
        throw ErrMsg("Median filter can only be added if there is one channel");
    }

    vector<bool> feature_modes(3, false);
    feature_modes[1] = true;
    std::vector<double> percentiles;
    percentiles.push_back(0.5);

    FeatureCompute * feature_ptr = new FeatureHist(100, percentiles);
    channels_features_equal[0].push_back(feature_ptr);
    add_feature(0, feature_ptr, feature_modes);
}

#ifdef SETPYTHON
void FeatureMgr::add_hist_feature(unsigned int num_bins, boost::python::list percentiles, bool use_diff)
{
    vector<bool> feature_modes(3, true);
    feature_modes[2] = use_diff;
    
    unsigned int num_percentiles = len(percentiles);

    vector<double> percentiles_vec;

    for (unsigned int i = 0; i < num_percentiles; ++i) {
        percentiles_vec.push_back(extract<double>(percentiles[i]));
    }
    FeatureCompute * feature_ptr = new FeatureHist(num_bins, percentiles_vec);
    for (unsigned int i = 0; i < num_channels; ++i) {
        channels_features_equal[i].push_back(feature_ptr);
        add_feature(i, feature_ptr, feature_modes);
    }
}
#else
void FeatureMgr::add_hist_feature(unsigned int num_bins, vector<double> percentiles, bool use_diff)
{
    vector<bool> feature_modes(3, true);
    feature_modes[2] = use_diff;
    

    FeatureCompute * feature_ptr = new FeatureHist(num_bins, percentiles);
    for (unsigned int i = 0; i < num_channels; ++i) {
        channels_features_equal[i].push_back(feature_ptr);
        add_feature(i, feature_ptr, feature_modes);
    }
}
#endif

void FeatureMgr::add_moment_feature(unsigned int num_moments, bool use_diff)
{
    vector<bool> feature_modes(3, true);
    feature_modes[2] = use_diff;

    FeatureCompute * feature_ptr0 = new FeatureCount;
    add_feature(0, feature_ptr0, feature_modes);
    channels_features_equal[0].push_back(feature_ptr0);
    for (unsigned int i = 1; i < num_channels; ++i) {
        channels_features_equal[i].push_back(0);
    }
    FeatureCompute * feature_ptr = new FeatureMoment(num_moments);
    for (unsigned int i = 0; i < num_channels; ++i) {
        channels_features_equal[i].push_back(feature_ptr);
        add_feature(i, feature_ptr, feature_modes);
    }
}

void FeatureMgr::add_inclusiveness_feature(bool use_diff)
{
    vector<bool> feature_modes(3, true);
    feature_modes[2] = use_diff;
 
    FeatureInclusiveness * feature_ptr0 = new FeatureInclusiveness;
    add_feature(0, feature_ptr0, feature_modes);
    channels_features_equal[0].push_back(feature_ptr0);
    for (unsigned int i = 1; i < num_channels; ++i) {
        channels_features_equal[i].push_back(0);
    }
}

void FeatureMgr::add_feature(unsigned int channel, FeatureCompute * feature, vector<bool>& feature_modes)
{
    specified_features = true; 
    assert(channel < num_channels);
    ++num_features;

    channels_features[channel].push_back(feature); 
    channels_features_modes[channel].push_back(feature_modes);
}

#ifdef SETPYTHON

void FeatureMgr::set_python_rf_function(object pyfunc_)
{
    pyfunc = pyfunc_;
    has_pyfunc = true;
}

#endif

void FeatureMgr::compute_node_features(RagNode<Label>* node, vector<double>& feature_results){

    std::vector<void*>* node1_caches = 0;


    if (node_caches.find(node) != node_caches.end()) {
        node1_caches = &(node_caches[node]);
    }

    compute_features2(0, node1_caches, feature_results, NULL, 1);

}

void FeatureMgr::compute_all_features(RagEdge<Label>* edge, vector<double>& feature_results){

    std::vector<void*>* edget_caches = 0;
    std::vector<void*>* node1_caches = 0;
    std::vector<void*>* node2_caches = 0;

    if (edge_caches.find(edge) != edge_caches.end()) {
        edget_caches = &(edge_caches[edge]);
    }

    RagNode<Label>* node1 = edge->get_node1();
    RagNode<Label>* node2 = edge->get_node2();

    if (node2->get_size() < node1->get_size()) {
        RagNode<Label>* temp_node = node2;
        node2 = node1;
        node1 = temp_node;
    }

    if (node_caches.find(node1) != node_caches.end()) {
        node1_caches = &(node_caches[node1]);
    }
    if (node_caches.find(node2) != node_caches.end()) {
        node2_caches = &(node_caches[node2]);
    }

    compute_features2(0, node1_caches, feature_results, edge, 1);

    compute_features2(0, node2_caches, feature_results, edge, 2);

    compute_features2(1, edget_caches, feature_results, edge, 0);

    compute_diff_features2(node1_caches, node2_caches, feature_results, edge);


}

void FeatureMgr::get_responses(RagEdge<Label>* edge, vector<double>& responses){
	
    vector<double> feature_results;
    
    compute_all_features(edge,feature_results);	

    eclfr->get_tree_responses(feature_results, responses);
}



void FeatureMgr::set_overlap_function()
{
    overlap = true;
}

double FeatureMgr::get_prob(RagEdge<Label>* edge)
{
    vector<double> feature_results;
    RagNode<Label>* node1 = edge->get_node1();
    RagNode<Label>* node2 = edge->get_node2();

#ifdef SETPYTHON
    std::vector<void*>* edget_caches = 0;
    std::vector<void*>* node1_caches = 0;
    std::vector<void*>* node2_caches = 0;

    if (edge_caches.find(edge) != edge_caches.end()) {
        edget_caches = &(edge_caches[edge]);
    }

    if (node2->get_size() < node1->get_size()) {
        RagNode<Label>* temp_node = node2;
        node2 = node1;
        node1 = temp_node;
    }

    if (node_caches.find(node1) != node_caches.end()) {
        node1_caches = &(node_caches[node1]);
    }
    if (node_caches.find(node2) != node_caches.end()) {
        node2_caches = &(node_caches[node2]);
    }
    
    compute_features(0, node1_caches, feature_results, edge, 1);
    compute_features(0, node2_caches, feature_results, edge, 2);
    compute_features(1, edget_caches, feature_results, edge, 0);
    compute_diff_features(node1_caches, node2_caches, feature_results, edge);
#else
    compute_all_features(edge,feature_results);
#endif

    /*std::cout << node1->get_node_id() << " " << node2->get_node_id() << std::endl;
    for (int i = 0; i < feature_results.size(); ++i) {
        std::cout << feature_results[i] << " ";
    }*/
    double prob = 0.0;
    if (has_pyfunc) {
#ifdef SETPYTHON
        /*
        object get_iter = iterator<vector<double> >();
        object iter = get_iter(feature_results);
        list pylist(iter);
*/
        boost::python::list pylist;
        for (unsigned int i = 0; i < feature_results.size(); ++i) {
            pylist.append(feature_results[i]);
        }
        prob = extract<double>(pyfunc(pylist));
#endif
    } else if (eclfr){
        prob = eclfr->predict(feature_results);
    } else if (overlap) {
        unsigned long long edge_size = edge->get_size();
        unsigned long long total_edge_size1 = 0;
        unsigned long long total_edge_size2 = 0;

        for (RagNode<Label>::edge_iterator eiter = node1->edge_begin(); eiter != node1->edge_end(); ++eiter) {
            if (!((*eiter)->is_preserve() || (*eiter)->is_false_edge())) {
                total_edge_size1 += (*eiter)->get_size();
            }
        }
        for (RagNode<Label>::edge_iterator eiter = node2->edge_begin(); eiter != node2->edge_end(); ++eiter) {
            if (!((*eiter)->is_preserve() || (*eiter)->is_false_edge())) {
                total_edge_size2 += (*eiter)->get_size();
            }
        }
        double prob1 = edge_size / double(total_edge_size1);
        double prob2 = edge_size / double(total_edge_size2);
        prob = prob1;
        if (overlap_max && (prob2 > prob)) {
            prob = prob2;
        } else if (!overlap_max && (prob2 < prob)) {
            prob = prob2;
        }
        if (edge_size < overlap_threshold) {
            prob = 0.0;
        }

        prob = 1-prob;
    } else {
        prob = feature_results[0];
    }
//    std::cout << prob << std::endl;

    return prob;
}

void FeatureMgr::merge_features2(RagNode<Label>* node1, RagNode<Label>* node2, RagEdge<Label>* edgeb)
{
    std::vector<void*>* node1_caches = 0; 
    std::vector<void*>* node2_caches = 0;
    std::vector<void*>* edgeb_caches = 0;

	
    node1->incr_size(node2->get_size()+edgeb->get_size());
    node1->incr_border_size(node2->get_border_size());

    if (node_caches.find(node1) != node_caches.end()) {
        node1_caches = &(node_caches[node1]);
    }
    if (node_caches.find(node2) != node_caches.end()) {
        node2_caches = &(node_caches[node2]);
    }
    if (edge_caches.find(edgeb) != edge_caches.end()) {
        edgeb_caches = &(edge_caches[edgeb]);
    }
    if (!node1_caches && !node2_caches) {
        return;
    }

    unsigned int pos = 0;
    vector<double> feature_results;
    for (int i = 0; i < num_channels; ++i) {
        vector<FeatureCompute*>& features = channels_features[i];
        for (int j = 0; j < features.size(); ++j) {
            if ((*node1_caches)[pos] && (*node2_caches)[pos]) {
                features[j]->merge_cache((*node1_caches)[pos], (*node2_caches)[pos]);
            }
	    if (edgeb_caches && (*edgeb_caches)[pos] && (*node1_caches)[pos])
                features[j]->merge_cache((*node1_caches)[pos], (*edgeb_caches)[pos]);
	
            ++pos;
        }
    }

    if (node2_caches) {
        node_caches.erase(node2);
    }
    if (edgeb_caches)
	edge_caches.erase(edgeb);	
}



void FeatureMgr::merge_features(RagNode<Label>* node1, RagNode<Label>* node2)
{
    std::vector<void*>* node1_caches = 0; 
    std::vector<void*>* node2_caches = 0;
    node1->incr_size(node2->get_size());
    node1->incr_border_size(node2->get_border_size());

    if (node_caches.find(node1) != node_caches.end()) {
        node1_caches = &(node_caches[node1]);
    }
    if (node_caches.find(node2) != node_caches.end()) {
        node2_caches = &(node_caches[node2]);
    }
    if (!node1_caches && !node2_caches) {
        return;
    }

    unsigned int pos = 0;
    vector<double> feature_results;
    for (int i = 0; i < num_channels; ++i) {
        vector<FeatureCompute*>& features = channels_features[i];
        for (int j = 0; j < features.size(); ++j) {
            if ((*node1_caches)[pos] && (*node2_caches)[pos]) {
                features[j]->merge_cache((*node1_caches)[pos], (*node2_caches)[pos]);
            }
            ++pos;
        }
    }

    if (node2_caches) {
        node_caches.erase(node2);
    }
}

void FeatureMgr::merge_features(RagEdge<Label>* edge1, RagEdge<Label>* edge2)
{
    std::vector<void*>* edge1_caches = 0; 
    std::vector<void*>* edge2_caches = 0;
    
    edge1->incr_size(edge2->get_size());
 
    if (edge_caches.find(edge1) != edge_caches.end()) {
        edge1_caches = &(edge_caches[edge1]);
    }
    if (edge_caches.find(edge2) != edge_caches.end()) {
        edge2_caches = &(edge_caches[edge2]);
    }

    unsigned int pos = 0;
    vector<double> feature_results;
    for (int i = 0; i < num_channels; ++i) {
        vector<FeatureCompute*>& features = channels_features[i];
        for (int j = 0; j < features.size(); ++j) {
            if ((*edge1_caches)[pos] && (*edge2_caches)[pos]) {
                features[j]->merge_cache((*edge1_caches)[pos], (*edge2_caches)[pos]);
            }
            ++pos;
        }
    }

    if (edge2_caches) {
        edge_caches.erase(edge2);
    }
}

void FeatureMgr::copy_channel_features(FeatureMgr *pfmgr){

    std::vector<std::vector<FeatureCompute*> >& pfmgr_channel_features = pfmgr->get_channel_features();

    num_channels = pfmgr->get_num_channels();		

    channels_features.resize(pfmgr_channel_features.size()); 	
    for (unsigned int i = 0; i < num_channels; ++i) {
        channels_features[i].resize(pfmgr_channel_features[i].size());
	for (int j = 0; j < channels_features[i].size(); ++j) {
	    channels_features[i][j] = pfmgr_channel_features[i][j];
        } 
    }

}

void FeatureMgr::copy_cache(std::vector<void*>& src_edge_caches, RagEdge<Label>* edge){
    

    bool cache_exists=false;	
    if (edge_caches.find(edge) != edge_caches.end()) {
        cache_exists=true;
    }
    else
	edge_caches[edge] = std::vector<void*>();		

    std::vector<void*>& dest_edge_caches = edge_caches[edge]; 

    unsigned int pos = 0;
    for (unsigned int i = 0; i < num_channels; ++i) {
        std::vector<FeatureCompute*>& features = channels_features[i];
        for (int j = 0; j < features.size(); ++j) {
	    if (cache_exists){
		features[j]->delete_cache(dest_edge_caches[pos]);
		dest_edge_caches[pos] = features[j]->create_cache(); 
	    }	
	    else	
                dest_edge_caches.push_back(features[j]->create_cache());

	    features[j]->copy_cache(src_edge_caches[pos],dest_edge_caches[pos]);	
            ++pos;
        } 
    }

}	

void FeatureMgr::copy_cache(std::vector<void*>& src_node_caches , RagNode<Label>* node1){
    

    bool cache_exists=false;	
    if (node_caches.find(node1) != node_caches.end()) {
        cache_exists=true;
    }
    else	
        node_caches[node1] = std::vector<void*>();

    std::vector<void*>& dest_node_caches = node_caches[node1]; 
		
	

    unsigned int pos = 0;
    for (unsigned int i = 0; i < num_channels; ++i) {
        std::vector<FeatureCompute*>& features = channels_features[i];
        for (int j = 0; j < features.size(); ++j) {
	    if (cache_exists){	
 	        features[j]->delete_cache(dest_node_caches[pos]);
		dest_node_caches[pos] = features[j]->create_cache();
	    }	
	    else	
 	        dest_node_caches.push_back(features[j]->create_cache());

	    features[j]->copy_cache(src_node_caches[pos],dest_node_caches[pos]);	
            ++pos;
        } 
    }

}	


void FeatureMgr::print_cache(RagEdge<Label>* edge){

    std::vector<void*>& dest_edge_caches = edge_caches[edge]; 

    unsigned int pos = 0;
    for (unsigned int i = 0; i < num_channels; ++i) {
        std::vector<FeatureCompute*>& features = channels_features[i];
        for (int j = 0; j < features.size(); ++j) {
	    features[j]->print_name();		
	    features[j]->print_cache(dest_edge_caches[pos]);	
            ++pos;
        } 
    }

}	
void FeatureMgr::print_cache(RagNode<Label>* node){

    std::vector<void*>& dest_node_caches = node_caches[node]; 

    unsigned int pos = 0;
    for (unsigned int i = 0; i < num_channels; ++i) {
        std::vector<FeatureCompute*>& features = channels_features[i];
        for (int j = 0; j < features.size(); ++j) {
	    features[j]->print_name();		
	    features[j]->print_cache(dest_node_caches[pos]);	
            ++pos;
        } 
    }

}

void FeatureMgr::clear_features()
{
    for (EdgeCaches::iterator iter = edge_caches.begin(); iter != edge_caches.end(); ++iter) {
        // creation of empty feature
        if (iter->second.size() == 0) {
            continue;
        }
        unsigned int pos = 0;
        for (int i = 0; i < num_channels; ++i) {
            vector<FeatureCompute*>& features = channels_features[i];
            for (int j = 0; j < features.size(); ++j) {
                features[j]->delete_cache(iter->second[pos]);
                ++pos;
            } 
        }
    }
    edge_caches.clear();

    for (NodeCaches::iterator iter = node_caches.begin(); iter != node_caches.end(); ++iter) {
        // creation of empty feature
        if (iter->second.size() == 0) {
            continue;
        }
        unsigned int pos = 0;
        for (int i = 0; i < num_channels; ++i) {
            vector<FeatureCompute*>& features = channels_features[i];
            for (int j = 0; j < features.size(); ++j) {
                features[j]->delete_cache(iter->second[pos]);
                ++pos;
            } 
        }
    }
    node_caches.clear();
}

FeatureMgr::~FeatureMgr()
{
    clear_features();
}


