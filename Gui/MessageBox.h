/*!
 * Utility for popping up information in the GUI.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include <QMessageBox>

/*!
 * Class that creates a message box from a string that
 * requires the user to click okay to close the box.
*/
class MessageBox {
  public:
    /*!
     * Constructor that creates a QMessageBox.
     * \param message char * message
    */
    MessageBox(const char * message)
    {
        msgbox.setText(message);
        msgbox.exec();
    }
  private:
    //! pop-up box with message
    QMessageBox msgbox;
};


#endif
