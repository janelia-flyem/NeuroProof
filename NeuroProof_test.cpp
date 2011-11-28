#include "DataStructures/Rag.h"
#include "Priority/GPR.h"
#include "Priority/LocalEdgePriority.h"
#include "Utilities/ScopeTime.h"
#include "ImportsExports/ImportExportRagPriority.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include <iostream>

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
#ifdef OLDPARSE
        Rag<Label> rag;
        ifstream fin(argv[1]);
        while (fin.getline(buffer, 1000000)) {
            stringstream strstream(buffer);
            unsigned int node_id;
            unsigned int connect_node;
            double prob;
            strstream >> node_id;
            assert(strstream);
            while (strstream >> connect_node) {
                strstream >> prob;
                assert(strstream);

                RagNode<unsigned int>* head_ragnode;
                RagNode<unsigned int>* connect_ragnode;
                RagEdge<unsigned int>* ragedge;

                head_ragnode = rag.find_rag_node(node_id);
                if (!head_ragnode) {
                    head_ragnode = rag.insert_rag_node(node_id);
                    head_ragnode->set_size(1);
                }

                connect_ragnode = rag.find_rag_node((unsigned int)(connect_node));
                if (!connect_ragnode) {
                    connect_ragnode = rag.insert_rag_node((unsigned int)(connect_node));
                    connect_ragnode->set_size(1);
                }

                // construct rag -- set fake volume, set ids as the ints
                if (!rag.find_rag_edge(head_ragnode, connect_ragnode)) {
                    ragedge = rag.insert_rag_edge(head_ragnode, connect_ragnode);
                    ragedge->set_weight(1.0 - prob);
                }
            }
        }
#endif

        Rag<Label>* rag = create_rag_from_jsonfile(argv[1]);
        if (!rag) {
            throw ErrMsg("Rag could not be created");
        }
        create_jsonfile_from_rag(rag, "temp.json");

        GPR<Label> gpr_metric(*rag, false);
        double gpr = 0.0;
        
        {
        ScopeTime timer;
        gpr = gpr_metric.calculateGPR(num_paths, num_threads);
        }

        cout << "Start GPR: " << gpr << endl;
     
        {
            ScopeTime timer;
            cout << "Number of edges: " << rag->get_num_edges() << endl;
            LocalEdgePriority<Label> priority_scheduler(*rag, 0.1, 0.9, 0.5);
            priority_scheduler.updatePriority();
            int num_edges_examined = 0;

            while (!priority_scheduler.isFinished()) {
                EdgePriority<Label>::Location location;
                boost::tuple<Label, Label> pair = priority_scheduler.getTopEdge(location);
                priority_scheduler.setEdge(pair, 0);
                ++num_edges_examined;
            }

            cout << "Num operations: " << num_edges_examined << endl;
            cout << "Percent edges examined: " << double(num_edges_examined) / rag->get_num_edges() * 100 << endl;
            double time_elapsed = double(timer.getElapsed());  
            cout << "Time per operation: " << time_elapsed/num_edges_examined << " seconds" << endl;
        }
        GPR<Label> gpr_metric2(*rag, false);
        double gpr2 = gpr_metric2.calculateGPR(num_paths, num_threads);
        cout << "Finish GPR: " << gpr2 << endl;
    } catch (ErrMsg& msg) {
        cerr << msg.str << endl;
        exit(-1);
    }

#ifdef OLDPARSE
    fin.close();
#endif

    return 0;
}
