#include <iostream>

#include <Utilities/ScopeTime.h>
#include <Classifier/vigraRFclassifier.h>
#include <Classifier/opencvRFclassifier.h>

#include <FeatureManager/FeatureMgr.h>
#include <BioPriors/BioStack.h>
#include <BioPriors/StackAgglomAlgs.h>

#include <json/json.h>
#include <sstream>
#include <iomanip>
#include <Python.h>
#include <boost/python.hpp>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

// http://docs.scipy.org/doc/numpy/reference/c-api.array.html#importing-the-api
#define PY_ARRAY_UNIQUE_SYMBOL libdvid_PYTHON_BINDINGS

#include <numpy/arrayobject.h>
#include <boost/algorithm/string/predicate.hpp>
#include "converters.hpp"

using namespace boost::python;
using namespace boost::algorithm;
using std::vector;
using std::cout; using std::endl;


namespace NeuroProof { namespace python {

// convert from hex to string bytes
void hex2bytes(string& original, string& processed)
{
    for (int m = 0; m < original.size(); m+=2) {
        std::istringstream istr(original.substr(m, 2));
        int byteval;
        istr >> std::hex >> byteval;
        processed += char(byteval);
    }
}

string byte2hex(string& original)
{
    // convert to hex string
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (int m = 0; m < original.size(); ++m) {
        ss << std::setw(2) << int((unsigned char)(original[m]));
    }
    return ss.str();
}


// JSON converter copied from libdvid
//!********************************************************************************************* 
//! This converts Json::Value objects into python dict objects. 
//! NOTE: This isn't an efficient conversion method.
//!*********************************************************************************************
struct json_value_to_dict
{
    json_value_to_dict()
    {
        using namespace boost::python;
        to_python_converter<Json::Value, json_value_to_dict>();
    }

    static PyObject* convert(Json::Value const & json_value)
    {
        using namespace boost::python;

        // For now, easiest thing to do is just export as
        //  string and re-parse via python's json module.
        std::ostringstream ss;
        ss << json_value;

        object json = import("json");
        return incref(json.attr("loads")( ss.str() ).ptr());
    }
};


Json::Value combine_edge_features(string edge1_str, string edge2_str, int num_channels)
{
    Json::Reader reader;
    Json::Value edge1, edge2;
    reader.parse(edge1_str, edge1); 
    reader.parse(edge2_str, edge2); 

    // create feature manager and load classifier
    FeatureMgrPtr feature_manager(new FeatureMgr(num_channels));
    feature_manager->set_basic_features(); 

    // create small rag with two vertices and edge 
    Rag_t* rag = new Rag_t;

    unsigned long long n1 = edge1["Id1"].asUInt64();
    unsigned long long n2 = edge2["Id2"].asUInt64();

    RagNode_t* rn1 = rag->insert_rag_node(n1);
    RagNode_t* rn2 = rag->insert_rag_node(n2);

    RagEdge_t* redge = rag->insert_rag_edge(rn1, rn2);

    string edge1_features = edge1["Features"].asString();
    string edge2_features = edge2["Features"].asString();
    
    // convert from hex to bytes
    string edge1_bytes;
    string edge2_bytes;
    hex2bytes(edge1_features, edge1_bytes);
    hex2bytes(edge2_features, edge2_bytes);

    feature_manager->deserialize_features((char*) edge1_bytes.c_str(), redge);

    // combine features  
    string feature_data = feature_manager->serialize_features((char*) edge2_bytes.c_str(), redge);


    unsigned long long edge1_size = edge1["Weight"].asUInt64();
    unsigned long long edge2_size = edge2["Weight"].asUInt64();

    // update edge size
    edge1["Weight"] = edge1_size + edge2_size;
    
    // update edge features
    edge1["Features"] = byte2hex(feature_data);
    
    delete rag;

    return edge1;
}

Json::Value combine_vertex_features(string node1_str, string node2_str, int num_channels)
{
    Json::Reader reader;
    Json::Value node1, node2;
    reader.parse(node1_str, node1); 
    reader.parse(node2_str, node2); 

    // create feature manager and load classifier
    FeatureMgrPtr feature_manager(new FeatureMgr(num_channels));
    feature_manager->set_basic_features(); 

    // create small rag with two vertices and edge 
    Rag_t* rag = new Rag_t;

    unsigned long long n1 = node1["Id"].asUInt64();
    unsigned long long n2 = node2["Id"].asUInt64();

    RagNode_t* rn1 = rag->insert_rag_node(n1);
    RagNode_t* rn2 = rag->insert_rag_node(n2);

    string node1_features = node1["Features"].asString();
    string node2_features = node2["Features"].asString();
    
    // convert from hex to bytes
    string node1_bytes;
    string node2_bytes;
    hex2bytes(node1_features, node1_bytes);
    hex2bytes(node2_features, node2_bytes);
    
    feature_manager->deserialize_features((char*) node1_bytes.c_str(), rn1);


    // combine features  
    string feature_data = feature_manager->serialize_features((char*) node2_bytes.c_str(), rn1);

    unsigned long long node1_size = node1["Weight"].asUInt64();
    unsigned long long node2_size = node2["Weight"].asUInt64();

    // update edge size
    node1["Weight"] = node1_size + node2_size;
    
    // update edge features
    node1["Features"] = byte2hex(feature_data);
    
    delete rag;

    return node1;
}

// extract features for nodes and edges in the given subvolume, return a list of features as JSON
Json::Value extract_features(VolumeLabelPtr labels, vector<VolumeProbPtr> prob_array)
{
    ScopeTime timer;

    // create feature manager and load classifier
    FeatureMgrPtr feature_manager(new FeatureMgr(prob_array.size()));
    feature_manager->set_basic_features(); 

    // create stack to hold segmentation state
    BioStack stack(labels); 
    stack.set_feature_manager(feature_manager);
    stack.set_prob_list(prob_array);

    // build graph
    // make new build_rag for stack (ignore 0s, add edge on greater than,
    // ignore 1 pixel border for vertex accum)
    stack.build_rag_batch();
    cout<< "Initial number of regions: "<< stack.get_num_labels()<< endl;	

    RagPtr rag = stack.get_rag();

    Json::Value json_data;

    int i = 0;
    for (Rag_t::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
        // empty nodes should get ignored because other processes will find
        if ((*iter)->get_size() == 0) {
            continue;
        }
        Json::Value node_data;
        node_data["Id"] = (*iter)->get_node_id();    
        node_data["Weight"] = (*iter)->get_size();    

        string feature_data = feature_manager->serialize_features(0, (*iter));
        
        // convert to hex string
        node_data["Features"] = byte2hex(feature_data);
        
        json_data["Vertices"][i] = node_data;
        ++i;
    } 

    i = 0;
    for (Rag_t::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter, ++i) {
        Json::Value edge_data;
        edge_data["Id1"] = (*iter)->get_node1()->get_node_id();    
        edge_data["Id2"] = (*iter)->get_node2()->get_node_id();    
        edge_data["Weight"] = (*iter)->get_size();    

        string feature_data = feature_manager->serialize_features(0, (*iter));

        // convert to hex string
        edge_data["Features"] = byte2hex(feature_data);

        json_data["Edges"][i] = edge_data;
    }

    return json_data; 
}

class ComputeProbPy {
  public:
    ComputeProbPy(std::string fn, int num_channels) : feature_manager(new FeatureMgr(num_channels))
    {
        feature_manager->set_basic_features(); 
        if (ends_with(fn, ".h5")) {
            eclfr = new VigraRFclassifier(fn.c_str());	
        } else if (ends_with(fn, ".xml")) {	
            eclfr = new OpencvRFclassifier(fn.c_str());	
        }
        feature_manager->set_classifier(eclfr);   	 
        rag = new Rag_t;
    }
    ~ComputeProbPy()
    {
        delete eclfr;
        delete rag;
    
    }
    double compute_prob(string edge_str, string node1_str, string node2_str)
    {
        Json::Reader reader;
        Json::Value edge, node1, node2;
        reader.parse(edge_str, edge); 
        reader.parse(node1_str, node1); 
        reader.parse(node2_str, node2); 

        unsigned long long n1 = node1["Id"].asUInt64();
        unsigned long long n2 = node2["Id"].asUInt64();
        
        // load node1 and node2 if does not exist already
        RagNode_t* rn1 = rag->find_rag_node(n1);
        RagNode_t* rn2 = rag->find_rag_node(n2);
        if (!rn1) {
            rn1 = rag->insert_rag_node(n1);
            string node1_features = node1["Features"].asString();
            
            // convert hex to bytes
            string curr_features;
            hex2bytes(node1_features, curr_features);

            feature_manager->deserialize_features((char*) curr_features.c_str(), rn1);
        }
        if (!rn2) {
            rn2 = rag->insert_rag_node(n2);
            string node2_features = node2["Features"].asString();

            // convert hex to bytes 
            string curr_features;
            hex2bytes(node2_features, curr_features);

            feature_manager->deserialize_features((char*) curr_features.c_str(), rn2);
        }

        // load edge if does not exist already
        RagEdge_t* redge = rag->find_rag_edge(rn1, rn2);
        if (!redge) {
            redge = rag->insert_rag_edge(rn1, rn2);
            string edge_features = edge["Features"].asString();
           
            // convert hex to bytes
            string curr_features;
            hex2bytes(edge_features, curr_features);
            
            feature_manager->deserialize_features((char*) curr_features.c_str(), redge);
        }        

        return feature_manager->get_prob(redge); 
    }

  private:
    FeatureMgrPtr feature_manager;
    EdgeClassifier* eclfr;
    Rag_t* rag;
};




BOOST_PYTHON_MODULE(_focusedproofreading_python)
{
    // http://docs.scipy.org/doc/numpy/reference/c-api.array.html#importing-the-api
    import_array();
    ndarray_to_segmentation();
    ndarray_to_predictionarray(); 
    json_value_to_dict();

    def("extract_features" , extract_features);
    def("combine_edge_features" , combine_edge_features);
    def("combine_vertex_features" , combine_vertex_features);
    
    class_<ComputeProbPy>("ComputeProb", init<std::string, int>())
        .def("compute_prob", &ComputeProbPy::compute_prob)
        ;
}


}}
