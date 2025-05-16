#include "../headers/Chat__Application.h"
#include "../headers/chatPage.h"
#include "../headers/forgotpass.h"
#include "../headers/login.h"
#include "../headers/signup.h"
#include "../server/server.h"

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

  // Handle All Buttons within login 

  /*
   * Contributors: Alaa and Malak
   * Function: Login to Signup Navigation
   * Description: Handles navigation from login page to signup page.
   * When the user clicks 'Sign Up' on the login screen, it clears any error
   * and navigates to the signup page (index 1).
   */
  connect(log->ui.pushButton, &QPushButton::clicked, this, [=]() {
    log->ui.errorLabel->clear(); // Clear any previous error
    ui.MainWidget->setCurrentIndex(1);
  });

  /*
   * Contributors: Alaa and Malak
   * Function: Login Authentication
   * Description: Validates login credentials and authenticates the user.
   * Checks input fields, validates email format, and attempts to log in.
   * If successful, clears the form and navigates to the chat page (index 2).
   */
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
    }
  }); 

  /*
   * Contributors: Alaa and Malak
   * Function: Forgot Password Navigation
   * Description: Handles navigation from login to forgot password page.
   * Clears any error message and navigates to the forgot password screen (index 3).
   */
  connect(log->ui.pushButton_2, &QPushButton::clicked, this, [=]() {
    log->ui.errorLabel->clear(); // Clear any previous error
    ui.MainWidget->setCurrentIndex(3);
  });

  // Handle All Buttons within Signup

  /*
   * Contributors: Alaa and Malak
   * Function: User Registration
   * Description: Validates signup form and registers a new user.
   * Performs validation on all fields, creates a new user account,
   * logs in automatically if successful, and navigates to the chat page (index 2).
   */
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
        
        // Create client object for new user
        Client* newClient = server::getInstance()->loginUser(email, password);
        if (newClient) {
            
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
    }
  });

  /*
   * Contributors: Alaa and Malak
   * Function: Signup to Login Navigation
   * Description: Handles navigation from signup page back to login page.
   * Clears any error message and returns to the login screen (index 0).
   */
  connect(sign->ui.pushButton_2, &QPushButton::clicked, this, [=]() {
    sign->ui.errorLabel->clear(); // Clear any previous error
    ui.MainWidget->setCurrentIndex(0);
  });

  /*
   * Contributors: Alaa and Malak
   * Function: Password Reset
   * Description: Implements the forgot password functionality.
   * Validates input fields, resets the user's password if email exists,
   * and navigates back to the login page (index 0) on success.
   */
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
        fo->ui.lineEdit_2->clear();
        fo->ui.lineEdit_3->clear();
        fo->ui.lineEdit_4->clear();
        fo->ui.errorLabel->clear();
        ui.MainWidget->setCurrentIndex(0);
    } else {
        fo->ui.errorLabel->setText(errorMsg);
    }
  });

  /*
   * Contributors: Alaa and Malak
   * Function: Forgot Password to Login Navigation
   * Description: Handles navigation from forgot password page back to login page.
   * Clears any error message and returns to the login screen (index 0).
   */
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
    
}
