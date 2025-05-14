#include "../headers/chatPage.h"
#include "../server/server.h"

#include <QAction>
#include <QDateTime>
#include <QEvent>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMouseEvent>
#include <QPalette>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QTextStream>
#include <QCheckBox>
#include <QTimer>

ChatPage::ChatPage(QWidget *parent)
    : QWidget(parent), currentUserId(-1), isSearching(false) {
    QHBoxLayout *mainLayout = new QHBoxLayout(this);

    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Navigation Panel (leftmost)
    createNavigationPanel(mainLayout);

    // Users Panel (second from left)
    createUsersPanel(mainLayout);

    // Chat Area (right side)
    createChatArea(mainLayout);

    // Clear any stale UI state to start with a clean slate
    chatHeader->setText("Select a user");
    messageInput->clear();
    messageLayout->addStretch();
    
<<<<<<< HEAD
=======
    // Reset to Chat tab (index 0)
    contentStack->setCurrentIndex(0);
    
    // Reset navigation buttons to show Chat button as selected
    for (int j = 0; j < navButtons.size(); j++) {
        navButtons[j]->setStyleSheet(
            QString(R"(
            background-color: %1;
            border-radius: 12px;
            color: white;
            font-size: 18px;
        )")
                .arg(j == 0 ? "#4d6eaa" : "#2e2e3e")); // Highlight Chat button
    }
    
    // Ensure users sidebar is visible
    usersSidebar->show();
    
    // Load user settings
    loadUserSettings();
    
    // Ensure the user is set to offline by default
    Client *client = server::getInstance()->getCurrentClient();
    if (client) {
        QString userId = client->getUserId();
        server::getInstance()->setUserOnlineStatus(userId, false);
        
        // Update checkbox to reflect offline status
        if (onlineStatusCheckbox) {
            onlineStatusCheckbox->blockSignals(true);
            onlineStatusCheckbox->setChecked(false);
            onlineStatusCheckbox->blockSignals(false);
            userSettings.isOnline = false;
            qDebug() << "Set default status to offline for user:" << userId;
        }
    }

>>>>>>> bfd0fc2 (handle the settings)
    // Load users from database
    loadUsersFromDatabase();

    // Show contacts by default and select the first contact
    updateUsersList();
    if (!userList.isEmpty()) {
        for (int i = 0; i < userList.size(); ++i) {
            if (userList[i].isContact || userList[i].hasMessages) {
                usersListWidget->setCurrentRow(i);
                currentUserId = i;
                updateChatArea(i);
                break;
            }
        }
    }
    
    // Set up timer to refresh online status every 10 seconds
    onlineStatusTimer = new QTimer(this);
    connect(onlineStatusTimer, &QTimer::timeout, this, &ChatPage::refreshOnlineStatus);
    onlineStatusTimer->start(10000); // 10 seconds
}

ChatPage::~ChatPage() {
    // Clean up any resources if needed
    // Qt's parent-child relationship will handle most cleanup automatically
}

void ChatPage::createNavigationPanel(QHBoxLayout *mainLayout) {
    // Navigation Sidebar (leftmost)
    QWidget *navSidebar = new QWidget;
    navSidebar->setFixedWidth(70);
    navSidebar->setStyleSheet("background-color: #12121a;");
    QVBoxLayout *navLayout = new QVBoxLayout(navSidebar);
    navLayout->setContentsMargins(10, 15, 10, 10);
    navLayout->setSpacing(15);

    // Avatar/Profile button
    QPushButton *profileBtn = new QPushButton();
    profileBtn->setFixedSize(50, 50);
    profileBtn->setStyleSheet(R"(
        background-color: #4d6eaa;
        border-radius: 25px;
        color: white;
        font-weight: bold;
        font-size: 20px;
    )");
    profileBtn->setText("ME");
    navLayout->addWidget(profileBtn);

    // Navigation buttons
    QStringList navItems = {"Chat", "Stories", "Calls", "Settings"};
    QStringList navIcons = {"💬", "📷", "📞", "⚙️"};

    for (int i = 0; i < navItems.size(); i++) {
        QPushButton *navBtn = new QPushButton(navIcons[i]);
        navBtn->setToolTip(navItems[i]);
        navBtn->setFixedSize(50, 50);
        navBtn->setStyleSheet(
            QString(R"(
            background-color: %1;
            border-radius: 12px;
            color: white;
            font-size: 18px;
        )")
                .arg(i == 0 ? "#4d6eaa" : "#2e2e3e")); // Highlight Chat button

        navLayout->addWidget(navBtn);

        // Store nav buttons for later access
        navButtons.append(navBtn);

        // Connect button signals
        connect(navBtn, &QPushButton::clicked, [this, i, navItems]() {
            // Update button styling
            for (int j = 0; j < navButtons.size(); j++) {
                navButtons[j]->setStyleSheet(
                    QString(R"(
                    background-color: %1;
                    border-radius: 12px;
                    color: white;
                    font-size: 18px;
                )")
                        .arg(j == i ? "#4d6eaa" : "#2e2e3e"));
            }

            // Show appropriate view in stack
            if (i < contentStack->count()) {
                contentStack->setCurrentIndex(i);
                
                // Show usersSidebar only when in Chat view (index 0)
                if (i == 0) {
                    usersSidebar->show();
                } else {
                    usersSidebar->hide();
                }
                
                // If showing settings, refresh the settings UI and ensure offline status
                if (i == 3) { // Settings index
                    loadUserSettings(); // Refresh settings when tab is selected
                    
                    // Always set to offline in settings page
                    if (onlineStatusCheckbox) {
                        Client *client = server::getInstance()->getCurrentClient();
                        if (client) {
                            QString userId = client->getUserId();
                            
                            // Set to offline
                            server::getInstance()->setUserOnlineStatus(userId, false);
                            
                            // Update UI to show offline
                            onlineStatusCheckbox->blockSignals(true);
                            onlineStatusCheckbox->setChecked(false);
                            onlineStatusCheckbox->blockSignals(false);
                            
                            // Update settings
                            userSettings.isOnline = false;
                            
                            qDebug() << "Set offline on settings page for user:" << userId;
                            
                            // Force refresh the user list to update online status
                            refreshOnlineStatus();
                        }
                    }
                }
            }
        });
    }

    navLayout->addStretch();
    
    // Add logout button at the bottom
    QPushButton *logoutBtn = new QPushButton("🚪");
    logoutBtn->setToolTip("Logout");
    logoutBtn->setFixedSize(50, 50);
    logoutBtn->setStyleSheet(R"(
        background-color: #E15554;
        border-radius: 12px;
        color: white;
        font-size: 18px;
    )");
    
    connect(logoutBtn, &QPushButton::clicked, this, &ChatPage::logout);
    navLayout->addWidget(logoutBtn);
    
    mainLayout->addWidget(navSidebar);
}

void ChatPage::createUsersPanel(QHBoxLayout *mainLayout) {
    // Users Sidebar (left side)
    usersSidebar = new QWidget;
    usersSidebar->setFixedWidth(250);
    usersSidebar->setStyleSheet(
        "background-color: #1e1e2e; border-right: 1px solid #2e2e3e;");
    QVBoxLayout *usersLayout = new QVBoxLayout(usersSidebar);
    usersLayout->setContentsMargins(10, 10, 10, 10);

    // Search bar
    searchInput = new QLineEdit;
    searchInput->setPlaceholderText("Search users...");
    searchInput->setStyleSheet(R"(
        background-color: #23233a;
        color: white;
        border-radius: 10px;
        padding: 8px;
        border: none;
    )");
    usersLayout->addWidget(searchInput);

    // Users list widget
    usersListWidget = new QListWidget;
    usersListWidget->setStyleSheet(R"(
        QListWidget {
            background-color: #23233a;
            border: none;
            color: white;
            outline: none;
        }
        QListWidget::item {
            padding: 8px;
            border-bottom: 1px solid #2e2e3e;
        }
        QListWidget::item:selected {
            background-color: #35355a;
        }
    )");
    usersListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    usersListWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    usersListWidget->setSpacing(0);

    usersLayout->addWidget(usersListWidget);

    // Connect search and selection
    connect(searchInput, &QLineEdit::textChanged, this, &ChatPage::filterUsers);
    connect(usersListWidget, &QListWidget::currentRowChanged, this,
            &ChatPage::handleUserSelected);

    mainLayout->addWidget(usersSidebar);
}

void ChatPage::createChatArea(QHBoxLayout *mainLayout) {
    // Content stack (for different views - chat, stories, etc.)
    contentStack = new QStackedWidget;
    contentStack->setStyleSheet("background-color: #12121a;");

    // 1. Chat View
    QWidget *chatView = new QWidget;
    QVBoxLayout *chatLayout = new QVBoxLayout(chatView);
    chatLayout->setContentsMargins(10, 10, 10, 10);
    chatLayout->setSpacing(10);

    // Chat header
    chatHeader = new QLabel("Select a user");
    chatHeader->setStyleSheet("color: white; font-size: 18px;");
    chatLayout->addWidget(chatHeader);

    // Message area
    messageArea = new QScrollArea;
    messageArea->setWidgetResizable(true);
    messageArea->setStyleSheet("border: none; background-color: #12121a;");

    QWidget *messageWidget = new QWidget;
    messageWidget->setStyleSheet("background-color: #12121a;");
    messageLayout = new QVBoxLayout(messageWidget);
    messageLayout->setSpacing(2);
    messageLayout->setContentsMargins(5, 5, 5, 5);
    messageLayout->addStretch();
    messageArea->setWidget(messageWidget);
    chatLayout->addWidget(messageArea);
    chatLayout->setStretchFactor(messageArea, 1);

    // Input area
    QWidget *inputArea = new QWidget;
    QHBoxLayout *inputLayout = new QHBoxLayout(inputArea);
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->setSpacing(10);

    messageInput = new QLineEdit;
    messageInput->setPlaceholderText("Type a message...");
    messageInput->setStyleSheet(R"(
        background-color: #2e2e3e;
        color: white;
        border-radius: 10px;
        padding: 10px;
        border: none;
    )");

    QPushButton *sendButton = new QPushButton("Send");
    sendButton->setStyleSheet(R"(
        background-color: #4e4e9e;
        color: white;
        border-radius: 10px;
        padding: 10px 20px;
        border: none;
    )");

    // Connect send button and enter key
    connect(sendButton, &QPushButton::clicked, this, &ChatPage::sendMessage);
    connect(messageInput, &QLineEdit::returnPressed, this,
            &ChatPage::sendMessage);

    messageInput->setMinimumHeight(40);
    sendButton->setMinimumHeight(40);

    inputLayout->addWidget(messageInput);
    inputLayout->addWidget(sendButton);
    chatLayout->addWidget(inputArea);

    contentStack->addWidget(chatView);

    // 2. Stories View
    QWidget *storiesView = new QWidget;
    QVBoxLayout *storiesLayout = new QVBoxLayout(storiesView);
    QLabel *storiesLabel = new QLabel("Stories Feature Coming Soon");
    storiesLabel->setAlignment(Qt::AlignCenter);
    storiesLabel->setStyleSheet("color: white; font-size: 24px;");
    storiesLayout->addWidget(storiesLabel);
    contentStack->addWidget(storiesView);

    // 3. Calls View
    QWidget *callsView = new QWidget;
    QVBoxLayout *callsLayout = new QVBoxLayout(callsView);
    QLabel *callsLabel = new QLabel("Calls Feature Coming Soon");
    callsLabel->setAlignment(Qt::AlignCenter);
    callsLabel->setStyleSheet("color: white; font-size: 24px;");
    callsLayout->addWidget(callsLabel);
    contentStack->addWidget(callsView);

    // 4. Settings View
    QWidget *settingsView = new QWidget;
    QVBoxLayout *settingsLayout = new QVBoxLayout(settingsView);
    settingsLayout->setAlignment(Qt::AlignTop);
    settingsLayout->setContentsMargins(40, 40, 40, 40);
    settingsLayout->setSpacing(20);
    
    // Settings Header
    QLabel *settingsHeader = new QLabel("User Settings");
    settingsHeader->setStyleSheet("color: white; font-size: 28px; font-weight: bold; margin-bottom: 20px;");
    settingsLayout->addWidget(settingsHeader);
    
    // Nickname Section
    QLabel *nicknameLabel = new QLabel("Nickname:");
    nicknameLabel->setStyleSheet("color: white; font-size: 16px;");
    nicknameEdit = new QLineEdit();
    nicknameEdit->setStyleSheet(R"(
        background-color: #2e2e3e;
        color: white;
        border-radius: 8px;
        padding: 10px;
        border: none;
        font-size: 15px;
    )");
    nicknameEdit->setFixedHeight(40);
    
    // Bio Section
    QLabel *bioLabel = new QLabel("Bio:");
    bioLabel->setStyleSheet("color: white; font-size: 16px;");
    bioEdit = new QTextEdit();
    bioEdit->setStyleSheet(R"(
        background-color: #2e2e3e;
        color: white;
        border-radius: 8px;
        padding: 10px;
        border: none;
        font-size: 15px;
    )");
    bioEdit->setFixedHeight(100);
    
    // Online Status (WiFi) Toggle
    QLabel *onlineLabel = new QLabel("WiFi (Online Status):");
    onlineLabel->setStyleSheet("color: white; font-size: 16px;");
    onlineStatusCheckbox = new QCheckBox("Show me as online to others");
    onlineStatusCheckbox->setStyleSheet(R"(
        color: white;
        font-size: 15px;
    )");
    
    // Connect online status toggle
    connect(onlineStatusCheckbox, &QCheckBox::stateChanged, this, &ChatPage::onlineStatusChanged);
    
    // Delete Account Button
    QPushButton *deleteAccountButton = new QPushButton("Delete Account");
    deleteAccountButton->setStyleSheet(R"(
        background-color: #E15554;
        color: white;
        border-radius: 8px;
        padding: 12px;
        border: none;
        font-size: 15px;
        font-weight: bold;
    )");
    deleteAccountButton->setFixedWidth(200);
    
    // Confirm Button
    QPushButton *confirmButton = new QPushButton("Save Changes");
    confirmButton->setStyleSheet(R"(
        background-color: #4d6eaa;
        color: white;
        border-radius: 8px;
        padding: 12px;
        border: none;
        font-size: 15px;
        font-weight: bold;
    )");
    confirmButton->setFixedWidth(200);
    
    // Add widgets to layout
    settingsLayout->addWidget(nicknameLabel);
    settingsLayout->addWidget(nicknameEdit);
    settingsLayout->addWidget(bioLabel);
    settingsLayout->addWidget(bioEdit);
    settingsLayout->addWidget(onlineLabel);
    settingsLayout->addWidget(onlineStatusCheckbox);
    
    // Add some spacing before the buttons
    settingsLayout->addSpacing(20);
    
    // Add buttons in a horizontal layout
    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    buttonsLayout->addWidget(deleteAccountButton);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(confirmButton);
    settingsLayout->addLayout(buttonsLayout);
    
    // Add a stretch at the end to push everything to the top
    settingsLayout->addStretch();
    
    // Connect signals to slots
    connect(confirmButton, &QPushButton::clicked, this, &ChatPage::saveUserSettings);
    connect(deleteAccountButton, &QPushButton::clicked, this, &ChatPage::deleteUserAccount);
    
    contentStack->addWidget(settingsView);

    mainLayout->addWidget(contentStack);
    mainLayout->setStretch(2, 1); // Make chat area take remaining space
}

void ChatPage::logout() {
    // Clear all UI elements to prevent showing previous user's messages
    
    // Clear chat area
    QLayoutItem *item;
    while ((item = messageLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
    messageLayout->addStretch(); // Re-add stretch to push messages up
    
    // Clear chat header
    chatHeader->setText("Select a user");
    
    // Clear user list
    usersListWidget->clear();
    userList.clear();
    userMessages.clear();
    
    // Clear input field
    messageInput->clear();
    
    // Reset current user
    currentUserId = -1;
    
    // Log out the current user
    server::getInstance()->logoutUser();
    
    qDebug() << "Logging out and clearing UI";
    
    // Navigate back to login page (index 0)
    QStackedWidget* mainStack = qobject_cast<QStackedWidget*>(parent());
    if (mainStack) {
        mainStack->setCurrentIndex(0);
    }
}

void ChatPage::loadUsersFromDatabase() {
    qDebug() << "Loading users from database...";
    Client *currentClient = server::getInstance()->getCurrentClient();
    if (!currentClient) {
        qDebug() << "No current client found!";
        return;
    }

    // Clear existing data
    userList.clear();
    userMessages.clear();
    usersListWidget->clear();
    
<<<<<<< HEAD
=======
    // Reset UI to default state
    contentStack->setCurrentIndex(0); // Show the chat view
    usersSidebar->show(); // Ensure user sidebar is visible
    
    // Show all nav buttons in default state (Chat selected)
    for (int j = 0; j < navButtons.size(); j++) {
        navButtons[j]->setStyleSheet(
            QString(R"(
            background-color: %1;
            border-radius: 12px;
            color: white;
            font-size: 18px;
        )")
                .arg(j == 0 ? "#4d6eaa" : "#2e2e3e")); // Highlight Chat button
    }
    
>>>>>>> bfd0fc2 (handle the settings)
    // Clear chat area
    QLayoutItem *item;
    while ((item = messageLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
    messageLayout->addStretch(); // Re-add stretch to push messages up
    
    // Reset current user
    currentUserId = -1;
    chatHeader->setText("Select a user");
<<<<<<< HEAD
=======
    messageInput->clear();
>>>>>>> bfd0fc2 (handle the settings)

    // Get all users from the server
    QVector<QPair<QString, QString>> allUsers =
        server::getInstance()->getAllUsers();
    qDebug() << "Retrieved users from server:" << allUsers.size();

    // Get all rooms to identify users with messages
    QVector<Room*> allRooms = currentClient->getAllRooms();
    QSet<QString> usersWithMessages;
    
    // Identify users who have conversations
    for (Room* room : allRooms) {
        QStringList participants = room->getName().split('_');
        for (const QString& participant : participants) {
            if (participant != currentClient->getUserId() && !participant.isEmpty()) {
                // Check if this room has any messages
                if (!room->getMessages().isEmpty()) {
                    usersWithMessages.insert(participant);
                    qDebug() << "User has messages:" << participant;
                }
            }
        }
    }

    QVector<UserInfo> usersWithMessagesVec; // To store users with messages (will be pinned)
    QVector<UserInfo> regularUsers;         // To store users without messages

<<<<<<< HEAD
=======
    // Get server instance for online status
    server *srv = server::getInstance();

>>>>>>> bfd0fc2 (handle the settings)
    // Add all users to the list except current user
    for (const auto &user : allUsers) {
        QString email = user.first;
        QString username = user.second;

        // Skip current user
        if (email == currentClient->getUserId()) {
            qDebug() << "Skipping current user:" << email;
            continue;
        }

        UserInfo userInfo;
        userInfo.name = username;
        userInfo.email = email;
        userInfo.isOnline = srv->isUserOnline(email);
        userInfo.status = userInfo.isOnline ? "Online" : "Offline";
        userInfo.lastMessage = "";
        userInfo.lastSeen = "";
        userInfo.isContact = currentClient->hasContact(email);
        userInfo.hasMessages = usersWithMessages.contains(email);

        // Check if we've had conversations with this user
        if (userInfo.hasMessages) {
            usersWithMessagesVec.append(userInfo);
            qDebug() << "Added user with messages:" << username << email;
        } else {
            regularUsers.append(userInfo);
            qDebug() << "Added regular user:" << username << email;
        }
    }

    // Combine the lists - users with messages first, then regular users
    userList = usersWithMessagesVec + regularUsers;
    qDebug() << "Total users loaded:" << userList.size() 
             << "(Pinned: " << usersWithMessagesVec.size() 
             << ", Regular: " << regularUsers.size() << ")";

    // Ensure userMessages is initialized for every user
    for (int i = 0; i < userList.size(); ++i) {
        if (userMessages.size() <= i)
            userMessages.resize(i + 1);
    }

    updateUsersList();
}

void ChatPage::updateUsersList() {
    usersListWidget->blockSignals(
        true); // Block signals to prevent unnecessary updates
    usersListWidget->clear();

    QString searchText = searchInput->text().toLower();
    for (int i = 0; i < userList.size(); ++i) {
        const UserInfo &user = userList[i];
        // Show only contacts by default unless searching
        if (!isSearching && !user.isContact && !user.hasMessages)
            continue;
        if (isSearching && !user.name.toLower().contains(searchText) &&
            !user.email.toLower().contains(searchText))
            continue;

        QListWidgetItem *item = createUserListItem(user, i);
        usersListWidget->addItem(item);
        
        // Properly cast the QVariant to QWidget* using qobject_cast
        QWidget *widget = item->data(Qt::UserRole + 1).value<QWidget*>();
        if (widget) {
            usersListWidget->setItemWidget(item, widget);
        }
    }

    usersListWidget->blockSignals(false); // Unblock signals
}

void ChatPage::filterUsers() {
    isSearching = !searchInput->text().isEmpty();
    updateUsersList();
}

void ChatPage::handleUserSelected(int row) {
    QListWidgetItem *item = usersListWidget->item(row);
    if (!item)
        return;

    bool ok;
    int index = item->data(Qt::UserRole).toInt(&ok);
    if (!ok || index < 0 || index >= userList.size())
        return;

    // Only update if the selected user changes
    if (currentUserId != index) {
        // Get latest bio and status for the selected user
        server *srv = server::getInstance();
        if (srv) {
            QString nickname, bio;
            if (srv->getUserSettings(userList[index].email, nickname, bio)) {
                // Update user status
                userList[index].isOnline = srv->isUserOnline(userList[index].email);
                userList[index].status = userList[index].isOnline ? "Online" : "Offline";
            }
        }
        
        currentUserId = index;
        updateChatArea(index);
    }
}

void ChatPage::updateChatArea(int index) {
    qDebug() << "Updating chat area for user index:" << index;

    if (index < 0 || index >= userList.size())
        return;

    // Update header
    const UserInfo &user = userList[index];
    
    // Get user's bio from server if available
    QString bio = "";
    server *srv = server::getInstance();
    QString nickname, fetchedBio;
    if (srv && srv->getUserSettings(user.email, nickname, fetchedBio)) {
        bio = fetchedBio;
    }
    
    // Create display text with status and bio
    QString headerText = user.name + " - " + user.status;
    if (!bio.isEmpty()) {
        headerText += " (" + bio + ")";
    }
    
    chatHeader->setText(headerText);

    // Clear existing messages from UI
    QLayoutItem *item;
    while ((item = messageLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
    messageLayout->addStretch(); // Re-add stretch to push messages up

    // Load messages for this user
    currentUserId = index;
    loadMessagesForCurrentUser();

    // Add messages to UI
    if (index < userMessages.size()) {
        const QVector<MessageInfo> &messages = userMessages[index];
        for (const MessageInfo &msg : messages) {
            addMessageToUI(msg.text, msg.isFromMe, msg.timestamp);
        }
    }

    // Scroll to bottom
    QScrollBar *vScrollBar = messageArea->verticalScrollBar();
    vScrollBar->setValue(vScrollBar->maximum());
}

void ChatPage::addToContacts(int userId) {
    if (userId >= 0 && userId < userList.size()) {
        Client *currentClient = server::getInstance()->getCurrentClient();
        if (currentClient) {
            // Use email as the contact ID instead of name
            QString contactId = userList[userId].email.isEmpty() ? 
                userList[userId].name : userList[userId].email;
            
            currentClient->addContact(contactId);
            userList[userId].isContact = true;
            updateUsersList();
            
            qDebug() << "Added contact with ID:" << contactId;
        }
    }
}

void ChatPage::sendMessage() {
    if (currentUserId < 0 || messageInput->text().isEmpty()) {
        return;
    }

    Client *client = server::getInstance()->getCurrentClient();
    if (!client) {
        return;
    }

    QString messageText = messageInput->text();
    Message msg(messageText, client->getUserId());

    // Use email instead of username for consistent room creation
    QString targetEmail = userList[currentUserId].email;
    qDebug() << "Sending message to user email:" << targetEmail;
    
    // If we have an email, use it. Otherwise fall back to name (for backward compatibility)
    QString targetId = targetEmail.isEmpty() ? userList[currentUserId].name : targetEmail;
    
    Room *room = client->getRoomWithUser(targetId);
    if (!room) {
        room = client->createRoom(targetId);
        if (!userList[currentUserId].isContact) {
            userList[currentUserId].isContact = true;
            client->addContact(targetId);
        }
    }

    // Add the message to the file and in-memory structure
    room->addMessage(msg);
    
    // Save messages to disk immediately
    room->saveMessages();
    qDebug() << "Saved message to disk for room:" << room->getRoomId();
    
    // Make sure the message is also added to the server's in-memory structure
    server *srv = server::getInstance();
    QVector<Message> roomMsgs = room->getMessages();
    srv->updateRoomMessages(room->getRoomId(), roomMsgs);
    
    // IMPORTANT: Add the room to the recipient's rooms in the server's in-memory structure
    // This ensures that when the recipient logs in, they'll see this conversation
    QString senderEmail = client->getUserId();
    QString roomId = room->getRoomId();
    
    // First, check if a Client object exists for the recipient
    if (!srv->hasClient(targetId)) {
        // The target user isn't logged in, so add the room to their data manually
        
        // 1. Make sure the contact list contains the sender
        srv->addContactForUser(targetId, senderEmail);
        
        // 2. Make sure the room exists in the recipient's rooms
        if (!srv->hasRoomForUser(targetId, roomId)) {
            // Create a copy of the room for the recipient
            Room *recipientRoom = new Room(room->getName());
            recipientRoom->setMessages(roomMsgs);
            srv->addRoomToUser(targetId, recipientRoom);
        }
    } else {
        // The target user is logged in, so update their Client object
        Client *targetClient = srv->getClient(targetId);
        if (targetClient) {
            // Add contact if needed
            if (!targetClient->hasContact(senderEmail)) {
                targetClient->addContact(senderEmail);
                qDebug() << "Added" << senderEmail << "as contact for logged in user" << targetId;
            }
            
            // Add room if needed
            if (!targetClient->getRoom(roomId)) {
                // Create a room with the same data
                Room *recipientRoom = new Room(room->getName());
                recipientRoom->setMessages(roomMsgs);
                targetClient->addRoom(recipientRoom);
                qDebug() << "Added room to logged in user" << targetId;
            }
        }
    }

    // Update the last message and UI
    userList[currentUserId].lastMessage = messageText;
    userList[currentUserId].lastSeen = "Just now";
    userList[currentUserId].hasMessages = true; // Mark user as having messages
    
    // If the users list needs reordering (if this wasn't already a pinned user)
    if (!userList[currentUserId].hasMessages) {
        // Reorder the user list to put this user at the top (since they now have messages)
        UserInfo userInfo = userList[currentUserId];
        userList.removeAt(currentUserId);
        userList.prepend(userInfo);
        
        // Messages index needs to be updated as well
        QVector<MessageInfo> messages = userMessages[currentUserId];
        userMessages.removeAt(currentUserId);
        userMessages.prepend(messages);
        
        // Update current user ID to the new position (0)
        currentUserId = 0;
    }
    
    updateUsersList();

    // Refresh the chat area to show the new message
    updateChatArea(currentUserId);

    // Clear the input field
    messageInput->clear();
    QScrollBar *vScrollBar = messageArea->verticalScrollBar();
    vScrollBar->setValue(vScrollBar->maximum());
}

void ChatPage::addMessageToUI(const QString &text, bool isFromMe,
                              const QDateTime &timestamp) {
    QWidget *bubbleRow = new QWidget();
    QHBoxLayout *rowLayout = new QHBoxLayout(bubbleRow);
    rowLayout->setContentsMargins(
        10, 3, 10, 3); // Reduced vertical spacing for a tighter look

    // Create message widget
    QLabel *messageText = new QLabel(text);
    messageText->setWordWrap(true);
    messageText->setTextFormat(Qt::TextFormat::PlainText);
    messageText->setStyleSheet(
        "color: #000000; font-size: 14px;"); // Black text for better contrast

    // Format timestamp
    QString timeStr = timestamp.toString("hh:mm");
    QLabel *timestampLabel = new QLabel(timeStr);
    timestampLabel->setStyleSheet(
        "font-size: 10px; color: #666666; margin: 0px 4px;");
    timestampLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // Create bubble content
    QWidget *bubbleContent = new QWidget();
    QHBoxLayout *contentLayout = new QHBoxLayout(bubbleContent);
    contentLayout->setContentsMargins(12, 8, 12, 8);
    contentLayout->setSpacing(5);

    contentLayout->addWidget(messageText);
    contentLayout->addWidget(timestampLabel);

    // Modernized bubble styling (Messenger/WhatsApp-like)
    QString bubbleColor =
        isFromMe ? "#DCF8C6"
                 : "#FFFFFF"; // Light green for sent, white for received
    QString borderRadiusStyle =
        isFromMe ? "border-top-right-radius: 18px; border-top-left-radius: "
                   "18px; border-bottom-left-radius: 18px; "
                   "border-bottom-right-radius: 4px;"
                 : "border-top-left-radius: 18px; border-top-right-radius: "
                   "18px; border-bottom-right-radius: 18px; "
                   "border-bottom-left-radius: 4px;";
    bubbleContent->setStyleSheet(QString(R"(
        background-color: %1;
        %2
        padding: 8px;
    )")
                                     .arg(bubbleColor)
                                     .arg(borderRadiusStyle));

    bubbleContent->setMaximumWidth(
        500); // Slightly wider for better readability
    bubbleContent->setProperty("isFromMe", isFromMe);
    bubbleContent->setProperty("messageText", text);
    bubbleContent->setProperty("timestamp", timestamp);
    bubbleContent->installEventFilter(this);

    // Position the bubble
    if (isFromMe) {
        rowLayout->addStretch();
        rowLayout->addWidget(bubbleContent, 0, Qt::AlignRight);
    } else {
        rowLayout->addWidget(bubbleContent, 0, Qt::AlignLeft);
        rowLayout->addStretch();
    }

    // Add to layout
    messageLayout->insertWidget(messageLayout->count() - 1, bubbleRow);
}

bool ChatPage::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QWidget *bubble = qobject_cast<QWidget *>(obj);
        if (bubble && bubble->property("isFromMe").toBool()) {
            showMessageOptions(bubble);
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void ChatPage::showMessageOptions(QWidget *bubble) {
    QMenu *menu = new QMenu(this);
    QAction *editAction = menu->addAction("Edit Message");
    QAction *deleteAction = menu->addAction("Delete Message");

    connect(editAction, &QAction::triggered, [this, bubble]() {
        QString currentText = bubble->property("messageText").toString();
        QString newText = QInputDialog::getText(
            this, "Edit Message", "Edit your message:", QLineEdit::Normal,
            currentText);
        if (!newText.isEmpty() && newText != currentText) {
            // Update in UI
            QLabel *messageLabel = bubble->findChild<QLabel *>();
            if (messageLabel) {
                messageLabel->setText(newText);
                bubble->setProperty("messageText", newText);

                // Update in data structure
                if (currentUserId >= 0) {
                    QDateTime timestamp =
                        bubble->property("timestamp").toDateTime();
                    for (int i = 0; i < userMessages[currentUserId].size();
                         i++) {
                        if (userMessages[currentUserId][i].timestamp ==
                            timestamp) {
                            userMessages[currentUserId][i].text = newText;
                            break;
                        }
                    }

                    // Update last message if needed
                    if (!userMessages[currentUserId].isEmpty() &&
                        userMessages[currentUserId].last().timestamp ==
                            timestamp) {
                        userList[currentUserId].lastMessage = newText;
                        updateUsersList();
                    }

                    // Get the room and update the message in the room
                    Client *client = server::getInstance()->getCurrentClient();
                    if (client) {
                        QString targetId = userList[currentUserId].email.isEmpty() ? 
                                          userList[currentUserId].name : userList[currentUserId].email;
                        Room *room = client->getRoomWithUser(targetId);
                        if (room) {
                            // Update the message in the room
                            QVector<Message> messages = room->getMessages();
                            for (int j = 0; j < messages.size(); j++) {
                                if (messages[j].getTimestamp() == timestamp) {
                                    // We can't modify the message directly, so remove and add a new one
                                    room->removeMessage(j);
                                    Message newMsg(newText, client->getUserId());
                                    room->addMessage(newMsg);
                                    break;
                                }
                            }
                            // Save changes to disk
                            room->saveMessages();
                            
                            // Update the server's in-memory message structure for both users
                            server *srv = server::getInstance();
                            QString roomId = room->getRoomId();
                            QVector<Message> updatedMsgs = room->getMessages();
                            
                            // Update messages in server
                            srv->updateRoomMessages(roomId, updatedMsgs);
                            
                            qDebug() << "Saved edited message to disk for room:" << room->getRoomId();
                        }
                    }
                }
            }
        }
    });

    connect(deleteAction, &QAction::triggered, [this, bubble]() {
        QDateTime timestamp = bubble->property("timestamp").toDateTime();

        // Remove from UI
        QWidget *bubbleRow = bubble->parentWidget();
        if (bubbleRow) {
            messageLayout->removeWidget(bubbleRow);
            delete bubbleRow;
        }

        // Remove from data structure
        if (currentUserId >= 0) {
            for (int i = 0; i < userMessages[currentUserId].size(); i++) {
                if (userMessages[currentUserId][i].timestamp == timestamp) {
                    userMessages[currentUserId].remove(i);
                    break;
                }
            }

            // Update last message if needed
            if (!userMessages[currentUserId].isEmpty()) {
                userList[currentUserId].lastMessage =
                    userMessages[currentUserId].last().text;
                updateUsersList();
            } else {
                userList[currentUserId].lastMessage = "";
                updateUsersList();
            }

            // Get the room and remove the message from the room
            Client *client = server::getInstance()->getCurrentClient();
            if (client) {
                QString targetId = userList[currentUserId].email.isEmpty() ? 
                                  userList[currentUserId].name : userList[currentUserId].email;
                Room *room = client->getRoomWithUser(targetId);
                if (room) {
                    // Find and remove the message
                    QVector<Message> messages = room->getMessages();
                    for (int j = 0; j < messages.size(); j++) {
                        if (messages[j].getTimestamp() == timestamp) {
                            room->removeMessage(j);
                            break;
                        }
                    }
                    // Save changes to disk
                    room->saveMessages();
                    
                    // Update the server's in-memory message structure for both users
                    server *srv = server::getInstance();
                    QString roomId = room->getRoomId();
                    QVector<Message> updatedMsgs = room->getMessages();
                    
                    // Update messages in server
                    srv->updateRoomMessages(roomId, updatedMsgs);
                    
                    qDebug() << "Saved changes after message deletion for room:" << room->getRoomId();
                }
            }
        }
    });

    menu->exec(QCursor::pos());
    delete menu;
}

QListWidgetItem *ChatPage::createUserListItem(const UserInfo &user, int index) {
    QWidget *itemWidget = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(itemWidget);
    layout->setContentsMargins(8, 8, 8, 8); // More padding
    layout->setSpacing(16); // More space between avatar and name

    // Use gradient colors based on whether the user has messages
    QString gradientColors = user.hasMessages ? 
        "stop:0 #6a82fb, stop:1 #fc5c7d" : // More vibrant for pinned users
        "stop:0 #808080, stop:1 #a0a0a0";  // Gray for regular users
    
<<<<<<< HEAD
=======
    // Create avatar with online indicator
    QWidget *avatarContainer = new QWidget();
    QVBoxLayout *avatarLayout = new QVBoxLayout(avatarContainer);
    avatarLayout->setContentsMargins(0, 0, 0, 0);
    avatarLayout->setSpacing(0);

>>>>>>> bfd0fc2 (handle the settings)
    QLabel *avatar = new QLabel(user.name.left(1).toUpper());
    avatar->setFixedSize(44, 44);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setStyleSheet(QString(R"(
        background: qlineargradient(x1:0, y1:0, x2:1, y2:1, %1);
        color: white;
        border-radius: 22px;
        font-weight: bold;
        font-size: 22px;
        border: 2px solid #23233a;
    )").arg(gradientColors));
<<<<<<< HEAD

    QWidget *textContainer = new QWidget;
    QVBoxLayout *textLayout = new QVBoxLayout(textContainer);
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(2);
=======
    
    avatarLayout->addWidget(avatar, 0, Qt::AlignCenter);
    
    // Add online status indicator
    QLabel *statusIndicator = new QLabel();
    statusIndicator->setFixedSize(12, 12);
    statusIndicator->setStyleSheet(QString(R"(
        background-color: %1;
        border-radius: 6px;
        border: 1px solid #23233a;
    )").arg(user.isOnline ? "#4CAF50" : "#9E9E9E")); // Green for online, gray for offline
    
    // Position status indicator at bottom right of avatar
    QHBoxLayout *indicatorLayout = new QHBoxLayout();
    indicatorLayout->addStretch();
    indicatorLayout->addWidget(statusIndicator);
    avatarLayout->addLayout(indicatorLayout);
>>>>>>> bfd0fc2 (handle the settings)

    QWidget *textContainer = new QWidget;
    QVBoxLayout *textLayout = new QVBoxLayout(textContainer);
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(2);

    // Create name label - removed bio
    QLabel *nameLabel = new QLabel(user.name);
    nameLabel->setStyleSheet("color: white; font-size: 17px; font-weight: 600;");
    
    textLayout->addWidget(nameLabel);
    
<<<<<<< HEAD
=======
    // Status label showing online/offline
    QLabel *statusLabel = new QLabel(user.status);
    statusLabel->setStyleSheet(QString("color: %1; font-size: 12px;")
                               .arg(user.isOnline ? "#4CAF50" : "#9E9E9E"));
    textLayout->addWidget(statusLabel);
    
>>>>>>> bfd0fc2 (handle the settings)
    // Add pin indicator for users with messages
    if (user.hasMessages) {
        QLabel *pinnedLabel = new QLabel("📌 Pinned chat");
        pinnedLabel->setStyleSheet("color: #a0a0a0; font-size: 12px;");
        textLayout->addWidget(pinnedLabel);
    } else if (!user.lastMessage.isEmpty()) {
        QLabel *lastMsgLabel = new QLabel(user.lastMessage);
        lastMsgLabel->setStyleSheet("color: #a0a0a0; font-size: 12px;");
        textLayout->addWidget(lastMsgLabel);
    }
    
<<<<<<< HEAD
    layout->addWidget(avatar);
=======
    layout->addWidget(avatarContainer);
>>>>>>> bfd0fc2 (handle the settings)
    layout->addWidget(textContainer, 1);

    QListWidgetItem *item = new QListWidgetItem;
    item->setSizeHint(itemWidget->sizeHint());
    item->setData(Qt::UserRole, index);
    
    // Store the widget pointer directly, don't use void* conversion
    item->setData(Qt::UserRole + 1, QVariant::fromValue(itemWidget));
    
    return item;
}

void ChatPage::loadMessagesForCurrentUser() {
    qDebug() << "Loading messages for user index:" << currentUserId;

    if (currentUserId < 0 || currentUserId >= userList.size())
        return;

    Client *client = server::getInstance()->getCurrentClient();
    if (!client)
        return;

    // Use email instead of username for consistent room lookup
    QString targetEmail = userList[currentUserId].email;
    QString targetName = userList[currentUserId].name;
    
    // If we have an email, use it. Otherwise fall back to name (for backward compatibility)
    QString targetId = targetEmail.isEmpty() ? targetName : targetEmail;
    qDebug() << "Loading messages with user ID:" << targetId;
    
    Room *room = client->getRoomWithUser(targetId);
    if (!room) {
        // Try with name as fallback
        if (!targetEmail.isEmpty()) {
            room = client->getRoomWithUser(targetName);
            qDebug() << "Room not found with email, trying with name:" << targetName;
        }
        
        if (!room) {
            qDebug() << "No room found for user:" << targetId;
            return;
        }
    }

    // Clear existing messages to prevent duplication
    userMessages[currentUserId].clear();

    // Load messages from the room
    room->loadMessages();
    const QVector<Message> &roomMessages = room->getMessages();

    // Add messages to userMessages, avoiding duplicates
    for (const Message &msg : roomMessages) {
        MessageInfo msgInfo;
        msgInfo.text = msg.getContent();
        msgInfo.isFromMe = (msg.getSender() == client->getUserId());
        msgInfo.timestamp = msg.getTimestamp();

        // Check for duplicates before adding
        bool isDuplicate = false;
        for (const MessageInfo &existingMsg : userMessages[currentUserId]) {
            if (existingMsg.text == msgInfo.text &&
                existingMsg.timestamp == msgInfo.timestamp &&
                existingMsg.isFromMe == msgInfo.isFromMe) {
                isDuplicate = true;
                break;
            }
        }
        if (!isDuplicate) {
            userMessages[currentUserId].append(msgInfo);
        }
    }

    qDebug() << "Loaded" << userMessages[currentUserId].size()
             << "messages for user" << targetId;
<<<<<<< HEAD
=======
}

void ChatPage::loadUserSettings() {
    Client *client = server::getInstance()->getCurrentClient();
    if (!client) {
        qDebug() << "Cannot load settings: No active client";
        return;
    }
    
    QString userId = client->getUserId();
    qDebug() << "Loading settings for user:" << userId;
    
    // Get settings from server
    QString nickname, bio;
    bool isOnline = false;
    
    // Check if server has settings
    bool hasSettings = server::getInstance()->getUserSettings(userId, nickname, bio);
    isOnline = server::getInstance()->isUserOnline(userId);
    
    // Store settings in memory
    userSettings.nickname = nickname.isEmpty() ? client->getUsername() : nickname;
    userSettings.bio = bio;
    userSettings.isOnline = isOnline;
    
    // Update UI elements with loaded settings
    if (nicknameEdit) {
        nicknameEdit->setText(userSettings.nickname);
    }
    
    if (bioEdit) {
        bioEdit->setText(userSettings.bio);
    }
    
    if (onlineStatusCheckbox) {
        onlineStatusCheckbox->blockSignals(true);
        onlineStatusCheckbox->setChecked(userSettings.isOnline);
        onlineStatusCheckbox->blockSignals(false);
    }
    
    qDebug() << "Settings loaded - Nickname:" << userSettings.nickname 
             << "Bio:" << userSettings.bio
             << "Online:" << userSettings.isOnline;
}

void ChatPage::saveUserSettings() {
    Client *client = server::getInstance()->getCurrentClient();
    if (!client) {
        qDebug() << "Cannot save settings: No active client";
        return;
    }
    
    // Get settings from UI
    QString newNickname = nicknameEdit->text();
    QString newBio = bioEdit->toPlainText();
    bool newOnlineStatus = onlineStatusCheckbox->isChecked();
    
    // Check if anything has changed
    bool hasChanges = (newNickname != userSettings.nickname) || 
                     (newBio != userSettings.bio) || 
                     (newOnlineStatus != userSettings.isOnline);
    
    if (!hasChanges) {
        QMessageBox::information(this, "No Changes", 
            "No changes were made to your settings.");
        return;
    }
    
    // Update local settings
    userSettings.nickname = newNickname;
    userSettings.bio = newBio;
    userSettings.isOnline = newOnlineStatus;
    
    QString userId = client->getUserId();
    qDebug() << "Saving settings for user:" << userId;
    
    // Save settings to server
    bool nicknameBioSuccess = server::getInstance()->updateUserSettings(userId, userSettings.nickname, userSettings.bio);
    bool onlineSuccess = server::getInstance()->setUserOnlineStatus(userId, userSettings.isOnline);
    
    if (nicknameBioSuccess && onlineSuccess) {
        qDebug() << "Settings saved - Nickname:" << userSettings.nickname 
                << "Bio:" << userSettings.bio
                << "Online:" << userSettings.isOnline;
        
        // If bio or nickname changed, force UI refresh
        if (newNickname != userSettings.nickname || newBio != userSettings.bio) {
            // Force refresh the UI to show updated bio in chat headers
            refreshOnlineStatus();
            qDebug() << "Refreshed UI to show updated bio/nickname";
        }
        
        // Show success message to user
        QMessageBox::information(this, "Settings Saved", 
            "Your settings have been saved successfully.");
            
        // Refresh UI
        refreshOnlineStatus();
    } else {
        QMessageBox::warning(this, "Error", 
            "There was a problem saving your settings. Please try again.");
    }
}

void ChatPage::deleteUserAccount() {
    // Ask for confirmation
    QMessageBox::StandardButton confirm = QMessageBox::question(
        this, 
        "Delete Account", 
        "Are you sure you want to delete your account? This action cannot be undone.",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (confirm == QMessageBox::Yes) {
        Client *client = server::getInstance()->getCurrentClient();
        if (!client) {
            QMessageBox::warning(this, "Error", "Cannot delete account: No active user");
            return;
        }
        
        QString userId = client->getUserId();
        qDebug() << "Deleting account for user:" << userId;
        
        // Delete user from server
        bool success = server::getInstance()->deleteUser(userId);
        
        if (success) {
            // Delete user file
            QString userFile = "../db/users/" + userId + ".txt";
            QFile::remove(userFile);
            
            QMessageBox::information(this, "Account Deleted", 
                "Your account has been deleted successfully.");
            
            // Logout and return to login screen
            logout();
        } else {
            QMessageBox::warning(this, "Error", 
                "Failed to delete account. Please try again later.");
        }
    }
}

void ChatPage::onlineStatusChanged(int state) {
    bool isOnline = (state == Qt::Checked);
    
    Client *client = server::getInstance()->getCurrentClient();
    if (!client) {
        qDebug() << "Cannot update online status: No active client";
        return;
    }
    
    QString userId = client->getUserId();
    userSettings.isOnline = isOnline;
    
    // Update the server with the new status immediately
    bool success = server::getInstance()->setUserOnlineStatus(userId, isOnline);
    
    if (success) {
        qDebug() << "Online status changed to:" << (isOnline ? "Online" : "Offline") << "for user:" << userId;
        
        // Force update of UserInfo in userList
        for (int i = 0; i < userList.size(); ++i) {
            // If this is the current user, update the isOnline property
            if (userList[i].email == userId) {
                userList[i].isOnline = isOnline;
                userList[i].status = isOnline ? "Online" : "Offline";
                break;
            }
        }
        
        // Update UI to reflect the change
        if (onlineStatusCheckbox) {
            onlineStatusCheckbox->setChecked(isOnline);
        }
        
        // Force refresh the user list to show updated status
        updateUsersList();
        
        // Show visual confirmation
        QMessageBox::information(this, "Status Updated", 
            QString("Your status is now %1").arg(isOnline ? "Online" : "Offline"));
    } else {
        qDebug() << "Failed to update online status";
        
        // Restore the checkbox to previous state if update failed
        if (onlineStatusCheckbox) {
            onlineStatusCheckbox->setChecked(!isOnline);
        }
        
        QMessageBox::warning(this, "Error", "Failed to update online status. Please try again.");
    }
    
    // Manually trigger a refresh of the UI
    refreshOnlineStatus();
}

void ChatPage::updateUserStatus(const QString &userId, bool isOnline) {
    // Update the status in the user list
    for (int i = 0; i < userList.size(); ++i) {
        if (userList[i].email == userId) {
            userList[i].status = isOnline ? "Online" : "Offline";
            userList[i].isOnline = isOnline;
            
            // If this is the current user being displayed, update the header
            if (i == currentUserId) {
                // Get user's bio from server if available
                QString bio = "";
                server *srv = server::getInstance();
                QString nickname, fetchedBio;
                if (srv && srv->getUserSettings(userId, nickname, fetchedBio)) {
                    bio = fetchedBio;
                }
                
                // Create display text with status and bio
                QString headerText = userList[i].name + " - " + userList[i].status;
                if (!bio.isEmpty()) {
                    headerText += " (" + bio + ")";
                }
                
                chatHeader->setText(headerText);
            }
            
            // Update the user in the list widget
            for (int j = 0; j < usersListWidget->count(); ++j) {
                QListWidgetItem *item = usersListWidget->item(j);
                int index = item->data(Qt::UserRole).toInt();
                if (index == i) {
                    // Refresh this item
                    QListWidgetItem *newItem = createUserListItem(userList[i], i);
                    usersListWidget->takeItem(j);
                    usersListWidget->insertItem(j, newItem);
                    break;
                }
            }
            
            break;
        }
    }
}

void ChatPage::refreshOnlineStatus() {
    // This method is called periodically by the timer to refresh online status of users
    server *srv = server::getInstance();
    if (!srv || !srv->getCurrentClient()) {
        return;
    }
    
    bool needsUiUpdate = false;
    
    // Check online status and bio for all users in the list
    for (int i = 0; i < userList.size(); ++i) {
        // Check online status changes
        bool isOnline = srv->isUserOnline(userList[i].email);
        bool onlineChanged = (userList[i].isOnline != isOnline);
        
        // Get user bio to check for changes
        QString nickname, bio;
        srv->getUserSettings(userList[i].email, nickname, bio);
        
        // Update user info if changes detected
        if (onlineChanged) {
            userList[i].isOnline = isOnline;
            userList[i].status = isOnline ? "Online" : "Offline";
            
            // If this is the current user being displayed, update the header
            if (i == currentUserId) {
                // Create display text with status and bio
                QString headerText = userList[i].name + " - " + userList[i].status;
                if (!bio.isEmpty()) {
                    headerText += " (" + bio + ")";
                }
                
                chatHeader->setText(headerText);
            }
            
            qDebug() << "Updated online status for" << userList[i].name 
                    << (isOnline ? "Online" : "Offline");
                    
            needsUiUpdate = true;
        }
    }
    
    // Only update the UI if there were changes
    if (needsUiUpdate) {
        updateUsersList();
    }
    
    // Check current user's online status and update checkbox if needed
    Client *client = srv->getCurrentClient();
    if (client) {
        QString userId = client->getUserId();
        bool isOnline = srv->isUserOnline(userId);
        
        if (onlineStatusCheckbox && onlineStatusCheckbox->isChecked() != isOnline) {
            // Update checkbox without triggering signal
            onlineStatusCheckbox->blockSignals(true);
            onlineStatusCheckbox->setChecked(isOnline);
            onlineStatusCheckbox->blockSignals(false);
        }
    }
>>>>>>> bfd0fc2 (handle the settings)
}
