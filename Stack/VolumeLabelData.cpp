#include "VolumeLabelData.h"

using namespace NeuroProof;

VolumeLabelPtr VolumeLabelData::create_volume()
{
    return VolumeLabelPtr(new VolumeLabelData); 
}

VolumeLabelPtr VolumeLabelData::create_volume(
        const char * h5_name, const char * dset)

{
    vigra::HDF5ImportInfo info(h5_name, dset);
    vigra_precondition(info.numDimensions() == 3, "Dataset must be 3-dimensional.");

    VolumeLabelData* volumedata = new VolumeLabelData;
    vigra::TinyVector<long long unsigned int,3> shape(info.shape().begin());
    volumedata->reshape(shape);
    vigra::readHDF5(info, *volumedata);

    try {
        vigra::HDF5ImportInfo info(h5_name, "transforms");
        vigra::TinyVector<long long unsigned int,2> tshape(info.shape().begin()); 
        vigra::MultiArray<2,Label_t> transforms(tshape);
        
        for (int row = 0; row < transforms.shape(0); ++row) {
            volumedata->label_mapping[transforms(row, 0)] = transforms(row,1);
        }
        volumedata->rebase_labels();
    } catch (std::runtime_error& err) {
    }

    return VolumeLabelPtr(volumedata); 
}

void VolumeLabelData::reassign_label(Label_t old_label, Label_t new_label)
{
    label_mapping[old_label] = new_label;

    for (std::vector<Label_t>::iterator iter = label_remapping_history[old_label].begin();
            iter != label_remapping_history[old_label].end(); ++iter) {
        label_mapping[*iter] = new_label;
    }

    label_remapping_history[new_label].push_back(old_label);
    label_remapping_history[new_label].insert(label_remapping_history[new_label].end(),
            label_remapping_history[old_label].begin(), label_remapping_history[old_label].end());
    label_remapping_history.erase(old_label);
} 

void VolumeLabelData::rebase_labels()
{
    if (!label_mapping.empty()) {
        for (VolumeLabelData::iterator iter = this->begin(); iter != this->end(); ++iter) {
            if (label_mapping.find(*iter) != label_mapping.end()) {
                *iter = label_mapping[*iter];
            }
        }
    }
    label_remapping_history.clear();
    label_mapping.clear();
}



