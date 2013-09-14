#ifndef STACKSESSION_H
#define STACKSESSION_H

#include "Dispatcher.h"
#include "Stack.h"
#include <tr1/unordered_map>
#include <string>

namespace NeuroProof {

class StackSession : public Dispatcher {
  public:
    // handles session and h5 stacks
    StackSession(std::string session_name);
    StackSession(Stack* stack_);
    StackSession(std::vector<std::string>& gray_images,
        std::string labelvolume);
    
    // will overwrite existing session
    void export_session(std::string session_name); 
    void save(); 

    void load_gt(std::string gt_name, bool build_rag=true);
    bool has_gt_stack();   
 
    bool has_session_name();
    std::string get_session_name();

    /*!
     * Keep track of certain labels by adding to an active list.
     * \param label volume label
    */
    void add_active_label(Label_t label)
    {
        RagPtr rag = stack->get_rag();
        RagNode_t *node = rag->find_rag_node(label);
        int color_id = 1;
        if (node->has_property("color")) {
            color_id = node->get_property<int>("color");
        }
        active_labels[label] = color_id;
    }

    /*!
     * Check to see if label in an active label.
     * \param label volume label
     * \return true if in list, false otherwise
    */
    bool is_active_label(Label_t label) const
    {
        return (active_labels.find(label) != active_labels.end());
    }

    /*!
     * Remove label from list if it exists.
     * \param label volume label
    */
    void rm_active_label(Label_t label)
    {
        active_labels.erase(label);
    }

    Stack* get_stack()
    {
        return stack;
    }

    void set_children_labels(Label_t label_id, int color_id);
    void get_rgb(int color_id, unsigned char& r,
        unsigned char& g, unsigned char& b);
    void set_undo_edge(Label_t node_base, Label_t node_absorb);
    bool undo_queue_empty();
    void unmerge_edge();

    bool get_active_labels(std::tr1::unordered_map<Label_t, int>& active_labels_);
    bool get_select_label(Label_t& select_curr, Label_t& select_old);
    bool get_show_all(bool& show_all_);
    bool get_reset_stack(VolumeLabelPtr& labelvol, RagPtr& rag);
    bool get_plane(unsigned int& plane_id);
    bool get_opacity(unsigned int& opacity_);
    void compute_label_colors(RagPtr rag);

    void toggle_gt();
    void set_reset_stack();
    void increment_plane();
    void decrement_plane();
    void toggle_show_all();
    void set_plane(unsigned int plane);
    void set_opacity(unsigned int opacity_);

    void select_label(unsigned int x, unsigned int y, unsigned z);
    void active_label(unsigned int x, unsigned int y, unsigned z);
    void reset_active_labels();
    bool is_gt_mode();
    void set_body_pair(Label_t node1, Label_t node2, Location location);
    bool get_zoom_loc(unsigned int& x, unsigned int& y, double& zoom_factor_);
    bool is_remove_edge(Label_t& node1, Label_t& node2);
    void merge_edge();
    void set_commit_edge(Label_t node_remove, Label_t node_keep, bool ignore_rag);
    int get_num_examined_edges();

  private:
    void select_label(Label_t current_label);
    void initialize();
    void set_zoom_loc(Location location, double zoom_factor_);
   
    Stack* stack;
    Stack* gt_stack;  
  
    std::tr1::unordered_map<Label_t, unsigned int> body_to_view_id;

    //! Contains a set of active labels for examination
    std::tr1::unordered_map<Label_t, int> active_labels;
    bool active_labels_changed;

    Label_t selected_id;
    Label_t old_selected_id;

    bool selected_id_changed; 
    bool show_all;
    bool show_all_changed;
    unsigned int active_plane;
    unsigned int opacity;
    bool active_plane_changed;
    bool opacity_changed;
    std::string saved_session_name;
    bool gt_mode;
    bool reset_stack;
    unsigned int x_zoom, y_zoom;
    bool zoom_loc;
    double zoom_factor;

    Label_t node1_focused;
    Label_t node2_focused;
    bool remove_edge;
    int undo_queue;
    int edges_examined;
};

}


#endif
