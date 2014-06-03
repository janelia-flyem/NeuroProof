/*!
 * Functionality for the main window QT widget controller.
 * It also helps coordinate functioanlity with other GUI
 * controllers such as the plane and body controller.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#ifndef STACKQTCONTROLLER_H
#define STACKQTCONTROLLER_H

#include "../Stack/StackObserver.h"
#include <QWidget>

class QApplication;

namespace NeuroProof {

class StackSession;
class StackQTUi;
class StackPlaneController;
class StackBodyController;

// qt controller needs to be aware of edge priority algorithms
// TODO: seperate controller to handle priority algorithm
class EdgeEditor;

/*!
 * Controller of type QObject that handles events emitted from
 * the qt main window view.  The controller helps call the appropriate
 * functions to update the stack session model that dispatches
 * events for all of its listeners.  Technically, this class is
 * also an observer, but it currently does not perform any actions
 * when the stack session state changes.
*/
class StackQTController : public QObject, public StackObserver {
    Q_OBJECT
  
  public:
    /*!
     * Constructor that creates the main qt window and loads the
     * stack session, if provided, after a small time delay.  The
     * constructor also connects Qt signals to the proper functions.
     * \param stack_session stack session model (0 means no initial session)
     * \param qapp_ qt application
    */
    StackQTController(StackSession* stack_session_, QApplication* qapp_);
   
    /*!
     * Deletes the body and plane controller if they were created.
     * It also deletes the stack session and the main qt window.  This
     * delete essentially quits the program.
    */ 
    ~StackQTController();

    /*!
     * No actions are performed when the stack session dispatches
     * an event.
    */
    void update();

  private:
    /*!
     * Internal function called when a new session is created or
     * when the controller is deleted.  It will destroy all created
     * controllers and the session.
    */  
    void clear_session();
    
    /*! Internal function called when in the training mode to update
     * the progress through the edges examined.
    */
    void update_progress();

    //! stack session model
    StackSession* stack_session;

    //! Qt application object
    QApplication* qapp;

    //! main Qt view
    StackQTUi* main_ui;

    //! controls the widget for planar views of the stack
    StackPlaneController* plane_controller;

    //! controls the widget for the 3D body views of bodies -- default disabled  
    StackBodyController* body_controller;

    //! handles interface to edge ordering algorithm
    EdgeEditor * priority_scheduler;

    bool training_mode;

  private slots:
    /*!
     * Handles open session button click event.  The user
     * chooses a previous session from the file prompt and that
     * session will be loaded into the viewers.
    */
    void open_session();
    
    /*!
     * Handles the quit button click event.  Stops the gui.
    */ 
    void quit_program();
   
    /*!
     * Handles the save button click event.  The session
     * is exported to the current location or a new session
     * is created based on user input.
    */
    void save_session();
    
    /*!
     * Handles the new session click event.  The user
     * is asked to select a location where the grayscale and
     * label volume data exists to created a new session.
    */
    void new_session();

    /*!
     * Handles the save as session click event.  The user
     * is asked to select a location to export the current session.
    */
    void save_as_session();

    /*!
     * Handles the training mode click event.  The stack session
     * is reset and a priority scheduler is created, if one was
     * not already created.  The current edge selected by
     * the scheduler is then selected through the stack session model.
    */
    void start_training();
    
    /*!
     * Handles the view mode click event.  The stack session
     * is reset and all bodies are shown in the planar view
     * (not just a specific edge).
    */
    void start_viewonly();
    
    /*!
     * Handles the add gt click event.  The user is given a prompt
     * that allows one to choose ground truth labels to associate
     * with the current stack session.
    */
    void add_gt();
    
    /*!
     * Handles the intial timer event if the constructor is supplied
     * with a stack session.  Otherwise, this is an internal function
     * that initializes the plane controller for the new stack session
     * and connects some of its functionality to Qt signals.
    */
    void load_views();
    
    /*!
     * Handles the shortcuts click event.  It provides information
     * on the current shortcuts used in the program.  TODO: allow 
     * the user to change these shortcuts.
    */
    void show_shortcuts();
    
    /*!
     * Handles the about click event.  It provides information about
     * the stack viewer program.
    */
    void show_about();
    
    /*!
     * Handles the help click event.  It provides information about
     * how to use the stack viewer program.
    */
    void show_help();
    
    /*!
     * Handles the toggle gt click event in the view mode.  It resets
     * the stack session to show either the current stack labels
     * or the ground truth labels.
    */ 
    void toggle_gt();
    
    /*!
     * Handles the 3D checkbox select event.  If checked, the 3D body
     * controller is created and active bodies are displayed in the
     * 3D body viewer.  If unselected, the body controller is destroyed.
    */
    void toggle3D();
    
    /*!
     * Handles the show body panel click event.  If the body dock widget
     * was x'd out, this will re-display it.
    */
    void view_body_panel();
    
    /*!
     * Handles the show tool panel click event.  If the tool dock widget
     * was x'd out, this will re-display it.
    */
    void view_tool_panel();

    /*!
     * Handles the current edge click event in the training mode.  This
     * action will re-center the stack session to the current edge.
    */ 
    void grab_current_edge();
   
    /*!
     * Handles the next edge click event in the training mode.  This
     * will commit the last edge decision result (merge or no merge) and
     * will get the next edge (if it exists) in the scheduler.
    */ 
    void grab_next_edge();
    
    /*!
     * Handles the merge edge click event in the training mode.  This
     * will instruct the stack session to show the body pair for the
     * current edge as merged.
    */
    void merge_edge();
    
    /*!
     * Handles the undo edge click event in the training mode.  This
     * will either undo the merge_edge action if it was performed or
     * it will go back the previous edge examined (undoing a merge if
     * performed).  The number of undos is limited by the stack session
     * and a message will pop-up if no undos are possible.
    */
    void undo_edge();

};

}

#endif
