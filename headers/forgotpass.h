#pragma once

#include <QtWidgets/QMainWindow>
#include "../build/ui_forgotpass.h"

class forgot : public QMainWindow
{
    Q_OBJECT

public:
    forgot(QWidget* parent = nullptr);
    ~forgot();

public:
    Ui::forgotpassClass ui;
};
