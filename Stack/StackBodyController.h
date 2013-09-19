/*!
 * Contains controller in MVC for generating 3D volume renderings
 * of a given body.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#ifndef STACKBODYCONTROLLER_H
#define STACKBODYCONTROLLER_H

#include "StackObserver.h"

class QWidget;

namespace NeuroProof {

// model for stack
class StackSession;

// view for 3D body viewer
class StackBodyView;

/*!
 * Controller derives is an observer to events triggered
 * in the model.  This controller sets up the 3D body view
*/
class StackBodyController : public StackObserver {
  public:
    /*!
     * Constructor that requires a stack session model and an
     * optional main window widget that will holder the body viewer.
     * \param stack_session session holding stack
     * \param widget_parent main widget that contains the body viewd
    */
    StackBodyController(StackSession* stack_session, QWidget* widget_parent = 0);
    
    /*!
     * Destructor that removes view from the parent and deletes the view.
    */
    ~StackBodyController();

    /*!
     * Initialize view linked to the controller.
    */
    virtual void initialize();
    
    /*!
     * No updates occur when the session dispatches an event.
    */
    virtual void update() {}
    
    /*!
     * Starts the view.
    */
    virtual void start();

  private:
    //! stack session model holds the segmentation stack
    StackSession* stack_session;

    //! 3D body view
    StackBodyView* view;

};

}

#endif
