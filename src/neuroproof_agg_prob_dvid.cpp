#include "../Stack/Stack.h"
#include "../FeatureManager/FeatureMgr.h"
#include "../Utilities/ScopeTime.h"
#include "../Utilities/OptionParser.h"
#include "../Rag/RagIO.h"

#include <libdvid/DVIDNode.h>

#include <boost/algorithm/string/predicate.hpp>
#include <iostream>


using namespace NeuroProof;

using std::cerr; using std::cout; using std::endl;
using std::string;
using std::vector;
using namespace boost::algorithm;
using std::tr1::unordered_set;

static const char * PROPERTY_KEY = "np-features";
static const char * PROB_KEY = "np-prob";

struct BuildOptions
{
    BuildOptions(int argc, char** argv) : dumpgraph(false)
    {
        OptionParser parser("Program that builds graph over defined region");

        // dvid string arguments
        parser.add_option(dvid_servername, "dvid-server",
                "name of dvid server", false, true); 
        parser.add_option(uuid, "uuid",
                "dvid node uuid", false, true); 
        parser.add_option(graph_name, "graph-name",
                "name of the graph name", false, true); 
        parser.add_option(num_channels, "num-chans",
                "Number of prediction channels (should be dynamic in future)", false, true); 
        parser.add_option(bodylist_name, "bodylist-name",
                "JSON file containing bodylist of ids to compute probability between (ignore duplicates)", false, true);
        parser.add_option(classifier_filename, "classifier-file",
                "opencv or vigra agglomeration classifier (should end in h5)", false, true); 

        // dump simple graph (no locations or synapse information) -- for debugging purposes
        parser.add_option(dumpgraph, "dumpfile", "Dump graph prob file");

        parser.parse_options(argc, argv);
    }

    // manadatory optionals
    string dvid_servername;
    string uuid;
    string graph_name;
    string bodylist_name; 
    string classifier_filename;
    int num_channels;

    bool dumpgraph;
};


void run_graph_prob(BuildOptions& options)
{
    try {
        // create DVID node accessor 
        libdvid::DVIDServer server(options.dvid_servername);
        libdvid::DVIDNode dvid_node(server, options.uuid);

        // read bodies from json
        Json::Reader json_reader;
        Json::Value json_data;
        ifstream fin(options.bodylist_name.c_str());
        if (!fin) {
            throw ErrMsg("Error: input file: " + options.bodylist_name + " cannot be opened");
        }
        if (!json_reader.parse(fin, json_data)) {
            throw ErrMsg("Error: Json incorrectly formatted");
        }
        fin.close();
        
        Json::Value body_list = json_data["body-list"];
        
        Rag_t* rag = new Rag_t;

        // build RAG from body list (get neighbors) -- put in RAG IO??
        for (unsigned int i = 0; i < body_list.size(); ++i) {
            Node_t node = body_list[i].asUInt();    
        
            // get vertex and edge weight 
            libdvid::Graph subgraph;
            dvid_node.get_vertex_neighbors(options.graph_name, (libdvid::VertexID) node, subgraph);
            
            // load graph and weight
            for (int j = 0; j < subgraph.vertices.size(); ++j) {
                RagNode_t* rag_node = rag->find_rag_node(Node_t(subgraph.vertices[j].id));
                if (!(rag->find_rag_node(subgraph.vertices[j].id))) {
                    rag_node = rag->insert_rag_node(subgraph.vertices[j].id);
                    rag_node->set_size((unsigned long long)(subgraph.vertices[j].weight));
                }
            }
            for (int j = 0; j < subgraph.edges.size(); ++j) {
                if (!(rag->find_rag_edge(subgraph.edges[j].id1,
                        subgraph.edges[j].id2))) {
                    RagNode_t* rag_node1 = rag->find_rag_node(Node_t(subgraph.edges[j].id1));
                    RagNode_t* rag_node2 = rag->find_rag_node(Node_t(subgraph.edges[j].id2));
                    RagEdge_t* rag_edge = rag->insert_rag_edge(rag_node1, rag_node2);
                    rag_edge->set_size((unsigned long long)(subgraph.edges[j].weight));
                }
            }
        }

        // how to dynamically read the number of channels? 
        FeatureMgrPtr feature_manager(new FeatureMgr(options.num_channels));
        feature_manager->set_basic_features(); 
     
        // set classifier 
        EdgeClassifier* eclfr;
        if (ends_with(options.classifier_filename, ".h5")) {
            eclfr = new VigraRFclassifier(options.classifier_filename.c_str());
        } else if (ends_with(options.classifier_filename, ".xml")) {	
            cout << "Warning: should be using VIGRA classifier" << endl;
            eclfr = new OpencvRFclassifier(options.classifier_filename.c_str());
        }        
        feature_manager->set_classifier(eclfr);   	 

        // iterate node and edges and load features
       
        // create vertex list
        vector<libdvid::Vertex> vertices;
        for (Rag_t::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
            vertices.push_back(libdvid::Vertex((*iter)->get_node_id(), (*iter)->get_size()));
        }

        // create edge list
        vector<libdvid::Edge> edges;
        for (Rag_t::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {
            edges.push_back(libdvid::Edge((*iter)->get_node1()->get_node_id(),
                        (*iter)->get_node2()->get_node_id(), (*iter)->get_size()));
        }

        // get vertex features 
        vector<libdvid::BinaryDataPtr> properties;
        libdvid::VertexTransactions transaction_ids; 

        // retrieve vertex properties
        dvid_node.get_properties(options.graph_name, vertices, PROPERTY_KEY, properties, transaction_ids);

        // update properties
        for (int i = 0; i < vertices.size(); ++i) {
            RagNode_t* node = rag->find_rag_node(vertices[i].id);
            assert((properties[i]->get_data().length() > 0));
            
            feature_manager->deserialize_features((char*) properties[i]->get_raw(), node);
        } 
        properties.clear();
        transaction_ids.clear();    

        // retrieve vertex properties
        dvid_node.get_properties(options.graph_name, edges, PROPERTY_KEY, properties, transaction_ids);

        // update properties
        for (int i = 0; i < edges.size(); ++i) {
            RagEdge_t* edge = rag->find_rag_edge(edges[i].id1, edges[i].id2);
            assert((properties[i]->get_data().length() > 0));

            feature_manager->deserialize_features((char*) properties[i]->get_raw(), edge);
        } 

        // can reuse transaction ids from before
        do {
            properties.clear();
            // get / put probability for each edge
            for (int i = 0; i < edges.size(); ++i) {
                // calc prob
                RagEdge_t* edge = rag->find_rag_edge(edges[i].id1, edges[i].id2);
                double prob = feature_manager->get_prob(edge); 
                edge->set_weight(prob);
                properties.push_back( 
                        libdvid::BinaryData::create_binary_data((char*)(&prob), sizeof(double))); 
            }

            // set vertex properties
            vector<libdvid::Edge> leftover_edges;
            dvid_node.set_properties(options.graph_name, edges, PROB_KEY, properties,
                    transaction_ids, leftover_edges); 
            edges = leftover_edges;

            transaction_ids.clear();
            if (!edges.empty()) {
                dvid_node.get_properties(options.graph_name, edges, PROB_KEY, properties, transaction_ids);
            }
        } while(!edges.empty());

        // dump prob graph 
        if (options.dumpgraph) {
            create_jsonfile_from_rag(rag, "graph.json");
        }

    } catch (std::exception& e) {
        cout << e.what() << endl;
    }
}

int main(int argc, char** argv) 
{
    BuildOptions options(argc, argv);
    ScopeTime timer;

    run_graph_prob(options);

    return 0;
}


