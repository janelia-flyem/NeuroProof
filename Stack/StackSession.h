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

    /*!
     * Keep track of certain labels by adding to an active list.
     * \param label volume label
    */
    void add_active_label(Label_t label)
    {
        active_labels.insert(label);
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

    void get_rgb(int color_id, unsigned char& r,
        unsigned char& g, unsigned char& b);

    bool get_active_labels(std::tr1::unordered_set<Label_t>& active_labels_);
    bool get_select_label(Label_t& select_curr, Label_t& select_old);
    bool get_show_all(bool& show_all_);
    bool get_plane(unsigned int& plane_id);
    void compute_label_colors();

    void increment_plane();
    void decrement_plane();
    void toggle_show_all();
    void set_plane(unsigned int plane);

    void select_label(unsigned int x, unsigned int y, unsigned z);
    void active_label(unsigned int x, unsigned int y, unsigned z);
    void reset_active_labels();

  private:
    void select_label(Label_t current_label);
    void initialize();
    
    Stack* stack;
    
    std::tr1::unordered_map<Label_t, unsigned int> body_to_view_id;

    //! Contains a set of active labels for examination
    std::tr1::unordered_set<Label_t> active_labels;
    bool active_labels_changed;

    Label_t selected_id;
    Label_t old_selected_id;

    bool selected_id_changed; 
    bool show_all;
    bool show_all_changed;
    unsigned int active_plane;
    bool active_plane_changed;
};

}


#endif
