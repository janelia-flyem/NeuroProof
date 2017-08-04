#include "VolumeLabelData.h"
#include <unordered_set>

using namespace NeuroProof;
using std::vector;
using std::unordered_set;

VolumeLabelPtr VolumeLabelData::create_volume()
{
    return VolumeLabelPtr(new VolumeLabelData); 
}

VolumeLabelPtr VolumeLabelData::create_volume(int xsize, int ysize, int zsize)
{
    VolumeLabelData* volumedata = new VolumeLabelData;
    vigra::TinyVector<long long unsigned int,3> shape(xsize, ysize, zsize);
    volumedata->reshape(shape);
    
    return VolumeLabelPtr(volumedata); 
}


void VolumeLabelData::get_label_history(Label_t label, std::vector<Label_t>& member_labels)
{
    member_labels = label_remapping_history[label];
}

void VolumeLabelData::reassign_label(Label_t old_label, Label_t new_label)
{
    // do not allow label reassignment unless the stack was originally rebased
    // all stacks are read in rebased anyway so this should never execute
    assert(label_mapping.find(old_label) == label_mapping.end());

    label_mapping[old_label] = new_label;

    for (std::vector<Label_t>::iterator iter = label_remapping_history[old_label].begin();
            iter != label_remapping_history[old_label].end(); ++iter) {
        label_mapping[*iter] = new_label;
    }

    // update the mappings of all labels previously mapped to the
    // old label
    label_remapping_history[new_label].push_back(old_label);
    label_remapping_history[new_label].insert(label_remapping_history[new_label].end(),
            label_remapping_history[old_label].begin(), label_remapping_history[old_label].end());
    label_remapping_history.erase(old_label);
} 


void VolumeLabelData::split_labels(Label_t curr_label, vector<Label_t>& split_labels)
{
    vector<Label_t>::iterator split_iter = split_labels.begin();
    ++split_iter;
    label_mapping.erase(split_labels[0]);

    for (; split_iter != split_labels.end(); ++split_iter) {
        label_mapping[*split_iter] = split_labels[0];
        label_remapping_history[(split_labels[0])].push_back(*split_iter);
    }

    unordered_set<Label_t> split_labels_set;
    for (int i = 0; i < split_labels.size(); ++i) {
        split_labels_set.insert(split_labels[i]);
    }
    
    vector<Label_t> base_labels;
    for (std::vector<Label_t>::iterator iter = label_remapping_history[curr_label].begin();
            iter != label_remapping_history[curr_label].end(); ++iter) {
        if (split_labels_set.find(*iter) == split_labels_set.end()) {
            base_labels.push_back(*iter);
        } 
    }
    label_remapping_history[curr_label] = base_labels;
}

void VolumeLabelData::rebase_labels()
{
    if (!label_mapping.empty()) {
        // linear pass throw entire volume if remappings have occured
        for (VolumeLabelData::iterator iter = this->begin(); iter != this->end(); ++iter) {
            if (label_mapping.find(*iter) != label_mapping.end()) {
                *iter = label_mapping[*iter];
            }
        }
    }
    label_remapping_history.clear();
    label_mapping.clear();
}



