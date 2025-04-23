#pragma once

#include <QtWidgets/QMainWindow>
#include<QDebug>
#include "build/ui_Chat__Application.h"

class Chat__Application : public QMainWindow
{
    Q_OBJECT

public:
    Chat__Application(QWidget *parent = nullptr);
    ~Chat__Application();

private:
    Ui::Chat__ApplicationClass ui;
};
