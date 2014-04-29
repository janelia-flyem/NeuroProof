#ifndef BIOSTACK_H
#define BIOSTACK_H

#include "../Stack/Stack.h"
#include <string>
#include <tr1/unordered_map>
#include <tr1/unordered_set>

namespace NeuroProof {

class BioStack : public Stack {
  public:
    BioStack(VolumeLabelPtr labels_) : Stack(labels_) {}
    BioStack(std::string stack_name) : Stack(stack_name) {}
    
    void read_prob_list(std::string prob_filename, std::string dataset_name);

    VolumeLabelPtr create_syn_label_volume();
    VolumeLabelPtr create_syn_gt_label_volume();
    void set_synapse_exclusions(const char * synapse_json);    
    void load_synapse_counts(std::tr1::unordered_map<Label_t, int>& synapse_counts);
    void load_synapse_labels(std::tr1::unordered_set<Label_t>& synapse_labels);

    bool is_mito(Label_t label);
    void serialize_graph_info(Json::Value& json_writer);
    
    void set_classifier();
    void save_classifier(std::string clfr_name);
    
    void set_edge_locations();

    virtual void build_rag();

  private:
    void add_edge_constraint(RagPtr rag, VolumeLabelPtr labelvol, unsigned int x1,
            unsigned int y1, unsigned int z1, unsigned int x2, unsigned int y2, unsigned int z2);
    VolumeLabelPtr create_syn_volume(VolumeLabelPtr labelvol);

    std::vector<std::vector<unsigned int> > synapse_locations; 
};



}


#endif
