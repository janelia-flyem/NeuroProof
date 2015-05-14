/*!
 * Functionality for allowing views and controllers to
 * receive updates from the stack when changes are made.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#ifndef STACKOBSERVER_H
#define STACKOBSERVER_H

/*!
 * Interface base class for observing Stack objects.
*/
class StackObserver {
  public:
    /*!
     * Call observer update functionality when
     * the stack changes.
    */
    virtual void update() = 0;

  protected:
    /*!
     * Prevent destruction of observer through
     * the base class pointer.
    */
    virtual ~StackObserver() {}
};

#endif
