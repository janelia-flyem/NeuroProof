/*!
 * Main view for the stack view application that defines
 * the layout of the widgets that contain editing tools,
 * the plane view, and the body view.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#ifndef STACKQTUI_H
#define STACKQTUI_H

// contains the automatically created ui from Qt designer
#include "ui_neuroproof_stack_viewer.h"

// main model for the stack viewer
#include "../Stack/StackSession.h"

namespace NeuroProof {

/*!
 * This class is a type of QMainWindow that contains
 * the main GUI elements.  Notably, it contains a tool
 * box dock widget, a body viewer dock widget, and a
 * plane widget.  A body view object should add the
 * appropriate dock widget as a parent.  This class is
 * an stack observer allowing it to make updates when
 * the stack object changes.
*/
class StackQTUi : public QMainWindow, public StackObserver {
    Q_OBJECT

  public:
    /*!
     * Constructor that displays the main window
     * and the defaults depending on whether a stack
     * session is supplied or not.
     * \param stack_session_ stack session model (0 for no session)
    */
    StackQTUi(StackSession* stack_session_);
    
    /*!
     * Destructor that removes the object as an observer
     * of the currently loaded stack session (if there is
     * one loaded).
    */
    ~StackQTUi();
    
    /*!
     * Detaches the GUI from the  current stack session
     * (if loaded).  It also disables widgets, such as the
     * tool panel.
    */
    void clear_session();   
    
    /*!
     * Removes the previous stack session (if loaded) from
     * the GUI and resets the elements for the new session.
     * \param stack_session_ stack session model
    */
    void load_session(StackSession* stack_session_);
    
    /*!
     * Updates the GUI widgets when the stack session changes
     * the relevant state and dispatches an event.  In particular,
     * it updates the opacity and plane changing sliders.
    */ 
    void update();
   
    //! public access to the ui main window TODO: prevent public access 
    Ui::MainWindow ui;

  public slots:
    /*!
     * Handles signals that occur when the plane slider changes.
     * Currently, the QT controller hooks this function up to
     * the action.  This should probably be done in the view
     * constructor to be a little clearer.
     * \param val plane value (max_plane - val gives actual plane location)
    */ 
    void slider_change(int val);

    /*!
     * Handles signals that occur when the opacity slider changes.
     * Currently, the QT controller hooks this function up to
     * the action.  This should probably be done in the view
     * constructor to be a little clearer.
     * \param val opacity value (10 for opaque, 0 for transparent)
    */ 
    void opacity_change(int val);

  private:
    //! stack session model
    StackSession * stack_session;

    //! maximum plane for current stack session
    unsigned int max_plane;
};

}

#endif
