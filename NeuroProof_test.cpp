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
        
#if 0
        create_jsonfile_from_rag(rag, "temp.json");

        GPR<Label> gpr_metric(*rag, false);
        double gpr = 0.0;
        
        {
        ScopeTime timer;
        gpr = gpr_metric.calculateGPR(num_paths, num_threads);
        }

        cout << "Start GPR: " << gpr << endl;
#endif



        srand(1);
        {
            ScopeTime timer;
            cout << "Number of edges: " << rag->get_num_edges() << endl;
            cout << "Number of nodes: " << rag->get_num_regions() << endl;
            LocalEdgePriority<Label> priority_scheduler(*rag, 0.1, 0.9, 0.1, json_vals);
            
            int num_edges_orig = priority_scheduler.getNumRemaining();
            cout << "Number of assignments: " << num_edges_orig << endl;           
 
            int num_edges_examined = 0;

            
            while (!priority_scheduler.isFinished()) {
                EdgePriority<Label>::Location location;
                boost::tuple<Label, Label> pair = priority_scheduler.getTopEdge(location);
            //    priority_scheduler.setEdge(pair, 0);
#if 1 
                Label node1 = boost::get<0>(pair);
                Label node2 = boost::get<1>(pair);
                //cout << node1 << " " << node2 << endl;
                RagEdge<Label>* temp_edge = rag->find_rag_edge(node1, node2);
                double weight = temp_edge->get_weight();
                int weightint = int(100 * weight);
                if (0) { //(rand() % 100) > (weightint)) {
                    //cout << "remove" << endl;
                    priority_scheduler.removeEdge(pair, true);
                } else {
                    //cout << "set" << endl;
                    priority_scheduler.removeEdge(pair, false);
                }
#endif
                ++num_edges_examined;
            }
/*
            int total_undos = 0;
            while (priority_scheduler.undo()) {
                ++total_undos;
            }
            cout << total_undos << endl;
            cout << "Num operations: " << num_edges_examined << endl;



            while (!priority_scheduler.isFinished()) {
                EdgePriority<Label>::Location location;
                boost::tuple<Label, Label> pair = priority_scheduler.getTopEdge(location);
            //    priority_scheduler.setEdge(pair, 0);
#if 1 
                Label node1 = boost::get<0>(pair);
                Label node2 = boost::get<1>(pair);
                //cout << node1 << " " << node2 << endl;
                RagEdge<Label>* temp_edge = rag->find_rag_edge(node1, node2);
                double weight = temp_edge->get_weight();
                int weightint = int(100 * weight);
                if (0) { // (rand() % 100) > (weightint)) {
                    //cout << "remove" << endl;
                    priority_scheduler.removeEdge(pair, true);
                } else {
                    //cout << "set" << endl;
                    priority_scheduler.removeEdge(pair, false);
                }
#endif
                ++num_edges_examined;
            }
*/

            cout << "Num operations: " << num_edges_examined << endl;
            cout << "Percent edges examined: " << double(num_edges_examined) / num_edges_orig * 100 << endl;
            double time_elapsed = double(timer.getElapsed());  
            cout << "Time per operation: " << time_elapsed/num_edges_examined << " seconds" << endl;
            
        }

#if 0
        GPR<Label> gpr_metric2(*rag, false);
        double gpr2 = gpr_metric2.calculateGPR(num_paths, num_threads);
        cout << "Finish GPR: " << gpr2 << endl;
#endif
    } catch (ErrMsg& msg) {
        cerr << msg.str << endl;
        exit(-1);
    }

#ifdef OLDPARSE
    fin.close();
#endif

    return 0;
}
