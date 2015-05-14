/*!
 * \file
 * The following program is used to analyze a segmentation graph.
 * When a segmentation is constructed for a image volume, each of
 * the regions in segmentation can be represented as nodes in a
 * graph and the adjancency of these regions as edges.  This program
 * assumes that each edge has some confidence associated with it
 * to indicate the likelihood of the edge being a true or false edge.
 * With this information, analyze_segmentation_graph can estimate
 * the amount of uncertainty in the graph based on the research found
 * in http://www.plosone.org/article/info%3Adoi%2F10.1371%2Fjournal.pone.0044448
 * [Plaza '12].  This program can also estimate how many edges
 * will likely need to be manually analyzed to improve the quality
 * of the segmentation graph.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

// algorithms used to estimate how many edges need to be examine
#include "../EdgeEditor/EdgeEditor.h"

// contains library for storing a region adjacency graph (RAG)
#include "../Rag/Rag.h"

// algorithms to calculate generalized probabilistic rand (GPR)
#include "../EdgeEditor/GPR.h"

// simple function for measuring runtime
#include "../Utilities/ScopeTime.h"

// utilties for importing rags
#include "../Rag/RagIO.h"

// utitlies for parsing options
#include "../Utilities/OptionParser.h"

#include <vector>
#include <fstream>
#include <sstream>
#include <cassert>
#include <iostream>
#include <json/json.h>
#include <json/value.h>
#include <boost/program_options.hpp>

using std::cerr; using std::cout; using std::endl;
using std::ifstream;
using std::string;
using std::stringstream;
using std::vector;
using namespace NeuroProof;

using namespace boost::program_options;
using std::exception;


/*!
 * Parses command options
 * \param argc number of arguments
 * \param argv array of argument strings
 * \param num_threads reference to number of threads to run GPR
 * \param node_threshold reference to threshold of node size uncertainty below which is ignored
 * \param synapse_threshold reference to threshold of synapse size uncertainty below which is ignored
 * \param graph_file reference to graph file in json format
 * \param random_seed random seed
 * \param calc_gpr enable gpr calculation (default false)
 * \param est_edit_distance enable edit distance calculation (default false) 
*/ 
void parse_options(int argc, char** argv, int& num_threads,
        int& node_threshold,
        double& synapse_threshold, string& graph_file, 
        int& random_seed, bool& calc_gpr,
        bool& est_edit_distance)
{
    OptionParser parser("Program that quantifies the uncertainty found in the segmentation graph");
    parser.add_option(num_threads, "num-threads",
            "Number of threads used in computing GPR"); 
    parser.add_option(calc_gpr, "calc-gpr,g",
            "Calculate the uncertainty in the graph using GPR"); 
    parser.add_option(est_edit_distance, "est-edit-distance,b",
            "Estimate the number of operations to prevent large errors in the graph"); 
    parser.add_option(node_threshold, "node-size-threshold",
            "Size threshold below which errors are considered insignificant"); 
    parser.add_option(node_threshold, "synapse-size-threshold",
            "Size threshold based on the number of synapse in the node below which are considered insignificant");
    parser.add_positional(graph_file, "graph-file", "graph file"); 
    parser.add_option(random_seed, "random-seed", "Set seed for random computation", true, false, true);
    parser.parse_options(argc, argv);
}

/*!
 * Helper function to create RAG from graph json (should be a constructor)
 * \param graph_file file in json format that contains graph
 * \return a pointer to a RAG
*/ 
Rag_t* read_graph(string graph_file, Json::Value& json_vals)
{
    ifstream fin(graph_file.c_str());
    Json::Reader json_reader;
    if (!json_reader.parse(fin, json_vals)) {
        throw ErrMsg("Error: Json incorrectly formatted");
    }
    fin.close();

    Rag_t* rag = create_rag_from_json(json_vals);
    if (!rag) {
        throw ErrMsg("Rag could not be created");
    }
    return rag;
}

/*!
 * Creates a GPR instance and run the GPR algorithm using only
 * the shortest path between pairs of nodes.  This calculation
 * is multithreaded.
 * \param rag graph whose uncertainty will be measured
 * \param num_threads number of threads used in computation
*/ 
void calc_gpr(Rag_t* rag, int num_threads)
{
    try {
        ScopeTime timer;

        GPR gpr_metric(*rag, false);
        double gpr = gpr_metric.calculateGPR(1, num_threads);
        cout << "GPR: " << gpr << endl;
    } catch (ErrMsg& msg) {
        cerr << msg.str << endl;
        exit(-1);
    }
}
/*!
 * Estimates the number of operations to 'fix' a segmentaiton graph.
 * The algorithm looks for edges whose confidence is low.  The algorithm
 * assigns certainty to these edges until the leftover uncertain edges
 * have topological impact below a certain threshold
 * \param priority_scheduler scheduler to find uncertain edges
 * \param rag graph whose uncertain edges are analyzed
*/
int get_num_edits(EdgeEditor& priority_scheduler, Rag_t* rag)
{
    int edges_examined = 0;
    while (!priority_scheduler.isFinished()) {
        EdgeEditor::Location location;

        // choose most impactful edge given pre-determined strategy
        boost::tuple<Node_t, Node_t> pair = priority_scheduler.getTopEdge(location);
        
        Node_t node1 = boost::get<0>(pair);
        Node_t node2 = boost::get<1>(pair);
        RagEdge_t* temp_edge = rag->find_rag_edge(node1, node2);
        double weight = temp_edge->get_weight();
        
        // simulate the edge as true or false as function of edge certainty
        int weightint = int(100 * weight);
        if ((rand() % 100) > (weightint)) {
            priority_scheduler.removeEdge(pair, true);
        } else {
            priority_scheduler.removeEdge(pair, false);
        }
        ++edges_examined;
    }
    
    // undo simulation to put Rag back to the initial state
    int total_undos = 0; 
    while (priority_scheduler.undo()) {
        ++total_undos;
    }
    assert(total_undos == edges_examined);
    return edges_examined;
}

/*!
 * Explore different strategies for estimating the number of uncertain
 * edges in the graph.
 * \param rag graph where uncertain edges are analyzed
 * \param node_threshold threshold that determines which node size change is impactful
 * \param synapse_threshold threshold that determines which synape size change is impactful
 * \param json_vals json data parsed from graph
*/
void est_edit_distance(Rag_t* rag, int node_threshold,
        double synapse_threshold, Json::Value& json_vals)
{
    try {
        ScopeTime timer;
        
        cout << "Node size threshold: " << node_threshold << endl;
        cout << "Synapse size threshold: " << synapse_threshold << endl;

        EdgeEditor priority_scheduler(*rag, 0.1, 0.9, 0.1, json_vals);

        // determine the number of node above a certain size that do not
        // touch a boundary
        vector<Node_t> violators = priority_scheduler.getQAViolators(node_threshold);
        cout << "Num nodes not touching boundary: " << violators.size() << endl; 
        
        // determine the number of nodes above a certain size with synapses
        // that do not touch a boundary
        violators = priority_scheduler.getQAViolators(UINT_MAX);
        cout << "Num nodes with synapses not touching boundary: " << violators.size() << endl; 

        // determine the number of edges to analze that leaves only small
        // uncertain bodies
        priority_scheduler.set_body_mode(node_threshold, 0);
        cout << "Estimated num edge operations (node entropy threshold): " 
            << get_num_edits(priority_scheduler, rag) << endl;

        // also determine number of edges to analyze but do not use global
        // affinity between nodes, use just the local edge uncertainty
        priority_scheduler.set_body_mode(node_threshold, 1);
        cout << "Estimated num edge operations (node entropy threshold with path length = 1): "
            << get_num_edits(priority_scheduler, rag) << endl;

        // determine number of edges by looking at edges only in defined
        // uncertainty ranges
        priority_scheduler.set_edge_mode(0.1, 0.9, 0.1);
        cout << "Estimated num edge operations (edge confidence to 90 percent): "
            << get_num_edits(priority_scheduler, rag) << endl;

        // determine number of edges to analyze that handles uncertainty
        // in the synapse graph
        priority_scheduler.set_synapse_mode(synapse_threshold);
        cout << "Estimated num edge operations (synpase entropy threshold): "
            << get_num_edits(priority_scheduler, rag) << endl;

        // determine number of edges to analyze to trace large bodies
        // to a boundary
        priority_scheduler.set_orphan_mode(node_threshold);
        cout << "Estimated num edge operations to connect orphans to a boundary: "
            << get_num_edits(priority_scheduler, rag) << endl;
    } catch (ErrMsg& msg) {
        cerr << msg.str << endl;
        exit(-1);
    }
}

/*!
 * Main function for analyzing segmentation graph
 * \param argc
 * \param argv
 * \return status
*/ 
int main(int argc, char** argv) 
{
    // default parameter values
    int num_threads = 1;
    int node_threshold = 25000;
    double synapse_threshold = 0.1;
    string graph_file;
    int random_seed = 1;
    bool enable_calc_gpr = false;
    bool enable_est_edit_distance = false;


    // load options from users
    parse_options(argc, argv, num_threads,
            node_threshold, synapse_threshold, graph_file,
            random_seed, enable_calc_gpr, enable_est_edit_distance);

    // always display the size of the graph
    Json::Value json_vals;
    Rag_t* rag = read_graph(graph_file, json_vals);        
    cout << "Graph edges: " << rag->get_num_edges() << endl;
    cout << "Graph nodes: " << rag->get_num_regions() << endl;

    // run gpr analysis -- random seed set as specified
    if (enable_calc_gpr) {
        srand(random_seed);
        cout << endl;
        cout << "*********Begin GPR Processing*********" << endl;
        calc_gpr(rag, num_threads);
        cout << "*******Finished GPR Processing********" << endl;
        cout << endl;
    }

    // run edit distance estimate -- random seed set as specified
    if (enable_est_edit_distance)
    {
        srand(random_seed);
        cout << endl;
        cout << "********Compute Edit Distances********" << endl;
        est_edit_distance(rag, node_threshold, synapse_threshold, json_vals);
        cout << "***Finished Compute Edit Distances****" << endl;
        cout << endl;
    }

    delete rag;
    return 0;
}
