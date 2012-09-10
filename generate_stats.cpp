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
using std::vector;

int run_estimator(LocalEdgePriority<Label>& priority_scheduler, Rag<Label>* rag)
{
    int num_edges_examined = 0;
    while (!priority_scheduler.isFinished()) {
        EdgePriority<Label>::Location location;
        boost::tuple<Label, Label> pair = priority_scheduler.getTopEdge(location);
        Label node1 = boost::get<0>(pair);
        Label node2 = boost::get<1>(pair);
        RagEdge<Label>* temp_edge = rag->find_rag_edge(node1, node2);
        double weight = temp_edge->get_weight();
        int weightint = int(100 * weight);
        if ((rand() % 100) > (weightint)) {
            priority_scheduler.removeEdge(pair, true);
        } else {
            priority_scheduler.removeEdge(pair, false);
        }
        ++num_edges_examined;
    }
    return num_edges_examined;
}

int main(int argc, char** argv) 
{
    if (argc == 1 || argc > 4) {
        cerr << "Usage: prog <json_file> <opt: nm rez (default 10)> <rand seed (default 1)>" << endl;
        exit(-1);
    }

    char buffer[1000000];
    double ignore_size = 25000.0;
    if (argc > 2) {
        int rez = atoi(argv[2]);
        double incr = 10.0/rez;
        ignore_size *= (incr * incr * incr);
    }
    int seed = 1;
    if (argc > 3) {
        seed = atoi(argv[3]);
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
        
        srand(seed);
        {
            ScopeTime timer;
            cout << "Number of edges: " << rag->get_num_edges() << endl;
            cout << "Number of nodes: " << rag->get_num_regions() << endl;
            LocalEdgePriority<Label> priority_scheduler(*rag, 0.1, 0.9, 0.1, json_vals);
        
            vector<Label> violators = priority_scheduler.getQAViolators(ignore_size);
            cout << "Num QA violators: " << violators.size() << endl; 
        
            priority_scheduler.set_body_mode(ignore_size, 0);
            int num_bodyedges_examined = run_estimator(priority_scheduler, rag);
            cout << "Estimated num body operations: " << num_bodyedges_examined << endl;
          
            int total_undos = 0; 
            while (priority_scheduler.undo()) {
                ++total_undos;
            }
            assert(total_undos == num_bodyedges_examined);

            priority_scheduler.set_synapse_mode(0.1);
            int num_synapseedges_examined = run_estimator(priority_scheduler, rag);
            cout << "Estimated num synapse operations: " << num_synapseedges_examined << endl;
           
            total_undos = 0; 
            while (priority_scheduler.undo()) {
                ++total_undos;
            }
            assert(total_undos == num_synapseedges_examined);

            priority_scheduler.set_orphan_mode(ignore_size, 0, 0);
            int num_orphanedges_examined = run_estimator(priority_scheduler, rag);
            cout << "Estimated num orphan operations: " << num_orphanedges_examined << endl;

            total_undos = 0; 
            while (priority_scheduler.undo()) {
                ++total_undos;
            }
            assert(total_undos == num_orphanedges_examined);

            int num_edges_examined = num_bodyedges_examined + num_orphanedges_examined + num_synapseedges_examined;
            double time_elapsed = double(timer.getElapsed());  
            cout << "Estimated num total operations (hopefully an upper bound): " << num_edges_examined << endl;
            //cout << "Time per operation: " << time_elapsed/num_edges_examined << " seconds" << endl;
        }

    } catch (ErrMsg& msg) {
        cerr << msg.str << endl;
        exit(-1);
    }

    return 0;
}
