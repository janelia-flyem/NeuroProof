#include "../Stack/StackPlaneController.h"
#include "../Stack/VolumeData.h"
#include "../Stack/VolumeLabelData.h"
#include "../Stack/Stack.h"
#include "../Stack/StackSession.h"

#include <QApplication>
#include <string>
#include <iostream>

using namespace NeuroProof;
using std::vector;
using std::string;
using std::cout; using std::endl;

int main(int argc, char** argv)
{
    QApplication qapp(argc, argv);
    
    // create a simple stack
    VolumeLabelPtr initial_labels = VolumeLabelData::create_volume(
            "/home/plazas/NeuroProof/examples/validation_sample/results/segmentation.h5", "stack");
    cout << "Loaded labels" << endl;
    Stack stack(initial_labels);

    // load grayscale
    vector<string> gray_images;
    const char* filebase = 
        "/home/plazas/NeuroProof/examples/validation_sample/grayscale_maps/iso.%05d.png";
    char buf[100];
    for (int i = 3490; i <= 4009; ++i) {
        sprintf(buf, filebase, i);
        gray_images.push_back(string(buf));
    }
    VolumeGrayPtr gray_data = VolumeGray::create_volume_from_images(gray_images);
    stack.set_grayvol(gray_data);
    cout << "Loaded gray" << endl;

    stack.build_rag();
    cout << "Built Rag" << endl;

    StackSession session(&stack);
    StackPlaneController controller(&session); 
    controller.initialize(); 
    cout << "Initialized controller" << endl;
    controller.start(); 
    
    qapp.exec();

    return 0;
}
