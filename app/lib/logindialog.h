#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

    QString getUsername();
    QString getPassword();
    bool isRegisterSelected();

private:
    Ui::LoginDialog *ui;
};

#endif // LOGINDIALOG_H
