#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_login.h"

class login : public QMainWindow
{
    Q_OBJECT

public:
    login(QWidget* parent = nullptr);
    ~login();

public:
    Ui::loginClass ui;
};
