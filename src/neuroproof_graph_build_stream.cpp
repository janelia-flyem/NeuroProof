#include "../BioPriors/BioStack.h"
#include "../FeatureManager/FeatureMgr.h"
#include "../Rag/RagIO.h"
#include "../Utilities/OptionParser.h"

#include <boost/algorithm/string/predicate.hpp>
#include <iostream>

//#include <fstream>

//using std::ofstream;

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

struct BuildOptions
{
    BuildOptions(int argc, char** argv) 
    {
        OptionParser parser("Program that builds graph over defined region");
        
        // will these be on disk or will they be streamed in ??
        parser.add_option(prediction_filename, "prediction-file",
                "ilastik h5 file (x,y,z,ch) that has pixel predictions");
        parser.add_option(classifier_filename, "classifier-file",
                "opencv or vigra agglomeration classifier (should end in h5)"); 
        parser.add_option(synapse_filename, "synapse-file",
                "Synapse file in JSON format should be based on global DVID coordinates.  Synapses outside of the segmentation ROI will be ignored.  Synapses are not loaded into DVID.  Synapses cannot have negative coordinates even though that is possible in DVID in general.");

        // iteractions with DVID -- not sure how to do this now ??
        //parser.add_option(dvidgraph_load_saved, "dvidgraph-load-saved",
      //          "This option will load graph probabilities and sizes saved (synapse file, predictions, and classifier should not be specified and dvigraph-update will be set false");

        parser.parse_options(argc, argv);
    }

    // optional build with features
    string prediction_filename;
    string classifier_filename;

    // add synapse option -- assume global coordinates
    string synapse_filename;


//    bool dvidgraph_load_saved;
};


void run_graph_build(BuildOptions& options)
{
    try {
        // retrieve coordinates from stream
        unsigned long long xsize,ysize,zsize;
        char buffer[8];
        cin.read(buffer, 8);
        xsize = *((unsigned long long*) buffer);
        cin.read(buffer, 8);
        ysize = *((unsigned long long*) buffer);
        cin.read(buffer, 8);
        zsize = *((unsigned long long*) buffer);

        // create buffer
        VolumeLabelPtr initial_labels = VolumeLabelData::create_volume(xsize, ysize, zsize);
        
        // set value from stream (only 32 bit for now)
        volume_forXYZ(*initial_labels, x, y, z) {
            cin.read(buffer, 8);
            unsigned long long temp_vert = *((unsigned long long*) buffer);
            initial_labels->set(x,y,z, (unsigned int)(temp_vert));
        }

        // create stack to hold segmentation state
        BioStack stack(initial_labels); 

        // make new build_rag for stack (ignore 0s, add edge on greater than,
        // ignore 1 pixel border for vertex accum
        stack.build_rag_batch();
            
        RagPtr rag = stack.get_rag();
       
        // iterate RAG vertices and nodes and update values
        int num_vertices = rag->get_num_regions();
        int num_edges = rag->get_num_edges();
        char* arr = new char[8*(num_vertices*2 + 2 + num_edges*3)];
        int offset = 0;
        
        // iterator
        unsigned long long * ptr = (unsigned long long*) arr;

        // load vertices
        *ptr = (unsigned long long)(num_vertices);
        ++ptr;
        for (Rag_t::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
            *ptr = (unsigned long long)((*iter)->get_node_id());
            ++ptr;
            double *temp = (double*) ptr;
            *temp = ((*iter)->get_size());
            ++ptr;
        } 

        //ofstream fout("temp.out");

        // load edges
        *ptr = (unsigned long long)(num_edges);
        ptr++;
        for (Rag_t::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {
            *ptr = (unsigned long long)((*iter)->get_node1()->get_node_id());
            ++ptr;
            *ptr = (unsigned long long)((*iter)->get_node2()->get_node_id());
            ++ptr;
            double *temp = (double*) ptr;
            //fout << (*iter)->get_node1()->get_node_id() << " " << (*iter)->get_node2()->get_node_id() << " " << (*iter)->get_size() << endl;
            *temp = ((*iter)->get_size());
            ++ptr;       
        } 
        //fout.close(); 

        string output(arr, 8*(num_vertices*2 + 2 + num_edges*3));
        cout << output << endl; 

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
    run_graph_build(options);

    return 0;
}


