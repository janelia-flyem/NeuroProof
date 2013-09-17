/*!
 * Implements mechanism for sending updates to objects
 * that are observers.  Objects that need to send updates
 * to observing objects should inherit from this class.
 * 
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "StackObserver.h"
#include <vector>

namespace NeuroProof {

/*!
 * Class for sending udpates to observing objects.
*/
class Dispatcher {
  public:
    /*!
     * Attach observer
     * \param observer stack observer pointer
    */ 
    void attach_observer(StackObserver* observer)
    {
        observers.push_back(observer);
    }

    /*!
     * Detach observer removes observer using by linearly traversing the list.
     * \param observer stack observer pointer
    */
    void detach_observer(StackObserver* observer)
    {
        for (std::vector<StackObserver*>::iterator iter = observers.begin();
                iter != observers.end(); ++iter) {
            if (*iter == observer) {
                observers.erase(iter);
                break;
            }
        }
    }

    /*!
     * Update observer after Stack changes.
    */
    void update_all()
    {
        for (std::vector<StackObserver*>::iterator iter = observers.begin();
                iter != observers.end(); ++iter) {
            (*iter)->update();
        }
    }

  private:
    //! list of observers -- attachment order determines update order
    std::vector<StackObserver*> observers;

};

}

#endif
