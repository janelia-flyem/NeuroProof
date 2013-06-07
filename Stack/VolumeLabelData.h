#ifndef VOLUMELABELDATA_H
#define VOLUMELABELDATA_H

#include "VolumeData.h"
#include <vector>
#include <tr1/unordered_map>

namespace NeuroProof {

class VolumeLabelData;

typedef unsigned int Label_t;
typedef boost::shared_ptr<VolumeLabelData> VolumeLabelPtr; 

class VolumeLabelData : public VolumeData<Label_t> {
  public:
    static VolumeLabelPtr create_volume();
    static VolumeLabelPtr create_volume(const char * h5_name, const char* dset);

    void reassign_label(Label_t old_label, Label_t new_label); 
    void rebase_labels();

    Label_t operator()(unsigned int x, unsigned int y, unsigned int z);

  private:
    VolumeLabelData() : VolumeData<Label_t>() {}

    std::tr1::unordered_map<Label_t, Label_t> label_mapping;
    std::tr1::unordered_map<Label_t, std::vector<Label_t> > label_remapping_history;

};

}

#endif
