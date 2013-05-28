#include "DataStructures/Stack.h"
#include "Priority/GPR.h"
#include "Priority/LocalEdgePriority.h"
#include "Utilities/ScopeTime.h"
#include "Rag/RagIO.h"

#include "Utilities/h5read.h"
#include "Utilities/h5write.h"
#include "Utilities/OptionParser.h"

#include <fstream>
#include <sstream>
#include <cassert>
#include <iostream>
#include <memory>
#include <json/json.h>
#include <json/value.h>

#include <ctime>
#include <cmath>
#include <cstring>

#include <boost/algorithm/string/predicate.hpp>
#include <tr1/unordered_map>

#include <json/json.h>
#include <json/value.h>

using std::cerr; using std::cout; using std::endl;
using std::ifstream;
using std::string;
using std::stringstream;
using namespace NeuroProof;
using std::vector;
using namespace boost::algorithm;

using std::tr1::unordered_map;

static const char * SEG_DATASET_NAME = "stack";
static const char * PRED_DATASET_NAME = "volume/predictions";

typedef boost::tuple<unsigned int, unsigned int, unsigned int> Location;

void remove_inclusions(StackPredict* stackp)
{
    cout<<"Inclusion removal ..."; 
    stackp->remove_inclusions();
    cout<<"done with "<< stackp->get_num_bodies()<< " regions\n";	
    stackp->compute_vi();  	

}

template <class T>
void padZero(T* data, const size_t* dims, int pad_length, T** ppadded_data){

    // implemented only for 3D arrays

    unsigned long int newsize = (dims[0]+2*pad_length)*(dims[1]+2*pad_length)*(dims[2]+2*pad_length);	
    *ppadded_data = new T[newsize];  	
    T* padded_data = *ppadded_data;

    memset((void*) padded_data, 0, sizeof(T)*newsize);

    unsigned int width, plane_size, width0, plane_size0, i0,j0,k0, i,j,k;

    for (i=pad_length, i0=0; i<= dims[0] ; i++, i0++)
        for (j=pad_length, j0=0; j<= dims[1]; j++, j0++)	
            for(k=pad_length, k0=0; k<= dims[2]; k++, k0++){
                plane_size = (dims[1]+2*pad_length)*(dims[2]+2*pad_length);	
                width = dims[2]+2*pad_length;	

                plane_size0 = dims[1]*dims[2];	
                width0 = dims[2];	

                padded_data[i*plane_size+ j*width + k] = data[i0*plane_size0 + j0*width0 + k0];	
            }
}

struct PredictOptions
{
    PredictOptions(int argc, char** argv) : synapse_filename(""), output_filename("segmentation.h5"),
        graph_filename("graph.json"), threshold(0.2), watershed_threshold(0), post_synapse_threshold(0.0),
        merge_mito(true), agglo_type(1), enable_transforms(true), postseg_classifier_filename("")
    {
        OptionParser parser("Program that predicts edge confidence for a graph and merges confident edges");

        // positional arguments
        parser.add_positional(watershed_filename, "watershed-file",
                "gala h5 file with label volume (z,y,x) and body mappings"); 
        parser.add_positional(prediction_filename, "prediction-file",
                "ilastik h5 file (x,y,z,ch) that has pixel predictions"); 
        parser.add_positional(classifier_filename, "classifier-file",
                "opencv or vigra agglomeration classifier"); 

        // optional arguments
        parser.add_option(synapse_filename, "synapse-file",
                "json file that contains synapse annotations that are used as constraints in merging"); 
        parser.add_option(output_filename, "output-file",
                "h5 file that will contain the output segmentation (z,y,x) and body mappings"); 
        parser.add_option(graph_filename, "graph-file",
                "json file that will contain the output graph"); 
        parser.add_option(threshold, "threshold",
                "segmentation threshold"); 
        parser.add_option(watershed_threshold, "watershed-threshold",
                "threshold used for removing small bodies as a post-process step"); 
        parser.add_option(postseg_classifier_filename, "postseg-classifier-file",
                "opencv or vigra agglomeration classifier to be used after agglomeration to assign confidence to the graph edges -- classifier-file used if not specified"); 
        parser.add_option(post_synapse_threshold, "post-synapse-threshold",
                "Merge synapses indepedent of constraints"); 

        // invisible arguments
        parser.add_option(merge_mito, "merge-mito",
                "perform separate mitochondrion merge phase", true, false, true); 
        parser.add_option(agglo_type, "agglo-type",
                "merge mode used", true, false, true); 
        parser.add_option(enable_transforms, "transforms",
                "enables using the transforms table when reading the segmentation", true, false, true); 

        parser.parse_options(argc, argv);
    }

    // manadatory positionals
    string watershed_filename;
    string prediction_filename;
    string classifier_filename;
   
    // optional (with default values) 
    string synapse_filename;
    string output_filename;
    string graph_filename;

    double threshold;
    int watershed_threshold; // might be able to increase default to 500
    string postseg_classifier_filename;
    double post_synapse_threshold;

    // hidden options (with default values)
    bool merge_mito;
    int agglo_type;
    bool enable_transforms;
};

int main(int argc, char** argv) 
{
    int          i, j, k;
    
    PredictOptions options(argc, argv);
   
    // options not set by the command line
    bool read_off_rwts = false;

    ScopeTime timer;

    unordered_map<Label, Label> sp2body;
    if (options.enable_transforms) {
        // read transforms from watershed/segmentation file
        H5Read * transforms = new H5Read(options.watershed_filename.c_str(), "transforms");
        unsigned long long* transform_data=0;	
        transforms->readData(&transform_data);	
        int transform_height = transforms->dim()[0];
        int transform_width = transforms->dim()[1];
        delete transforms;

        // create sp to body map
        int tpos = 0;
        sp2body[0] = 0;
        for (int i = 0; i < transform_height; ++i, tpos+=2) {
            sp2body[(transform_data[tpos])] = transform_data[tpos+1];
        }
        delete[] transform_data;
    }

    H5Read * watershed = new H5Read(options.watershed_filename.c_str(),SEG_DATASET_NAME);	
    Label* watershed_data=NULL;	
    watershed->readData(&watershed_data);	
    int depth =	 watershed->dim()[0];
    int height = watershed->dim()[1];
    int width =	 watershed->dim()[2];
    size_t dims[3];
    dims[0] = depth;
    dims[1] = height;
    dims[2] = width;
    delete watershed; 

    // map supervoxel ids to body ids
    unsigned long int total_size = depth * height * width;
    if (options.enable_transforms) {
        for (unsigned long int i = 0; i < total_size; ++i) {
            watershed_data[i] = sp2body[(watershed_data[i])];
        }
    }

    int pad_len=1;
    Label *zp_watershed_data=NULL;
    padZero(watershed_data, dims,pad_len,&zp_watershed_data);	
    delete[] watershed_data;


    StackPredict* stackp = new StackPredict(zp_watershed_data, depth+2*pad_len, height+2*pad_len, width+2*pad_len, pad_len);
    stackp->set_feature_mgr(new FeatureMgr());
	

    H5Read prediction(options.prediction_filename.c_str(),PRED_DATASET_NAME);	
    float* prediction_data=0;
    prediction.readData(&prediction_data);	
 
    double* prediction_single_ch = new double[depth*height*width];
    double* prediction_ch0 = new double[depth*height*width];
    	
    size_t cube_size, plane_size, element_size=prediction.dim()[prediction.total_dim()-1];
    size_t position, count;		    	
    for (int ch=0; ch < element_size; ch++){
	count = 0;
        for(i=0; i<depth; i++){
	    cube_size = height*depth*element_size;	
	    for(j=0; j<height; j++){
		plane_size = depth*element_size;
		for(k=0; k<width; k++){
		    position = k*cube_size + j*plane_size + i*element_size + ch;
		    prediction_single_ch[count] = prediction_data[position];
		    count++;					
		}		
	    }	
	}
	
	double* zp_prediction_single_ch = 0;
	padZero(prediction_single_ch,dims,pad_len,&zp_prediction_single_ch);
	if (ch == 0)
	    memcpy(prediction_ch0, prediction_single_ch, depth*height*width*sizeof(double));	

        stackp->add_empty_channel();
	stackp->add_prediction_channel(zp_prediction_single_ch);
    }

    delete [] prediction_data;
    delete [] prediction_single_ch;

    stackp->set_basic_features();



    EdgeClassifier* eclfr;
    if (ends_with(options.classifier_filename, ".h5"))
    	eclfr = new VigraRFclassifier(options.classifier_filename.c_str());	
    else if (ends_with(options.classifier_filename, ".xml")) 	
	eclfr = new OpencvRFclassifier(options.classifier_filename.c_str());	

    stackp->get_feature_mgr()->set_classifier(eclfr);   	 

    cout<<"Building RAG ..."; 	
    stackp->build_rag();
    cout<<"done with "<< stackp->get_num_bodies()<< " regions\n";	
    
    // add synapse constraints (send json to stack function)
    if (options.synapse_filename != "") {   
        stackp->set_exclusions(options.synapse_filename);
    }

    remove_inclusions(stackp);
    stackp->compute_vi();  	
    
    switch (options.agglo_type) {
        case 0: 
            cout<<"Agglomerating (flat) upto threshold "<< options.threshold<< " ..."; 
            stackp->agglomerate_rag_flat(options.threshold);	
            break;
        case 1:
            cout<<"Agglomerating (agglo) upto threshold "<< options.threshold<< " ..."; 
            stackp->agglomerate_rag(options.threshold, false);
            break;        
        case 2:
            cout<<"Agglomerating (mrf) upto threshold "<< options.threshold<< " ..."; 
            stackp->agglomerate_rag_mrf(options.threshold, read_off_rwts,
                    options.output_filename, options.classifier_filename);	
            break;
        case 3:
            cout<<"Agglomerating (queue) upto threshold "<< options.threshold<< " ..."; 
            stackp->agglomerate_rag_queue(options.threshold, false);	
            break;
        case 4:
            cout<<"Agglomerating (flat) upto threshold "<< options.threshold<< " ..."; 
            stackp->agglomerate_rag_flat(options.threshold, false, options.output_filename,
                    options.classifier_filename);	
            break;
        default: throw ErrMsg("Illegal agglomeration type specified");
    }
    cout << "Done with "<< stackp->get_num_bodies()<< " regions\n";
    

    if (options.post_synapse_threshold > 0.00001) {
        cout << "Agglomerating (agglo) ignoring synapse constraints upto threshold "
            << options.post_synapse_threshold << endl;
        string dummy1, dummy2;
        stackp->agglomerate_rag(options.post_synapse_threshold, false,
                dummy1, dummy2, true);
        cout << "Done with "<< stackp->get_num_bodies()<< " regions\n";
    }
    
    remove_inclusions(stackp);

    if (options.merge_mito){
	cout<<"Merge Mitochondria (border-len) ..."; 
	stackp->merge_mitochondria_a();
    	cout<<"done with "<< stackp->get_num_bodies()<< " regions\n";	

        remove_inclusions(stackp);        
    } 	

    if (options.watershed_threshold > 0) {
        size_t dims_out[3];
        dims_out[0]=depth; dims_out[1]= height; dims_out[2]= width;
        Label * temp_label_volume1D = stackp->get_label_volume();       	    
        cout << "Removing small bodies ... ";
        int num_removed = stackp->absorb_small_regions2(prediction_ch0, temp_label_volume1D,
                options.watershed_threshold);
        Label * padded_data;
        padZero(temp_label_volume1D, dims_out, pad_len, &padded_data);
        delete[] temp_label_volume1D;	

        stackp->reinit_watershed(padded_data);
        cout << num_removed << " removed" << endl;	
    }
    delete[] prediction_ch0;

    // recompute rag
    stackp->reinit_rag();
    stackp->get_feature_mgr()->clear_features();
    if (options.postseg_classifier_filename == "") {
        options.postseg_classifier_filename = options.classifier_filename;
    }

    delete eclfr;
    if (ends_with(options.postseg_classifier_filename, ".h5"))
    	eclfr = new VigraRFclassifier(options.postseg_classifier_filename.c_str());	
    else if (ends_with(options.postseg_classifier_filename, ".xml")) 	
	eclfr = new OpencvRFclassifier(options.postseg_classifier_filename.c_str());	
    
    stackp->get_feature_mgr()->set_classifier(eclfr);   	 
    stackp->build_rag();
    


    // add synapse constraints (send json to stack function)
    if (options.synapse_filename != "") {   
        stackp->set_exclusions(options.synapse_filename);
    }
    stackp->determine_edge_locations();
   
    // set edge properties for export 
    Rag<Label>* rag = stackp->get_rag();
    for (Rag<Label>::edges_iterator iter = rag->edges_begin();
           iter != rag->edges_end(); ++iter) {
        if (!((*iter)->is_false_edge())) {
            double val = stackp->get_edge_weight((*iter));
            (*iter)->set_weight(val);
        }
        Label x = 0;
        Label y = 0;
        Label z = 0;
        try {
            stackp->get_edge_loc((*iter), x, y, z);
        } catch (ErrMsg& msg) {
            //
        }
        (*iter)->set_property("location", Location(x,y,z));
    }

    // write out graph json
    Json::Value json_writer;
    ofstream fout(options.graph_filename.c_str());
    if (!fout) {
        throw ErrMsg("Error: output file " + options.graph_filename + " could not be opened");
    }
    
    bool status = create_json_from_rag(stackp->get_rag(), json_writer, false);
    if (!status) {
        throw ErrMsg("Error in rag export");
    }
    stackp->write_graph_json(json_writer);
    fout << json_writer;
    fout.close();
 

   


    // write out transforms (identity)
    hsize_t dims_out2[2];
    dims_out2[0] = rag->get_num_regions() + 1;
    dims_out2[1] = 2;
    unsigned long long * transform_data = new unsigned long long [(rag->get_num_regions()+1) * 2];
    transform_data[0] = 0;
    transform_data[1] = 0;
    int tpos = 2;
    for (Rag<Label>::nodes_iterator iter = rag->nodes_begin();
           iter != rag->nodes_end(); ++iter, tpos+=2) {
        transform_data[tpos] = (*iter)->get_node_id();
        transform_data[tpos+1] = (*iter)->get_node_id();
    }




    // write out label volume
    Label* temp_label_volume1D = stackp->get_label_volume();       	    



    hsize_t dims_out[3];
    dims_out[0]=depth; dims_out[1]= height; dims_out[2]= width;
    H5Write(options.output_filename.c_str(),SEG_DATASET_NAME,3,dims_out, temp_label_volume1D,
        "transforms",2,dims_out2, transform_data,
        options.output_filename == options.watershed_filename);
    
    delete eclfr;
    delete stackp; 
    delete[] temp_label_volume1D;
    delete[] transform_data;

    return 0;
}


