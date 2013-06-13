#ifndef BIOSTACK_H
#define BIOSTACK_H

#include "../Stack/Stack.h"

namespace NeuroProof {

class BioStack : public Stack2 {
  public:
    BioStack(VolumeLabelPtr labels_) : Stack2(labels_) {}

    std::vector<std::vector<unsigned int> >& get_synapse_locations()
    {
        return synapse_locations;
    }
    
    void set_synapse_locations(
        std::vector<std::vector<unsigned int> >& synapse_locations_)
    {
        synapse_locations = synapse_locations_;
    }

  private:
    std::vector<std::vector<unsigned int> > synapse_locations; 
};



}


#endif
