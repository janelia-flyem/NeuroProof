#include "../FeatureManager/FeatureMgr.h"
#include "../Rag/Rag.h"
#include "../Rag/RagIO.h"
#include "../Utilities/ErrMsg.h"
#include "../Stack/Stack.h"
#include "../BioPriors/StackAgglomAlgs.h"

#include <boost/python.hpp>

#include <iostream>

using namespace NeuroProof;
using namespace boost::python;
using std::cout;
using std::vector;
using std::tr1::unordered_map;
using std::tr1::unordered_set;

RagNode_t* (Rag_t::*find_rag_node_ptr)(Label_t) = &Rag_t::find_rag_node;
RagNode_t* (Rag_t::*insert_rag_node_ptr)(Label_t) = &Rag_t::insert_rag_node;
RagEdge_t* (Rag_t::*insert_rag_edge_ptr)(RagNode_t*, RagNode_t*) = &Rag_t::insert_rag_edge;
RagEdge_t* (Rag_t::*find_rag_edge_ptr1)(Label_t, Label_t) = &Rag_t::find_rag_edge;
RagEdge_t* (Rag_t::*find_rag_edge_ptr2)(RagNode_t*, RagNode_t*) = &Rag_t::find_rag_edge;

// label volume should have a 1 pixel 0 padding surrounding it
class StackPython : public Stack {
  public:
    StackPython(VolumeLabelPtr labels_) : Stack(labels_) 
    {
        VolumeLabelPtr emptypointer;
        stack2 = new Stack(emptypointer);
    }

    void build_rag_padded()
    {
        if (!labelvol) {
            throw ErrMsg("No label volume defined for stack");
        }

        rag = RagPtr(new Rag_t);

        vector<double> predictions(prob_list.size(), 0.0);
        unordered_set<Label_t> labels;

        // adjust because of padding used by gala
        unsigned int maxx = get_xsize() - 2; 
        unsigned int maxy = get_ysize() - 2; 
        unsigned int maxz = get_zsize() - 2; 

        volume_forXYZ(*labelvol, x, y, z) {
            Label_t label = (*labelvol)(x,y,z); 

            if (!label) {
                continue;
            }

            RagNode_t * node = rag->find_rag_node(label);

            // create node
            if (!node) {
                node =  rag->insert_rag_node(label); 
            }
            node->incr_size();

            // load all prediction values for a given x,y,z 
            for (unsigned int i = 0; i < prob_list.size(); ++i) {
                predictions[i] = (*(prob_list[i]))(x,y,z);
            }

            // add array of features/predictions for a given node
            if (feature_manager) {
                feature_manager->add_val(predictions, node);
            }

            Label_t label2 = 0, label3 = 0, label4 = 0, label5 = 0, label6 = 0, label7 = 0;
            if (x > 1) label2 = (*labelvol)(x-1,y,z);
            if (x < maxx) label3 = (*labelvol)(x+1,y,z);
            if (y > 1) label4 = (*labelvol)(x,y-1,z);
            if (y < maxy) label5 = (*labelvol)(x,y+1,z);
            if (z > 1) label6 = (*labelvol)(x,y,z-1);
            if (z < maxz) label7 = (*labelvol)(x,y,z+1);

            // if it is not a 0 label and is different from the current label, add edge
            if (label2 && (label != label2)) {
                rag_add_edge(label, label2, predictions);
                labels.insert(label2);
            }
            if (label3 && (label != label3) && (labels.find(label3) == labels.end())) {
                rag_add_edge(label, label3, predictions);
                labels.insert(label3);
            }
            if (label4 && (label != label4) && (labels.find(label4) == labels.end())) {
                rag_add_edge(label, label4, predictions);
                labels.insert(label4);
            }
            if (label5 && (label != label5) && (labels.find(label5) == labels.end())) {
                rag_add_edge(label, label5, predictions);
                labels.insert(label5);
            }
            if (label6 && (label != label6) && (labels.find(label6) == labels.end())) {
                rag_add_edge(label, label6, predictions);
                labels.insert(label6);
            }
            if (label7 && (label != label7) && (labels.find(label7) == labels.end())) {
                rag_add_edge(label, label7, predictions);
            }

            // if it is on the border of the image, increase the boundary size
            if (!label2 || !label3 || !label4 || !label5 || !label6 || !label7) {
                node->incr_boundary_size();
            }
            labels.clear();
        }
    }

    void build_rag_border(bool reset_edges)
    {
        vector<VolumeProbPtr> prob_list2 = stack2->get_prob_list();

        vector<double> predictions(prob_list.size(), 0.0);
        vector<double> predictions2(prob_list2.size(), 0.0);

        VolumeLabelPtr labelvol2 =  stack2->get_labelvol();

        for (int z = 1; z < (labelvol->shape(2)-1); ++z) { 
            for (int y = 1; y < (labelvol->shape(1)-1); ++y) {
                for (int x = 1; x < (labelvol->shape(0)-1); ++x) {

                    Label_t label0 = (*labelvol)(x,y,z);
                    Label_t label1 = (*labelvol2)(x,y,z);
                    
                    bool ignore = false; 
                   
                    if (mask1 && mask2) { 
                        Label_t mask_val0 = (*mask1)(x,y,z);
                        Label_t mask_val1 = (*mask2)(x,y,z);

                        if (!mask_val0 || !mask_val1) {
                            ignore = true;
                        }
                    }

                    if (!label0 || !label1) {
                        continue;
                    }
                    //assert(label0 != label1);

                    RagNode_t * node = rag->find_rag_node(label0);
                    if (!node) {
                        node = rag->insert_rag_node(label0);
                    }

                    RagNode_t * node2 = rag->find_rag_node(label1);

                    if (!node2) {
                        node2 = rag->insert_rag_node(label1);
                    }

                    for (unsigned int i = 0; i < prob_list.size(); ++i) {
                        predictions[i] = (*(prob_list[i]))(x,y,z);
                    }
                    for (unsigned int i = 0; i < prob_list2.size(); ++i) {
                        predictions2[i] = (*(prob_list2[i]))(x,y,z);
                    }

                    feature_manager->add_val(predictions, node);
                    feature_manager->add_val(predictions2, node2);
                    
                    node->incr_size();
                    node2->incr_size();

                    if (label0 == label1) {
                        // create dummy node and init features
                        RagNode_t * node0 = rag->find_rag_node(Label_t(0));
                        if (!node0) {
                            node0 = rag->insert_rag_node(Label_t(0));
                            feature_manager->add_val(predictions, node0);
                            node->incr_size();
                        }
                        rag_add_edge(label0, Label_t(0), predictions);
                    } else {
                        rag_add_edge(label0, label1, predictions);
                        rag_add_edge(label0, label1, predictions2);
                    
                        if (ignore) {
                            RagEdge_t* edge = rag->find_rag_edge(label0, label1);
                            unsigned long long current0s = 0;
                            if (edge->has_property("num-zeros")) {
                                current0s = edge->get_property<unsigned long long>("num-zeros");
                            }
                            current0s += 2;
                            edge->set_property("num-zeros", current0s);
                        } 
                    }

                    node->incr_boundary_size();
                    node2->incr_boundary_size();
                }
            }
        }
  
        if (reset_edges) { 
            for (Rag_t::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {
                if ((*iter)->get_size() > 0) {
                    double prob = 1.1;
                    double save_prob = 1.1;
                    if (((*iter)->get_node1()->get_node_id() == 0) ||
                            ((*iter)->get_node2()->get_node_id() == 0)) {
                        prob = 1.0;
                        save_prob = 1.0;
                    } else {
                        if ((*iter)->has_property("orig-prob")) {
                            prob = (*iter)->get_property<double>("orig-prob");
                            save_prob = (*iter)->get_property<double>("save-prob");
                            (*iter)->rm_property("orig-prob"); 
                            (*iter)->rm_property("save-prob"); 
                        }
                    }
                    double prob2 = feature_manager->get_prob(*iter);
                    double save_prob2 = (*iter)->get_property<double>("save-prob");

                    (*iter)->set_property("orig-prob", std::min(prob, prob2));
                    (*iter)->set_property("save-prob", std::min(save_prob, save_prob2));
                }
            }

            for (Rag_t::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {
                (*iter)->set_size(0);
                (*iter)->set_property("num-zeros", ((unsigned long long)(0)));
            }
        }
    }

    void set_saved_probs()
    {
        for (Rag_t::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {
            (*iter)->set_property("orig-prob", (*iter)->get_property<double>("save-prob")); 
        }
    }

    boost::python::list get_transformations()
    {
        boost::python::list transforms;

        unordered_map<Label_t, Label_t> label_mappings = labelvol->get_mappings();

        for (unordered_map<Label_t, Label_t>::iterator iter = label_mappings.begin();
                iter != label_mappings.end(); ++iter)
        {
            if (iter->first != iter->second) {
                transforms.append(boost::python::make_tuple(iter->first, iter->second));
            }
        } 

        return transforms;
    }
   
    void agglomerate(double threshold)
    {
        agglomerate_stack(*this, threshold, false);
    } 

    bool add_edge_constraint(boost::python::tuple loc1, boost::python::tuple loc2)
    {
        unsigned int x1 = boost::python::extract<int>(loc1[0]);
        unsigned int y1 = boost::python::extract<int>(loc1[1]);
        unsigned int z1 = boost::python::extract<int>(loc1[2]);
        unsigned int x2 = boost::python::extract<int>(loc2[0]);
        unsigned int y2 = boost::python::extract<int>(loc2[1]);
        unsigned int z2 = boost::python::extract<int>(loc2[2]);

        Label_t body1 = get_body_id(x1, y1, z1);    
        Label_t body2 = get_body_id(x2, y2, z2);    

        if (body1 == body2) {
            return false;
        }
        if (!body1 || !body2) {
            return true;
        }

        RagEdge_t* edge = rag->find_rag_edge(body1, body2);

        if (!edge) {
            RagNode_t* node1 = rag->find_rag_node(body1);
            RagNode_t* node2 = rag->find_rag_node(body2);
            edge = rag->insert_rag_edge(node1, node2);
            edge->set_weight(1.0);
            edge->set_false_edge(true);
        }
        edge->set_preserve(true);

        return true;
    } 
    
    void determine_edge_locations(bool use_probs=false)
    {
        Stack::determine_edge_locations(best_edge_z, best_edge_loc, use_probs);      
    }

    // adjust assuming the input has a 1 pixel padding
    boost::python::tuple get_edge_loc(RagEdge_t* edge)
    {
        if (best_edge_loc.find(edge) == best_edge_loc.end()) {
            throw ErrMsg("Edge location was not loaded!");
        }

        Location loc = best_edge_loc[edge];
        unsigned int x = boost::get<0>(loc) - 1;
        unsigned int y = boost::get<1>(loc) - 1;
        unsigned int z = boost::get<2>(loc) - 1;

        return boost::python::make_tuple(x, y, z); 
    }

    // adjust assuming the input has a 1 pixel padding
    Label_t get_body_id(unsigned int x, unsigned int y, unsigned int z)
    {
        return (*labelvol)(x+1,labelvol->shape(1)-y-2,z+1);
    }

    FeatureMgr* get_feature_mgr()
    {
        return feature_manager.get();
    }

    Rag_t* get_rag()
    {
        return rag.get();
    }

    double get_edge_weight(RagEdge_t* edge)
    {
        return feature_manager->get_prob(edge);
    }

    void add_empty_channel()
    {
        feature_manager->add_channel();
    }

    bool is_orphan(RagNode_t* node)
    {
        return !(node->is_boundary());
    }
    
    void noop() {}

    Stack* get_stack2()
    {
        return stack2;
    }

    void set_mask(VolumeLabelPtr mask1_, VolumeLabelPtr mask2_)
    {
        mask1 = mask1_;
        mask2 = mask2_;
    }

    ~StackPython()
    {
        delete stack2;
    }
  private:
    Stack* stack2;
    EdgeCount best_edge_z;
    EdgeLoc best_edge_loc;
    VolumeLabelPtr mask1;
    VolumeLabelPtr mask2;
};



class RagNode_edgeiterator_wrapper {
  public:
    RagNode_edgeiterator_wrapper(RagNode_t* rag_node_) : rag_node(rag_node_) {}
    RagNode_t::edge_iterator edge_begin()
    {
        return rag_node->edge_begin();
    }
    RagNode_t::edge_iterator edge_end()
    {
        return rag_node->edge_end();
    }
  private:
    RagNode_t* rag_node;
};

RagNode_edgeiterator_wrapper ragnode_get_edges(RagNode_t& rag_node) {
    return RagNode_edgeiterator_wrapper(&rag_node);
}


class Rag_edgeiterator_wrapper {
  public:
    Rag_edgeiterator_wrapper(Rag_t* rag_) : rag(rag_) {}
    Rag_t::edges_iterator edges_begin()
    {
        return rag->edges_begin();
    }
    Rag_t::edges_iterator edges_end()
    {
        return rag->edges_end();
    }
  private:
    Rag_t* rag;
};

Rag_edgeiterator_wrapper rag_get_edges(Rag_t& rag) {
    return Rag_edgeiterator_wrapper(&rag);
}


class Rag_nodeiterator_wrapper {
  public:
    Rag_nodeiterator_wrapper(Rag_t* rag_) : rag(rag_) {}
    Rag_t::nodes_iterator nodes_begin()
    {
        return rag->nodes_begin();
    }
    Rag_t::nodes_iterator nodes_end()
    {
        return rag->nodes_end();
    }
  private:
    Rag_t* rag;
};

Rag_nodeiterator_wrapper rag_get_nodes(Rag_t& rag)
{
    return Rag_nodeiterator_wrapper(&rag);
}

void write_volume_to_buffer(StackPython* stack, object np_buffer)
{
    unsigned width, height, depth; 
    boost::python::tuple np_buffer_shape(np_buffer.attr("shape"));

    width = boost::python::extract<unsigned>(np_buffer_shape[2]);
    height = boost::python::extract<unsigned>(np_buffer_shape[1]);
    depth = boost::python::extract<unsigned>(np_buffer_shape[0]);

    VolumeLabelPtr labelvol = stack->get_labelvol();
        
    if ((width != (labelvol->shape(0)-2)) || (height != (labelvol->shape(1)-2))
            || (depth != (labelvol->shape(2)-2))) {
        throw ErrMsg("Buffer has wrong dimensions"); 
    }
   
    // a volume that is buffered is passed to the algorithm 
    for (int z = 1; z < (labelvol->shape(2)-1); ++z) { 
        for (int y = 1; y < (labelvol->shape(1)-1); ++y) {
            for (int x = 1; x < (labelvol->shape(0)-1); ++x) {
                np_buffer[boost::python::make_tuple(z-1,y-1,x-1)] = (*labelvol)(x,y,z);
            }
        }
    } 
}

StackPython* init_stack()
{
    VolumeLabelPtr emptypointer;
    StackPython* temp_stack = new StackPython(emptypointer);
    temp_stack->set_feature_manager(FeatureMgrPtr(new FeatureMgr)); 
    temp_stack->set_rag(RagPtr(new Rag_t()));
    return temp_stack;
}

VolumeLabelPtr init_watershed(object watershed)
{
    boost::python::tuple watershed_shape(watershed.attr("shape"));

    unsigned int width = boost::python::extract<unsigned int>(watershed_shape[2]);
    unsigned int height = boost::python::extract<unsigned int>(watershed_shape[1]);
    unsigned int depth = boost::python::extract<unsigned int>(watershed_shape[0]);

    VolumeLabelPtr labels = VolumeLabelData::create_volume();
    labels->reshape(vigra::MultiArrayShape<3>::type(width, height, depth));

    volume_forXYZ(*labels,x,y,z) {
        labels->set(x,y,z, boost::python::extract<double>(watershed[boost::python::make_tuple(z,y,x)]));
    }
    return labels;
}

void dilate_supervoxels(object supervoxels)
{
    VolumeLabelPtr labelvol = init_watershed(supervoxels);
    Stack stack(labelvol);
    stack.dilate_labelvol(1);
    labelvol = stack.get_labelvol();

    for (int z = 0; z < (labelvol->shape(2)); ++z) { 
        for (int y = 0; y < (labelvol->shape(1)); ++y) {
            for (int x = 0; x < (labelvol->shape(0)); ++x) {
                supervoxels[boost::python::make_tuple(z,y,x)] = (*labelvol)(x,y,z);
            }
        }
    } 
}

// how to add features to RAG?
void reinit_stack(StackPython* stack, object watershed)
{
    // there will be 0 padding around image
    VolumeLabelPtr labelvol = init_watershed(watershed);
    
    stack->set_labelvol(labelvol);

    vector<VolumeProbPtr> prob_list;
    stack->set_prob_list(prob_list);
}

void init_masks(StackPython* stack, object mask1, object mask2)
{
    VolumeLabelPtr mask1_labels = init_watershed(mask1);
    VolumeLabelPtr mask2_labels = init_watershed(mask2);
    stack->set_mask(mask1_labels, mask2_labels); 
}


void reinit_stack2(StackPython* stack, object watershed)
{
    // there will be 0 padding around image
    VolumeLabelPtr labelvol = init_watershed(watershed);
    
    Stack* stack2 = stack->get_stack2();
    stack2->set_labelvol(labelvol);

    vector<VolumeProbPtr> prob_list;
    stack2->set_prob_list(prob_list);
}


VolumeProbPtr create_prediction(object prediction)
{
    unsigned width, height, depth; 
    boost::python::tuple prediction_shape(prediction.attr("shape"));
    width = boost::python::extract<unsigned>(prediction_shape[2]);
    height = boost::python::extract<unsigned>(prediction_shape[1]);
    depth = boost::python::extract<unsigned>(prediction_shape[0]);

    VolumeProbPtr prob = VolumeProb::create_volume();
    prob->reshape(vigra::MultiArrayShape<3>::type(width, height, depth));

    volume_forXYZ(*prob,x,y,z) {
        (*prob)(x,y,z) = boost::python::extract<double>(prediction[boost::python::make_tuple(z,y,x)]);
    }
    
    return prob;
}   

void add_prediction_channel(StackPython* stack, object prediction)
{
    VolumeProbPtr prob = create_prediction(prediction);
    stack->add_prob(prob);
}

void add_prediction_channel2(StackPython* stack, object prediction)
{
    Stack* stack2 = stack->get_stack2();
    VolumeProbPtr prob = create_prediction(prediction);
    stack2->add_prob(prob);
}


BOOST_PYTHON_MODULE(libNeuroProofRag)
{
    // (return: Rag, params: file_name)
    def("create_rag_from_jsonfile", create_rag_from_jsonfile, return_value_policy<manage_new_object>());
    // (return true/false, params: rag, file_name)
    def("create_jsonfile_from_rag", create_jsonfile_from_rag);

    def("reinit_stack", reinit_stack);
    def("reinit_stack2", reinit_stack2);
    def("init_masks", init_masks);
    def("init_stack", init_stack, return_value_policy<manage_new_object>());
    def("add_prediction_channel", add_prediction_channel);
    def("add_prediction_channel2", add_prediction_channel2);
    def("write_volume_to_buffer", write_volume_to_buffer);
    def("dilate_supervoxels", dilate_supervoxels);

    // initialization actually occurs within custom build
    class_<FeatureMgr>("FeatureMgr", no_init)
        .def("set_python_rf_function", &FeatureMgr::set_python_rf_function)
        .def("set_overlap_function", &FeatureMgr::set_overlap_function)
        .def("set_overlap_cutoff", &FeatureMgr::set_overlap_cutoff)
        .def("set_border_weight", &FeatureMgr::set_border_weight)
        .def("set_overlap_max", &FeatureMgr::set_overlap_max)
        .def("set_overlap_min", &FeatureMgr::set_overlap_min)
        .def("add_hist_feature", &FeatureMgr::add_hist_feature)
        .def("add_moment_feature", &FeatureMgr::add_moment_feature)
        .def("add_inclusiveness_feature", &FeatureMgr::add_inclusiveness_feature)
        ;

    // initialization actually occurs within custom build
    class_<StackPython>("Stack", no_init)
        // returns number of bodies
        .def("get_num_bodies", &StackPython::get_num_labels)
        // agglomerate to a threshold
        .def("agglomerate_rag", &StackPython::agglomerate)
        // build rag based on loaded features and prediction images 
        .def("build_rag", &StackPython::build_rag_padded)
        .def("set_saved_probs", &StackPython::set_saved_probs)
        .def("build_rag_border", &StackPython::build_rag_border)
        .def("add_empty_channel", &StackPython::add_empty_channel)
        .def("get_transformations", &StackPython::get_transformations)
        .def("disable_nonborder_edges", &StackPython::noop)
        .def("enable_nonborder_edges", &StackPython::noop)
        .def("get_feature_mgr", &StackPython::get_feature_mgr, return_value_policy<reference_existing_object>())
        // remove inclusions 
        .def("remove_inclusions", &StackPython::remove_inclusions)
        // set edge constraints, will create false edges
        .def("add_edge_constraint", &StackPython::add_edge_constraint)
        .def("get_rag", &StackPython::get_rag, return_value_policy<reference_existing_object>())
        .def("get_body_id", &StackPython::get_body_id)
        .def("determine_edge_locations", &StackPython::determine_edge_locations)
        .def("get_edge_weight", &StackPython::get_edge_weight)
        .def("get_edge_loc", &StackPython::get_edge_loc)
        .def("is_orphan", &StackPython::is_orphan)
        ;

    // denormalized edge data structure (unique for a node pair)
    class_<RagEdge_t, boost::noncopyable>("RagEdge", no_init)
        // get first rag node connected to edge
        .def("get_node1", &RagEdge_t::get_node1, return_value_policy<reference_existing_object>()) 
        // get second rag node connected to edge
        .def("get_node2", &RagEdge_t::get_node2, return_value_policy<reference_existing_object>()) 
        // returns a double value for the weight
        .def("get_weight", &RagEdge_t::get_weight) 
        // set a double value for the weight
        .def("set_weight", &RagEdge_t::set_weight)
        .def("is_preserve", &RagEdge_t::is_preserve)
        .def("is_false_edge", &RagEdge_t::is_false_edge)
        ;
    
    // denormalized node data structure (unique for a node id)
    class_<RagNode_t, boost::noncopyable>("RagNode", no_init)
        // number of nodes connected to node
        .def("node_degree", &RagNode_t::node_degree)
        // normalized, unique id for node (node_id, node_id is a normalized id for an edge)
        .def("get_node_id", &RagNode_t::get_node_id)
        // size as 64 bit unsigned
        .def("get_size", &RagNode_t::get_size)
        // size as 64 bit unsigned
        .def("set_size", &RagNode_t::set_size)
        // returns an iterator to the connected edges
        .def("get_edges", ragnode_get_edges)
        // returns an iterator to the connected nodes
        .def("set_size", &RagNode_t::set_size)
        .def("__iter__", range<return_value_policy<reference_existing_object> >(&RagNode_t::node_begin, &RagNode_t::node_end))
        ;
 
    class_<Rag_t>("Rag", init<>())
        // copy constructor supported -- denormolized rag copied correctly, associated properties
        // added in python will just copy the object reference
        .def(init<const Rag_t&>())
        // returns an interator to the nodes in the rag
        .def("get_edges", rag_get_edges)
        // returns an interator to the edges in the rag
        .def("get_nodes", rag_get_nodes)
        // returns the number of nodes 
        .def("get_num_regions", &Rag_t::get_num_regions)
        // returns the number of edges 
        .def("get_num_edges", &Rag_t::get_num_edges)
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
        .def("remove_rag_edge", &Rag_t::remove_rag_edge)
        // delete rag node and connecting edges and remove properties associated with them (params: rag_node)
        .def("remove_rag_node", &Rag_t::remove_rag_node)
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


