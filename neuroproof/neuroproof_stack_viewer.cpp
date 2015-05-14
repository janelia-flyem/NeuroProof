/*!
 * \file
 * The following program allows a user to view a label volume
 * and to compare this result to another ground truth.  The main
 * functionality to support this is implemented in the Gui
 * module.  This program is called from the command line and
 * can be given a previously created session.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#include "../Utilities/OptionParser.h"

// main GUI model
#include "../Stack/StackSession.h"

// main GUI controller
#include "../Gui/StackQTController.h"

#include <QApplication>
#include <string>
#include <iostream>

using namespace NeuroProof;
using std::string;
using std::cout; using std::cerr; using std::endl;
using std::vector;

/*!
 * Maintains options for calling the stack viewer.
*/
struct ViewerOptions
{
    /*!
     * Takes command line arguments and parses them
     * \param argc number of arguments
     * \param argv array of char* arguments
    */
    ViewerOptions(int argc, char** argv) 
    {
        OptionParser parser("Tool for visualization segmentation and training agglomeration");

        parser.add_option(session_name, "session-name",
                "Name of previously created NeuroProofStackView session directory");

        parser.parse_options(argc, argv);
    }

    //! optional: name of directory containing session 
    string session_name;
};

/*!
 * Entry point for program
 * \param argc number of arguments
 * \param argv array of char* arguments
 * \return status of program (0 for proper exit)
*/
int main(int argc, char** argv)
{
    ViewerOptions options(argc, argv);
    // load QT application
    QApplication qapp(argc, argv);
    
    StackSession* stack_session = 0;

    // check is there was a session specified and load session
    if (options.session_name != "") {
        try {
             stack_session = new StackSession(options.session_name);
        } catch (ErrMsg& msg) {
            cerr << "Session could not be created: " << 
                options.session_name << endl;
        }

    } 
  
    // initialize controller with previous session or empty session  
    StackQTController controller(stack_session, &qapp);

    // start gui
    qapp.exec();

    return 0;
}
