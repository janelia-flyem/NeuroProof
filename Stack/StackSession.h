/*!
 * Major model for in stack viewer MVC.  This contains information
 * about how the stack is currently being viewed and helps serialize
 * and deserialize a stack from disk.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#ifndef STACKSESSION_H
#define STACKSESSION_H

// model dispatches events
#include "Dispatcher.h"
#include "../BioPriors/BioStack.h"
#include <tr1/unordered_map>
#include <string>

namespace NeuroProof {

/*!
 * A type of dispatcher that is a model containing the state
 * of a given stack.  The StackSession must have a stack
 * associated with it but can be initialized from disk.
*/
class StackSession : public Dispatcher {
  public:
    /*!
     * Contructor that deserializes data from a session
     * location that contains a label volume, grayscale volume,
     * RAG, and session information json.
     * \param session_name name of directory containing session
    */
    StackSession(std::string session_name);
    
    /*!
     * Constructor that takes a stack.  If no RAG exists, a
     * RAG is generated.  TODO: Implement
     * \param stack_ segmentation stack
    */
    StackSession(Stack* stack_);
    
    /*!
     * Constructor that takes a list of grayscale images 
     * and a label volume to create a session.  A RAG is
     * created from the label volume.
     * \param gray_images list of gray images in stack order
     * \param labelvolume location of label volume
    */
    StackSession(std::vector<std::string>& gray_images,
        std::string labelvolume);

    /*!
     * Serialize stack to disk by writing an h5 with a label
     * volume and grayscale volume, RAG, and session json
     * file with session information.  It will overwrite
     * any session information currently in the directory.
     * \param session location of session to be exported
    */
    void export_session(std::string session_name); 
   
    /*!
     * Calls export session with the current session location.
     * If no session name is currently saved, an expception
     * is thrown.
    */ 
    void save(); 

    /*!
     * Add a ground truth to the stack session.  The RAG
     * for the GT should be built but this function is called
     * internally where a RAG may already exist.
     * \param gt_name location of ground truth h5 file
     * \param build_rag true means build a RAG
    */ 
    void load_gt(std::string gt_name, bool build_rag=true);
    
    /*!
     * Checks whether a ground truth exists in the stack.
     * \return true if a GT stack exists.
    */
    bool has_gt_stack();   

    /*!
     * Checks whether a session name is associated with a
     * session.  A session name is associated after an
     * export and when loading from a session location.
     * \return true if a session name exists.
    */ 
    bool has_session_name();
    
    /*!
     * Retrieves the session name for the given session.
     * \return session name or an empty string if no session name
    */
    std::string get_session_name();

    /*!
     * Retrieves the stack associated with the session.
     * \return segmentation stack
    */
    BioStack* get_stack()
    {
        return stack;
    }

    /*!
     * For a given color id, an RGB value is derived.
     * \param color_id color id
     * \param r 8-bit red value
     * \param g 8-bit green value
     * \param b 8-bit blue value
    */
    void get_rgb(int color_id, unsigned char& r,
            unsigned char& g, unsigned char& b);
    
    /*!
     * Undo the given edge that was previously examined.
     * If this edge was previously merged, the nodes are split.
     * Undos cannot occur beyond a reset of the session.  Session
     * resets can happen when the GUI shifts between different
     * viewing modes.
     * \param node_base node id
     * \param node_absorb node id that could have been removed
    */
    void set_undo_edge(Label_t node_base, Label_t node_absorb);

    /*!
     * Un-does the merge between two nodes by changing the color
     * map to indicate two different bodies.
    */
    void unmerge_edge();

    /*!
     * Check if there can be anymore undo operations.
     * \return true if more undos can occur
    */
    bool undo_queue_empty();
   
    /*!
     * Retrieve the selected active labels in the current session.  This
     * function is often used to probe the session state after observers
     * are updated.
     * \param active_labels_ map of labels to colors that are currently active
     * \return true if the active labels have changed for the current dispatch
    */
    bool get_active_labels(std::tr1::unordered_map<Label_t, int>& active_labels_);
    
    /*!
     * Retrieve which label was currently clicked.  This is not an active label
     * but typically indicates whether a label color should be toggled on or off.
     * \param select_curr label id selected
     * \param select_old last label id selected
     * \return true if the selected label changed for the current dispatch
    */
    bool get_select_label(Label_t& select_curr, Label_t& select_old);
    
    /*!
     * Probes state on whether all bodies should be highlighted or not.
     * \param show_all_ true if all bodies label colors should be shown
     * \return true if this variable changed for the current dispatch
    */
    bool get_show_all(bool& show_all_);
    
    /*!
     * Probes state on whether the stack has been reset meening that
     * the stack label volume and RAG may have changed.
     * \param labelvol label volume for the current stack
     * \param rag RAG for the current stack
     * \return true if the stack was reset for the current dispatch
    */
    bool get_reset_stack(VolumeLabelPtr& labelvol, RagPtr& rag);
    
    /*!
     * Retrieves the current plane being examined in the stack.
     * \param plane_id current plane
     * \return true if this plane was changed for the current dispatch
    */
    bool get_plane(unsigned int& plane_id);
    
    /*!
     * Retrieves the current opacity of color in the session.
     * \param opacity_ opacity value of the color
     * \return true if this value changed for the current dispatch
    */
    bool get_opacity(unsigned int& opacity_);
   
    /*!
     * Retrieve the current location that the sesion points in the stack.
     * \param x integer for x location in stack
     * \param y integer for y location in stack
     * \param zoom_factor_ amount zoom in (>1 zoom in, <1 zoom out)
     * \return true if the zoom location changed for the current dispatch
    */ 
    bool get_zoom_loc(unsigned int& x, unsigned int& y, double& zoom_factor_);

    /*!
     * Checks whether the edge defined by two nodes has been merged but has
     * not yet been committed.
     * \param node1 label id 1
     * \param node2 label id 2
     * \return true if edge has been merged
    */
    bool is_remove_edge(Label_t& node1, Label_t& node2);
    

    /*!
     * Computes the color value for each node in the RAG by
     * using a greedy-based vertex coloring algorithm in RagUtils.
     * \param rag RAG to be colored
    */
    void compute_label_colors(RagPtr rag);

    /*!
     * If ground truth exists, this will switch the ground truth
     * stack with the label stack and reset the session.
    */
    void toggle_gt();

    /*!
     * Mechanism for resetting the stack which will cause will
     * empty all the active labels, reset the zoom, reset the undo
     * queue, rest the colors, and force observers to update the
     * label volume and RAG.
    */ 
    void set_reset_stack();
    
    /*!
     * Increase plane value (z) by 1.
    */
    void increment_plane();
    
    /*!
     * Decrease plane value (z) by 1.
    */
    void decrement_plane();
    
    /*!
     * Toggle the coloring of all of the labels.
    */
    void toggle_show_all();
    
    /*!
     * Jump to a specified plane.
     * \param plane id for a given plane in the stack
    */
    void set_plane(unsigned int plane);
   

    void set_merge_bodies();
    void set_next_bodies();

    /*!
     * Set a certain opacity for color.  0 means no color
     * and 10 means opaque.
     * \param opacity integer value for opacity
    */
    void set_opacity(unsigned int opacity_);

    bool get_merge_bodies(bool& merge_bodies_);
    bool get_next_bodies(bool& next_bodies_);

    /*!
     * Selects a label (for toggling color typically) for the given
     * x, y, and z location.
     * \param x integer for x location in stack
     * \param y integer for y location in stack
     * \param z integer for z location in stack
    */
    void select_label(unsigned int x, unsigned int y, unsigned z);
    
    /*!
     * Selects a label to be added or removed from the active
     * label list for the given x, y, and z location.
     * \param x integer for x location in stack
     * \param y integer for y location in stack
     * \param z integer for z location in stack
    */
    void active_label(unsigned int x, unsigned int y, unsigned z);
    
    /*!
     * Erase all of the active labels.
    */
    void reset_active_labels();
    
    /*!
     * Checks whether the ground truth has been swapped with the label volume.
     * \return true if the ground truth is the current label volume
    */
    bool is_gt_mode();
    
    /*!
     * Adds the two bodies (presumably defining an edge) as active labels.
     * The labels are colored differently and the session sets the stack
     * location to that specified.
     * \param node1 label id 1
     * \param node2 label id 2
     * \param location location corresponding to edge between node1 and node2
    */
    void set_body_pair(Label_t node1, Label_t node2, Location location);

    /*!
     * Merge two nodes that are currently active in the session.  The 
     * colors associated with the nodes are changed but the actual merge
     * is not committed.
    */
    void merge_edge();
    
    /*!
     * Commits the edge to be either merged or not merged.  The undo
     * queue and the number of processed edges are incremented.
     * \param node_remove node to remove if a merge occurred
     * \param node_keep node to be kept if a merge occured
     * \param ignore_rag true means do not modify RAG (should be true for now)
    */
    void set_commit_edge(Label_t node_remove, Label_t node_keep, bool ignore_rag);
    
    /*!
     * Retrieve the number of edges committed to be true or false edges.
     * \return number of edges committed
    */
    int get_num_examined_edges();

  private:
    /*!
     * Keep track of certain labels by adding to an active list.
     * \param label volume label
    */
    void add_active_label(Label_t label);

    /*!
     * Helper function that sets all labels that were previously
     * merged onto a given label to a specified color.
     * \param label_id label id that other ids were reassigned to
     * \param color_id color to be assigned to the children labels
    */
    void set_children_labels(Label_t label_id, int color_id);

  public:
    /*!
     * Helper function to select a given label.
     * \param current_label label selected
    */ 
    void select_label(Label_t current_label);
   
  private:
    /*!
     * Sets all values to their default.  Called by the constructors.
    */
    void initialize();
    
    /*!
     * Zooms to a given location and saves the specified zoom factor.
     * \param location current location in stack
     * \param zoom_factor_ amount zoom in (>1 zoom in, <1 zoom out)
    */
    void set_zoom_loc(Location location, double zoom_factor_);
   
    //! active stack 
    BioStack* stack;

    //! ground truth stack
    BioStack* gt_stack;  
  
    //! contains a set of active labels for examination
    std::tr1::unordered_map<Label_t, int> active_labels;
   
    //! true if active labels have changed
    bool active_labels_changed;

    //! current label id selected (doesn't have to be an active label)
    Label_t selected_id;

    //! previous label id selected
    Label_t old_selected_id;

    //! true if selected id changed
    bool selected_id_changed;

    //! true if all labels should be colored
    bool show_all;

    bool next_bodies;
    bool next_bodies_changed;
    bool merge_bodies;
    bool merge_bodies_changed;

    //! true if show all value changed
    bool show_all_changed;

    //! current active plane
    unsigned int active_plane;

    //! current opacity of color (0 transparent, 10 opaque)
    unsigned int opacity;

    //! true if active plane changed
    bool active_plane_changed;

    //! true if opactiy changed
    bool opacity_changed;

    //! current session name
    std::string saved_session_name;

    //! true if ground truth stack is the active stack
    bool gt_mode;

    //! true if the stack was reset
    bool reset_stack;

    //! current x and y locations in the stack
    unsigned int x_zoom, y_zoom;

    //! true if zoom location changed
    bool zoom_loc;

    //! factor of zoom (>1 zoom in)
    double zoom_factor;

    //! label 1 corresponding to current edge being examined
    Label_t node1_focused;
    
    //! label 2 corresponding to current edge being examined
    Label_t node2_focused;

    //! true if edge should be removed
    bool remove_edge;

    //! number of actions that can be undone
    int undo_queue;

    //! number of edges examined (saved on export and restored on session load)
    int edges_examined;
};

}


#endif
