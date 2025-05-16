#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_signup.h"

class signup : public QMainWindow
{
    Q_OBJECT

public:
    signup(QWidget* parent = nullptr);
    ~signup();

public:
    Ui::signupClass ui;
};
