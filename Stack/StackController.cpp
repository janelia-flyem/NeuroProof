#include "../FeatureManager/FeatureManager.h"
#include "StackController.h"
#include "../Rag/RagUtils.h"
#include "../Refactoring/RagUtils2.h"
#include "../Algorithms/FeatureJoinAlgs.h"
#include "../Rag/RagIO.h"

#include <fstream>

// needed for erosion/dilation algorithms
#include <vigra/multi_morphology.hxx>

// needed for seeded watershed
#include <vigra/seededregiongrowing3d.hxx>


using std::vector;
using std::tr1::unordered_set;
using std::tr1::unordered_map;

namespace NeuroProof {

// assume all label volumes are written to "stack" for now
static const char * SEG_DATASET_NAME = "stack";

void StackController::build_rag()
{
    FeatureMgrPtr feature_mgr = stack->get_feature_manager();
    vector<VolumeProbPtr> prob_list = stack->get_prob_list();
    VolumeLabelPtr labelvol = stack->get_labelvol();

    if (!labelvol) {
        throw ErrMsg("No label volume defined for stack");
    }

    Rag_uit * rag = new Rag_uit;

    vector<double> predictions(prob_list.size(), 0.0);
    unordered_set<Label_t> labels;
   
    unsigned int maxx = get_xsize() - 1; 
    unsigned int maxy = get_ysize() - 1; 
    unsigned int maxz = get_zsize() - 1; 
 
    volume_forXYZ(*labelvol, x, y, z) {
        Label_t label = (*labelvol)(x,y,z); 
        
        if (!label) {
            continue;
        }

        RagNode_uit * node = rag->find_rag_node(label);

        // create node
        if (!node) {
            node =  rag->insert_rag_node(label); 
        }
        node->incr_size();
        
        // load all prediction values for a given x,y,z 
        for (unsigned int i = 0; i < prob_list.size(); ++i) {
            predictions[i] = (*(prob_list[i]))(x,y,z);
        }

        // add array of features/predictions for a given node
        if (feature_mgr) {
            feature_mgr->add_val(predictions, node);
        }

        Label_t label2 = 0, label3 = 0, label4 = 0, label5 = 0, label6 = 0, label7 = 0;
        if (x > 0) label2 = (*labelvol)(x-1,y,z);
        if (x < maxx) label3 = (*labelvol)(x+1,y,z);
        if (y > 0) label4 = (*labelvol)(x,y-1,z);
        if (y < maxy) label5 = (*labelvol)(x,y+1,z);
        if (z > 0) label6 = (*labelvol)(x,y,z-1);
        if (z < maxz) label7 = (*labelvol)(x,y,z+1);

        // if it is not a 0 label and is different from the current label, add edge
        if (label2 && (label != label2)) {
            rag_add_edge(rag, label, label2, predictions, feature_mgr.get());
            labels.insert(label2);
        }
        if (label3 && (label != label3) && (labels.find(label3) == labels.end())) {
            rag_add_edge(rag, label, label3, predictions, feature_mgr.get());
            labels.insert(label3);
        }
        if (label4 && (label != label4) && (labels.find(label4) == labels.end())) {
            rag_add_edge(rag, label, label4, predictions, feature_mgr.get());
            labels.insert(label4);
        }
        if (label5 && (label != label5) && (labels.find(label5) == labels.end())) {
            rag_add_edge(rag, label, label5, predictions, feature_mgr.get());
            labels.insert(label5);
        }
        if (label6 && (label != label6) && (labels.find(label6) == labels.end())) {
            rag_add_edge(rag, label, label6, predictions, feature_mgr.get());
            labels.insert(label6);
        }
        if (label7 && (label != label7) && (labels.find(label7) == labels.end())) {
            rag_add_edge(rag, label, label7, predictions, feature_mgr.get());
        }

        // if it is on the border of the image, increase the boundary size
        if (!label2 || !label3 || !label4 || !label5 || !label6 || !label7) {
            node->incr_boundary_size();
        }
        labels.clear();
    }
   
    // load the new rag 
    stack->set_rag(RagPtr(rag));
}

int StackController::remove_inclusions()
{
    int num_removed = 0;

    RagPtr rag = stack->get_rag();
    vector<vector<OrderedPair> > biconnected_components; 

    find_biconnected_components(rag, biconnected_components); 
    
    FeatureMgrPtr feature_mgr = stack->get_feature_manager();
    
    FeatureCombine node_combine_alg(feature_mgr.get(), rag.get()); 

    // merge nodes in biconnected_components (ignore components with '0' node)
    for (int i = 0; i < biconnected_components.size(); ++i) {
        bool found_zero = false;
        unordered_set<Label_t> merge_nodes;
        for (int j = 0; j < biconnected_components[i].size()-1; ++j) {
            Node_uit node1 = biconnected_components[i][j].region1;     
            Node_uit node2 = biconnected_components[i][j].region2;
            if (!node1 || !node2) {
                found_zero = true;
            }
            assert(node1 != node2);

            merge_nodes.insert(node1);
            merge_nodes.insert(node2);
        }
        if (!found_zero) {
            Node_uit articulation_label =
                biconnected_components[i][biconnected_components[i].size()-1].region1;
            RagNode_uit* articulation_node = rag->find_rag_node(articulation_label);

            bool found_preserve = false; 
            for (unordered_set<Label_t>::iterator iter = merge_nodes.begin();
                    iter != merge_nodes.end(); ++iter) {
                Node_uit node2 = *iter;

                RagNode_uit* rag_node = rag->find_rag_node(node2);
                assert(rag_node);
                if (articulation_node != rag_node) {
                    for (RagNode_uit::edge_iterator edge_iter = rag_node->edge_begin();
                            edge_iter != rag_node->edge_end(); ++edge_iter) {
                        // if the edge is protected than don't remove component
                        if ((*edge_iter)->is_preserve()) {
                            found_preserve = true;
                            break;
                        }
                    } 
                }
                if (found_preserve) {
                    break;
                }
            }

            if (found_preserve) {
                continue;
            }

            // reassign labels in component to be removed
            for (unordered_set<Label_t>::iterator iter = merge_nodes.begin();
                    iter != merge_nodes.end(); ++iter) {
                Node_uit node2 = *iter;
                RagNode_uit* rag_node = rag->find_rag_node(node2);
                if (articulation_node != rag_node) {
                    RagNode_uit::node_iterator n_iter = rag_node->node_begin();
                    Node_uit node1 = (*n_iter)->get_node_id();
                    merge_labels(node2, node1, &node_combine_alg);
                }
            }
        }
    }
    return num_removed;
}

void StackController::dilate_gt_labelvol(int disc_size)
{
    VolumeLabelPtr gtvol = stack->get_gt_labelvol(); 
    gtvol = dilate_label_edges(gtvol, disc_size);
    stack->set_gt_labelvol(gtvol);
}

void StackController::dilate_labelvol(int disc_size)
{
    VolumeLabelPtr labelvol = stack->get_labelvol(); 
    labelvol = dilate_label_edges(labelvol, disc_size);
    stack->set_labelvol(labelvol);
}

int StackController::remove_small_regions(int threshold,
        unordered_set<Label_t>& exclusions)
{
    VolumeProbPtr empty_pred;
    return absorb_small_regions(empty_pred, threshold, exclusions);
}

int StackController::absorb_small_regions(VolumeProbPtr boundary_pred,
            int threshold, unordered_set<Label_t>& exclusions)
{
    VolumeLabelPtr labelvol = stack->get_labelvol(); 
    
    std::tr1::unordered_map<Label_t, unsigned long long> regions_sz;
    labelvol->rebase_labels();

    for (VolumeLabelData::iterator iter = labelvol->begin();
            iter != labelvol->end(); ++iter) {
        regions_sz[*iter]++;
    }

    int num_removed = 0;    
    unordered_set<Label_t> small_regions;

    for(unordered_map<Label_t, unsigned long long>::iterator it = regions_sz.begin();
                it != regions_sz.end(); it++) {
        // do not remove small bodies that are in the exclusions set
	if ((exclusions.find(it->first) == exclusions.end()) &&
                ((it->second) < threshold)) {
	    small_regions.insert(it->first);
	    ++num_removed;	
        }
    }

    for (VolumeLabelData::iterator iter = labelvol->begin();
            iter != labelvol->end(); ++iter) {
	if (small_regions.find(*iter) != small_regions.end()) {
	    *iter = 0;
        }
    }    
    
    // if a boundary volume is provided, perform a seeded watershed
    if (boundary_pred) {    
        vigra::ArrayOfRegionStatistics<vigra::SeedRgDirectValueFunctor<double> > stats;
        vigra::seededRegionGrowing3D(srcMultiArrayRange(*boundary_pred), destMultiArray(*labelvol),
                destMultiArray(*labelvol), stats);

        stack->set_rag(RagPtr());
    }

    return num_removed;
}

// TODO: keep gt rag with gt_labelvol 
int StackController::match_regions_overlap(Label_t label,
        unordered_set<Label_t>& candidate_regions, RagPtr gt_rag,
        unordered_set<Label_t>& labels_matched, unordered_set<Label_t>& gtlabels_matched)
{
    RagPtr rag = stack->get_rag();
    unsigned long long seg_size = rag->find_rag_node(label)->get_size();
    int matched = 0; 
    
    unordered_map<Label_t, vector<LabelCount> >::iterator mit = contingency.find(label);
    
    if (mit != contingency.end()) {
        vector<LabelCount>& gt_vec = mit->second;
        for (int j=0; j< gt_vec.size();j++){
            if (candidate_regions.find(gt_vec[j].lbl) != candidate_regions.end()) {
                unsigned long long gt_size = gt_rag->find_rag_node(gt_vec[j].lbl)->get_size();

                // use 50% overlap threshold to find a match
                // can match multiple gt labels with label
                if ((gt_vec[j].count / double(gt_size)) > 0.5) {
                    gtlabels_matched.insert(gt_vec[j].lbl);
                    labels_matched.insert(label);
                    ++matched;
                } else if ((gt_vec[j].count / double(seg_size)) > 0.5) {
                    gtlabels_matched.insert(gt_vec[j].lbl);
                    labels_matched.insert(label);
                    ++matched;
                }
            }
        }
    }

    return matched;
}




void StackController::compute_groundtruth_assignment()
{
    VolumeLabelPtr gtvol = stack->get_gt_labelvol(); 

    // recompute contigency table
    compute_contingency_table();
    assignment.clear();  	
    
    for(unordered_map<Label_t, vector<LabelCount> >::iterator mit = contingency.begin();
            mit != contingency.end(); ++mit){
        Label_t i = mit->first;
        vector<LabelCount>& gt_vec = mit->second; 	
        size_t max_count=0;
        Label_t max_label=0;
        for (unsigned int j=0; j < gt_vec.size(); ++j){
            // assign based to gt body with maximum overlap
            if (gt_vec[j].count > max_count){
                max_count = gt_vec[j].count;
                max_label = gt_vec[j].lbl;
            }	
        }
        assignment[i] = max_label;
    }	
}

int StackController::find_edge_label(Label_t label1, Label_t label2)
{
    int edge_label = 0;
    Label_t label1gt = assignment[label1];
    Label_t label2gt = assignment[label2];

    if (!label1gt){
        cerr << "no grountruth node " << label1 << endl;
    }  	
    if (!label2gt){
        cerr << "no grountruth node " << label2 << endl;
    }  	

    // -1 is merge, 1 is no merge
    edge_label = (label1gt == label2gt)? -1 : 1;
        
    return edge_label;	
}

void StackController::serialize_stack(const char* h5_name, const char* graph_name, 
        bool optimal_prob_edge_loc)
{
    serialize_graph(graph_name, optimal_prob_edge_loc);
    serialize_labels(h5_name);
}

void StackController::serialize_graph(const char* graph_name, bool optimal_prob_edge_loc)
{
    EdgeCount best_edge_z;
    EdgeLoc best_edge_loc;
    determine_edge_locations(best_edge_z, best_edge_loc, optimal_prob_edge_loc);
    
    RagPtr rag = stack->get_rag();
    FeatureMgrPtr feature_mgr = stack->get_feature_manager();
    
    // set edge properties for export 
    for (Rag_uit::edges_iterator iter = rag->edges_begin();
           iter != rag->edges_end(); ++iter) {
        if (!((*iter)->is_false_edge())) {
            double val = feature_mgr->get_prob((*iter));
            (*iter)->set_weight(val);
        }
        Label_t x = 0;
        Label_t y = 0;
        Label_t z = 0;
        
        if (best_edge_loc.find(*iter) != best_edge_loc.end()) {
            Location loc = best_edge_loc[*iter];
            x = boost::get<0>(loc);
            // assume y is bottom of image
            // (technically ignored by raveler so okay)
            y = boost::get<1>(loc); //height - boost::get<1>(loc) - 1;
            z = boost::get<2>(loc);
        }
        
        (*iter)->set_property("location", Location(x,y,z));
    }

    Json::Value json_writer;
    bool status = create_json_from_rag(rag.get(), json_writer, false);
    if (!status) {
        throw ErrMsg("Error in rag export");
    }

    // biopriors might write specific information -- calls derived function
    serialize_graph_info(json_writer);

    int id = 0;
    for (Rag_uit::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
        if (!((*iter)->is_boundary())) {
            json_writer["orphan_bodies"][id] = (*iter)->get_node_id();
            ++id;
        } 
    }
    
    // write out graph json
    ofstream fout(graph_name);
    if (!fout) {
        throw ErrMsg("Error: output file " + string(graph_name) + " could not be opened");
    }
    
    fout << json_writer;
    fout.close();
}

void StackController::serialize_labels(const char* h5_name)
{
    VolumeLabelPtr final_labels = stack->get_labelvol(); 
    final_labels->rebase_labels();
    final_labels->serialize(h5_name, SEG_DATASET_NAME);
}


VolumeLabelPtr StackController::dilate_label_edges(VolumeLabelPtr labelvol, int disc_size)
{
    if (disc_size <= 0) {
        throw ErrMsg("Incorrect disc size specified");
    }

    if (!labelvol) {
        throw ErrMsg("No label volume defined for stack");
    }
    // disc size of 1 creates initial boundaries
    labelvol = generate_boundary(labelvol);
    
    if (disc_size > 1) {
        VolumeLabelPtr labelmask = VolumeLabelData::create_volume();
        labelmask->reshape(labelvol->shape());
        
        vigra::multiBinaryErosion(srcMultiArrayRange(*labelvol),
                destMultiArray(*labelmask), disc_size-1); 

        (*labelvol) *= (*labelmask);
    }

    return labelvol;
}

void StackController::compute_vi(double& merge, double& split)
{
    multimap<double, Label_t> label_ranked;
    multimap<double, Label_t> gt_ranked;
    compute_vi(merge, split, label_ranked, gt_ranked);
}

void StackController::compute_contingency_table()
{
    VolumeLabelPtr labelvol = stack->get_labelvol();
    VolumeLabelPtr gt_labelvol = stack->get_gt_labelvol();
    if (!labelvol) {
        throw ErrMsg("No label volume available to compute VI");
    }
    if (!gt_labelvol) {
        throw ErrMsg("No GT volume available to compute VI");
    }

    contingency.clear();	
   
    volume_forXYZ(*labelvol,x,y,z) {
        Label_t wlabel = (*labelvol)(x,y,z);
        Label_t glabel = (*gt_labelvol)(x,y,z);

        if (!wlabel || !glabel) {
            continue;
        }
        unordered_map<Label_t, vector<LabelCount> >::iterator mit = 
                contingency.find(wlabel);
        if (mit != contingency.end()){
            vector<LabelCount>& gt_vec = mit->second;
            int j;
            for (j=0; j< gt_vec.size(); ++j) {
                if (gt_vec[j].lbl == glabel){
                    (gt_vec[j].count)++;
                    break;
                }
            }
            if (j==gt_vec.size()){
                LabelCount lc(glabel,1);
                gt_vec.push_back(lc);	
            }
        } else{
            vector<LabelCount> gt_vec;	
            gt_vec.push_back(LabelCount(glabel,1));	
            contingency.insert(make_pair(wlabel, gt_vec));	
        }
    }		
}


void StackController::determine_edge_locations(EdgeCount& best_edge_z,
        EdgeLoc& best_edge_loc, bool use_probs)
{
    VolumeLabelPtr labelvol = stack->get_labelvol();
    vector<VolumeProbPtr> prob_list = stack->get_prob_list();
    RagPtr rag = stack->get_rag(); 
    
    unsigned int maxx = get_xsize() - 1; 
    unsigned int maxy = get_ysize() - 1; 
    unsigned int maxz = get_zsize() - 1; 

    // find z plane with the most edge points or edge probability points
    for (unsigned int z = 0; z < get_zsize(); ++z) {
        EdgeCount curr_edge_z;
        EdgeLoc curr_edge_loc;
        
        for (unsigned int y = 0; y < get_ysize(); ++y) {
            for (unsigned int x = 0; x < get_xsize(); ++x) {
                Label_t label = (*labelvol)(x,y,z); 
                if (!label) {
                    continue;
                }

                Label_t label2 = 0, label3 = 0, label4 = 0, label5 = 0, label6 = 0, label7 = 0;
                if (x > 0) label2 = (*labelvol)(x-1,y,z);
                if (x < maxx) label3 = (*labelvol)(x+1,y,z);
                if (y > 0) label4 = (*labelvol)(x,y-1,z);
                if (y < maxy) label5 = (*labelvol)(x,y+1,z);
                if (z > 0) label6 = (*labelvol)(x,y,z-1);
                if (z < maxz) label7 = (*labelvol)(x,y,z+1);


                double incr = 1.0;
                if (use_probs) {
                    // pick plane with a lot of low edge probs
                    incr = 1.0 - (*(prob_list[0]))(x,y,z);;
                }

                if (label2 && (label != label2)) {
                    RagEdge_uit* edge = rag->find_rag_edge(label, label2);
                    curr_edge_z[edge] += incr;  
                    curr_edge_loc[edge] = Location(x,y,z);  
                }
                if (label3 && (label != label3)) {
                    RagEdge_uit* edge = rag->find_rag_edge(label, label3);
                    curr_edge_z[edge] += incr;  
                    curr_edge_loc[edge] = Location(x,y,z);  
                }
                if (label4 && (label != label4)) {
                    RagEdge_uit* edge = rag->find_rag_edge(label, label4);
                    curr_edge_z[edge] += incr;  
                    curr_edge_loc[edge] = Location(x,y,z);  
                }
                if (label5 && (label != label5)) {
                    RagEdge_uit* edge = rag->find_rag_edge(label, label5);
                    curr_edge_z[edge] += incr;  
                    curr_edge_loc[edge] = Location(x,y,z);  
                }
                if (label6 && (label != label6)) {
                    RagEdge_uit* edge = rag->find_rag_edge(label, label6);
                    curr_edge_z[edge] += incr;  
                    curr_edge_loc[edge] = Location(x,y,z);  
                }
                if (label7 && (label != label7)) {
                    RagEdge_uit* edge = rag->find_rag_edge(label, label7);
                    curr_edge_z[edge] += incr; 
                    curr_edge_loc[edge] = Location(x,y,z);  
                }

            }
        }
    
        for (EdgeCount::iterator iter = curr_edge_z.begin(); iter != curr_edge_z.end(); ++iter) {
            if (iter->second > best_edge_z[iter->first]) {
                best_edge_z[iter->first] = iter->second;
                best_edge_loc[iter->first] = curr_edge_loc[iter->first];
            }
        }
    }

    return;
}


void StackController::set_body_exclusions(string exclusions_json)
{
    Json::Reader json_reader;
    Json::Value json_vals;
    unordered_set<Label_t> exclusion_set;
    
    ifstream fin(exclusions_json.c_str());
    if (!fin) {
        throw ErrMsg("Error: input file: " + exclusions_json + " cannot be opened");
    }
    if (!json_reader.parse(fin, json_vals)) {
        throw ErrMsg("Error: Json incorrectly formatted");
    }
    fin.close();
    
    // all exclusions should be in a json list
    Json::Value exclusions = json_vals["exclusions"];
    for (unsigned int i = 0; i < json_vals["exclusions"].size(); ++i) {
        exclusion_set.insert(exclusions[i].asUInt());
    }

    VolumeLabelPtr labelvol = stack->get_labelvol();

    // 0 out all bodies in the exclusion list    
    volume_forXYZ(*labelvol, x, y, z) {
        Label_t label = (*labelvol)(x,y,z);
        if (exclusion_set.find(label) != exclusion_set.end()) {
            labelvol->set(x,y,z,0);
        }
    }
}

void StackController::compute_vi(double& merge, double& split, 
        multimap<double, Label_t>& label_ranked, multimap<double, Label_t>& gt_ranked)
{
    VolumeLabelPtr labelvol = stack->get_labelvol();
    VolumeLabelPtr gt_labelvol = stack->get_gt_labelvol();

    if (!labelvol) {
        throw ErrMsg("No label volume available to compute VI");
    }
    if (!gt_labelvol) {
        throw ErrMsg("No GT volume available to compute VI");
    }

    compute_contingency_table();

    double sum_all=0;

    int nn = contingency.size();

    unordered_map<Label_t, double> wp;    
    unordered_map<Label_t, double> gp;    

    for(unordered_map<Label_t, vector<LabelCount> >::iterator mit = contingency.begin();
            mit != contingency.end(); ++mit){
        Label_t i = mit->first;
        vector<LabelCount>& gt_vec = mit->second; 	
        wp.insert(make_pair(i,0.0));
        for (int j=0; j< gt_vec.size();j++){
            unsigned int count = gt_vec[j].count;
            Label_t gtlabel = gt_vec[j].lbl;

            (wp.find(i)->second) += count;	

            if(gp.find(gtlabel) != gp.end()) {
                (gp.find(gtlabel)->second) += count;
            } else {
                gp.insert(make_pair(gtlabel,count));
            }	

            sum_all += count;
        }
        int tt=1;
    }

    double HgivenW=0;
    double HgivenG=0;

    unordered_map<Label_t, double> seg_overmerge;
    unordered_map<Label_t, double> gt_overmerge;

    for(unordered_map<Label_t, vector<LabelCount> >::iterator mit = contingency.begin();
            mit != contingency.end(); ++mit){
        Label_t i = mit->first;
        vector<LabelCount>& gt_vec = mit->second; 	
        for (int j=0; j< gt_vec.size();j++){
            unsigned int count = gt_vec[j].count;
            Label_t gtlabel = gt_vec[j].lbl;

            double p_wg = count/sum_all;
            double p_w = wp.find(i)->second/sum_all; 	

            HgivenW += p_wg* log(p_w/p_wg)/log(2.0);
            seg_overmerge[i] += p_wg* log(p_w/p_wg)/log(2.0);

            double p_g = gp.find(gtlabel)->second/sum_all;	

            HgivenG += p_wg * log(p_g/p_wg)/log(2.0);	
            gt_overmerge[gtlabel] += p_wg * log(p_g/p_wg)/log(2.0);
        }
    }

    // load all labels and gt labels with the VI contribution
    for (std::tr1::unordered_map<Label_t, double>::iterator iter = seg_overmerge.begin();
            iter != seg_overmerge.end(); ++iter) {
        label_ranked.insert(make_pair(iter->second, iter->first));
    }
    for (std::tr1::unordered_map<Label_t, double>::iterator iter = gt_overmerge.begin();
            iter != gt_overmerge.end(); ++iter) {
        gt_ranked.insert(make_pair(iter->second, iter->first));
    }
    merge = HgivenW;
    split = HgivenG;
}

// might not be necessary if the update agrees with ground truth
void StackController::update_assignment(Label_t label_remove, Label_t label_keep)
{
    if (assignment.empty()) {
        return;
    }
    
    Label_t label_remove_gtlbl = assignment[label_remove];
    unsigned long long label_remove_gtoverlap = 0;

    // won't be found if label no longer exists after an image transformation
    bool found = false;
    vector<LabelCount>& gt_vecr = contingency[label_remove]; 
    for (unsigned int j = 0; j < gt_vecr.size(); ++j){
        if (gt_vecr[j].lbl == label_remove_gtlbl){
            label_remove_gtoverlap = gt_vecr[j].count;
            found = true;
            break;
        }	
    }

    found = false;
    vector<LabelCount>& gt_veck = contingency[label_keep]; 
    for (unsigned int j = 0; j < gt_veck.size(); ++j){
        if (gt_veck[j].lbl == label_remove_gtlbl){
            (gt_veck[j].count) += label_remove_gtoverlap;
            found = true;
            break;
        }	
    }
    
    if (!found){ // i.e.,  a false merge
        LabelCount lc(label_remove_gtlbl, label_remove_gtoverlap);
        gt_veck.push_back(lc);
    }

    unsigned long long max_count = 0;
    Label_t max_label = 0;
    for (unsigned int j = 0; j < gt_veck.size(); j++){
        if (gt_veck[j].count > max_count){
            max_count = gt_veck[j].count;
            max_label = gt_veck[j].lbl;
        }	
    }

    assignment[label_keep] = max_label;
}

void StackController::merge_labels(Label_t label_remove, Label_t label_keep,
        RagNodeCombineAlg* combine_alg, bool ignore_rag)
{
    // might be unnecessary, does nothing without ground truth
    update_assignment(label_remove, label_keep);
    
    // update rag by merging nodes
    RagPtr rag = stack->get_rag();
    if (!ignore_rag && rag) {
        rag_join_nodes(*rag, rag->find_rag_node(label_keep),
            rag->find_rag_node(label_remove), combine_alg);  
    } 
    
    VolumeLabelPtr labelvol = stack->get_labelvol(); 
    labelvol->reassign_label(label_remove, label_keep); 
}

VolumeLabelPtr StackController::generate_boundary(VolumeLabelPtr labelvol)
{
    VolumeLabelPtr labelvol_new = VolumeLabelData::create_volume();
    *labelvol_new = *labelvol;

    unsigned int maxx = get_xsize() - 1; 
    unsigned int maxy = get_ysize() - 1; 
    unsigned int maxz = get_zsize() - 1; 
    
    volume_forXYZ(*labelvol, x, y, z) {
        Label_t label = (*labelvol)(x,y,z); 
        if (!label) {
            continue;
        }

        Label_t label2 = 0, label3 = 0, label4 = 0, label5 = 0, label6 = 0, label7 = 0;
        
        if (x > 0) label2 = (*labelvol)(x-1,y,z);
        if (x < maxx) label3 = (*labelvol)(x+1,y,z);
        if (y > 0) label4 = (*labelvol)(x,y-1,z);
        if (y < maxy) label5 = (*labelvol)(x,y+1,z);
        if (z > 0) label6 = (*labelvol)(x,y,z-1);
        if (z < maxz) label7 = (*labelvol)(x,y,z+1);

        if (label2 && (label != label2)) {
            labelvol_new->set(x,y,z,0);
        } else if (label3 && (label != label3)) {
            labelvol_new->set(x,y,z,0);
        } else if (label4 && (label != label4)) {
            labelvol_new->set(x,y,z,0);
        } else if (label5 && (label != label5)) {
            labelvol_new->set(x,y,z,0);
        } else if (label6 && (label != label6)) {
            labelvol_new->set(x,y,z,0);
        } else if (label7 && (label != label7)) {
            labelvol_new->set(x,y,z,0);
        }   
    }

    return labelvol_new;
}

}

