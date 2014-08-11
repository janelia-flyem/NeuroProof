#include "StackSession.h"
#include "../Rag/RagUtils.h"
#include "Stack.h"
#include "../Rag/RagIO.h"
#include "../EdgeEditor/EdgeEditor.h"
#include "../FeatureManager/FeatureMgr.h"
#include <json/json.h>
#include <json/value.h>
#include <fstream>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>

using namespace NeuroProof;
using std::tr1::unordered_map;
using namespace boost::algorithm;
using namespace boost::filesystem;
using std::string;
using std::vector;
using std::ifstream; using std::ofstream;

// to be called from command line
StackSession::StackSession(string session_name)
{
    // set all initial variables
    initialize();
  
    if (ends_with(session_name, ".h5")) {
        stack = new BioStack(session_name);
        stack->build_rag();
    } else { //cirectory read
        // load label volume to examine
        stack = new BioStack(session_name + "/stack.h5"); 
        string rag_name = session_name + "/graph.json";
        Rag_t* rag = create_rag_from_jsonfile(rag_name.c_str());
        stack->set_rag(RagPtr(rag));
	
	//*Toufiq* read the classifier
        
        // load gt volume to examine
        string gt_name = session_name + "/gtstack.h5";
        if (exists(gt_name.c_str())) {
            load_gt(gt_name, false); 
            string gtrag_name = session_name + "/gtgraph.json";
            Rag_t* gtrag = create_rag_from_jsonfile(gtrag_name.c_str());
            gt_stack->set_rag(RagPtr(gtrag));
        }
      
        // record session name for saving  
        saved_session_name = session_name;
    }

    if (!(stack->get_labelvol())) {
        throw ErrMsg("Label volume not defined for stack");
    }
    if (!(stack->get_grayvol())) {
        throw ErrMsg("Gray volume not defined for stack");
    }

    // load session specific values
    Json::Reader json_reader;
    Json::Value json_reader_vals;
    string session_file = session_name + "/session.json";
    ifstream fin(session_file.c_str());
    if (!fin) {
        throw ErrMsg("Could not open session json file");
    }
    if (!json_reader.parse(fin, json_reader_vals)) {
        throw ErrMsg("Error: Session json incorrectly formatted");
    }
    edges_examined = json_reader_vals.get("edges-examined", 0).asUInt();
    fin.close();
}

void StackSession::load_gt(string gt_name, bool build_rag)
{
    BioStack* new_gt_stack;
    try {
        new_gt_stack = new BioStack(gt_name);
        
        // verify that dimensions are the same between GT and label volume        
        if (new_gt_stack->get_xsize() != stack->get_xsize() ||
            new_gt_stack->get_ysize() != stack->get_ysize() ||
            new_gt_stack->get_zsize() != stack->get_zsize()) {
            throw ErrMsg("Dimensions of ground truth do not match label volume");
        }
        if (build_rag) {
            new_gt_stack->build_rag();
        }
        new_gt_stack->set_grayvol(stack->get_grayvol());
    } catch (std::runtime_error &error) {
        throw ErrMsg(gt_name + string(" not loaded"));
    }

    if (gt_stack) {
        delete gt_stack;
    }
    gt_stack = new_gt_stack;
}

// to be called from gui controller
StackSession::StackSession(vector<string>& gray_images, string labelvolume)
{
    initialize();
    try {
        VolumeLabelPtr initial_labels = VolumeLabelData::create_volume(
                labelvolume.c_str(), "stack");
        stack = new BioStack(initial_labels);

        VolumeGrayPtr gray_data = VolumeGray::create_volume_from_images(gray_images);
        stack->set_grayvol(gray_data);
        
        // create a new rag
        stack->build_rag();
    } catch (std::runtime_error &error) {
        throw ErrMsg("Unspecified load error");
    }
}

void StackSession::save()
{
    // cannot save unless a session name has already been saved
    if (saved_session_name == "") {
        throw ErrMsg("Session has not been created");
    }
    export_session(saved_session_name);
}

void StackSession::export_session(string session_name)
{
    // flip stacks just for export if gt toggled
    Stack* stack_exp = stack;
    Stack* gtstack_exp = gt_stack;
    if (gt_mode) {
        stack_exp = gt_stack;
        gtstack_exp = stack; 
    }    

    if (stack_exp) {
        try {
            path dir(session_name.c_str()); 
            create_directories(dir);

            string stack_name = session_name + "/stack.h5"; 
            string graph_name = session_name + "/graph.json"; 
            stack_exp->serialize_stack(stack_name.c_str(), graph_name.c_str(), false);
	    //*Toufiq* save the classifier
            string classifier_name = session_name + "/sp_classifier.xml"; 
	    if(stack!=NULL){
		if (stack->get_feature_manager()!=NULL){
		  if (stack->get_feature_manager()->get_classifier()!=NULL)
		    stack->get_feature_manager()->get_classifier()->save_classifier(classifier_name.c_str());
		}
	    }

            Json::Value json_writer;
            string session_file = session_name + "/session.json";
            ofstream fout(session_file.c_str());
            json_writer["edges-examined"] = edges_examined; 
            fout << json_writer;
            fout.close();

            if (gtstack_exp) {
                // do not export grayscale from ground truth -- already exported
                gtstack_exp->set_grayvol(VolumeGrayPtr());
                string gtstack_name = session_name + "/gtstack.h5"; 
                string gtgraph_name = session_name + "/gtgraph.json"; 
                gtstack_exp->serialize_stack(gtstack_name.c_str(), gtgraph_name.c_str(), false);
                gtstack_exp->set_grayvol(stack->get_grayvol());
            }
        } catch (...) {
            throw ErrMsg(session_name.c_str());
        }
        saved_session_name = session_name;
    }
}

bool StackSession::has_gt_stack()
{
    return gt_stack != 0;
} 

bool StackSession::has_session_name()
{
    return saved_session_name != "";
}

string StackSession::get_session_name()
{
    return saved_session_name;
}

void StackSession::compute_label_colors(RagPtr rag)
{
    // call from RagUtils
    compute_graph_coloring(rag);
}

void StackSession::increment_plane()
{
    if (active_plane < (stack->get_grayvol()->shape(2)-1)) {
        set_plane(active_plane+1);
    }
}

void StackSession::initialize()
{
    stack = 0;
    gt_stack = 0;
    active_labels_changed = false;
    selected_id = 0;
    old_selected_id = 0;
    selected_id_changed = false;
    show_all = true;
    next_bodies = false;
    next_bodies_changed = false;
    merge_bodies = false;
    merge_bodies_changed = false;
    show_all_changed = false;
    active_plane = 0;
    active_plane_changed = false;
    opacity = 3;
    opacity_changed = false;
    saved_session_name = string("");
    gt_mode = false;
    reset_stack = false;
    zoom_loc = false;
    remove_edge = false;
    undo_queue = 0;
    edges_examined = 0;
}

void StackSession::decrement_plane()
{
    if (active_plane > 0) {
        set_plane(active_plane-1);
    }
}

void StackSession::toggle_gt()
{
    if (!gt_stack) {
        throw ErrMsg("GT stack not defined");
    }
    gt_mode = !gt_mode;
    BioStack* temp_stack;
    temp_stack = stack;
    stack = gt_stack;
    gt_stack = temp_stack;

    set_reset_stack();
}

void StackSession::set_reset_stack()
{
    printf("reset stack\n");
    RagPtr rag = stack->get_rag();
   
    // colors are not recomputed unless the old colors are deleted 
    for (Rag_t::nodes_iterator iter = rag->nodes_begin();
            iter != rag->nodes_end(); ++iter) {
        (*iter)->rm_property("color");
    }

    undo_queue = 0;
    reset_active_labels();
    reset_stack = true;
    update_all();
    reset_stack = false;

    // center the zoom to the middle of the image on plane 0
    unsigned int x = stack->get_xsize() / 2;
    unsigned int y = stack->get_ysize() / 2;
    Location location(x, y, 0);
    set_zoom_loc(location, 1.0);
    
    printf("done reset stack\n");
}

bool StackSession::get_reset_stack(VolumeLabelPtr& labelvol, RagPtr& rag)
{
    labelvol = stack->get_labelvol();
    rag = stack->get_rag();

    return reset_stack;
}

bool StackSession::is_gt_mode()
{
    return gt_mode;
}

void StackSession::set_opacity(unsigned int opacity_)
{   
    opacity = opacity_; 
    opacity_changed = true;
    update_all();
    opacity_changed = false;
}

void StackSession::set_next_bodies()
{
    next_bodies = true;
    next_bodies_changed = true;
    update_all();
    next_bodies = false;
    next_bodies_changed = false;
}

void StackSession::set_merge_bodies()
{
    merge_bodies = true;
    merge_bodies_changed = true;
    update_all();
    merge_bodies = false;
    merge_bodies_changed = false;
}

void StackSession::set_plane(unsigned int plane)
{   
    active_plane = plane; 
    active_plane_changed = true;
    update_all();
    active_plane_changed = false;
}

void StackSession::toggle_show_all()
{
    show_all = !show_all;
    show_all_changed = true;
    update_all();
    show_all_changed = false;
}

bool StackSession::get_plane(unsigned int& plane_id)
{
    plane_id = active_plane;
    return active_plane_changed;
}

bool StackSession::get_opacity(unsigned int& opacity_)
{
    opacity_ = opacity;
    return opacity_changed;
}
bool StackSession::get_show_all(bool& show_all_)
{
    show_all_ = show_all;
    return show_all_changed;
}

bool StackSession::get_merge_bodies(bool& merge_bodies_)
{
    merge_bodies_ = merge_bodies;
    bool changed = merge_bodies_changed;
    merge_bodies_changed = false;
    return changed;
}

bool StackSession::get_next_bodies(bool& next_bodies_)
{
    next_bodies_ = next_bodies;
    bool changed = next_bodies_changed;
    next_bodies_changed = false;
    return changed;
}


bool StackSession::get_select_label(Label_t& select_curr, Label_t& select_old)
{
    select_curr = selected_id;
    select_old = old_selected_id;
    return selected_id_changed;
}

void StackSession::get_rgb(int color_id, unsigned char& r,
        unsigned char& g, unsigned char& b)
{
    unsigned int val = color_id % 18; 
    switch (val) {
        case 0:
            r = 0xff; g = b = 0; break;
        case 1:
            g = 0xff; r = b = 0; break;
        case 2:
            b = 0xff; r = g = 0; break;
        case 3:
            r = g = 0xff; b = 0; break;
        case 14:
            r = b = 0xff; g = 0; break;
        case 8:
            g = b = 0xff; r = 0; break;
        case 6:
            r = 0x7f; g = b = 0; break;
        case 16:
            g = 0x7f; r = b = 0; break;
        case 9:
            b = 0x7f; r = g = 0; break;
        case 5:
            r = g = 0x7f; b = 0; break;
        case 13:
            r = b = 0x7f; g = 0; break;
        case 11:
            g = b = 0x7f; r = 0; break;
        case 12:
            r = 0xff; g = b = 0x7f; break;
        case 10:
            g = 0xff; r = b = 0x7f; break;
        case 17:
            b = 0xff; r = g = 0x7f; break;
        case 15:
            r = g = 0xff; b = 0x7f; break;
        case 7:
            r = b = 0xff; g = 0x7f; break;
        case 4:
            g = b = 0xff; r = 0x7f; break;
    }
}

void StackSession::add_active_label(Label_t label)
{
    RagPtr rag = stack->get_rag();
    RagNode_t *node = rag->find_rag_node(label);
    int color_id = 1;
    if (node->has_property("color")) {
        color_id = node->get_property<int>("color");
    }
    active_labels[label] = color_id;
}

void StackSession::active_label(unsigned int x, unsigned int y, unsigned int z)
{
    VolumeLabelPtr labelvol = stack->get_labelvol();
    Label_t current_label = (*labelvol)(x,y,z);
    
    if (!current_label) {
        // ignore selection if off image or on boundary
        return;
    }
   
    if (active_labels.find(current_label) != active_labels.end()) {
        active_labels.erase(current_label);        
    } else {
        add_active_label(current_label); 
    }
    
    select_label(selected_id);

    active_labels_changed = true;
    update_all();
    active_labels_changed = false;
}



void StackSession::select_label(unsigned int x, unsigned int y, unsigned int z)
{
    VolumeLabelPtr labelvol = stack->get_labelvol();
    Label_t current_label = (*labelvol)(x,y,z);

    select_label(current_label);    
}

void StackSession::select_label(Label_t current_label)
{
    if (!current_label) {
        // ignore selection if off image or on boundary
        return;
    }
    if (!active_labels.empty() &&
            (active_labels.find(current_label) == active_labels.end())) {
        return;
    }
    
    old_selected_id = selected_id;
    if (current_label != selected_id) {
        selected_id = current_label;
    } else {
        selected_id = 0;
    }
    selected_id_changed = true;
    update_all();
    selected_id_changed = false;
}

bool StackSession::get_active_labels(unordered_map<Label_t, int>& active_labels_)
{
    active_labels_ = active_labels;
    return active_labels_changed;
}

void StackSession::set_zoom_loc(Location location, double zoom_factor_)
{
    set_plane(boost::get<2>(location));
    x_zoom = boost::get<0>(location); 
    y_zoom = boost::get<1>(location); 
    zoom_factor = zoom_factor_;

    zoom_loc = true;
    update_all();
    zoom_loc = false;
}

bool StackSession::get_zoom_loc(unsigned int& x, unsigned int& y, double& zoom_factor_)
{
    x = x_zoom; 
    y = y_zoom;
    zoom_factor_ = zoom_factor;
    
    return zoom_loc; 
}

void StackSession::set_children_labels(Label_t label_id, int color_id)
{
    VolumeLabelPtr labelvol = stack->get_labelvol();
    vector<Label_t> member_labels;
    labelvol->get_label_history(label_id, member_labels);

    for (int i = 0; i < member_labels.size(); ++i) {
        active_labels[(member_labels[i])] = color_id;
    }
}

void StackSession::set_body_pair(Label_t node1, Label_t node2, Location location)
{
    old_selected_id = 0;
    selected_id = 0;
    remove_edge = false;

    node1_focused = node1;
    node2_focused = node2;

    // add node1, node2 to selected labels
    active_labels.clear();
    active_labels[node1] = 1;
    active_labels[node2] = 2;

    // add any nodes that were previously merged onto node1 and node2
    set_children_labels(node1, 1);
    set_children_labels(node2, 2);
 
    active_labels_changed = true;
    update_all();
    active_labels_changed = false;

    set_zoom_loc(location, 4.0);
}

void StackSession::set_commit_edge(Label_t node_remove, Label_t node_keep, bool ignore_rag)
{
    // merge the labels if the edge was removed
    if (remove_edge) {
        LowWeightCombine join_alg; 
        stack->merge_labels(node_remove, node_keep, &join_alg, ignore_rag);
    }
    ++undo_queue;
    ++edges_examined;
}

void StackSession::merge_edge()
{
    // does not commit result but changes the color maps
    active_labels[node2_focused] = 1;    
    set_children_labels(node2_focused, 1);
    remove_edge = true;   
 
    active_labels_changed = true;
    update_all();
    active_labels_changed = false;
}

int StackSession::get_num_examined_edges()
{
    return edges_examined;
}

void StackSession::set_undo_edge(Label_t node_base, Label_t node_absorb)
{
    --undo_queue;
    --edges_examined;
    VolumeLabelPtr labelvol = stack->get_labelvol();

    if (labelvol->is_mapped(node_absorb)) {
        // undo merge
        vector<Label_t> member_labels;
        labelvol->get_label_history(node_base, member_labels);
       
        vector<Label_t> split_labels;
        bool grab_labels = false;
        // since undos are in order, the labels to be split off are in order
        for (int i = 0; i < member_labels.size(); ++i) {
            if (member_labels[i] == node_absorb) {
                grab_labels = true;
            }
            if (grab_labels) {
                split_labels.push_back(member_labels[i]);
            }
        }
 
        labelvol->split_labels(node_base, split_labels);  
    }
}

bool StackSession::undo_queue_empty()
{
    if (undo_queue == 0) {
        return true;
    }
    return false;
}

void StackSession::unmerge_edge()
{
    active_labels[node2_focused] = 2;    
    set_children_labels(node2_focused, 2);
    remove_edge = false;   
 
    active_labels_changed = true;
    update_all();
    active_labels_changed = false;
}

bool StackSession::is_remove_edge(Label_t& node1, Label_t& node2)
{
    node1 = node1_focused;
    node2 = node2_focused;
    return remove_edge;   
}

void StackSession::reset_active_labels()
{
    active_labels.clear();
    active_labels_changed = true;
    show_all = true;
    show_all_changed = true;
    update_all();
    active_labels_changed = false;
    show_all_changed = false;
}


