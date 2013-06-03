#ifndef MITOTYPEPROPERTY_H
#define MITOTYPEPROPERTY_H

#include <vector>

namespace NeuroProof {

class MitoTypeProperty {
  public:
    MitoTypeProperty() : node_type(0), sum_mitop(0), npixels(0), mito_channel(2) { }
    MitoTypeProperty(int mito_channel_) : node_type(0), sum_mitop(0), npixels(0),
        mito_channel(mito_channel_) { }

    void update(std::vector<double>& predictions)
    {
        double mitop = predictions[mito_channel];

        sum_mitop += mitop; 
        npixels++;
    }        
    void set_type()
    {
        double mito_pct = sum_mitop/npixels;	
        assert(mito_pct <= 1.0);

        if (mito_pct >= 0.35) {        
            node_type = 2;
        } else {
            node_type = 1;	
        }   
    }
    void set_type(int ptype)
    {
        node_type=ptype;
    }	
    int get_node_type() {return node_type;}

  private:
    int node_type;
    double sum_mitop;	
    double npixels;
    int mito_channel;    
};
}


#endif
