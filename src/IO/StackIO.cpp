#include "StackIO.h"
#include "RagIO.h"
#include <Stack/Stack.h>

// used for importing h5 files
#include <vigra/hdf5impex.hxx>
#include <vigra/impex.hxx>


// assume all label volumes are written to "stack" for now
static const char * SEG_DATASET_NAME = "stack";

// assume all grayscale volumes are written to "gray" for now
static const char * GRAY_DATASET_NAME = "gray";

/*!
 * Support function called by 'serialize_stack' that actually writes
 * the graph to json on disk.
 * \param graph_name name of graph json file
 * \param optimal_prob_edge_loc determine strategy to select edge location
 * \param disable_prob_comp determines whether saved prob values are used
*/
void export_stack_graph(const char* graph_name, bool optimal_prob_edge_loc, bool disable_prob_comp = false);


/*!
 * Support function called by 'serialize_stack' that actually writes
 * the volume labels to h5 on disk.
 * \param h5_name name of h5 file
*/
void export_labelsh5(const char* h5_name);


Stack import_h5stack(std::string stack_name)
{
    Stack stack(VolumeLabelPtr());
    
    try {
        labelvol = VolumeLabelData::create_volume(stack_name.c_str(),
                SEG_DATASET_NAME);
    } catch (std::runtime_error &error) {
        throw ErrMsg(stack_name + string(" not loaded"));
    }
 
    try {
        grayvol = VolumeGray::create_volume(stack_name.c_str(),
                GRAY_DATASET_NAME);
    } catch (std::runtime_error &error) {
        // allow stacks without grayscale volumes
    }
}



}

VolumeLabelPtr import_h5labels(const char * h5_name,
        const char * dset, bool use_transforms)
{
    vigra::HDF5ImportInfo info(h5_name, dset);
    vigra_precondition(info.numDimensions() == 3, "Dataset must be 3-dimensional.");

    VolumeLabelPtr volumedata = VolumeLabelData::create_volume();
    vigra::TinyVector<long long unsigned int,3> shape(info.shape().begin());
    volumedata->reshape(shape);
    vigra::readHDF5(info, *volumedata);

    if (use_transforms) {
        // looks for a dataset called transforms which is a label
        // to label mapping
        try {
            vigra::HDF5ImportInfo info(h5_name, "transforms");
            vigra::TinyVector<long long unsigned int,2> tshape(info.shape().begin()); 
            vigra::MultiArray<2,long long unsigned int> transforms(tshape);
            vigra::readHDF5(info, transforms);

            for (int row = 0; row < transforms.shape(1); ++row) {
                volumedata->label_mapping[transforms(0,row)] = transforms(1,row);
            }
            // rebase all of the labels so the initial label hash is empty
            volumedata->rebase_labels();
        } catch (std::runtime_error& err) {
        }
    }

    return volumedata; 
}

template <typename T>
boost::shared_ptr<VolumeData<T> >::import_3Dh5vol(
        const char * h5_name, const char * dset)

{
    vigra::HDF5ImportInfo info(h5_name, dset);
    vigra_precondition(info.numDimensions() == 3, "Dataset must be 3-dimensional.");

    shared_ptr<Volumedata<T> > volumedata = VolumeData<T>::create_volume();

    vigra::TinyVector<long long unsigned int,3> shape(info.shape().begin());
    volumedata->reshape(shape);
    // read h5 file into volumedata with correctly set shape
    vigra::readHDF5(info, *volumedata);

    return volumedata; 
}

template <typename T>
vector<shared_ptr<VolumeData<T> > > 
    import_3Dh5vol_array(const char * h5_name, const char * dset)
{
    vigra::HDF5ImportInfo info(h5_name, dset);
    vigra_precondition(info.numDimensions() == 4, "Dataset must be 4-dimensional.");

    vigra::TinyVector<long long unsigned int,4> shape(info.shape().begin());
    vigra::MultiArray<4, T> volumedata_temp(shape);
    vigra::readHDF5(info, volumedata_temp);
    
    // since the X,Y,Z,ch is read in as ch,Z,Y,X transpose
    volumedata_temp = volumedata_temp.transpose();

    vector<VolumeProbPtr> vol_array;
    vigra::TinyVector<long long unsigned int,3> shape2;

    // tranpose the shape dimensions as well
    shape2[0] = shape[3];
    shape2[1] = shape[2];
    shape2[2] = shape[1];

    // for each channel, create volume data and push in array
    for (int i = 0; i < shape[0]; ++i) {
        shared_ptr<Volumedata<T> > volumedata = VolumeData<T>::create_volume();
        vigra::TinyVector<vigra::MultiArrayIndex, 1> channel(i);
        (*volumedata) = volumedata_temp.bindOuter(channel); 
        
        vol_array.push_back(boost::shared_ptr<VolumeData<T> >(volumedata));
    }

    return vol_array; 
}

template <typename T>
vector<boost::shared_ptr<VolumeData<T> > > 
    import_3Dh5vol_array(const char * h5_name, const char * dset,
            unsigned int dim1size)
{
    vigra::HDF5ImportInfo info(h5_name, dset);
    vigra_precondition(info.numDimensions() == 4, "Dataset must be 4-dimensional.");

    vigra::TinyVector<long long unsigned int,4> shape(info.shape().begin());
    vigra::MultiArray<4, T> volumedata_temp(shape);
    vigra::readHDF5(info, volumedata_temp);
    
    // since the X,Y,Z,ch is read in as ch,Z,Y,X transpose
    volumedata_temp = volumedata_temp.transpose();

    vector<VolumeProbPtr> vol_array;
    vigra::TinyVector<long long unsigned int,3> shape2;

    // tranpose the shape dimensions as well
    shape2[0] = shape[3];
    shape2[1] = shape[2];
    shape2[2] = shape[1];

    // prediction must be the same size or larger than the label volume
    if (dim1size > shape2[0]) {
        throw ErrMsg("Label volume has a larger dimension than the prediction volume provided");
    }
    
    // extract border from shape and size of label volume
    unsigned int border = (shape2[0] - dim1size) / 2;

    // if a border needs to be applied the volume should be equal size in all dimensions
    // TODO: specify borders for each dimension
    if (border > 0) {
        if ((shape2[0] != shape2[1]) || (shape2[0] != shape2[2])) {
            throw ErrMsg("Dimensions of prediction should be equal in X, Y, Z");
        }
    }



    // for each channel, create volume data and push in array
    for (int i = 0; i < shape[0]; ++i) {
        shared_ptr<Volumedata<T> > volumedata = VolumeData<T>::create_volume();
        vigra::TinyVector<vigra::MultiArrayIndex, 1> channel(i);
        (*volumedata) = (volumedata_temp.bindOuter(channel)).subarray(
                vigra::Shape3(border, border, border), vigra::Shape3(shape2[0]-border,
                    shape2[1]-border, shape2[2]-border)); 
        
        vol_array.push_back(boost::shared_ptr<VolumeData<T> >(volumedata));
    }

    return vol_array; 
}

shared_ptr<VolumeData<unsigned char> > import_8bit_images(
        vector<string>& file_names)
{
    assert(!file_names.empty());
    vigra::ImageImportInfo info_init(file_names[0].c_str());
    
    if (!info_init.isGrayscale()) {
        throw ErrMsg("Cannot read non-grayscale image stack");
    }

    shared_ptr<Volumedata<unsigned char> > volumedata =
        VolumeData<unsigned char>::create_volume();
    volumedata->reshape(vigra::MultiArrayShape<3>::type(info_init.width(),
                info_init.height(), file_names.size()));

    for (int i = 0; i < file_names.size(); ++i) {
        vigra::ImageImportInfo info(file_names[i].c_str());
        vigra::BImage image(info.width(), info.height());
        vigra::importImage(info,destImage(image));
        for (int y = 0; y < int(info.height()); ++y) {
            for (int x = 0; x < int(info.width()); ++x) {
                (*volumedata)(x,y,i) = image(x,y); 
            }
        }  
    }

    return volumedata;
}


void export_3Dh5vol(shared_ptr<VolumeData<T> > volume,
        const char* h5_name, const char * h5_path)
{
    // x,y,z data will be written as z,y,x in the h5 file by default
    vigra::writeHDF5(h5_name, h5_path, *volume);
}

void import_stack_exclusions(Stack* stack, string exclusions_json)
{
    Json::Reader json_reader;
    Json::Value json_vals;
    unordered_set<Label_t> exclusion_set;
    
    ifstream fin(exclusions_json.c_str());
    if (!fin) {
        throw ErrMsg("Error: input file: " + exclusions_json + " cannot be opened");
    }
    if (!json_reader.parse(fin, json_vals)) {
        throw ErrMsg("Error: Json incorrectly formatted");
    }
    fin.close();
    
    // all exclusions should be in a json list
    Json::Value exclusions = json_vals["exclusions"];
    for (unsigned int i = 0; i < json_vals["exclusions"].size(); ++i) {
        exclusion_set.insert(exclusions[i].asUInt());
    }

    VolumeLabelPtr labelvol = stack->get_labelvol();

    // 0 out all bodies in the exclusion list    
    volume_forXYZ(*labelvol, x, y, z) {
        Label_t label = (*labelvol)(x,y,z);
        if (exclusion_set.find(label) != exclusion_set.end()) {
            labelvol->set(x,y,z,0);
        }
    }
}

void export_stack(Stack* stack, const char* h5_name, const char* graph_name, 
        bool optimal_prob_edge_loc, bool disable_prob_comp)
{
    if (graph_name != 0) {
        export_stack_graph(stack, graph_name,
                optimal_prob_edge_loc, disable_prob_comp);
    }
    export_labelsh5(stack, h5_name);
   
    if (grayvol) {
        export_3Dh5vol(graphvol, h5_name, GRAY_DATASET_NAME)
    } 
}

void Stack::export_labelsh5(Stack* stack, const char* h5_name)
{
    stack->get_labelvol()->rebase_labels();
    export_3Dh5vol(labelvol-stack->get_labelvol(), h5_name, SEG_DATASET_NAME)
}


void export_stack_graph(Stack* stack, const char* graph_name,
        bool optimal_prob_edge_loc, bool disable_prob_comp)
{
    EdgeCount best_edge_z;
    EdgeLoc best_edge_loc;
    stack->determine_edge_locations(best_edge_z, best_edge_loc, optimal_prob_edge_loc);
    RagPtr rag = stack->get_rag();

    // set edge properties for export 
    for (Rag_t::edges_iterator iter = rag->edges_begin();
           iter != rag->edges_end(); ++iter) {
        if (!((*iter)->is_false_edge()) && !disable_prob_comp) {
            if (feature_manager) {
                double val = feature_manager->get_prob((*iter));
                (*iter)->set_weight(val);
            } 
        }
        Label_t x = 0;
        Label_t y = 0;
        Label_t z = 0;
        
        if (best_edge_loc.find(*iter) != best_edge_loc.end()) {
            Location loc = best_edge_loc[*iter];
            x = boost::get<0>(loc);
            // assume y is bottom of image
            // (technically ignored by raveler so okay)
            y = boost::get<1>(loc); //height - boost::get<1>(loc) - 1;
            z = boost::get<2>(loc);
        }
        
        (*iter)->set_property("location", Location(x,y,z));
    }

    Json::Value json_writer;
    bool status = create_json_from_rag(rag.get(), json_writer);
    if (!status) {
        throw ErrMsg("Error in rag export");
    }

    // biopriors might write specific information -- calls derived function
    serialize_graph_info(json_writer);

    int id = 0;
    for (Rag_t::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
        if (!((*iter)->is_boundary())) {
            json_writer["orphan_bodies"][id] = (*iter)->get_node_id();
            ++id;
        } 
    }
    
    // write out graph json
    ofstream fout(graph_name);
    if (!fout) {
        throw ErrMsg("Error: output file " + string(graph_name) + " could not be opened");
    }
    
    fout << json_writer;
    fout.close();
}
