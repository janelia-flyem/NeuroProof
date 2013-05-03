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
#include <vector>

#include <boost/program_options.hpp>

using std::cerr; using std::cout; using std::endl;
using std::ifstream;
using std::string;
using std::stringstream;
using std::vector;
using namespace NeuroProof;

using namespace boost::program_options;
using std::exception;

// input: graph file
// options: calculate gpr, estimate edit distance graph, estimate edit distance border, estimate edit distance synapse
// output: estimate important bodies json
// options: num threads, image stack resolution, random seed, body size threshold, synapse threshold


void parse_options(int argc, char** argv, int& num_threads,
        int& node_threshold,
        double& synapse_threshold, string& graph_file, 
        int& random_seed, bool& calc_gpr,
        bool& est_edit_distance)
{
    string prgm_message = "analyze_segmentation_graph [graph-file]\n\tProgram that quantifies the uncertainty found in the segmentation graph";
    options_description generic_options("Options");
    generic_options.add_options()
        ("help,h", "Show help message")
        ("calc-gpr,g", value<bool>(&calc_gpr)->zero_tokens(),
         "Calculate the uncertainty in the graph using GPR")
        ("est-edit-distance,b", value<bool>(&est_edit_distance)->zero_tokens(),
         "Estimate the number of operations to prevent large errors in the graph")
        ("num-threads", value<int>(&num_threads), 
         "Number of threads used in computing GPR")
        ("node-size-threshold", value<int>(&node_threshold),
         "Size threshold below which errors are considered insignificant")
        ("synapse-size-threshold", value<double>(&synapse_threshold),
         "Size threshold based on the number of synapse in the node below which are considered insignificant");

    options_description hidden_options("Hidden options");
    hidden_options.add_options()
        ("graph-file", value<string>(&graph_file)->required(), "graph file")
        ("random-seed", value<int>(&random_seed),
         "Set seed for random computation");

    options_description cmdline_options;
    cmdline_options.add(generic_options).add(hidden_options);
    positional_options_description pos;
    pos.add("graph-file", 1);

    variables_map vm;
    
    try {
        store(command_line_parser(argc, argv).options(cmdline_options).positional(pos).run(), vm);
        if (vm.count("help")) {
            cout << prgm_message << endl;
            cout << generic_options <<endl;
            exit(0);
        }
        notify(vm);
    } catch (exception& e) {
        cout << "ERROR: " << e.what() << endl;
        cout << prgm_message << endl;
        cout << generic_options <<endl;
        exit(-1);
    }
}

Rag<Label>* read_graph(string graph_file)
{
    ifstream fin(graph_file.c_str());
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
    return rag;
}


void calc_gpr(Rag<Label>* rag, int num_threads)
{
    try {
        ScopeTime timer;

        GPR<Label> gpr_metric(*rag, false);
        double gpr = gpr_metric.calculateGPR(1, num_threads);
        cout << "GPR: " << gpr << endl;
    } catch (ErrMsg& msg) {
        cerr << msg.str << endl;
        exit(-1);
    }
}

int get_num_edits(LocalEdgePriority<Label>& priority_scheduler, Rag<Label>* rag)
{
    int edges_examined = 0;
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
        ++edges_examined;
    }
    
    int total_undos = 0; 
    while (priority_scheduler.undo()) {
        ++total_undos;
    }
    assert(total_undos == edges_examined);
    return edges_examined;
}


void est_edit_distance(Rag<Label>* rag, int node_threshold, double synapse_threshold)
{
    try {
        ScopeTime timer;
        
        cout << "Node size threshold: " << node_threshold << endl;
        cout << "Synapse size threshold: " << synapse_threshold << endl;

        Json::Value json_vals;
        LocalEdgePriority<Label> priority_scheduler(*rag, 0.1, 0.9, 0.1, json_vals);

        vector<Label> violators = priority_scheduler.getQAViolators(node_threshold);
        cout << "Num nodes not touching boundary: " << violators.size() << endl; 
        violators = priority_scheduler.getQAViolators(UINT_MAX);
        cout << "Num nodes with synapses not touching boundary: " << violators.size() << endl; 

        priority_scheduler.set_body_mode(node_threshold, 0);
        cout << "Estimated num edge operations (node entropy threshold): " 
            << get_num_edits(priority_scheduler, rag) << endl;

        priority_scheduler.set_body_mode(node_threshold, 1);
        cout << "Estimated num edge operations (node entropy threshold with path length = 1): "
            << get_num_edits(priority_scheduler, rag) << endl;

        priority_scheduler.set_edge_mode(0.1, 0.9, 0.1);
        cout << "Estimated num edge operations (edge confidence to 90 percent): "
            << get_num_edits(priority_scheduler, rag) << endl;

        priority_scheduler.set_synapse_mode(synapse_threshold);
        cout << "Estimated num edge operations (synpase entropy threshold): "
            << get_num_edits(priority_scheduler, rag) << endl;

        priority_scheduler.set_orphan_mode(node_threshold, 0, 0);
        cout << "Estimated num edge operations to connect orphans to a boundary: "
            << get_num_edits(priority_scheduler, rag) << endl;
    } catch (ErrMsg& msg) {
        cerr << msg.str << endl;
        exit(-1);
    }
}


int main(int argc, char** argv) 
{
    int num_threads = 1;
    int node_threshold = 25000;
    double synapse_threshold = 0.1;
    string graph_file;
    int random_seed = 1;
    bool enable_calc_gpr = false;
    bool enable_est_edit_distance = false;

    parse_options(argc, argv, num_threads,
            node_threshold, synapse_threshold, graph_file,
            random_seed, enable_calc_gpr, enable_est_edit_distance);

    Rag<Label>* rag = read_graph(graph_file);        
    cout << "Graph edges: " << rag->get_num_edges() << endl;
    cout << "Graph nodes: " << rag->get_num_regions() << endl;

    if (enable_calc_gpr) {
        srand(random_seed);
        cout << endl;
        cout << "*********Begin GPR Processing*********" << endl;
        calc_gpr(rag, num_threads);
        cout << "*******Finished GPR Processing********" << endl;
        cout << endl;
    }

    if (enable_est_edit_distance)
    {
        srand(random_seed);
        cout << endl;
        cout << "********Compute Edit Distances********" << endl;
        est_edit_distance(rag, node_threshold, synapse_threshold);
        cout << "***Finished Compute Edit Distances****" << endl;
        cout << endl;
    }

    return 0;
}
