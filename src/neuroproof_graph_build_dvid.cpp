#include "../BioPriors/BioStack.h"
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
using std::tr1::unordered_map;

static const char * PRED_DATASET_NAME = "volume/predictions";
static const char * PROPERTY_KEY = "np-features";
static const char * PROB_KEY = "np-prob";
static const char * SYNAPSE_KEY = "synapse";

// set synapses for stack -- don't flip y, use global offsets
void set_synapse_exclusions(BioStack& stack, const char* synapse_json,
        int xoffset, int yoffset, int zoffset)
{
    VolumeLabelPtr labelvol = stack.get_labelvol();
    RagPtr rag = stack.get_rag();

    unsigned int xsize = labelvol->shape(0);
    unsigned int ysize = labelvol->shape(1);
    unsigned int zsize = labelvol->shape(2);

    vector<vector<unsigned int> > synapse_locations;

    Json::Reader json_reader;
    Json::Value json_reader_vals;
    
    ifstream fin(synapse_json);
    if (!fin) {
        throw ErrMsg("Error: input file: " + string(synapse_json) + " cannot be opened");
    }
    if (!json_reader.parse(fin, json_reader_vals)) {
        throw ErrMsg("Error: Json incorrectly formatted");
    }
    fin.close();
 
    Json::Value synapses = json_reader_vals["data"];

    // synapses not allowed at 0 or max id since false buffer is added
    for (int i = 0; i < synapses.size(); ++i) {
        vector<vector<unsigned int> > locations;
        Json::Value location = synapses[i]["T-bar"]["location"];
        if (!location.empty()) {
            vector<unsigned int> loc;
            
            int xloc = location[(unsigned int)(0)].asUInt();
            int yloc = location[(unsigned int)(1)].asUInt();
            int zloc = location[(unsigned int)(2)].asUInt();
            xloc -= xoffset;
            yloc -= yoffset;
            zloc -= zoffset;

            if ((xloc > 0) && (yloc > 0) && (zloc > 0) &&
                (xloc < (xsize-1)) && (yloc < (ysize-1)) && (zloc < (zsize-1))) {
                loc.push_back(xloc);
                loc.push_back(yloc);
                loc.push_back(zloc);
                synapse_locations.push_back(loc);
                locations.push_back(loc);
            }
        }
        
        Json::Value psds = synapses[i]["partners"];
        for (int i = 0; i < psds.size(); ++i) {
            Json::Value location = psds[i]["location"];
            if (!location.empty()) {
                vector<unsigned int> loc;
            
                int xloc = location[(unsigned int)(0)].asUInt();
                int yloc = location[(unsigned int)(1)].asUInt();
                int zloc = location[(unsigned int)(2)].asUInt();
                xloc -= xoffset;
                yloc -= yoffset;
                zloc -= zoffset;

                if ((xloc > 0) && (yloc > 0) && (zloc > 0) &&
                    (xloc < (xsize-1)) && (yloc < (ysize-1)) && (zloc < (zsize-1))) {
                    loc.push_back(xloc);
                    loc.push_back(yloc);
                    loc.push_back(zloc);
                    
                    synapse_locations.push_back(loc);
                    locations.push_back(loc);
                }
            }
        }

        for (int iter1 = 0; iter1 < locations.size(); ++iter1) {
            for (int iter2 = (iter1 + 1); iter2 < locations.size(); ++iter2) {
                stack.add_edge_constraint(rag, labelvol,
                    locations[iter1][0], locations[iter1][1],
                    locations[iter1][2], locations[iter2][0],
                    locations[iter2][1], locations[iter2][2]);           
            }
        }
    }

    stack.set_synapse_exclusions(synapse_locations);
}


struct BuildOptions
{
    BuildOptions(int argc, char** argv) : x(0), y(0), z(0), xsize(0), ysize(0),
        zsize(0), dvidgraph_load_saved(false), dvidgraph_update(true), dumpgraph(false)
    {
        OptionParser parser("Program that builds graph over defined region");

        // dvid string arguments
        parser.add_option(dvid_servername, "dvid-server",
                "name of dvid server", false, true); 
        parser.add_option(uuid, "uuid",
                "dvid node uuid", false, true); 
        parser.add_option(graph_name, "graph-name",
                "name of the graph name", false, true); 
        parser.add_option(labels_name, "label-name",
                "name of the label volume", false, true); 

        parser.add_option(prediction_filename, "prediction-file",
                "ilastik h5 file (x,y,z,ch) that has pixel predictions");

        // roi for image
        parser.add_option(x, "x", "x starting point", false, true); 
        parser.add_option(y, "y", "y starting point", false, true); 
        parser.add_option(z, "z", "z starting point", false, true); 

        parser.add_option(xsize, "xsize", "x size", false, true); 
        parser.add_option(ysize, "ysize", "y size", false, true); 
        parser.add_option(zsize, "zsize", "z size", false, true); 
        
        parser.add_option(classifier_filename, "classifier-file",
                "opencv or vigra agglomeration classifier (should end in h5)"); 

        // iteractions with DVID
        parser.add_option(dvidgraph_load_saved, "dvidgraph-load-saved",
                "This option will load graph probabilities and sizes saved (synapse file, predictions, and classifier should not be specified and dvigraph-update will be set false");
        parser.add_option(dvidgraph_update, "dvidgraph-update", "Enable the writing of features and graph information to DVID");

        parser.add_option(synapse_filename, "synapse-file",
                "Synapse file in JSON format should be based on global DVID coordinates.  Synapses outside of the segmentation ROI will be ignored.  Synapses are not loaded into DVID.  Synapses cannot have negative coordinates even though that is possible in DVID in general.");

        // for debugging purposes 
        parser.add_option(dumpgraph, "dumpgraph", "Dump segmentation file");

        parser.parse_options(argc, argv);
    }

    // manadatory optionals
    string dvid_servername;
    string uuid;
    string graph_name;
    string labels_name; 
    string classifier_filename;

    // optional build with features
    string prediction_filename;

    // add synapse option -- assume global coordinates
    string synapse_filename;


    int x, y, z, xsize, ysize, zsize;
    bool dumpgraph;
    bool dvidgraph_load_saved;
    bool dvidgraph_update;
};


void run_graph_build(BuildOptions& options)
{
    try {
        // option validation
        if (options.dvidgraph_load_saved && options.dvidgraph_update) {
            cout << "Warning: graph update not possible when loading saved information" << endl;
            options.dvidgraph_update = false;
        }

        // synapse file should not be included either
        if (options.dvidgraph_load_saved && ((options.classifier_filename != "") ||
                (options.prediction_filename != "") || (options.synapse_filename != ""))) {
            throw ErrMsg("Classifier, prediction file, and synapses should not be specified when using saved DVID values");
        }
        
        // create DVID node accessor 
        libdvid::DVIDServer server(options.dvid_servername);
        libdvid::DVIDNode dvid_node(server, options.uuid);
       
        // establish ROI (make a 1 pixel border)
        libdvid::tuple start; start.push_back(options.x-1); start.push_back(options.y-1); start.push_back(options.z-1);
        libdvid::tuple sizes; sizes.push_back(options.xsize+2); sizes.push_back(options.ysize+2); sizes.push_back(options.zsize+2);
        libdvid::tuple channels; channels.push_back(0); channels.push_back(1); channels.push_back(2);
      
        // retrieve volume 
        libdvid::DVIDLabelPtr labels;
        dvid_node.get_volume_roi(options.labels_name, start, sizes, channels, labels);

        // load labels into VIGRA
        unsigned long long* ptr = (unsigned long long int*) labels->get_raw();
        unsigned long long total_size = (options.xsize+2) * (options.ysize+2) * (options.zsize+2);

        VolumeLabelPtr initial_labels = VolumeLabelData::create_volume(options.xsize+2,
                options.ysize+2, options.zsize+2);
        unsigned long long iter = 0;
        // 64 bit numbers not currently supported!!
        volume_forXYZ(*initial_labels, x, y, z) {
            initial_labels->set(x,y,z, (unsigned int)(ptr[iter]));
            ++iter;
        }
        cout << "Read watershed" << endl;

        // create stack to hold segmentation state
        BioStack stack(initial_labels); 

        if (options.prediction_filename != "") {
            vector<VolumeProbPtr> prob_list = VolumeProb::create_volume_array(
                    options.prediction_filename.c_str(), PRED_DATASET_NAME, stack.get_xsize());
            FeatureMgrPtr feature_manager(new FeatureMgr(prob_list.size()));
            feature_manager->set_basic_features(); 
            stack.set_feature_manager(feature_manager);
           
            // set classifier 
            EdgeClassifier* eclfr;
            if (ends_with(options.classifier_filename, ".h5")) {
                eclfr = new VigraRFclassifier(options.classifier_filename.c_str());
                feature_manager->set_classifier(eclfr);   	 
            } else if (ends_with(options.classifier_filename, ".xml")) {	
                cout << "Warning: should be using VIGRA classifier" << endl;
                eclfr = new OpencvRFclassifier(options.classifier_filename.c_str());
                feature_manager->set_classifier(eclfr);   	 
            }     

            stack.set_prob_list(prob_list);
        }

        // make new build_rag for stack (ignore 0s, add edge on greater than,
        // ignore 1 pixel border for vertex accum
        cout<<"Building RAG ..."; 	
        stack.build_rag_batch();
        cout<<"done with "<< stack.get_num_labels()<< " nodes\n";	
            
        RagPtr rag = stack.get_rag();
       
        // do not update dvid graph in updating is turned off
        if (options.dvidgraph_update) {
            // iterate RAG vertices and nodes and update values
            vector<libdvid::Vertex> vertices;
            for (Rag_t::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
                vertices.push_back(libdvid::Vertex((*iter)->get_node_id(), (*iter)->get_size()));
            } 
            dvid_node.update_vertices(options.graph_name, vertices); 

            vector<libdvid::Edge> edges;
            for (Rag_t::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {
                edges.push_back(libdvid::Edge((*iter)->get_node1()->get_node_id(),
                            (*iter)->get_node2()->get_node_id(), (*iter)->get_size()));
            } 
            dvid_node.update_edges(options.graph_name, edges);

            // update node features to the DVID graph
            if ((options.prediction_filename != "")) {
                do {
                    vector<libdvid::BinaryDataPtr> properties;
                    libdvid::VertexTransactions transaction_ids; 

                    // retrieve vertex properties
                    dvid_node.get_properties(options.graph_name, vertices, PROPERTY_KEY, properties, transaction_ids);

                    // update properties
                    for (int i = 0; i < vertices.size(); ++i) {
                        RagNode_t* node = rag->find_rag_node(vertices[i].id);

                        if (node->get_size() == 0) {
                            stack.get_feature_manager()->create_cache(node);
                        } 

                        char* curr_data = 0; 
                        if ((properties[i]->get_data().length() > 0)) {
                            curr_data = (char*) properties[i]->get_raw();
                        }
                        string modified_feature = 
                            stack.get_feature_manager()->serialize_features(curr_data, node);
                        properties[i] = 
                            libdvid::BinaryData::create_binary_data(modified_feature.c_str(), modified_feature.length());
                    } 

                    // set vertex properties
                    vector<libdvid::Vertex> leftover_vertices;
                    dvid_node.set_properties(options.graph_name, vertices, PROPERTY_KEY, properties,
                            transaction_ids, leftover_vertices); 

                    vertices = leftover_vertices; 
                } while(!vertices.empty());

                // update edge features to the DVID graph
                do {
                    vector<libdvid::BinaryDataPtr> properties;
                    libdvid::VertexTransactions transaction_ids; 

                    // retrieve vertex properties
                    dvid_node.get_properties(options.graph_name, edges, PROPERTY_KEY, properties, transaction_ids);

                    // update properties
                    for (int i = 0; i < edges.size(); ++i) {
                        RagEdge_t* edge = rag->find_rag_edge(edges[i].id1, edges[i].id2);

                        char* curr_data = 0; 
                        if ((properties[i]->get_data().length() > 0)) {
                            curr_data = (char*) properties[i]->get_raw();
                        }
                        string modified_feature = 
                            stack.get_feature_manager()->serialize_features(curr_data, edge);
                        properties[i] = 
                            libdvid::BinaryData::create_binary_data(modified_feature.c_str(), modified_feature.length()); 
                    } 

                    // set vertex properties
                    vector<libdvid::Edge> leftover_edges;
                    dvid_node.set_properties(options.graph_name, edges, PROPERTY_KEY, properties,
                            transaction_ids, leftover_edges); 
                    edges = leftover_edges; 
                } while(!edges.empty());
            }
        }

        // remove 'fake' edges (kept till now since edge size can be non-zero and loaded into dvid)  
        vector<RagEdge_t*> edges_delete;
        for (Rag_t::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {
            if (((*iter)->get_node1()->get_size() == 0) ||
                    ((*iter)->get_node2()->get_size() == 0) ) {
                edges_delete.push_back(*iter);
            } 
        }
        for (int i = 0; i < edges_delete.size(); ++i) {
            if (options.prediction_filename != "") {
                stack.get_feature_manager()->remove_edge(edges_delete[i]); 
            }
            rag->remove_rag_edge(edges_delete[i]);
        }

        // remove 'fake' vertices
        vector<RagNode_t*> nodes_delete;
        for (Rag_t::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
            if ((*iter)->get_size() == 0) {
                nodes_delete.push_back(*iter);
            }
        } 
        for (int i = 0; i < nodes_delete.size(); ++i) {
            if (options.prediction_filename != "") {
                stack.get_feature_manager()->remove_node(nodes_delete[i]);
            } 
            rag->remove_rag_node(nodes_delete[i]);
        }
          
        // populate graph with saved values in DVID 
        if (options.dvidgraph_load_saved) {
            // delete edges that contain a node with no weight to prevent creating an
            // edge that exists beyond the ROI (should rarely happen)
            vector<libdvid::Edge> edges;
            for (Rag_t::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {
                edges.push_back(libdvid::Edge((*iter)->get_node1()->get_node_id(),
                                (*iter)->get_node2()->get_node_id(), 0));
            }

            // grab stored size for all vertices
            vector<libdvid::Vertex> vertices;
            for (Rag_t::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
                vertices.push_back(libdvid::Vertex((*iter)->get_node_id(), (*iter)->get_size()));
            } 
          
            libdvid::Graph subgraph; 
            dvid_node.get_subgraph(options.graph_name, vertices, subgraph); 

            for (int i = 0; i < subgraph.vertices.size(); ++i) {
                RagNode_t* rag_node = rag->find_rag_node(Node_t(subgraph.vertices[i].id));
                rag_node->set_size((unsigned long long)(subgraph.vertices[i].weight));
            } 

            // set edge probability manually
            vector<libdvid::BinaryDataPtr> properties;
            libdvid::VertexTransactions transaction_ids; 

            // update synapse from global graph (don't include constraints for non-important
            // edges); add constraints; load synapse_counts stored to BioStack to be used 
            dvid_node.get_properties(options.graph_name, vertices, SYNAPSE_KEY, properties, transaction_ids);
            unordered_map<Label_t, int> synapse_counts;
            for (int i = 0; i < vertices.size(); ++i) {
                RagNode_t* node = rag->find_rag_node(vertices[i].id);
                // skip empty vertices
                if (node->get_size() == 0) {
                    continue;
                }
                
                // if synapse information exists
                if (properties[i]->get_data().length() != 0) {
                    unsigned long long* val_array = (unsigned long long*) properties[i]->get_raw();
                    unsigned long long synapse_count = *val_array;
                    synapse_counts[node->get_node_id()] = synapse_count;
                    ++val_array;
                    int num_partners = *val_array;
                    ++val_array;

                    // load partner constraints
                    for (int j = 0; j < num_partners; ++j, ++val_array) {
                        Node_t node2 = *val_array;
                        RagNode_t* rag_node2 = rag->find_rag_node(node2);
                        if (rag_node2 && (vertices[i].id < node2) && (rag_node2->get_size() > 0)) {
                            RagEdge_t* edge = rag->find_rag_edge(node, rag_node2);
                            if (!edge) {
                                edge = rag->insert_rag_edge(node, rag_node2);
                                edge->set_weight(1.0);
                                edge->set_false_edge(true);
                            }
                            edge->set_preserve(true);
                        }
                    }
                }
            }
            stack.load_saved_synapse_counts(synapse_counts);

            // retrieve vertex properties
            properties.clear();
            transaction_ids.clear();
            dvid_node.get_properties(options.graph_name, edges, PROB_KEY, properties, transaction_ids);

            // update edge weight
            for (int i = 0; i < edges.size(); ++i) {
                RagEdge_t* edge = rag->find_rag_edge(edges[i].id1, edges[i].id2);

                // edge and probability value should exist
                if (!edge) {
                    throw ErrMsg("Edge was not found in DVID DB");
                }

                if (properties[i]->get_data().length() == 0) {
                    throw ErrMsg("Edge probability was not stored at the edge");
                }
                
                double* edge_prob = (double*) properties[i]->get_raw();
                edge->set_weight(*edge_prob); 
            }
        }
       
        // disable computation of probability if DVID saved values are used 
        if (options.dumpgraph) {
            // load synapses only graph is produced 
            if (options.synapse_filename != "") {
                set_synapse_exclusions(stack, options.synapse_filename.c_str(),
                        options.x-1, options.y-1, options.z-1);
            }

            stack.serialize_stack("stack.h5", "graph.json", false,
                    options.dvidgraph_load_saved);
        }
    } catch (ErrMsg& err) {
        cout << err.str << endl;
        exit(1);
    } catch (std::exception& e) {
        cout << e.what() << endl;
        exit(1);
    }
}

int main(int argc, char** argv) 
{
    BuildOptions options(argc, argv);
    ScopeTime timer;

    run_graph_build(options);

    return 0;
}


