#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include <QMessageBox>

class MessageBox {
  public:
    MessageBox(const char * message)
    {
        msgbox.setText(message);
        msgbox.exec();
    }
  private:
    QMessageBox msgbox;
};


#endif
