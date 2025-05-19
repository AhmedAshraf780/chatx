#include "../headers/Chat__Application.h"
#include "../headers/chatPage.h"
#include "../headers/forgotpass.h"
#include "../headers/login.h"
#include "../headers/signup.h"
#include "../server/server.h"
#include <QStyleFactory>

Chat__Application::Chat__Application(QWidget *parent) : QMainWindow(parent) {
  ui.setupUi(this);
  login *log = new login(this); 
  signup *sign = new signup(this);
  ChatPage *chat = new ChatPage(this);
  forgot *fo = new forgot(this);

  ui.MainWidget->addWidget(log); // index 0
  ui.MainWidget->addWidget(sign); // 1
  ui.MainWidget->addWidget(chat); // 2
  ui.MainWidget->addWidget(fo); // 3

  // Clear login fields when switching to login screen
  connect(ui.MainWidget, &QStackedWidget::currentChanged, [=](int index) {
    if (index == 0) { // Login screen
      log->ui.lineEdit->clear();  // Clear email field
      log->ui.lineEdit_2->clear(); // Clear password field
      log->ui.errorLabel->clear(); // Clear error message
    }
  });

  // Handle All Buttons within login 

  // Handle sign up Button when you don't have an account
  connect(log->ui.pushButton, &QPushButton::clicked, this, [=]() {
    log->ui.errorLabel->clear(); // Clear any previous error
    ui.MainWidget->setCurrentIndex(1);
  });

  // Handle Login Validations
  connect(log->ui.pushButton_5, &QPushButton::clicked, this, [=]() {
    QString email = log->ui.lineEdit->text();
    QString pass = log->ui.lineEdit_2->text();
    
    // Check for empty fields
    if (email.trimmed().isEmpty() || pass.trimmed().isEmpty()) {
        log->ui.errorLabel->setText("Please fill in all fields");
        return;
    }

    // Validate email format
    if (!server::getInstance()->isValidEmail(email)) {
        log->ui.errorLabel->setText("Please enter a valid email address");
        return;
    }
    
    // Try to login and get client object
    Client* client = server::getInstance()->loginUser(email, pass);
    if (client) {
        qDebug() << "Login successful for user:" << client->getUsername();
        log->ui.lineEdit->clear();
        log->ui.lineEdit_2->clear();
        log->ui.errorLabel->clear();
        
        // Properly initialize the chat page
        chat->startStatusUpdateTimer(); // Start the status timer
        
        // Initialize chat page with the client's data - properly loads user list
        QApplication::setOverrideCursor(Qt::WaitCursor); // Show loading cursor
        chat->loadUsersFromDatabase();
        QApplication::restoreOverrideCursor(); // Restore cursor
        
        // Switch to chat page
        ui.MainWidget->setCurrentIndex(2);
        
        // Force refresh of online status after a short delay
        QTimer::singleShot(500, chat, &ChatPage::forceRefreshOnlineStatus);
    } else {
        log->ui.errorLabel->setText("Invalid email or password. Please try again.");
        qDebug() << "Login failed: Invalid credentials";
    }
  }); 

  // Handle forgot password 
  connect(log->ui.pushButton_2, &QPushButton::clicked, this, [=]() {
    log->ui.errorLabel->clear(); // Clear any previous error
    ui.MainWidget->setCurrentIndex(3);
  });

  // Handle All Buttons within Signup

  // Handle SignUp Validation and save users Data
  connect(sign->ui.pushButton, &QPushButton::clicked, this, [=]() {
    QString username = sign->ui.lineEdit->text();
    QString email = sign->ui.lineEdit_2->text();
    QString password = sign->ui.lineEdit_3->text();
    QString confirmPassword = sign->ui.lineEdit_4->text();
    
    // Check for empty fields
    if (username.trimmed().isEmpty() || email.trimmed().isEmpty() || 
        password.trimmed().isEmpty() || confirmPassword.trimmed().isEmpty()) {
        sign->ui.errorLabel->setText("Please fill in all fields");
        return;
    }

    // Validate email format
    if (!server::getInstance()->isValidEmail(email)) {
        sign->ui.errorLabel->setText("Please enter a valid email address");
        return;
    }

    // Validate password length
    if (password.length() < 4) {
        sign->ui.errorLabel->setText("Password must be at least 4 characters long");
        return;
    }

    // Check if passwords match
    if (password != confirmPassword) {
        sign->ui.errorLabel->setText("Passwords do not match");
        return;
    }
    
    QString errorMessage;
    if (server::getInstance()->registerUser(username, email, password, confirmPassword, errorMessage)) {
        qDebug() << "Signup successful";
        
        // Create client object for new user
        Client* newClient = server::getInstance()->loginUser(email, password);
        if (newClient) {
            qDebug() << "Created client for new user:" << newClient->getUsername();
            
            // Properly initialize the chat page
            chat->startStatusUpdateTimer(); // Start the status timer
            
            // Initialize chat page with the new client's data
            QApplication::setOverrideCursor(Qt::WaitCursor); // Show loading cursor
            chat->loadUsersFromDatabase();
            QApplication::restoreOverrideCursor(); // Restore cursor
            
            // Clear form and switch to chat page
            sign->ui.lineEdit->clear();
            sign->ui.lineEdit_2->clear();
            sign->ui.lineEdit_3->clear();
            sign->ui.lineEdit_4->clear();
            sign->ui.errorLabel->clear();
            
            // Go directly to chat page
            ui.MainWidget->setCurrentIndex(2);
            
            // Force refresh of online status after a short delay
            QTimer::singleShot(500, chat, &ChatPage::forceRefreshOnlineStatus);
        } else {
            sign->ui.errorLabel->setText("Error creating user session. Please try logging in.");
            ui.MainWidget->setCurrentIndex(0); // Go back to login page
        }
    } else {
        sign->ui.errorLabel->setText(errorMessage);
        qDebug() << "Signup failed:" << errorMessage;
    }
  });

  // Handle login button -> back to login immediately
  connect(sign->ui.pushButton_2, &QPushButton::clicked, this, [=]() {
    sign->ui.errorLabel->clear(); // Clear any previous error
    ui.MainWidget->setCurrentIndex(0);
  });

  // Forgot Password Logic and events
  connect(fo->ui.pushButton, &QPushButton::clicked, this, [=]() {
    QString email = fo->ui.lineEdit_2->text();
    QString password = fo->ui.lineEdit_3->text();
    QString confirmPass = fo->ui.lineEdit_4->text();

    // Check for empty fields
    if (email.trimmed().isEmpty() || password.trimmed().isEmpty() || 
        confirmPass.trimmed().isEmpty()) {
        fo->ui.errorLabel->setText("Please fill in all fields");
        return;
    }

    // Validate email format
    if (!server::getInstance()->isValidEmail(email)) {
        fo->ui.errorLabel->setText("Please enter a valid email address");
        return;
    }

    // Validate password length
    if (password.length() < 4) {
        fo->ui.errorLabel->setText("Password must be at least 4 characters long");
        return;
    }

    // Check if passwords match
    if (password != confirmPass) {
        fo->ui.errorLabel->setText("Passwords do not match");
        return;
    }

    QString errorMsg;
    if(server::getInstance()->resetPassword(email, password, confirmPass, errorMsg)) {
        qDebug() << "Reset password successfully";
        fo->ui.lineEdit_2->clear();
        fo->ui.lineEdit_3->clear();
        fo->ui.lineEdit_4->clear();
        fo->ui.errorLabel->clear();
        ui.MainWidget->setCurrentIndex(0);
    } else {
        fo->ui.errorLabel->setText(errorMsg);
        qDebug() << "Reset password failed:" << errorMsg;
    }
  });

  // Handle back to login from forgot password
  connect(fo->ui.pushButton_2, &QPushButton::clicked, this, [=]() {
    fo->ui.errorLabel->clear(); // Clear any previous error
    ui.MainWidget->setCurrentIndex(0);
  });
}

Chat__Application::~Chat__Application() {
    // Stop timers first
    ChatPage *chatPage = qobject_cast<ChatPage*>(ui.MainWidget->widget(2));
    if (chatPage) {
        chatPage->stopStatusUpdateTimer();
    }
    
    // Use the public shutdown method to properly save data and logout
    if (server::getInstance()) {
        server::getInstance()->shutdown();
    }
    
    qDebug() << "Application closing - all resources cleaned up";
}
