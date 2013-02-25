#include "FeatureManager/FeatureManager.h"
#include "DataStructures/Rag.h"
#include "Algorithms/RagAlgs.h"
#include "ImportsExports/ImportExportRagPriority.h"
#include "Utilities/ErrMsg.h"



#include <boost/python.hpp>

//#include <boost/numeric/ublas/matrix.hpp>
#include <iostream>

#include "DataStructures/Stack.h"

using namespace NeuroProof;
using namespace boost::python;
using std::cout;

typedef Rag<Label> Rag_ui;
typedef RagNode<Label> RagNode_ui;
typedef RagEdge<Label> RagEdge_ui;
        

RagNode<Label>* (Rag_ui::*find_rag_node_ptr)(Label) = &Rag_ui::find_rag_node;
RagNode<Label>* (Rag_ui::*insert_rag_node_ptr)(Label) = &Rag_ui::insert_rag_node;
RagEdge<Label>* (Rag_ui::*insert_rag_edge_ptr)(RagNode<Label>*, RagNode<Label>*) = &Rag_ui::insert_rag_edge;
RagEdge<Label>* (Rag_ui::*find_rag_edge_ptr1)(Label, Label) = &Rag_ui::find_rag_edge;
RagEdge<Label>* (Rag_ui::*find_rag_edge_ptr2)(RagNode<Label>*, RagNode<Label>*) = &Rag_ui::find_rag_edge;


void (*rag_add_property_ptr1)(Rag_ui*, RagEdge_ui*, std::string, object) = rag_add_property<Label, object>;
object (*rag_retrieve_property_ptr1)(Rag_ui*, RagEdge_ui*, std::string) = rag_retrieve_property<Label, object>;
void (*rag_bind_edge_property_list_ptr)(Rag_ui*, std::string) = rag_bind_edge_property_list<Label>;

void (*rag_add_property_ptr2)(Rag_ui*, RagNode_ui*, std::string, object) = rag_add_property<Label, object>;
object (*rag_retrieve_property_ptr2)(Rag_ui*, RagNode_ui*, std::string) = rag_retrieve_property<Label, object>;
void (*rag_bind_node_property_list_ptr)(Rag_ui*, std::string) = rag_bind_node_property_list<Label>;

void (*rag_remove_property_ptr1)(Rag_ui*, RagEdge_ui*, std::string) = rag_remove_property<Label>;
void (*rag_remove_property_ptr2)(Rag_ui*, RagNode_ui*, std::string) = rag_remove_property<Label>;

// add some documentation and produce a simple case (showing that copying works and use of generic properties)

// exception handling??
// allow import/export from json to read location property 
// add a serialize and load rag function -- serialize/deserialize individual properties too (Thrift?)
// add a sort function for default types??
// add specific templates for list and double, etc
// expose property list interface
// make c++ rag algorithms to traverse graph, shortest path, etc


// add specific boost types that allows for specific copies -- how to return templated type unless specific??
// !!pass internal reference already for objects where I want to hold the pointer -- transfer pointer perhaps for other situations like a copy of a Rag -- manage_new_object
// ??still can't pass simple types as references or pointers
// ?? return tuple of pointers??


class RagNode_edgeiterator_wrapper {
  public:
    RagNode_edgeiterator_wrapper(RagNode_ui* rag_node_) : rag_node(rag_node_) {}
    RagNode_ui::edge_iterator edge_begin()
    {
        return rag_node->edge_begin();
    }
    RagNode_ui::edge_iterator edge_end()
    {
        return rag_node->edge_end();
    }
  private:
    RagNode_ui* rag_node;
};

RagNode_edgeiterator_wrapper ragnode_get_edges(RagNode_ui& rag_node) {
    return RagNode_edgeiterator_wrapper(&rag_node);
}


class Rag_edgeiterator_wrapper {
  public:
    Rag_edgeiterator_wrapper(Rag_ui* rag_) : rag(rag_) {}
    Rag_ui::edges_iterator edges_begin()
    {
        return rag->edges_begin();
    }
    Rag_ui::edges_iterator edges_end()
    {
        return rag->edges_end();
    }
  private:
    Rag_ui* rag;
};

Rag_edgeiterator_wrapper rag_get_edges(Rag_ui& rag) {
    return Rag_edgeiterator_wrapper(&rag);
}


class Rag_nodeiterator_wrapper {
  public:
    Rag_nodeiterator_wrapper(Rag_ui* rag_) : rag(rag_) {}
    Rag_ui::nodes_iterator nodes_begin()
    {
        return rag->nodes_begin();
    }
    Rag_ui::nodes_iterator nodes_end()
    {
        return rag->nodes_end();
    }
  private:
    Rag_ui* rag;
};

Rag_nodeiterator_wrapper rag_get_nodes(Rag_ui& rag)
{
    return Rag_nodeiterator_wrapper(&rag);
}


void write_volume_to_buffer(Stack* stack, object np_buffer)
{
    unsigned width, height, depth; 
    boost::python::tuple np_buffer_shape(np_buffer.attr("shape"));

    width = boost::python::extract<unsigned>(np_buffer_shape[2]);
    height = boost::python::extract<unsigned>(np_buffer_shape[1]);
    depth = boost::python::extract<unsigned>(np_buffer_shape[0]);

    if ((width != (stack->get_width()-2)) || (height != (stack->get_height()-2)) || (depth != (stack->get_depth()-2))) {
        throw ErrMsg("Buffer has wrong dimensions"); 
    }
    
    Label * temp_label_volume = stack->get_label_volume();
    Label * temp_label_iter = temp_label_volume;

    for (unsigned int z = 0; z < depth; ++z) {
        for (unsigned int y = 0; y < height; ++y) {
            for (unsigned int x = 0; x < width; ++x) {
                np_buffer[boost::python::make_tuple(z,y,x)] = *(temp_label_iter);
                ++temp_label_iter; 
            }
        }
    }
 
    delete temp_label_volume;
}

Stack* init_stack()
{
    Stack * stack = new Stack; 
    stack->set_feature_mgr(new FeatureMgr());
    return stack;
}

unsigned int* init_watershed(object watershed, unsigned int& width, unsigned int& height, unsigned int& depth)
{
    boost::python::tuple watershed_shape(watershed.attr("shape"));

    width = boost::python::extract<unsigned>(watershed_shape[2]);
    height = boost::python::extract<unsigned>(watershed_shape[1]);
    depth = boost::python::extract<unsigned>(watershed_shape[0]);

    unsigned int * watershed_array = new unsigned int[width*height*depth];

    unsigned int plane_size = width * height;

    for (unsigned int z = 0; z < depth; ++z) {
        int z_spot = z * plane_size;
        for (unsigned int y = 0; y < height; ++y) {
            int y_spot = y * width;
            for (unsigned int x = 0; x < width; ++x) {
                unsigned long long curr_spot = x+y_spot+z_spot;
                watershed_array[curr_spot] =
                    (unsigned int)(boost::python::extract<double>(watershed[boost::python::make_tuple(z,y,x)]));
            }
        }
    }
    return watershed_array; 

}


// ?! how to add features to RAG
void reinit_stack(Stack* stack, object watershed)
{
    // there will be 0 padding around image
    unsigned int width, height, depth; 
    unsigned int * watershed_array = init_watershed(watershed, width, height, depth);
   
    stack->reinit_stack(watershed_array, depth, height, width, 1);
}

void reinit_stack2(Stack* stack, object watershed)
{
    // there will be 0 padding around image
    unsigned int width, height, depth; 
    unsigned int * watershed_array = init_watershed(watershed, width, height, depth);
   
    stack->reinit_stack2(watershed_array, depth, height, width, 1);
}


double * create_prediction(object prediction)
{
    unsigned width, height, depth; 
    boost::python::tuple prediction_shape(prediction.attr("shape"));
    width = boost::python::extract<unsigned>(prediction_shape[2]);
    height = boost::python::extract<unsigned>(prediction_shape[1]);
    depth = boost::python::extract<unsigned>(prediction_shape[0]);

    double * prediction_array = new double[width*height*depth];
    unsigned int plane_size = width * height;

    for (unsigned int z = 0; z < depth; ++z) {
        int z_spot = z * plane_size;
        for (unsigned int y = 0; y < height; ++y) {
            int y_spot = y * width;
            for (unsigned int x = 0; x < width; ++x) {
                unsigned long long curr_spot = x+y_spot+z_spot;
                prediction_array[curr_spot] =
                    boost::python::extract<double>(prediction[boost::python::make_tuple(z,y,x)]);
            }
        }
    }
    return prediction_array;
}   

void add_prediction_channel(Stack* stack, object prediction)
{
    double * prediction_array = create_prediction(prediction);
    stack->add_prediction_channel(prediction_array);
}

void add_prediction_channel2(Stack* stack, object prediction)
{
    double * prediction_array = create_prediction(prediction);
    stack->add_prediction_channel2(prediction_array);
}


BOOST_PYTHON_MODULE(libNeuroProofRag)
{
    // (return: Rag, params: file_name)
    def("create_rag_from_jsonfile", create_rag_from_jsonfile, return_value_policy<manage_new_object>());
    // (return true/false, params: rag, file_name)
    def("create_jsonfile_from_rag", create_jsonfile_from_rag);

    def("reinit_stack", reinit_stack);
    def("reinit_stack2", reinit_stack2);
    def("init_stack", init_stack, return_value_policy<manage_new_object>());
    def("add_prediction_channel", add_prediction_channel);
    def("add_prediction_channel2", add_prediction_channel2);
    def("write_volume_to_buffer", write_volume_to_buffer);

    // add property to a rag (params: <rag>, <edge/node>, <property_string>, <data>)
    def("rag_add_property", rag_add_property_ptr1);
    def("rag_add_property", rag_add_property_ptr2);

    // remove property from a rag (params: <rag>, <edge/node>, <property_string>)
    def("rag_remove_property", rag_remove_property_ptr1);
    def("rag_remove_property", rag_remove_property_ptr2);
    
    // retrieve property from a rag (return <data>, params: <rag>, <edge/node>, <property_string>)
    def("rag_retrieve_property", rag_retrieve_property_ptr1);
    def("rag_retrieve_property", rag_retrieve_property_ptr2);

    // delete and unbind property list from rag (params: <rag>, <property_string>)
    def("rag_unbind_property_list", rag_unbind_property_list<Label>);

    // create and bind edge property list to rag (params: <rag>, <property_string>)
    def("rag_bind_edge_property_list", rag_bind_edge_property_list_ptr);
    // create and bind node property list to rag (params: <rag>, <property_string>)
    def("rag_bind_node_property_list", rag_bind_node_property_list_ptr);

    // initialization actually occurs within custom build
    class_<FeatureMgr>("FeatureMgr", no_init)
        .def("set_python_rf_function", &FeatureMgr::set_python_rf_function)
        .def("add_hist_feature", &FeatureMgr::add_hist_feature)
        .def("add_moment_feature", &FeatureMgr::add_moment_feature)
        .def("add_inclusiveness_feature", &FeatureMgr::add_inclusiveness_feature)
        ;

    // initialization actually occurs within custom build
    class_<Stack>("Stack", no_init)
        // returns number of bodies
        .def("get_num_bodies", &Stack::get_num_bodies)
        // agglomerate to a threshold
        .def("agglomerate_rag", &Stack::agglomerate_rag)
        // build rag based on loaded features and prediction images 
        .def("build_rag", &Stack::build_rag)
        .def("build_rag_border", &Stack::build_rag_border)
        .def("add_empty_channel", &Stack::add_empty_channel)
        .def("get_transformations", &Stack::get_transformations)
        .def("disable_nonborder_edges", &Stack::disable_nonborder_edges)
        .def("enable_nonborder_edges", &Stack::enable_nonborder_edges)
        .def("get_feature_mgr", &Stack::get_feature_mgr, return_value_policy<reference_existing_object>())
        // remove inclusions 
        .def("remove_inclusions", &Stack::remove_inclusions)
        // set edge constraints, will create false edges
        .def("add_edge_constraint", &Stack::add_edge_constraint)
        .def("get_rag", &Stack::get_rag, return_value_policy<reference_existing_object>())
        .def("get_body_id", &Stack::get_body_id)
        .def("determine_edge_locations", &Stack::determine_edge_locations)
        .def("get_edge_weight", &Stack::get_edge_weight)
        .def("get_edge_loc", &Stack::get_edge_loc)
        .def("is_orphan", &Stack::is_orphan)
        ;




    // denormalized edge data structure (unique for a node pair)
    class_<RagEdge_ui>("RagEdge", no_init)
        // get first rag node connected to edge
        .def("get_node1", &RagEdge_ui::get_node1, return_value_policy<reference_existing_object>()) 
        // get second rag node connected to edge
        .def("get_node2", &RagEdge_ui::get_node2, return_value_policy<reference_existing_object>()) 
        // returns a double value for the weight
        .def("get_weight", &RagEdge_ui::get_weight) 
        // set a double value for the weight
        .def("set_weight", &RagEdge_ui::set_weight)
        .def("is_preserve", &RagEdge_ui::is_preserve)
        .def("is_false_edge", &RagEdge_ui::is_false_edge)
        ;
    
    // denormalized node data structure (unique for a node id)
    class_<RagNode_ui>("RagNode", no_init)
        // number of nodes connected to node
        .def("node_degree", &RagNode_ui::node_degree)
        // normalized, unique id for node (node_id, node_id is a normalized id for an edge)
        .def("get_node_id", &RagNode_ui::get_node_id)
        // size as 64 bit unsigned
        .def("get_size", &RagNode_ui::get_size)
        // size as 64 bit unsigned
        .def("set_size", &RagNode_ui::set_size)
        // returns an iterator to the connected edges
        .def("get_edges", ragnode_get_edges)
        // returns an iterator to the connected nodes
        .def("set_size", &RagNode_ui::set_size)
        .def("__iter__", range<return_value_policy<reference_existing_object> >(&RagNode_ui::node_begin, &RagNode_ui::node_end))
        ;
 
    class_<Rag_ui>("Rag", init<>())
        // copy constructor supported -- denormolized rag copied correctly, associated properties
        // added in python will just copy the object reference
        .def(init<const Rag_ui&>())
        // returns an interator to the nodes in the rag
        .def("get_edges", rag_get_edges)
        // returns an interator to the edges in the rag
        .def("get_nodes", rag_get_nodes)
        // returns the number of nodes 
        .def("get_num_regions", &Rag_ui::get_num_regions)
        // returns the number of edges 
        .def("get_num_edges", &Rag_ui::get_num_edges)
        // create a new node (return rag_node, params: unique unsigned int) 
        .def("insert_rag_node", insert_rag_node_ptr, return_value_policy<reference_existing_object>())
        // return rag node (return none or node, params: unique unsigned int)
        .def("find_rag_node", find_rag_node_ptr, return_value_policy<reference_existing_object>())
        // return a rag edge (return none or edge, params: node id 1, node id 2)
        .def("find_rag_edge", find_rag_edge_ptr1, return_value_policy<reference_existing_object>())
        // return a rag edge (return none or edge, params: rag node id 1, rag node id 2)
        .def("find_rag_edge", find_rag_edge_ptr2, return_value_policy<reference_existing_object>())
        // insert a rag edge (return none or edge, params: rag node id 1, rag node id 2)
        .def("insert_rag_edge", insert_rag_edge_ptr, return_value_policy<reference_existing_object>())
        // delete rag edge and remove properties associated with edge (params: rag_edge)
        .def("remove_rag_edge", &Rag_ui::remove_rag_edge)
        // delete rag node and connecting edges and remove properties associated with them (params: rag_node)
        .def("remove_rag_node", &Rag_ui::remove_rag_node)
        ;


    // ------- Iterator Interface (should not be explicitly accessed by user) -----------
    // wrapper class for RagNode edge iterator
    class_<RagNode_edgeiterator_wrapper>("RagNode_edgeiterator", no_init)
        .def("__iter__", range<return_value_policy<reference_existing_object> >(&RagNode_edgeiterator_wrapper::edge_begin, &RagNode_edgeiterator_wrapper::edge_end))
        ;
    // wrapper class for Rag edges iterator
    class_<Rag_edgeiterator_wrapper>("Rag_edgeiterator", no_init)
        .def("__iter__", range<return_value_policy<reference_existing_object> >(&Rag_edgeiterator_wrapper::edges_begin, &Rag_edgeiterator_wrapper::edges_end))
        ;
    // wrapper class for Rag nodes iterator
    class_<Rag_nodeiterator_wrapper>("Rag_nodeiterator", no_init)
        .def("__iter__", range<return_value_policy<reference_existing_object> >(&Rag_nodeiterator_wrapper::nodes_begin, &Rag_nodeiterator_wrapper::nodes_end))
        ;
}




/*
stupid& extra_blah(Rag_ui& self, object obj)
{
    stupid& b = (extract<stupid&>(obj));
} 


void* extract_vtk_wrapped_pointer(PyObject* obj)
{
    char thisStr[] = "__this__";
    if (!PyObject_HasAttrString(obj, thisStr))
        return NULL;

    PyObject* thisAttr = PyObject_GetAttrString(obj, thisStr);
    if (thisAttr == NULL)
        return NULL;

    const char* str = PyString_AsString(thisAttr);
    if(str == 0 || strlen(str) < 1)
        return NULL;

    char hex_address[32], *pEnd;
    char *_p_ = strstr(str, "_p_vtk");
    if(_p_ == NULL) return NULL;
    char *class_name = strstr(_p_, "vtk");
    if(class_name == NULL) return NULL;
    strcpy(hex_address, str+1);
    hex_address[_p_-str-1] = '\0';

    long address = strtol(hex_address, &pEnd, 16);

    vtkObjectBase* vtk_object = (vtkObjectBase*)((void*)address);
    if(vtk_object->IsA(class_name))
    {
        return vtk_object;
    }

    return NULL;
}

*/

//#define VTK_PYTHON_CONVERSION(type) \
//    /* register the from-python converter */ \
//    converter::registry::insert(&extract_vtk_wrapped_pointer, type_id<type>());
 



//lvalue_from_pytype<extract_identity<int>,&int>();
//return_internal_reference<1, with_custodian_and_ward<1, 2, with_custodian_and_ward<1, 3> > >());
// copy_const_reference
//object set_object(object obj)
//
//
//
//class_<Base, boost::noncopyable>("Base", no_init);
