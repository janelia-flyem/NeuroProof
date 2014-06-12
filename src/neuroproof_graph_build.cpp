#include "../Stack/Stack.h"
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



// ?! have example create label volume with vertex near border (check edge cases and orientation)

struct BuildOptions
{
    BuildOptions(int argc, char** argv) : x(0), y(0), z(0), xsize(0), ysize(0), zsize(0), dumpfile(false)
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

        // roi for image
        parser.add_option(x, "x", "x starting point", false, true); 
        parser.add_option(y, "y", "y starting point", false, true); 
        parser.add_option(z, "z", "z starting point", false, true); 

        parser.add_option(xsize, "xsize", "x size", false, true); 
        parser.add_option(ysize, "ysize", "y size", false, true); 
        parser.add_option(zsize, "zsize", "z size", false, true); 
      
        // for debugging purposes 
        parser.add_option(dumpfile, "dumpfile", "Dump segmentation file");

        parser.parse_options(argc, argv);
    }

    // manadatory optionals
    string dvid_servername;
    string uuid;
    string graph_name;
    string labels_name; 

    int x, y, z, xsize, ysize, zsize;
    bool dumpfile;
};


void run_graph_build(BuildOptions& options)
{
    try {
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
        Stack stack(initial_labels); 

        // make new build_rag for stack (ignore 0s, add edge on greater than,
        // ignore 1 pixel border for vertex accum
        cout<<"Building RAG ..."; 	
        stack.build_rag_batch();
        cout<<"done with "<< stack.get_num_labels()<< " nodes\n";	

        // iterate RAG vertices and nodes and update values
        RagPtr rag = stack.get_rag();
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

        if (options.dumpfile) {
            stack.serialize_stack("debugsegstack.h5", 0, false);
        }

    } catch (std::exception& e) {
        cout << e.what() << endl;
    }
}

int main(int argc, char** argv) 
{
    BuildOptions options(argc, argv);
    ScopeTime timer;

    run_graph_build(options);

    return 0;
}


