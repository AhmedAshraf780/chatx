#include "Chat__Application.h"
#include "chatPage.h"
#include "login.h"
#include "signup.h"

Chat__Application::Chat__Application(QWidget *parent) : QMainWindow(parent) {
  ui.setupUi(this);
  login *log = new login(this);
  signup *sign = new signup(this);
  ChatPage *chat = new ChatPage(this);

  ui.MainWidget->addWidget(log);
  ui.MainWidget->addWidget(sign);
  ui.MainWidget->addWidget(chat);
  connect(log->ui.pushButton, &QPushButton::clicked, this, [=]() {
    qDebug() << "LogIn";
    ui.MainWidget->setCurrentIndex(2);
  });
  connect(sign->ui.pushButton_2, &QPushButton::clicked, this, [=]() {
    qDebug() << "signup";
    ui.MainWidget->setCurrentIndex(0);
  });
}

Chat__Application::~Chat__Application() {}
