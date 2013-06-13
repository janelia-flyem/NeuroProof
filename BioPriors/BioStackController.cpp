#include "BioStackController.h"
#include "MitoTypeProperty.h"
#include "../Refactoring/RagUtils2.h"

#include <json/value.h>
#include <json/reader.h>
#include <vector>

using std::tr1::unordered_set;
using std::tr1::unordered_map;

namespace NeuroProof {

VolumeLabelPtr BioStackController::create_syn_label_volume()
{
    VolumeLabelPtr labelvol = biostack->get_labelvol();

    if (!labelvol) {
        throw ErrMsg("No label volume defined for stack");
    }

    return create_syn_volume(labelvol);
}

VolumeLabelPtr BioStackController::create_syn_gt_label_volume()
{
    VolumeLabelPtr gt_labelvol = biostack->get_gt_labelvol();

    if (!gt_labelvol) {
        throw ErrMsg("No GT label volume defined for stack");
    }

    return create_syn_volume(gt_labelvol);
}

VolumeLabelPtr BioStackController::create_syn_volume(VolumeLabelPtr labelvol)
{
    vector<Label_t> labels;
    vector<vector<unsigned int> > synapse_locations = biostack->get_synapse_locations();

    for (int i = 0; i < synapse_locations.size(); ++i) {
        Label_t label = (*labelvol)(synapse_locations[i][0],
            synapse_locations[i][1], synapse_locations[i][2]); 
        labels.push_back(label);
    }
    
    VolumeLabelPtr synvol = VolumeLabelData::create_volume();   
    synvol->reshape(VolumeLabelData::difference_type((labels.size(), 1, 1)));

    for (int i = 0; i < labels.size(); ++i) {
        synvol->set(i, 0, 0, labels[i]);  
    }

    return synvol;
}

void BioStackController::build_rag_mito()
{
    FeatureMgrPtr feature_mgr = biostack->get_feature_manager();
    vector<VolumeProbPtr> prob_list = biostack->get_prob_list();
    VolumeLabelPtr labelvol = biostack->get_labelvol();

    if (!labelvol) {
        throw ErrMsg("No label volume defined for stack");
    }

    Rag_uit * rag = new Rag_uit;

    vector<double> predictions(prob_list.size(), 0.0);
    unordered_set<Label> labels;
   
    unsigned int maxx = get_xsize() - 1; 
    unsigned int maxy = get_ysize() - 1; 
    unsigned int maxz = get_zsize() - 1; 
    unordered_map<Label_t, MitoTypeProperty> mito_probs;
 
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
        mito_probs[label].update(predictions); 

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
   
    for (Rag_uit::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
        Label_t id = (*iter)->get_node_id();
        MitoTypeProperty mtype = mito_probs[id];
        mtype.set_type(); 
        (*iter)->set_property("mito-type", mtype);
    }

    biostack->set_rag(RagPtr(rag));

}


void BioStackController::set_synapse_exclusions(const char* synapse_json)
{
    RagPtr rag = biostack->get_rag();
    VolumeLabelPtr labelvol = biostack->get_labelvol();

    if (!rag) {
        throw ErrMsg("No RAG defined for stack");
    }

    std::vector<std::vector<unsigned int> > synapse_locations; 
    
    Json::Reader json_reader;
    Json::Value json_reader_vals;
    
    ifstream fin(synapse_json);
    if (!fin) {
        throw ErrMsg("Error: input file: " + string(synapse_json) + " cannot be opened");
    }
    if (!json_reader.parse(fin, json_reader_vals)) {
        throw ErrMsg("Error: Json incorrectly formatted");
    }
    fin.close();
 
    Json::Value synapses = json_reader_vals["data"];

    for (int i = 0; i < synapses.size(); ++i) {
        vector<vector<unsigned int> > locations;
        Json::Value location = synapses[i]["T-bar"]["location"];
        if (!location.empty()) {
            vector<unsigned int> loc;
            loc.push_back(location[(unsigned int)(0)].asUInt());
            loc.push_back(location[(unsigned int)(1)].asUInt());
            loc.push_back(location[(unsigned int)(2)].asUInt());
            synapse_locations.push_back(loc);
            locations.push_back(loc);
        }
        Json::Value psds = synapses[i]["partners"];
        for (int i = 0; i < psds.size(); ++i) {
            Json::Value location = psds[i]["location"];
            if (!location.empty()) {
                vector<unsigned int> loc;
                loc.push_back(location[(unsigned int)(0)].asUInt());
                loc.push_back(location[(unsigned int)(1)].asUInt());
                loc.push_back(location[(unsigned int)(2)].asUInt());
                synapse_locations.push_back(loc);
                locations.push_back(loc);
            }
        }

        for (int iter1 = 0; iter1 < locations.size(); ++iter1) {
            for (int iter2 = (iter1 + 1); iter2 < locations.size(); ++iter2) {
                add_edge_constraint(rag, labelvol, locations[iter1][0], locations[iter1][1],
                    locations[iter1][2], locations[iter2][0], locations[iter2][1], locations[iter2][2]);           
            }
        }
    }

    biostack->set_synapse_locations(synapse_locations);
}

void BioStackController::add_edge_constraint(RagPtr rag, VolumeLabelPtr labelvol, unsigned int x1,
        unsigned int y1, unsigned int z1, unsigned int x2, unsigned int y2, unsigned int z2)
{
    Label_t label1 = (*labelvol)(x1,y1,z1);
    Label_t label2 = (*labelvol)(x2,y2,z2);

    if (label1 && label2 && (label1 != label2)) {
        RagEdge_uit* edge = rag->find_rag_edge(label1, label2);
        if (!edge) {
            RagNode_uit* node1 = rag->find_rag_node(label1);
            RagNode_uit* node2 = rag->find_rag_node(label2);
            edge = rag->insert_rag_edge(node1, node2);
            edge->set_weight(1.0);
            edge->set_false_edge(true);
        }
        edge->set_preserve(true);
    }
}

}
