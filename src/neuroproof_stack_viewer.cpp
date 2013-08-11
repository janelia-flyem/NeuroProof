#include "../Utilities/OptionParser.h"
#include "../Stack/StackSession.h"
#include "../Gui/StackQTController.h"

#include <QApplication>
#include <string>
#include <iostream>

using namespace NeuroProof;
using std::string;
using std::cout; using std::cerr; using std::endl;
using std::vector;

struct ViewerOptions
{
    ViewerOptions(int argc, char** argv) 
    {
        OptionParser parser("Tool for visualization segmentation and training agglomeration");

        parser.add_option(stack_volume, "stack-volume",
                "h5 file of 8bit grayscale (z,y,z) (dataset=gray) and label volume (z,y,z) (dataset=stack)"); 
        parser.add_option(session_name, "session-name",
                "Name of previously created NeuroProofStackView session directory");

        parser.parse_options(argc, argv);
    }

    // optional (with default values) 
    string stack_volume;
    string session_name;
};

int main(int argc, char** argv)
{
    ViewerOptions options(argc, argv);
    QApplication qapp(argc, argv);
    
    StackSession* stack_session = 0;
    bool created_session = true;

    if (options.session_name != "") {
        try {
             stack_session = new StackSession(options.session_name);
        } catch (ErrMsg& msg) {
            cerr << "Session could not be created: " << 
                options.session_name << endl;
        }

    } else if ((options.stack_volume != "")) { 
        try {
            stack_session = new StackSession(options.stack_volume);
        } catch (ErrMsg& msg) {
            cerr << "Session could not be created from: " <<
                options.stack_volume << endl;
        }
    }
    
    StackQTController controller(stack_session);

    qapp.exec();
    delete stack_session;

    return 0;

#if 0
    vector<string> gray_images;
    const char* filebase = 
        "/home/plazas/NeuroProof/examples/validation_sample/grayscale_maps/iso.%05d.png";
    char buf[100];
    for (int i = 3490; i <= 4009; ++i) {
        sprintf(buf, filebase, i);
        gray_images.push_back(string(buf));
    }

    StackSession temp(gray_images,
            string("/home/plazas/NeuroProof/examples/validation_sample/results/segmentation.h5"));
    temp.export_session("/home/plazas/neuroproof_session");
#endif


}
