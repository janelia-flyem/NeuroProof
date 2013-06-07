#include "VolumeLabelData.h"
#include "VolumeData.h"
#include <iostream>
#include <vector>

using namespace std;
using namespace NeuroProof;

int main(int argc, char**argv)
{
    VolumeLabelPtr labels = VolumeLabelData::create_volume(argv[1], "stack");
    vector<VolumeProbPtr> preds = VolumeProb::create_volume_array(argv[2], "volume/predictions");

    cout << "Pred channels: " << preds.size() << endl;
    cout << "Label shape: " << labels->shape(0) << " " << labels->shape(1) << 
        " " << labels->shape(2) << endl;
    cout << "Pred shape: " << preds[0]->shape(0) << " " << preds[1]->shape(1) << 
        " " << preds[2]->shape(2) << endl;

    return 0;
}
