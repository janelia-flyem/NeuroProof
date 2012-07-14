#include "DataStructures/Rag.h"
#include "Priority/GPR.h"
#include "Priority/LocalEdgePriority.h"
#include "Utilities/ScopeTime.h"
#include "ImportsExports/ImportExportRagPriority.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include <iostream>
#include <json/json.h>
#include <json/value.h>

using std::cerr; using std::cout; using std::endl;
using std::ifstream;
using std::string;
using std::stringstream;
using namespace NeuroProof;

int main(int argc, char** argv) 
{
    if (argc == 1 || argc > 4) {
        cerr << "Usage: prog <json_file> <opt: num_threads> <opt: num_paths>" << endl;
        exit(-1);
    }

    char buffer[1000000];
    int num_threads = 1;
    int num_paths = 1;
    if (argc > 2) {
        num_threads = atoi(argv[2]);
    }
    if (argc > 3) {
        num_paths = atoi(argv[3]);
    }
    
    try {
        ifstream fin(argv[1]);
        Json::Reader json_reader;
        Json::Value json_vals;
        if (!json_reader.parse(fin, json_vals)) {
            throw ErrMsg("Error: Json incorrectly formatted");
        }
        fin.close();


        Rag<Label>* rag = create_rag_from_json(json_vals);
        if (!rag) {
            throw ErrMsg("Rag could not be created");
        }
        
        {
            ScopeTime timer;
            cout << "Number of edges: " << rag->get_num_edges() << endl;
            cout << "Number of nodes: " << rag->get_num_regions() << endl;
                    
            GPR<Label> gpr_metric2(*rag, false);
            double gpr2 = gpr_metric2.calculateGPR(num_paths, num_threads);
            cout << "Finish GPR: " << gpr2 << endl;
        }
    } catch (ErrMsg& msg) {
        cerr << msg.str << endl;
        exit(-1);
    }

    return 0;
}
