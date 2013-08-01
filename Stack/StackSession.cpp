#include "StackSession.h"
#include "../Rag/RagUtils.h"

using namespace NeuroProof;
using std::tr1::unordered_set;

void StackSession::compute_label_colors()
{
    RagPtr rag = stack->get_rag();
    compute_graph_coloring(rag);
}

void StackSession::increment_plane()
{
    if (active_plane < (stack->get_grayvol()->shape(2)-1)) {
        set_plane(active_plane+1);
    }
}

void StackSession::decrement_plane()
{
    if (active_plane > 0) {
        set_plane(active_plane-1);
    }
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

bool StackSession::get_show_all(bool& show_all_)
{
    show_all_ = show_all;
    return show_all_changed;
}

bool StackSession::get_select_label(Label_t& select_curr, Label_t& select_old)
{
    select_curr = selected_id;
    select_old = old_selected_id;
    return selected_id_changed;
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
        active_labels.insert(current_label);
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

bool StackSession::get_active_labels(unordered_set<Label_t>& active_labels_)
{
    active_labels_ = active_labels;
    return active_labels_changed;
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


