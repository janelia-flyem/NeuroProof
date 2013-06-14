#include "StackController.h"
#include "../Rag/RagUtils.h"
#include "../Refactoring/RagUtils2.h"
#include "../Algorithms/FeatureJoinAlgs.h"

#include <vigra/multi_morphology.hxx>

using std::vector;
using std::tr1::unordered_set;
using std::tr1::unordered_map;

namespace NeuroProof {

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
    unordered_set<Label> labels;
   
    unsigned int maxx = get_xsize() - 1; 
    unsigned int maxy = get_ysize() - 1; 
    unsigned int maxz = get_zsize() - 1; 
 
    volume_forXYZ(*labelvol, x, y, z) {
        Label_t label = (*labelvol)(x,y,z); 
        
        if (!label) {
            continue;
        }

        RagNode_uit * node = rag->find_rag_node(label);

        if (!node) {
            node =  rag->insert_rag_node(label); 
        }
        node->incr_size();
                
        for (unsigned int i = 0; i < prob_list.size(); ++i) {
            predictions[i] = (*(prob_list[i]))(x,y,z);
        }
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

        if (!label2 || !label3 || !label4 || !label5 || !label6 || !label7) {
            node->incr_boundary_size();
        }
        labels.clear();
    }
    
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
            for (unordered_set<Label>::iterator iter = merge_nodes.begin();
                    iter != merge_nodes.end(); ++iter) {
                Node_uit node2 = *iter;

                RagNode_uit* rag_node = rag->find_rag_node(node2);
                assert(rag_node);
                if (articulation_node != rag_node) {
                    for (RagNode_uit::edge_iterator edge_iter = rag_node->edge_begin();
                            edge_iter != rag_node->edge_end(); ++edge_iter) {
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

            for (unordered_set<Label>::iterator iter = merge_nodes.begin();
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


VolumeLabelPtr StackController::dilate_label_edges(VolumeLabelPtr labelvol, int disc_size)
{
    if (disc_size < 0) {
        throw ErrMsg("Incorrect disc size specified");
    }

    if (!labelvol) {
        throw ErrMsg("No label volume defined for stack");
    }

    labelvol = generate_boundary(labelvol);
    
    if (disc_size > 0) {
        VolumeLabelPtr labelmask = VolumeLabelData::create_volume();
        labelmask->reshape(labelvol->shape());
        
        vigra::multiBinaryErosion(srcMultiArrayRange(*labelvol),
                destMultiArray(*labelmask), disc_size); 

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

    if (labelvol) {
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
        unordered_map<Label, vector<LabelCount> >::iterator mit = 
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


void StackController::compute_vi(double& merge, double& split, 
        multimap<double, Label_t>& label_ranked, multimap<double, Label_t>& gt_ranked)
{
    VolumeLabelPtr labelvol = stack->get_labelvol();
    VolumeLabelPtr gt_labelvol = stack->get_gt_labelvol();

    if (labelvol) {
        throw ErrMsg("No label volume available to compute VI");
    }
    if (!gt_labelvol) {
        throw ErrMsg("No GT volume available to compute VI");
    }

    if (!updated) {
        // ?!
        compute_contingency_table();
        updated = true;
    }

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
            Label gtlabel = gt_vec[j].lbl;

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

    unordered_map<Label, double> seg_overmerge;
    unordered_map<Label, double> gt_overmerge;

    for(unordered_map<Label_t, vector<LabelCount> >::iterator mit = contingency.begin();
            mit != contingency.end(); ++mit){
        Label i = mit->first;
        vector<LabelCount>& gt_vec = mit->second; 	
        for (int j=0; j< gt_vec.size();j++){
            unsigned int count = gt_vec[j].count;
            Label gtlabel = gt_vec[j].lbl;

            double p_wg = count/sum_all;
            double p_w = wp.find(i)->second/sum_all; 	

            HgivenW += p_wg* log(p_w/p_wg)/log(2.0);
            seg_overmerge[i] += p_wg* log(p_w/p_wg)/log(2.0);

            double p_g = gp.find(gtlabel)->second/sum_all;	

            HgivenG += p_wg * log(p_g/p_wg)/log(2.0);	
            gt_overmerge[gtlabel] += p_wg * log(p_g/p_wg)/log(2.0);
        }
    }

    for (std::tr1::unordered_map<Label, double>::iterator iter = seg_overmerge.begin();
            iter != seg_overmerge.end(); ++iter) {
        label_ranked.insert(make_pair(iter->second, iter->first));
    }
    for (std::tr1::unordered_map<Label, double>::iterator iter = gt_overmerge.begin();
            iter != gt_overmerge.end(); ++iter) {
        gt_ranked.insert(make_pair(iter->second, iter->first));
    }
    merge = HgivenW;
    split = HgivenG;
}

// retain label2 
void StackController::merge_labels(Label_t label1, Label_t label2,
        RagNodeCombineAlg* combine_alg)
{
    updated = false;
    RagPtr rag = stack->get_rag();
    if (rag) {
        rag_join_nodes(*rag, rag->find_rag_node(label2),
            rag->find_rag_node(label1), combine_alg);  
    } 
    
    VolumeLabelPtr labelvol = stack->get_labelvol(); 

    if (!labelvol) {
        throw ErrMsg("No label volume defined for stack");
    }

    labelvol->reassign_label(label1, label2); 
}

VolumeLabelPtr StackController::generate_boundary(VolumeLabelPtr labelvol)
{
    VolumeLabelPtr labelvol_new = VolumeLabelData::create_volume();
    
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



