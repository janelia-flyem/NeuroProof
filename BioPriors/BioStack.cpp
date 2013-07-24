#include "../FeatureManager/FeatureMgr.h"
#include "BioStack.h"
#include "MitoTypeProperty.h"

#include <json/value.h>
#include <json/reader.h>
#include <vector>

using std::tr1::unordered_set;
using std::tr1::unordered_map;
using std::vector;

namespace NeuroProof {

VolumeLabelPtr BioStack::create_syn_label_volume()
{
    if (!labelvol) {
        throw ErrMsg("No label volume defined for stack");
    }

    return create_syn_volume(labelvol);
}

VolumeLabelPtr BioStack::create_syn_gt_label_volume()
{
    if (!gt_labelvol) {
        throw ErrMsg("No GT label volume defined for stack");
    }

    return create_syn_volume(gt_labelvol);
}

VolumeLabelPtr BioStack::create_syn_volume(VolumeLabelPtr labelvol2)
{
    vector<Label_t> labels;

    for (int i = 0; i < synapse_locations.size(); ++i) {
        Label_t label = (*labelvol2)(synapse_locations[i][0],
            synapse_locations[i][1], synapse_locations[i][2]); 
        labels.push_back(label);
    }
    
    VolumeLabelPtr synvol = VolumeLabelData::create_volume();   
    synvol->reshape(VolumeLabelData::difference_type(labels.size(), 1, 1));

    for (int i = 0; i < labels.size(); ++i) {
        synvol->set(i, 0, 0, labels[i]);  
    }
    return synvol;
}

void BioStack::load_synapse_counts(unordered_map<Label_t, int>& synapse_counts)
{
    for (int i = 0; i < synapse_locations.size(); ++i) {
        Label_t body_id = (*labelvol)(synapse_locations[i][0],
                synapse_locations[i][1], synapse_locations[i][2]);

        if (body_id) {
            synapse_counts[body_id]++;
        }
    }
}

void BioStack::load_synapse_labels(unordered_set<Label_t>& synapse_labels)
{
    for (int i = 0; i < synapse_locations.size(); ++i) {
        Label_t body_id = (*labelvol)(synapse_locations[i][0],
                synapse_locations[i][1], synapse_locations[i][2]);
        synapse_labels.insert(body_id);
    }
}

bool BioStack::is_mito(Label_t label)
{
    RagNode_t* rag_node = rag->find_rag_node(label);

    MitoTypeProperty mtype;
    try {
        mtype = rag_node->get_property<MitoTypeProperty>("mito-type");
    } catch (ErrMsg& msg) {
    }

    if ((mtype.get_node_type()==2)) {	
        return true;
    }
    return false;
}

void BioStack::build_rag_mito()
{
    if (!labelvol) {
        throw ErrMsg("No label volume defined for stack");
    }

    rag = RagPtr(new Rag_t);

    vector<double> predictions(prob_list.size(), 0.0);
    unordered_set<Label_t> labels;
   
    unsigned int maxx = get_xsize() - 1; 
    unsigned int maxy = get_ysize() - 1; 
    unsigned int maxz = get_zsize() - 1; 
    unordered_map<Label_t, MitoTypeProperty> mito_probs;
 
    volume_forXYZ(*labelvol, x, y, z) {
        Label_t label = (*labelvol)(x,y,z); 
        
        if (!label) {
            continue;
        }

        RagNode_t * node = rag->find_rag_node(label);

        if (!node) {
            node =  rag->insert_rag_node(label); 
        }
        node->incr_size();
                
        for (unsigned int i = 0; i < prob_list.size(); ++i) {
            predictions[i] = (*(prob_list[i]))(x,y,z);
        }
        if (feature_manager) {
            feature_manager->add_val(predictions, node);
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
            rag_add_edge(label, label2, predictions);
            labels.insert(label2);
        }
        if (label3 && (label != label3) && (labels.find(label3) == labels.end())) {
            rag_add_edge(label, label3, predictions);
            labels.insert(label3);
        }
        if (label4 && (label != label4) && (labels.find(label4) == labels.end())) {
            rag_add_edge(label, label4, predictions);
            labels.insert(label4);
        }
        if (label5 && (label != label5) && (labels.find(label5) == labels.end())) {
            rag_add_edge(label, label5, predictions);
            labels.insert(label5);
        }
        if (label6 && (label != label6) && (labels.find(label6) == labels.end())) {
            rag_add_edge(label, label6, predictions);
            labels.insert(label6);
        }
        if (label7 && (label != label7) && (labels.find(label7) == labels.end())) {
            rag_add_edge(label, label7, predictions);
        }

        if (!label2 || !label3 || !label4 || !label5 || !label6 || !label7) {
            node->incr_boundary_size();
        }
        labels.clear();
    }
   
    for (Rag_t::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
        Label_t id = (*iter)->get_node_id();
        MitoTypeProperty mtype = mito_probs[id];
        mtype.set_type(); 
        (*iter)->set_property("mito-type", mtype);
    }
}


void BioStack::set_synapse_exclusions(const char* synapse_json)
{
    unsigned int ysize = labelvol->shape(1);

    if (!rag) {
        throw ErrMsg("No RAG defined for stack");
    }

    synapse_locations.clear();

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
            loc.push_back(ysize - location[(unsigned int)(1)].asUInt() - 1);
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
                loc.push_back(ysize - location[(unsigned int)(1)].asUInt() - 1);
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

}
    
void BioStack::serialize_graph_info(Json::Value& json_writer)
{
    unordered_map<Label_t, int> synapse_counts;
    load_synapse_counts(synapse_counts);
    int id = 0;
    for (unordered_map<Label_t, int>::iterator iter = synapse_counts.begin();
            iter != synapse_counts.end(); ++iter, ++id) {
        Json::Value synapse_pair;
        synapse_pair[(unsigned int)(0)] = iter->first;
        synapse_pair[(unsigned int)(1)] = iter->second;
        json_writer["synapse_bodies"][id] =  synapse_pair;
    }
}

void BioStack::add_edge_constraint(RagPtr rag, VolumeLabelPtr labelvol2, unsigned int x1,
        unsigned int y1, unsigned int z1, unsigned int x2, unsigned int y2, unsigned int z2)
{
    Label_t label1 = (*labelvol2)(x1,y1,z1);
    Label_t label2 = (*labelvol2)(x2,y2,z2);

    if (label1 && label2 && (label1 != label2)) {
        RagEdge_t* edge = rag->find_rag_edge(label1, label2);
        if (!edge) {
            RagNode_t* node1 = rag->find_rag_node(label1);
            RagNode_t* node2 = rag->find_rag_node(label2);
            edge = rag->insert_rag_edge(node1, node2);
            edge->set_weight(1.0);
            edge->set_false_edge(true);
        }
        edge->set_preserve(true);
    }
}

}
