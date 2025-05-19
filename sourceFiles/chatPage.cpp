#include "../headers/chatPage.h"
#include "../server/server.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPixmap>
#include <QPointer>
#include <QPushButton>
#include <QRandomGenerator>
#include <QScrollArea>
#include <QScrollBar>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QTextEdit>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>

ChatPage::ChatPage(QWidget *parent)
    : QWidget(parent), currentUserId(-1), isSearching(false),
      currentGroupId(-1), isInGroupChat(false) {
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    // hossam

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

    // Instead of forcing offline, respect the user's current status
    Client *client = server::getInstance()->getCurrentClient();
    if (client) {
        QString userId = client->getUserId();

        // Get current status from server
        bool isCurrentlyOnline = server::getInstance()->isUserOnline(userId);

        // Update checkbox to reflect current status
        if (onlineStatusCheckbox) {
            onlineStatusCheckbox->blockSignals(true);
            onlineStatusCheckbox->setChecked(isCurrentlyOnline);
            onlineStatusCheckbox->blockSignals(false);
            userSettings.isOnline = isCurrentlyOnline;
            qDebug() << "Preserved existing status for user:" << userId
                     << (isCurrentlyOnline ? "online" : "offline");
        }
    }

    // Load users from database
    loadUsersFromDatabase();
    
    // Also load groups from database
    loadGroupsFromDatabase();

    // Show contacts by default and try to select the first contact
    updateUsersList();
    
    // First try to select a user contact
    bool userSelected = false;
    if (!userList.isEmpty()) {
        for (int i = 0; i < userList.size(); ++i) {
            if (userList[i].isContact || userList[i].hasMessages) {
                usersListWidget->setCurrentRow(i);
                currentUserId = i;
                updateChatArea(i);
                userSelected = true;
                break;
            }
        }
    }
    
    // If no user contacts, but we have groups, go to groups tab and select the first group
    if (!userSelected && !groupList.isEmpty()) {
        qDebug() << "No user contacts found, but there are " << groupList.size() << " groups. Showing groups instead.";
        
        // Switch to groups tab
        for (int j = 0; j < navButtons.size(); j++) {
            navButtons[j]->setStyleSheet(
                QString(R"(
                background-color: %1;
                border-radius: 12px;
                color: white;
                font-size: 18px;
            )")
                    .arg(j == 1 ? "#4d6eaa" : "#2e2e3e")); // Highlight Groups button
        }
        
        // Update UI to show groups
        isInGroupChat = true;
        updateGroupsList();
        
        // Select the first group
        if (usersListWidget && usersListWidget->count() > 0) {
            usersListWidget->setCurrentRow(0);
            currentGroupId = 0;
            updateGroupChatArea(0);
        }
    }

    // Set up timer to refresh online status more frequently (every 3 seconds)
    onlineStatusTimer = new QTimer(this);
    connect(onlineStatusTimer, &QTimer::timeout, this,
            &ChatPage::refreshOnlineStatus);
    onlineStatusTimer->start(3000); // 3 seconds instead of 10

    // Make sure profile avatar is properly initialized
    QTimer::singleShot(500, this, &ChatPage::updateProfileAvatar);
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

    // Create profile avatar with online indicator (similar to other user
    // avatars)
    QWidget *profileAvatarContainer = new QWidget();
    profileAvatarContainer->setFixedSize(60, 60);
    profileAvatarContainer->setObjectName(
        "profileAvatarContainer"); // Set object name for event filtering
    QVBoxLayout *profileAvatarLayout = new QVBoxLayout(profileAvatarContainer);
    profileAvatarLayout->setContentsMargins(0, 0, 0, 0);
    profileAvatarLayout->setSpacing(0);

    // Get current user info
    Client *client = server::getInstance()->getCurrentClient();
    QString displayName = "ME";
    bool isOnline = false;

    if (client) {
        QString userId = client->getUserId();
        QString nickname, bio;
        if (server::getInstance()->getUserSettings(userId, nickname, bio)) {
            displayName = nickname.isEmpty() ? client->getUsername() : nickname;
        } else {
            displayName = client->getUsername();
        }
        isOnline = server::getInstance()->isUserOnline(userId);
    }

    // Create the avatar label with first letter of username
    profileAvatar = new QLabel(displayName.left(1).toUpper());
    profileAvatar->setFixedSize(50, 50);
    profileAvatar->setAlignment(Qt::AlignCenter);

    // Use gradient colors like other avatars but with a unique color for the
    // current user
    QString gradientColors =
        "stop:0 #4d6eaa, stop:1 #7a5ca3"; // Blue to purple gradient for current
                                          // user

    profileAvatar->setStyleSheet(QString(R"(
        background: qlineargradient(x1:0, y1:0, x2:1, y2:1, %1);
        color: white;
        border-radius: 25px;
        font-weight: bold;
        font-size: 22px;
        border: 2px solid #23233a;
    )")
                                     .arg(gradientColors));

    profileAvatarLayout->addWidget(profileAvatar, 0, Qt::AlignCenter);

    // Add online status indicator
    profileStatusIndicator = new QLabel();
    profileStatusIndicator->setFixedSize(12, 12);
    profileStatusIndicator->setStyleSheet(
        QString(R"(
        background-color: %1;
        border-radius: 6px;
        border: 1px solid #23233a;
    )")
            .arg(isOnline ? "#4CAF50"
                          : "#9E9E9E")); // Green for online, gray for offline

    // Position status indicator at bottom right of avatar
    QHBoxLayout *profileIndicatorLayout = new QHBoxLayout();
    profileIndicatorLayout->addStretch();
    profileIndicatorLayout->addWidget(profileStatusIndicator);
    profileAvatarLayout->addLayout(profileIndicatorLayout);

    // Make the avatar clickable to show settings
    profileAvatarContainer->setCursor(Qt::PointingHandCursor);
    profileAvatarContainer->installEventFilter(this);

    navLayout->addWidget(profileAvatarContainer, 0, Qt::AlignCenter);

    // Make sure the avatar shows the right letter and status immediately
    QTimer::singleShot(100, this, &ChatPage::updateProfileAvatar);

    // Navigation buttons
    QStringList navItems = {"Chat", "Stories", "Calls", "Blocked", "Settings"};
    QStringList navIcons = {"💬", "📷", "🎧", "⚠️", "⚙️"};

    for (int i = 0; i < navItems.size(); i++) {
        QPushButton *navBtn = new QPushButton(navIcons[i]);
        navBtn->setToolTip(navItems[i]);
        navBtn->setFixedSize(50, 50);
        navBtn->setStyleSheet(
            QString(R"(
            background-color: %1;
            border-radius: 12px;
            color: white;
            font-size: 30px;
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
                    font-size: 30px;
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

                // If showing settings, refresh the settings UI and ensure
                // offline status
                if (i == 4) { // Settings index is now 4 because we added
                              // Blocked at index 3
                    loadUserSettings(); // Refresh settings when tab is selected

                    // Refresh the UI but don't change the status
                    if (onlineStatusCheckbox) {
                        Client *client =
                            server::getInstance()->getCurrentClient();
                        if (client) {
                            QString userId = client->getUserId();

                            // Get current status from server
                            bool isCurrentlyOnline =
                                server::getInstance()->isUserOnline(userId);

                            // Update checkbox to reflect current status
                            onlineStatusCheckbox->blockSignals(true);
                            onlineStatusCheckbox->setChecked(isCurrentlyOnline);
                            onlineStatusCheckbox->blockSignals(false);

                            // Update settings
                            userSettings.isOnline = isCurrentlyOnline;

                            qDebug()
                                << "Settings tab loaded - current status:"
                                << (isCurrentlyOnline ? "online" : "offline");

                            // Force refresh the user list to update online
                            // status
                            refreshOnlineStatus();
                        }
                    }
                }

                // If showing blocked users, load the latest blocked users list
                if (i == 3) { // Blocked index is 3
                    loadBlockedUsersPane();
                }
            }
        });
    }

    navLayout->addStretch();

    // Add logout button at the bottom
    QPushButton *logoutBtn = new QPushButton("✖️");
    logoutBtn->setToolTip("Logout");
    logoutBtn->setFixedSize(50, 50);
    logoutBtn->setStyleSheet(R"(
        background-color: #E15554;
        border-radius: 12px;
        color: white;
        font-size: 30px;
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

    // Search bar with refresh button in a horizontal layout
    QWidget *searchWidget = new QWidget;
    QHBoxLayout *searchLayout = new QHBoxLayout(searchWidget);
    searchLayout->setContentsMargins(0, 0, 0, 0);
    searchLayout->setSpacing(5);

    searchInput = new QLineEdit;
    searchInput->setPlaceholderText("Search users...");
    searchInput->setStyleSheet(R"(
        background-color: #23233a;
        color: white;
        border-radius: 10px;
        padding: 8px;
        border: none;
    )");

    // Add refresh button
    QPushButton *refreshButton = new QPushButton("🔄");
    refreshButton->setToolTip("Refresh Online Status");
    refreshButton->setFixedSize(30, 30);
    refreshButton->setStyleSheet(R"(
        background-color: #4e4e9e;
        color: white;
        border-radius: 15px;
        padding: 5px;
        border: none;
    )");

    connect(refreshButton, &QPushButton::clicked, this,
            &ChatPage::forceRefreshOnlineStatus);

    searchLayout->addWidget(searchInput);
    searchLayout->addWidget(refreshButton);

    usersLayout->addWidget(searchWidget);

    // Add "Create Group" button
    QPushButton *createGroupButton = new QPushButton("+ Create Group");
    createGroupButton->setStyleSheet(R"(
        background-color: #4d6eaa;
        color: white;
        border-radius: 10px;
        padding: 8px;
        border: none;
        font-weight: bold;
        margin-top: 5px;
        margin-bottom: 10px;
    )");
    connect(createGroupButton, &QPushButton::clicked, this,
            &ChatPage::showCreateGroupDialog);
    usersLayout->addWidget(createGroupButton);

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
    storiesLayout->setContentsMargins(20, 20, 20, 20);
    storiesLayout->setSpacing(15);

    // Stories Header
    QLabel *storiesHeader = new QLabel("Stories");
    storiesHeader->setStyleSheet(
        "color: white; font-size: 24px; font-weight: bold;");
    storiesLayout->addWidget(storiesHeader);

    // Create a horizontal layout for story circles
    QWidget *storiesContainer = new QWidget;
    QHBoxLayout *storiesCirclesLayout = new QHBoxLayout(storiesContainer);
    storiesCirclesLayout->setContentsMargins(0, 10, 0, 10);
    storiesCirclesLayout->setSpacing(15);
    storiesCirclesLayout->setAlignment(Qt::AlignLeft);

    // "Add Story" circle
    QWidget *addStoryWidget = new QWidget;
    addStoryWidget->setFixedSize(80, 100);
    addStoryWidget->setCursor(Qt::PointingHandCursor);
    QVBoxLayout *addStoryLayout = new QVBoxLayout(addStoryWidget);
    addStoryLayout->setAlignment(Qt::AlignCenter);

    QLabel *addStoryCircle = new QLabel("+");
    addStoryCircle->setFixedSize(70, 70);
    addStoryCircle->setAlignment(Qt::AlignCenter);
    addStoryCircle->setStyleSheet(R"(
        background-color: #4e4e9e;
        border-radius: 35px;
        color: white;
        font-size: 32px;
        font-weight: bold;
    )");

    QLabel *addStoryText = new QLabel("Add Story");
    addStoryText->setStyleSheet("color: white; font-size: 12px;");
    addStoryText->setAlignment(Qt::AlignCenter);

    addStoryLayout->addWidget(addStoryCircle, 0, Qt::AlignCenter);
    addStoryLayout->addWidget(addStoryText, 0, Qt::AlignCenter);

    // Make the add story widget clickable
    addStoryWidget->installEventFilter(this);
    addStoryWidget->setObjectName("addStoryWidget");

    storiesCirclesLayout->addWidget(addStoryWidget);

    // Add placeholder for other users' stories (will be populated dynamically)
    storyWidgetsLayout = storiesCirclesLayout;
    storiesLayout->addWidget(storiesContainer);

    // Stories feed (vertical scrollable area)
    QScrollArea *storiesFeed = new QScrollArea;
    storiesFeed->setWidgetResizable(true);
    storiesFeed->setStyleSheet("background-color: #12121a; border: none;");

    QWidget *storiesFeedContent = new QWidget;
    storiesFeedContent->setStyleSheet("background-color: #12121a;");
    storiesFeedLayout = new QVBoxLayout(storiesFeedContent);
    storiesFeedLayout->setAlignment(Qt::AlignTop);
    storiesFeedLayout->setSpacing(15);
    storiesFeedLayout->setContentsMargins(0, 10, 0, 10);

    // Add a message when no stories are available
    QLabel *noStoriesLabel = new QLabel("No stories available. Add one now!");
    noStoriesLabel->setStyleSheet(
        "color: #6e6e8e; font-size: 16px; padding: 20px;");
    noStoriesLabel->setAlignment(Qt::AlignCenter);
    storiesFeedLayout->addWidget(noStoriesLabel);
    noStoriesLabel->setObjectName("noStoriesLabel");

    storiesFeed->setWidget(storiesFeedContent);
    storiesLayout->addWidget(storiesFeed);

    contentStack->addWidget(storiesView);

    // 3. Calls View
    QWidget *callsView = new QWidget;
    QVBoxLayout *callsLayout = new QVBoxLayout(callsView);
    QLabel *callsLabel = new QLabel("Calls Feature Coming Soon");
    callsLabel->setAlignment(Qt::AlignCenter);
    callsLabel->setStyleSheet("color: white; font-size: 24px;");
    callsLayout->addWidget(callsLabel);
    contentStack->addWidget(callsView);

    // 4. Blocked Users View
    QWidget *blockedView = new QWidget;
    QVBoxLayout *blockedLayout = new QVBoxLayout(blockedView);
    blockedLayout->setContentsMargins(20, 20, 20, 20);
    blockedLayout->setSpacing(15);

    // Blocked Users Header
    QLabel *blockedUsersHeader = new QLabel("Blocked Users");
    blockedUsersHeader->setStyleSheet(
        "color: white; font-size: 24px; font-weight: bold;");
    blockedLayout->addWidget(blockedUsersHeader);

    // Create a scrollable area for blocked users list
    QScrollArea *blockedUsersScrollArea = new QScrollArea();
    blockedUsersScrollArea->setWidgetResizable(true);
    blockedUsersScrollArea->setStyleSheet(
        "background-color: #1a1a2a; border-radius: 8px; border: none;");

    // Container widget for the blocked users list
    QWidget *blockedUsersContainer = new QWidget();
    blockedUsersLayout = new QVBoxLayout(blockedUsersContainer);
    blockedUsersLayout->setSpacing(8);
    blockedUsersLayout->setContentsMargins(15, 15, 15, 15);
    blockedUsersLayout->setAlignment(Qt::AlignTop);

    // Add a "No blocked users" label as a placeholder
    QLabel *noBlockedUsersLabel =
        new QLabel("You haven't blocked any users yet.");
    noBlockedUsersLabel->setStyleSheet(
        "color: #6e6e8e; font-size: 16px; padding: 20px;");
    noBlockedUsersLabel->setAlignment(Qt::AlignCenter);
    blockedUsersLayout->addWidget(noBlockedUsersLabel);
    noBlockedUsersLabel->setObjectName("noBlockedUsersLabel");

    blockedUsersScrollArea->setWidget(blockedUsersContainer);
    blockedLayout->addWidget(blockedUsersScrollArea);

    contentStack->addWidget(blockedView);

    // 5. Settings View
    QWidget *settingsView = new QWidget;
    QVBoxLayout *settingsLayout = new QVBoxLayout(settingsView);
    settingsLayout->setContentsMargins(20, 20, 20, 20);
    settingsLayout->setSpacing(15);

    // Settings Header
    QLabel *settingsHeader = new QLabel("User Settings");
    settingsHeader->setStyleSheet("color: white; font-size: 28px; font-weight: "
                                  "bold; margin-bottom: 20px;");
    settingsLayout->addWidget(settingsHeader);

    // Avatar section
    QLabel *avatarLabel = new QLabel("Profile Picture:");
    avatarLabel->setStyleSheet("color: white; font-size: 16px;");
    settingsLayout->addWidget(avatarLabel);

    // Avatar preview and change button in horizontal layout
    QWidget *avatarWidget = new QWidget();
    QHBoxLayout *avatarLayout = new QHBoxLayout(avatarWidget);

    // Create a circular preview label for the avatar
    avatarPreview = new QLabel();
    avatarPreview->setFixedSize(100, 100);
    avatarPreview->setAlignment(Qt::AlignCenter);
    avatarPreview->setStyleSheet("background-color: #2e2e3e; border-radius: "
                                 "50px; border: 2px solid #23233a;");

    // Button to change avatar
    QPushButton *changeAvatarButton = new QPushButton("Change Profile Picture");
    changeAvatarButton->setStyleSheet(R"(
        background-color: #4e4e9e;
        color: white;
        border-radius: 8px;
        padding: 12px;
        border: none;
        font-size: 15px;
    )");
    changeAvatarButton->setFixedWidth(200);

    // Connect button to avatar change function
    connect(changeAvatarButton, &QPushButton::clicked, this,
            &ChatPage::changeAvatar);

    avatarLayout->addWidget(avatarPreview);
    avatarLayout->addWidget(changeAvatarButton);
    avatarLayout->addStretch();

    settingsLayout->addWidget(avatarWidget);

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

    // Password Change Section
    QLabel *passwordLabel = new QLabel("Change Password:");
    passwordLabel->setStyleSheet("color: white; font-size: 16px;");

    currentPasswordEdit = new QLineEdit();
    currentPasswordEdit->setEchoMode(QLineEdit::Password);
    currentPasswordEdit->setPlaceholderText("Current Password");
    currentPasswordEdit->setStyleSheet(R"(
        background-color: #2e2e3e;
        color: white;
        border-radius: 8px;
        padding: 10px;
        border: none;
        font-size: 15px;
    )");
    currentPasswordEdit->setFixedHeight(40);

    newPasswordEdit = new QLineEdit();
    newPasswordEdit->setEchoMode(QLineEdit::Password);
    newPasswordEdit->setPlaceholderText("New Password");
    newPasswordEdit->setStyleSheet(R"(
        background-color: #2e2e3e;
        color: white;
        border-radius: 8px;
        padding: 10px;
        border: none;
        font-size: 15px;
    )");
    newPasswordEdit->setFixedHeight(40);

    confirmPasswordEdit = new QLineEdit();
    confirmPasswordEdit->setEchoMode(QLineEdit::Password);
    confirmPasswordEdit->setPlaceholderText("Confirm New Password");
    confirmPasswordEdit->setStyleSheet(R"(
        background-color: #2e2e3e;
        color: white;
        border-radius: 8px;
        padding: 10px;
        border: none;
        font-size: 15px;
    )");
    confirmPasswordEdit->setFixedHeight(40);

    // Online Status (WiFi) Toggle
    QLabel *onlineLabel = new QLabel("WiFi (Online Status):");
    onlineLabel->setStyleSheet("color: white; font-size: 16px;");
    onlineStatusCheckbox = new QCheckBox("Show me as online to others");
    onlineStatusCheckbox->setStyleSheet(R"(
        color: white;
        font-size: 15px;
    )");

    // Connect online status toggle
    connect(onlineStatusCheckbox, &QCheckBox::stateChanged, this,
            &ChatPage::onlineStatusChanged);

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
    settingsLayout->addWidget(passwordLabel);
    settingsLayout->addWidget(currentPasswordEdit);
    settingsLayout->addWidget(newPasswordEdit);
    settingsLayout->addWidget(confirmPasswordEdit);
    settingsLayout->addWidget(onlineLabel);
    settingsLayout->addWidget(onlineStatusCheckbox);

    // Add some spacing before the buttons
    settingsLayout->addSpacing(20);

    // Add buttons in a horizontal layout
    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(confirmButton);
    settingsLayout->addLayout(buttonsLayout);

    // Add a stretch at the end to push everything to the top
    settingsLayout->addStretch();

    // Connect signals to slots
    connect(confirmButton, &QPushButton::clicked, this,
            &ChatPage::saveUserSettings);

    contentStack->addWidget(settingsView);

    mainLayout->addWidget(contentStack);
    mainLayout->setStretch(2, 1); // Make chat area take remaining space
}

void ChatPage::logout() {
    // Get the current client before logging out
    Client *client = server::getInstance()->getCurrentClient();
    QString userId = client ? client->getUserId() : "";
    bool wasOnline = false;

    if (client) {
        // Save the current status before logout
        wasOnline = server::getInstance()->isUserOnline(userId);
        qDebug() << "Before logout, user" << userId << "was"
                 << (wasOnline ? "online" : "offline");
    }

    // Stop the online status timer
    if (onlineStatusTimer) {
        onlineStatusTimer->stop();
    }

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

    // If the user was online, put them back online
    if (!userId.isEmpty() && wasOnline) {
        // Restore the online status after logout
        server::getInstance()->setUserOnlineStatus(userId, true);
        qDebug() << "User" << userId
                 << "status restored to online after logout";
    }

    qDebug() << "Logging out and clearing UI";

    // Navigate back to login page (index 0)
    QStackedWidget *mainStack = qobject_cast<QStackedWidget *>(parent());
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

    // Also load groups
    loadGroupsFromDatabase();

    // Reset UI to default state
    contentStack->setCurrentIndex(0); // Show the chat view
    usersSidebar->show();             // Ensure user sidebar is visible

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
    messageInput->clear();

    // Get all users from the server
    QVector<QPair<QString, QString>> allUsers =
        server::getInstance()->getAllUsers();
    qDebug() << "Retrieved users from server:" << allUsers.size();

    if (allUsers.isEmpty()) {
        qDebug() << "WARNING: No users returned from getAllUsers()";
    }

    // Get all rooms to identify users with messages
    QVector<Room *> allRooms = currentClient->getAllRooms();
    QSet<QString> usersWithMessages;

    // Identify users who have conversations
    for (Room *room : allRooms) {
        QStringList participants = room->getName().split('_');
        for (const QString &participant : participants) {
            if (participant != currentClient->getUserId() &&
                !participant.isEmpty()) {
                // Check if this room has any messages
                if (!room->getMessagesAsVector().isEmpty()) {
                    usersWithMessages.insert(participant);
                    qDebug() << "User has messages:" << participant;
                }
            }
        }
    }

    QVector<UserInfo>
        usersWithMessagesVec; // To store users with messages (will be pinned)
    QVector<UserInfo> regularUsers; // To store users without messages

    // Get server instance for online status
    server *srv = server::getInstance();

    // Get the list of blocked users
    QVector<QString> blockedUsers =
        srv->getBlockedUsers(currentClient->getUserId());
    QSet<QString> blockedUsersSet;
    for (const QString &blocked : blockedUsers) {
        blockedUsersSet.insert(blocked);
    }

    // Add all users to the list except current user and blocked users
    for (const auto &user : allUsers) {
        QString email = user.first;     // This is the email (userId)
        QString nickname = user.second; // This is the nickname, not username

        // Skip current user and blocked users
        if (email == currentClient->getUserId() ||
            blockedUsersSet.contains(email)) {
            qDebug() << "Skipping user:" << email
                     << (blockedUsersSet.contains(email) ? "(blocked)"
                                                         : "(self)");
            continue;
        }

        // Debug the user we're adding
        qDebug() << "Processing user - Email:" << email << "Nickname:" << nickname;

        // Create a user info entry with properly populated fields
        UserInfo userInfo;
        userInfo.name = nickname;  // Display name is the nickname
        userInfo.email = email;    // Email is the consistent identifier
        userInfo.isOnline = srv->isUserOnline(email);
        userInfo.status = userInfo.isOnline ? "Online" : "Offline";
        userInfo.lastMessage = "";
        userInfo.lastSeen = "";
        userInfo.isContact = currentClient->hasContact(email);
        
        // Check if we have messages with this user
        userInfo.hasMessages = false;
        for (Room *room : allRooms) {
            QString roomId = room->getRoomId();
            if (roomId.contains(email) && !room->getMessagesAsVector().isEmpty()) {
                userInfo.hasMessages = true;
                usersWithMessages.insert(email);
                qDebug() << "User has messages - Room ID:" << roomId;
                break;
            }
        }

        // Add to the appropriate list
        if (userInfo.hasMessages) {
            usersWithMessagesVec.append(userInfo);
            qDebug() << "Added user with messages:" << nickname << "(" << email << ")";
        } else {
            regularUsers.append(userInfo);
            qDebug() << "Added regular user:" << nickname << "(" << email << ")";
        }
    }

    // Combine the lists - users with messages first, then regular users
    userList = usersWithMessagesVec + regularUsers;
    if (currentUserId < 0 || currentUserId >= userList.size())
        chatHeader->setText("Select a user");
    else
        updateChatArea(currentUserId);

    // Update profile avatar with current user's information
    updateProfileAvatar();

    qDebug() << "Total users loaded:" << userList.size()
             << "(Pinned: " << usersWithMessagesVec.size()
             << ", Regular: " << regularUsers.size() << ")";

    // Ensure userMessages is initialized for every user
    userMessages.resize(userList.size());

    // IMPORTANT: Clear the list widget BEFORE adding new items
    usersListWidget->clear();

    // Update the UI with the loaded users - this populates the user list widget
    updateUsersList();

    // Debug: Check if the list widget has items now
    qDebug() << "usersListWidget now has" << usersListWidget->count()
             << "items";

    // Start the status timer if it's not already running
    startStatusUpdateTimer();
}

void ChatPage::updateUsersList() {
    qDebug() << "Updating users list with " << userList.size() << " users and "
             << groupList.size() << " groups";

    // Block signals during update
    usersListWidget->blockSignals(true);

    // Clear the list first
    usersListWidget->clear();

    // Get the current client and server instance
    Client *currentClient = server::getInstance()->getCurrentClient();
    server *srv = server::getInstance();

    if (!currentClient || !srv) {
        usersListWidget->blockSignals(false);
        return;
    }

    // Get the list of blocked users for filtering
    QVector<QString> blockedUsers =
        srv->getBlockedUsers(currentClient->getUserId());
    QSet<QString> blockedUsersSet;
    for (const QString &blocked : blockedUsers) {
        blockedUsersSet.insert(blocked);
    }

    // Ensure all users have current status information and filter out blocked
    // users
    QVector<UserInfo> filteredUsers;
    for (int i = 0; i < userList.size(); ++i) {
        // Skip blocked users
        if (blockedUsersSet.contains(userList[i].email)) {
            continue;
        }

        // Update online status for each user
        userList[i].isOnline = srv->isUserOnline(userList[i].email);
        userList[i].status = userList[i].isOnline ? "Online" : "Offline";

        // Update nickname and bio if available
        QString nickname, bio;
        if (srv->getUserSettings(userList[i].email, nickname, bio)) {
            if (!nickname.isEmpty()) {
                userList[i].name = nickname;
            }
        }

        filteredUsers.append(userList[i]);
    }

    // Update the user list with filtered users
    userList = filteredUsers;

    // Determine which users to display based on search and contact status
    QString searchText = searchInput->text().toLower();
    int countAdded = 0;

    // First add all groups at the top of the list
    for (int i = 0; i < groupList.size(); ++i) {
        const GroupInfo &group = groupList[i];

        // Only show groups that match search or if we're not searching
        if (!isSearching || group.name.toLower().contains(searchText)) {
            QListWidgetItem *item = createGroupListItem(group, i);
            usersListWidget->addItem(item);

            QWidget *widget = item->data(Qt::UserRole + 1).value<QWidget *>();
            if (widget) {
                usersListWidget->setItemWidget(item, widget);
            }

            countAdded++;
        }
    }

    // Add a separator if we have both groups and users
    if (!groupList.isEmpty() && !userList.isEmpty()) {
        QListWidgetItem *separator = new QListWidgetItem();
        separator->setFlags(Qt::NoItemFlags);
        separator->setSizeHint(QSize(usersListWidget->width(), 20));

        QWidget *separatorWidget = new QWidget();
        QVBoxLayout *separatorLayout = new QVBoxLayout(separatorWidget);
        separatorLayout->setContentsMargins(0, 5, 0, 5);

        QLabel *separatorLabel = new QLabel("Direct Messages");
        separatorLabel->setAlignment(Qt::AlignCenter);
        separatorLabel->setStyleSheet(
            "color: #6e6e8e; font-size: 12px; background-color: transparent;");

        separatorLayout->addWidget(separatorLabel);

        usersListWidget->addItem(separator);
        usersListWidget->setItemWidget(separator, separatorWidget);
    }

    // Then add individual users
    for (int i = 0; i < userList.size(); ++i) {
        const UserInfo &user = userList[i];

        // Determine if this user should be shown
        bool showUser = false;

        // When searching, show only matches
        if (isSearching) {
            showUser = user.name.toLower().contains(searchText) ||
                       user.email.toLower().contains(searchText);
        }
        // Otherwise show contacts and users with messages
        else {
            showUser = user.isContact || user.hasMessages;
        }

        // If we should show this user, add to the list
        if (showUser) {
            QListWidgetItem *item = createUserListItem(user, i);
            usersListWidget->addItem(item);

            // Properly set the item widget
            QWidget *widget = item->data(Qt::UserRole + 1).value<QWidget *>();
            if (widget) {
                usersListWidget->setItemWidget(item, widget);
            }

            countAdded++;
        }
    }

    qDebug() << "Added " << countAdded << " items to the list widget";

    // If we didn't add any users but have users in our list, add them all
    if (countAdded == 0 && !userList.isEmpty()) {
        qDebug() << "No users matched filter criteria, showing all users";

        for (int i = 0; i < userList.size(); ++i) {
            QListWidgetItem *item = createUserListItem(userList[i], i);
            usersListWidget->addItem(item);

            QWidget *widget = item->data(Qt::UserRole + 1).value<QWidget *>();
            if (widget) {
                usersListWidget->setItemWidget(item, widget);
            }
        }
    }

    // Re-enable signals
    usersListWidget->blockSignals(false);
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
    if (!ok)
        return;

    // Check if this is a group or user
    bool isGroup = item->data(Qt::UserRole + 2).toBool();

    if (isGroup) {
        // Handle group selection
        if (index < 0 || index >= groupList.size())
            return;

        // Only update if the selected group changes or we're switching from a
        // user to a group
        if (currentGroupId != index || !isInGroupChat) {
            // Reset user chat
            currentUserId = -1;
            isInGroupChat = true;
            currentGroupId = index;

            // Update chat area for the group
            updateGroupChatArea(index);
        }
    } else {
        // Handle user selection
        if (index < 0 || index >= userList.size())
            return;

        // Only update if the selected user changes or we're switching from a
        // group to a user
        if (currentUserId != index || isInGroupChat) {
            // Reset group chat
            currentGroupId = -1;
            isInGroupChat = false;

            // Get latest bio and status for the selected user
            server *srv = server::getInstance();
            if (srv) {
                QString nickname, bio;
                if (srv->getUserSettings(userList[index].email, nickname,
                                         bio)) {
                    // Update user status
                    userList[index].isOnline =
                        srv->isUserOnline(userList[index].email);
                    userList[index].status =
                        userList[index].isOnline ? "Online" : "Offline";
                }
            }

            currentUserId = index;
            updateChatArea(index);
        }
    }
}

void ChatPage::updateChatArea(int index) {
    qDebug() << "Updating chat area for user index:" << index;

    if (index < 0 || index >= userList.size())
        return;

    // Update header with latest info
    const UserInfo &user = userList[index];

    // Always get the most current user info from server
    server *srv = server::getInstance();
    if (srv) {
        // Get latest online status
        bool isOnline = srv->isUserOnline(user.email);
        userList[index].isOnline = isOnline;
        userList[index].status = isOnline ? "Online" : "Offline";

        // Get latest nickname and bio
        QString nickname, bio;
        if (srv->getUserSettings(user.email, nickname, bio)) {
            // Update chat header with all current information
            QString headerText = user.name + " - " + userList[index].status;
            if (!bio.isEmpty()) {
                headerText += " (" + bio + ")";
            }
            chatHeader->setText(headerText);
        } else {
            // Fallback if settings can't be retrieved
            chatHeader->setText(user.name + " - " + userList[index].status);
        }
        
        // Mark messages as read if the user is viewing this chat and recipient is online
        if (isOnline) {
            markMessagesAsRead(index);
        }
    } else {
        // Fallback if server isn't available
        chatHeader->setText(user.name);
    }

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
            QString contactId = userList[userId].email.isEmpty()
                                    ? userList[userId].name
                                    : userList[userId].email;

            currentClient->addContact(contactId);
            userList[userId].isContact = true;
            updateUsersList();

            qDebug() << "Added contact with ID:" << contactId;
        }
    }
}

void ChatPage::sendMessage() {
    if (messageInput->text().isEmpty()) {
        return;
    }

    Client *client = server::getInstance()->getCurrentClient();
    if (!client) {
        return;
    }

    QString messageText = messageInput->text();
    QString senderId = client->getUserId();

    if (isInGroupChat) {
        // Sending a message to a group
        if (currentGroupId < 0 || currentGroupId >= groupList.size()) {
            return;
        }

        GroupInfo &group = groupList[currentGroupId];

        // Create message info
        MessageInfo msgInfo;
        msgInfo.text = messageText;
        msgInfo.isFromMe = true;
        msgInfo.sender = senderId;
        msgInfo.timestamp = QDateTime::currentDateTime();

        // Add to group messages
        if (currentGroupId >= groupMessages.size()) {
            groupMessages.resize(currentGroupId + 1);
        }
        groupMessages[currentGroupId].append(msgInfo);

        // Update last message in group info
        group.lastMessage = messageText;
        group.lastSeen = "Just now";
        group.hasMessages = true;

        // Save message to group chat file
        QDir dir;
        if (!dir.exists("../db/groups/" + group.groupId + "/messages")) {
            dir.mkpath("../db/groups/" + group.groupId + "/messages");
        }

        // Save in a text format
        QFile file("../db/groups/" + group.groupId + "/messages/messages.txt");

        // If file exists, append to it
        if (file.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            out << msgInfo.sender << "|"
                << msgInfo.timestamp.toString(Qt::ISODate) << "|"
                << msgInfo.text << "\n";
            file.close();

            qDebug() << "Saved group message to disk for group:"
                     << group.groupId;
        } else {
            qDebug() << "Failed to save group message to file";
        }

        // Notify all group members about the new message
        for (const QString &memberId : group.members) {
            if (memberId != senderId) {
                // Instead of using server->notifyGroupMessage, just log the
                // notification This would be implemented in a real app with
                // proper notifications
                qDebug() << "Group message notification: To:" << memberId
                         << "Group:" << group.groupId << "From:" << senderId
                         << "Message:" << messageText;

                // If the member is online, we could queue a message for them
                // directly For now, this is just a placeholder for the real
                // implementation
            }
        }

        // Update the UI
        updateGroupChatArea(currentGroupId);
    } else {
        // Individual chat message
        if (currentUserId < 0 || currentUserId >= userList.size()) {
            return;
        }

        const UserInfo &user = userList[currentUserId];

        // Add message to ChatPage data structures
        MessageInfo msgInfo;
        msgInfo.text = messageText;
        msgInfo.isFromMe = true;
        msgInfo.timestamp = QDateTime::currentDateTime();
        
        // Add to user messages
        if (currentUserId >= userMessages.size()) {
            userMessages.resize(currentUserId + 1);
        }
        userMessages[currentUserId].append(msgInfo);

        // Add message to Room data structure (which will persist to disk)
        QString targetId = user.email.isEmpty() ? user.name : user.email;
        
        // Automatically mark the message as read if recipient is online
        bool recipientOnline = server::getInstance()->isUserOnline(targetId);
        
        // Create the message - notice we don't need to manually set read status here
        // as it's false by default unless recipient is online
        Message msg(messageText, senderId);
        
        // Set read status based on recipient's online status
        if (recipientOnline) {
            msg.markAsRead();
            qDebug() << "Message automatically marked as read since recipient is online:" << targetId;
        }

        // Attempt to get or create the room with proper diagnostics
        qDebug() << "Attempting to find room with user:" << targetId;
        Room *room = client->getRoomWithUser(targetId);
        
        if (!room) {
            qDebug() << "No existing room found, creating new room with user:" << targetId;
            room = client->createRoom(targetId);
            
            if (!userList[currentUserId].isContact) {
                userList[currentUserId].isContact = true;
                client->addContact(targetId);
                qDebug() << "Added user to contacts:" << targetId;
            }
        }

        // Add the message to the file and in-memory structure
        room->addMessage(msg);

        // Save messages to disk immediately
        room->saveMessages();
        qDebug() << "Saved message to disk for room:" << room->getRoomId();

        // Make sure the message is also added to the server's in-memory
        // structure
        server *srv = server::getInstance();
        QVector<Message> roomMsgs = room->getMessagesAsVector();
        srv->updateRoomMessages(room->getRoomId(), roomMsgs);

        // Check if the target user is logged in
        if (srv->hasClient(targetId)) {
            Client *targetClient = srv->getClient(targetId);
            // If the target user is logged in, check if they need to be added as a contact
            if (!srv->hasContactForUser(targetId, senderId)) {
            srv->addContactForUser(targetId, senderId);
                qDebug() << "Added" << senderId << "as contact for user" << targetId;
            }

            // Check if the room needs to be created for the recipient
            if (!srv->hasRoomForUser(targetId, room->getRoomId())) {
                Room *recipientRoom = new Room(room->getName());
                recipientRoom->setRoomId(room->getRoomId());
                recipientRoom->setMessages(roomMsgs);
                srv->addRoomToUser(targetId, recipientRoom);
                qDebug() << "Added room to user" << targetId;
        } else {
            // The target user is logged in, so update their Client object
            Client *targetClient = srv->getClient(targetId);
            if (targetClient) {
                // Add contact if needed
                if (!targetClient->hasContact(senderId)) {
                    targetClient->addContact(senderId);
                    qDebug() << "Added" << senderId
                             << "as contact for logged in user" << targetId;
                }

                // Update their room with the new message
                    Room *recipientRoom = targetClient->getRoom(room->getRoomId());
                if (!recipientRoom) {
                    // Create new room for recipient if it doesn't exist
                    recipientRoom = new Room(room->getName());
                    recipientRoom->setMessages(roomMsgs);
                    targetClient->addRoom(recipientRoom);
                    qDebug() << "Added room to logged in user" << targetId;
                } else {
                    // Update existing room
                    recipientRoom->setMessages(roomMsgs);
                    qDebug() << "Updated room for logged in user" << targetId;
                    }
                }
            }
        }

        // Update the last message and UI
        userList[currentUserId].lastMessage = messageText;
        userList[currentUserId].lastSeen = "Just now";
        userList[currentUserId].hasMessages =
            true; // Mark user as having messages

        // If the users list needs reordering (if this wasn't already a pinned
        // user)
        if (!userList[currentUserId].hasMessages) {
            // Reorder the user list to put this user at the top (since they now
            // have messages)
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
    }

    // Clear the input field
    messageInput->clear();
    QScrollBar *vScrollBar = messageArea->verticalScrollBar();
    vScrollBar->setValue(vScrollBar->maximum());
}

void ChatPage::displayLatestMessage(int userIndex) {
    if (userIndex < 0 || userIndex >= userList.size()) {
        qDebug() << "Invalid user index:" << userIndex;
        return;
    }

    // Get the user's information
    UserInfo &user = userList[userIndex];
    
    // Get the current client
    Client *currentClient = server::getInstance()->getCurrentClient();
    if (!currentClient) {
        qDebug() << "No current client found";
        return;
    }
    
    // Get the room between current user and selected user
    QString roomId = Room::generateRoomId(currentClient->getUserId(), user.email);
    Room *room = currentClient->getRoom(roomId);
    
    if (!room) {
        qDebug() << "No room found for user:" << user.name;
        return;
    }
    
    // Using stack's top() method to get the most recent message without traversing all messages
    if (!room->getMessages().isEmpty()) {
        Message latestMsg = room->getLatestMessage();
        
        QDateTime timestamp = latestMsg.getTimestamp();
        QString sender = latestMsg.getSender();
        QString content = latestMsg.getContent();
        
        // Create a notification with the latest message
        QMessageBox msgBox;
        msgBox.setWindowTitle("Latest Message");
        msgBox.setText("Most recent message from " + user.name + ":");
        msgBox.setInformativeText(content + "\n\nSent at: " + timestamp.toString("yyyy-MM-dd hh:mm:ss"));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
        
        qDebug() << "Displayed latest message from" << user.name << ":" << content;
    } else {
        QMessageBox::information(this, "No Messages", 
                                "No messages found in the conversation with " + user.name);
    }
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

    // Create read receipts indicator for outgoing messages
    QLabel *readReceiptLabel = nullptr;
    bool isRead = false;
    
    if (isFromMe && currentUserId >= 0) {
        // Find the message in the room to check its read status
        Client *client = server::getInstance()->getCurrentClient();
        if (client) {
            QString senderId = client->getUserId();
            QString recipientId = userList[currentUserId].email;
            QString roomId = Room::generateRoomId(senderId, recipientId);
            Room *room = client->getRoom(roomId);
            
            if (room) {
                // Find this message based on timestamp to get its read status
                QList<Message> messages = room->getMessages();
                for (const Message &msg : messages) {
                    if (msg.getTimestamp() == timestamp && msg.getSender() == senderId) {
                        isRead = msg.getReadStatus();
                        break;
                    }
                }
            }
        }
        
        // Create the read receipt checkmarks
        readReceiptLabel = new QLabel();
        readReceiptLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        
        // Use Unicode checkmarks (✓ and ✓✓) as a simple implementation
        // In a real app, you might want to use actual images for better styling
        if (isRead) {
            // Two blue checkmarks for read
            readReceiptLabel->setText("✓✓");
            readReceiptLabel->setStyleSheet("font-size: 12px; color: #4169E1;"); // Royal blue for read
        } else {
            // Two gray checkmarks for delivered but not read
            readReceiptLabel->setText("✓✓");
            readReceiptLabel->setStyleSheet("font-size: 12px; color: #A0A0A0;"); // Gray for delivered
        }
        
        // Set property for later reference
        readReceiptLabel->setProperty("isRead", isRead);
    }

    // Create bubble content
    QWidget *bubbleContent = new QWidget();
    QHBoxLayout *contentLayout = new QHBoxLayout(bubbleContent);
    contentLayout->setContentsMargins(12, 8, 12, 8);
    contentLayout->setSpacing(5);

    contentLayout->addWidget(messageText);
    contentLayout->addWidget(timestampLabel);
    
    // Add read receipt indicator if this is an outgoing message
    if (isFromMe && readReceiptLabel) {
        contentLayout->addWidget(readReceiptLabel);
    }

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
    bubbleContent->setProperty("isRead", isRead);
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

        if (bubble) {
            // Handle story-related clicks
            if (bubble->property("storyType").isValid()) {
                QString storyType = bubble->property("storyType").toString();
                QVariant storyData = bubble->property("storyData");

                if (storyData.isValid()) {
                    StoryInfo story = storyData.value<StoryInfo>();
                    viewStory(story);
                    return true;
                }
            }

            // Handle "Add Story" button click
            if (bubble->objectName() == "addStoryWidget") {
                createStory();
                return true;
            }

            // Handle message bubble clicks
            if (bubble->property("isFromMe").toBool()) {
                showMessageOptions(bubble);
                return true;
            }

            // Handle profile avatar container clicks
            if (obj->parent() &&
                    obj->parent()->objectName() == "profileAvatarContainer" ||
                bubble && bubble->objectName() == "profileAvatarContainer") {
                // Switch to settings tab (index 3)
                contentStack->setCurrentIndex(3);

                // Update nav button styling
                for (int j = 0; j < navButtons.size(); j++) {
                    navButtons[j]->setStyleSheet(
                        QString(R"(
                        background-color: %1;
                        border-radius: 12px;
                        color: white;
                        font-size: 18px;
                    )")
                            .arg(j == 3
                                     ? "#4d6eaa"
                                     : "#2e2e3e")); // Highlight Settings button
                }

                // Hide users sidebar
                usersSidebar->hide();

                // Load latest user settings
                loadUserSettings();

                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void ChatPage::showMessageOptions(QWidget *bubble) {
    QMenu *menu = new QMenu(this);
    QAction *editAction = menu->addAction("Edit Message");

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
                        QString targetId =
                            userList[currentUserId].email.isEmpty()
                                ? userList[currentUserId].name
                                : userList[currentUserId].email;
                        Room *room = client->getRoomWithUser(targetId);
                        if (room) {
                            // Update the message in the room
                            QVector<Message> messages = room->getMessagesAsVector();
                            for (int j = 0; j < messages.size(); j++) {
                                if (messages[j].getTimestamp() == timestamp) {
                                    // We can't modify the message directly, so
                                    // remove and add a new one
                                    room->removeMessage(j);
                                    Message newMsg(newText,
                                                   client->getUserId());
                                    room->addMessage(newMsg);
                                    break;
                                }
                            }
                            // Save changes to disk
                            room->saveMessages();

                            // Update the server's in-memory message structure
                            // for both users
                            server *srv = server::getInstance();
                            QString roomId = room->getRoomId();
                            QVector<Message> updatedMsgs = room->getMessagesAsVector();

                            // Update messages in server
                            srv->updateRoomMessages(roomId, updatedMsgs);

                            qDebug() << "Saved edited message to disk for room:"
                                     << room->getRoomId();
                        }
                    }
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
    QString gradientColors =
        user.hasMessages ? "stop:0 #6a82fb, stop:1 #fc5c7d"
                         :                    // More vibrant for pinned users
            "stop:0 #808080, stop:1 #a0a0a0"; // Gray for regular users

    // Create avatar with online indicator
    QWidget *avatarContainer = new QWidget();
    QVBoxLayout *avatarLayout = new QVBoxLayout(avatarContainer);
    avatarLayout->setContentsMargins(0, 0, 0, 0);
    avatarLayout->setSpacing(0);

    // Check if user has a custom avatar image
    QString avatarPath = server::getInstance()->getUserAvatar(user.email);
    QLabel *avatar = new QLabel();
    avatar->setFixedSize(44, 44);
    avatar->setAlignment(Qt::AlignCenter);

    if (!avatarPath.isEmpty() && QFile::exists(avatarPath)) {
        // Load and display the user's custom avatar - FIX: Create a new QPixmap
        // instance for each user
        QPixmap userAvatar(avatarPath); // Direct construction from file path to
                                        // ensure new instance
        if (!userAvatar.isNull()) {
            // Scale the avatar to fit
            QPixmap scaledAvatar = userAvatar.scaled(
                avatar->width(), avatar->height(), Qt::KeepAspectRatio,
                Qt::SmoothTransformation);

            // Apply circular mask to the avatar - FIX: Create new QPixmap for
            // the masked version
            QPixmap circularAvatar(scaledAvatar.size());
            circularAvatar.fill(Qt::transparent);

            QPainter painter(&circularAvatar);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);

            QPainterPath path;
            path.addEllipse(0, 0, scaledAvatar.width(), scaledAvatar.height());
            painter.setClipPath(path);
            painter.drawPixmap(0, 0, scaledAvatar);
            painter.end(); // Explicitly end the painter before using the pixmap

            // Set new pixmap to the avatar label
            avatar->setPixmap(circularAvatar);
            avatar->setStyleSheet(
                "border-radius: 22px; border: 2px solid #23233a;");

            // Debug print to track avatar loading
            qDebug() << "Loaded custom avatar for user:" << user.email
                     << "from path:" << avatarPath;
        } else {
            // Fallback to letter if image can't be loaded
            avatar->setText(user.name.left(1).toUpper());
            avatar->setStyleSheet(QString(R"(
        background: qlineargradient(x1:0, y1:0, x2:1, y2:1, %1);
        color: white;
        border-radius: 22px;
        font-weight: bold;
        font-size: 22px;
        border: 2px solid #23233a;
    )")
                                      .arg(gradientColors));

            qDebug() << "Failed to load avatar for user:" << user.email
                     << "from path:" << avatarPath;
        }
    } else {
        // Use text-based avatar if no custom avatar
        avatar->setText(user.name.left(1).toUpper());
        avatar->setStyleSheet(QString(R"(
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, %1);
            color: white;
            border-radius: 22px;
            font-weight: bold;
            font-size: 22px;
            border: 2px solid #23233a;
        )")
                                  .arg(gradientColors));

        qDebug() << "Using text avatar for user:" << user.email
                 << "(no custom avatar)";
    }

    avatarLayout->addWidget(avatar, 0, Qt::AlignCenter);

    // Add online status indicator
    QLabel *statusIndicator = new QLabel();
    statusIndicator->setFixedSize(12, 12);
    statusIndicator->setStyleSheet(
        QString(R"(
        background-color: %1;
        border-radius: 6px;
        border: 1px solid #23233a;
    )")
            .arg(user.isOnline
                     ? "#4CAF50"
                     : "#9E9E9E")); // Green for online, gray for offline

    // Position status indicator at bottom right of avatar
    QHBoxLayout *indicatorLayout = new QHBoxLayout();
    indicatorLayout->addStretch();
    indicatorLayout->addWidget(statusIndicator);
    avatarLayout->addLayout(indicatorLayout);

    QWidget *textContainer = new QWidget;
    QVBoxLayout *textLayout = new QVBoxLayout(textContainer);
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(2);

    // Create name label - removed bio
    QLabel *nameLabel = new QLabel(user.name);
    nameLabel->setStyleSheet(
        "color: white; font-size: 17px; font-weight: 600;");

    textLayout->addWidget(nameLabel);

    // Status label showing online/offline
    QLabel *statusLabel = new QLabel(user.status);
    statusLabel->setStyleSheet(QString("color: %1; font-size: 12px;")
                                   .arg(user.isOnline ? "#4CAF50" : "#9E9E9E"));
    textLayout->addWidget(statusLabel);

    // // Add pin indicator for users with messages
    // if (user.hasMessages) {
    //     QLabel *pinnedLabel = new QLabel("📌 Pinned chat");
    //     pinnedLabel->setStyleSheet("color: #a0a0a0; font-size: 12px;");
    //     textLayout->addWidget(pinnedLabel);
    // } else if (!user.lastMessage.isEmpty()) {
    //     QLabel *lastMsgLabel = new QLabel(user.lastMessage);
    //     lastMsgLabel->setStyleSheet("color: #a0a0a0; font-size: 12px;");
    //     textLayout->addWidget(lastMsgLabel);
    // }

    layout->addWidget(avatarContainer);
    layout->addWidget(textContainer, 1);

    QListWidgetItem *item = new QListWidgetItem;
    item->setSizeHint(itemWidget->sizeHint());
    item->setData(Qt::UserRole, index);

    // Store the widget pointer directly, don't use void* conversion
    item->setData(Qt::UserRole + 1, QVariant::fromValue(itemWidget));

    // Make the item widget accept context menu events
    itemWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(itemWidget, &QWidget::customContextMenuRequested,
            [this, index](const QPoint &pos) { showUserOptions(index); });

    return item;
}

// Add new method to show user options menu
void ChatPage::showUserOptions(int userIndex) {
    if (userIndex < 0 || userIndex >= userList.size())
        return;

    const UserInfo &user = userList[userIndex];

    QMenu *menu = new QMenu(this);

    // Only show Block User option
    QAction *blockAction = menu->addAction("Block User");
    connect(blockAction, &QAction::triggered,
            [this, userIndex]() { blockUser(userIndex); });

    // Add contact option if not a contact
    if (!userList[userIndex].isContact) {
        QAction *addContactAction = new QAction("Add to Contacts", this);
        connect(addContactAction, &QAction::triggered, [this, userIndex]() {
            addToContacts(userIndex);
        });
        menu->addAction(addContactAction);
    }
    
    menu->exec(QCursor::pos());
    delete menu;
}

// Add new method to remove a user from contacts
void ChatPage::removeUserFromContacts(int userIndex) {
    if (userIndex < 0 || userIndex >= userList.size())
        return;

    Client *currentClient = server::getInstance()->getCurrentClient();
    if (!currentClient)
        return;

    QString userEmail = userList[userIndex].email;
    QString userName = userList[userIndex].name;

    // Get contact ID (prefer email over name)
    QString contactId = userEmail.isEmpty() ? userName : userEmail;

    // Confirm removal
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Remove Contact",
        "Are you sure you want to remove " + userName + " from your contacts?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // Remove contact - client method returns void, not bool
        currentClient->removeContact(contactId);

        // Update UI
        userList[userIndex].isContact = false;
        QMessageBox::information(this, "Contact Removed",
                                 userName +
                                     " has been removed from your contacts.");

        // Update UI
        updateUsersList();
    }
}

// Add new method to block a user
void ChatPage::blockUser(int userIndex) {
    if (userIndex < 0 || userIndex >= userList.size())
        return;

    Client *currentClient = server::getInstance()->getCurrentClient();
    if (!currentClient)
        return;

    QString userEmail = userList[userIndex].email;
    QString userName = userList[userIndex].name;

    // Confirm block
    QMessageBox::StandardButton reply =
        QMessageBox::question(this, "Block User",
                              "Are you sure you want to block " + userName +
                                  "? They won't be able to contact you.",
                              QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // Block implementation - first remove from contacts if needed
        if (userList[userIndex].isContact) {
            QString contactId = userEmail.isEmpty() ? userName : userEmail;
            currentClient->removeContact(contactId);
            userList[userIndex].isContact = false;
        }

        // Get current user's ID
        QString currentUserId = currentClient->getUserId();

        // Use the server to block the user directly since Client method doesn't
        // exist yet
        bool success = server::getInstance()->blockUserForClient(
            currentUserId, userEmail.isEmpty() ? userName : userEmail);

        if (success) {
            // Remove from UI immediately
            userList.removeAt(userIndex);
            userMessages.removeAt(userIndex);

            // Reset current user if it was the blocked one
            if (this->currentUserId == userIndex) {
                this->currentUserId = -1;
                chatHeader->setText("Select a user");

                // Clear chat area
                QLayoutItem *item;
                while ((item = messageLayout->takeAt(0)) != nullptr) {
                    if (item->widget()) {
                        delete item->widget();
                    }
                    delete item;
                }
                messageLayout->addStretch();
            } else if (this->currentUserId > userIndex) {
                // Adjust currentUserId if needed - fix the decrement
                this->currentUserId = this->currentUserId - 1;
            }

            // Update UI
            updateUsersList();

            QMessageBox::information(this, "User Blocked",
                                     userName +
                                         " has been blocked successfully.");
        } else {
            QMessageBox::warning(this, "Error",
                                 "Failed to block " + userName +
                                     ". Please try again.");
        }
    }
}

void ChatPage::loadMessagesForCurrentUser() {
    qDebug() << "----------------------";
    qDebug() << "Loading messages for user index:" << currentUserId;

    if (currentUserId < 0 || currentUserId >= userList.size()) {
        qDebug() << "ERROR: Invalid user index:" << currentUserId;
        return;
    }

    Client *client = server::getInstance()->getCurrentClient();
    if (!client) {
        qDebug() << "ERROR: No active client found";
        return;
    }

    qDebug() << "Current client - UserID:" << client->getUserId() 
             << "Username:" << client->getUsername()
             << "Email:" << client->getEmail();

    // Always use email for consistent room lookup
    QString targetEmail = userList[currentUserId].email;
    QString targetName = userList[currentUserId].name;

    // Log the info we're using for debugging
    qDebug() << "Target user info - Email:" << targetEmail << "Name:" << targetName;

    // Check if this is a full email address
    bool isFullEmail = targetEmail.contains("@");
    qDebug() << "Target email contains @ symbol:" << isFullEmail;

    // IMPORTANT: Only use the email as targetId - never use nickname
    QString targetId;
    if (targetEmail.isEmpty()) {
        qDebug() << "ERROR: Target user has empty email, cannot proceed safely.";
        return;
    } else {
        targetId = targetEmail;
        qDebug() << "Using email for message recipient:" << targetId;
    }
    
    qDebug() << "Loading messages with user ID:" << targetId;

    // Get all rooms from client for debugging
    QVector<Room*> allRooms = client->getAllRooms();
    qDebug() << "Available rooms (" << allRooms.size() << "):";
    for (Room* r : allRooms) {
        qDebug() << " - Room:" << r->getRoomId() << "Name:" << r->getName();
    }

    // Try to get room with the target user using email only
    qDebug() << "Trying to find room for user:" << targetId;
    Room *room = client->getRoomWithUser(targetId);
    
    if (!room) {
        qDebug() << "ERROR: No room found for user:" << targetId;
        return;
    }

    qDebug() << "SUCCESS: Found room with ID:" << room->getRoomId();

    // Ensure userMessages has enough space
    if (currentUserId >= userMessages.size()) {
        userMessages.resize(currentUserId + 1);
    }

    // Clear existing messages to prevent duplication
    userMessages[currentUserId].clear();

    // Load messages from the room
    room->loadMessages();
    const QVector<Message> &roomMessages = room->getMessagesAsVector();
    qDebug() << "Room has" << roomMessages.size() << "messages";

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
    qDebug() << "----------------------";
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
    bool hasSettings =
        server::getInstance()->getUserSettings(userId, nickname, bio);
    isOnline = server::getInstance()->isUserOnline(userId);

    // Store settings in memory
    userSettings.nickname =
        nickname.isEmpty() ? client->getUsername() : nickname;
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

    // Update profile avatar
    updateProfileAvatar();

    // Update avatar preview in settings
    if (avatarPreview) {
        if (userSettings.hasCustomAvatar &&
            !userSettings.avatarImage.isNull()) {
            // Scale image to fit the preview
            QPixmap scaledAvatar = userSettings.avatarImage.scaled(
                avatarPreview->width(), avatarPreview->height(),
                Qt::KeepAspectRatio, Qt::SmoothTransformation);

            // Apply circular mask to preview
            QPixmap circularAvatar(scaledAvatar.size());
            circularAvatar.fill(Qt::transparent);

            QPainter painter(&circularAvatar);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);

            QPainterPath path;
            path.addEllipse(0, 0, scaledAvatar.width(), scaledAvatar.height());
            painter.setClipPath(path);
            painter.drawPixmap(0, 0, scaledAvatar);

            avatarPreview->setPixmap(circularAvatar);
            avatarPreview->setStyleSheet(
                "border-radius: 50px; border: 2px solid #23233a;");
        } else {
            // Show placeholder with first letter
            avatarPreview->setText(userSettings.nickname.left(1).toUpper());
            avatarPreview->setStyleSheet(R"(
                background-color: #4d6eaa;
                color: white;
                border-radius: 50px;
                font-weight: bold;
                font-size: 40px;
                border: 2px solid #23233a;
            )");
        }
    }

    // Load blocked users list
    loadBlockedUsers();

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

    // Check if nickname or bio has changed
    bool hasChanges = (newNickname != userSettings.nickname) ||
                      (newBio != userSettings.bio) ||
                      (newOnlineStatus != userSettings.isOnline);

    // Check if password fields are filled
    bool passwordChangeRequested = !currentPasswordEdit->text().isEmpty() ||
                                   !newPasswordEdit->text().isEmpty() ||
                                   !confirmPasswordEdit->text().isEmpty();

    // If no changes and no password change, return
    if (!hasChanges && !passwordChangeRequested) {
        QMessageBox::information(this, "No Changes",
                                 "No changes were made to your settings.");
        return;
    }

    // If password change requested, handle it separately
    if (passwordChangeRequested) {
        changePassword();
    }

    // If nickname or bio changed, update them
    if (hasChanges) {
        // Update local settings
        userSettings.nickname = newNickname;
        userSettings.bio = newBio;
        userSettings.isOnline = newOnlineStatus;

        QString userId = client->getUserId();
        qDebug() << "Saving settings for user:" << userId;

        // Save settings to server
        bool nicknameBioSuccess = server::getInstance()->updateUserSettings(
            userId, userSettings.nickname, userSettings.bio);
        bool onlineSuccess = server::getInstance()->setUserOnlineStatus(
            userId, userSettings.isOnline);

        if (nicknameBioSuccess && onlineSuccess) {
            qDebug() << "Settings saved - Nickname:" << userSettings.nickname
                     << "Bio:" << userSettings.bio
                     << "Online:" << userSettings.isOnline;

            // Update profile avatar
            updateProfileAvatar();

            // If bio or nickname changed, force UI refresh
            if (newNickname != userSettings.nickname ||
                newBio != userSettings.bio) {
                // Force refresh the UI to show updated bio in chat headers
                refreshOnlineStatus();
                qDebug() << "Refreshed UI to show updated bio/nickname";
            }

            // Show success message to user
            QMessageBox::information(
                this, "Settings Saved",
                "Your settings have been saved successfully.");

            // Refresh UI
            refreshOnlineStatus();
        } else {
            QMessageBox::warning(
                this, "Error",
                "There was a problem saving your settings. Please try again.");
        }
    }
}

void ChatPage::changePassword() {
    Client *client = server::getInstance()->getCurrentClient();
    if (!client) {
        QMessageBox::warning(this, "Error",
                             "Cannot change password: No active user");
        return;
    }

    QString userId = client->getUserId();
    QString currentPassword = currentPasswordEdit->text();
    QString newPassword = newPasswordEdit->text();
    QString confirmPassword = confirmPasswordEdit->text();

    // Validate input
    if (currentPassword.isEmpty()) {
        QMessageBox::warning(this, "Error",
                             "Please enter your current password.");
        return;
    }

    if (newPassword.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter a new password.");
        return;
    }

    if (confirmPassword.isEmpty()) {
        QMessageBox::warning(this, "Error",
                             "Please confirm your new password.");
        return;
    }

    // Try to change password
    QString errorMessage;
    if (server::getInstance()->changePassword(userId, currentPassword,
                                              newPassword, confirmPassword,
                                              errorMessage)) {
        // Success
        QMessageBox::information(
            this, "Success", "Your password has been changed successfully.");

        // Clear password fields
        currentPasswordEdit->clear();
        newPasswordEdit->clear();
        confirmPasswordEdit->clear();
    } else {
        // Error
        QMessageBox::warning(this, "Error", errorMessage);
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

    // Show a small indicator that the change is being processed
    QApplication::setOverrideCursor(Qt::WaitCursor);

    // Update the server with the new status immediately
    bool success = server::getInstance()->setUserOnlineStatus(userId, isOnline);

    if (success) {
        qDebug() << "Online status changed to:"
                 << (isOnline ? "Online" : "Offline") << "for user:" << userId;

        // Update profile avatar and status indicator
        updateProfileAvatar();

        // Force immediate refresh of all online statuses in UI
        refreshOnlineStatus();

        // Restore cursor
        QApplication::restoreOverrideCursor();

        // Show visual confirmation - using a less intrusive method
        chatHeader->setText(QString("Status changed to %1. Refreshing...")
                                .arg(isOnline ? "Online" : "Offline"));

        // Set a one-shot timer to update the header back to normal after 2
        // seconds
        QTimer::singleShot(2000, this, [this]() {
            if (currentUserId >= 0 && currentUserId < userList.size()) {
                // Get user's bio from server
                QString nickname, bio;
                server *srv = server::getInstance();
                if (srv && srv->getUserSettings(userList[currentUserId].email,
                                                nickname, bio)) {
                    // Create display text with status and bio
                    QString headerText = userList[currentUserId].name + " - " +
                                         userList[currentUserId].status;
                    if (!bio.isEmpty()) {
                        headerText += " (" + bio + ")";
                    }
                    chatHeader->setText(headerText);
                }
            }
        });
    } else {
        qDebug() << "Failed to update online status";

        // Restore cursor
        QApplication::restoreOverrideCursor();

        // Restore the checkbox to previous state if update failed
        if (onlineStatusCheckbox) {
            onlineStatusCheckbox->blockSignals(true);
            onlineStatusCheckbox->setChecked(!isOnline);
            onlineStatusCheckbox->blockSignals(false);
        }

        QMessageBox::warning(
            this, "Error", "Failed to update online status. Please try again.");
    }
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
                QString headerText =
                    userList[i].name + " - " + userList[i].status;
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
                    QListWidgetItem *newItem =
                        createUserListItem(userList[i], i);
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
    // This method is called periodically by the timer to refresh online status
    // of users
    server *srv = server::getInstance();
    if (!srv || !srv->getCurrentClient()) {
        return;
    }

    // CRITICAL FIX: Force updates every 3 seconds to catch all changes
    bool needsUiUpdate = true;  // Force update every time
    bool statusChanged = false; // Track if any status has changed

    // Check online status and bio for all users in the list
    for (int i = 0; i < userList.size(); ++i) {
        // Always check online status - this ensures we pick up changes from
        // other clients
        bool isOnline = srv->isUserOnline(userList[i].email);
        bool onlineChanged = (userList[i].isOnline != isOnline);

        if (onlineChanged) {
            userList[i].isOnline = isOnline;
            userList[i].status = isOnline ? "Online" : "Offline";
            statusChanged = true;

            // If this is the current user being displayed, update the header
            if (i == currentUserId) {
                // Get user's bio from server
                QString nickname, bio;
                srv->getUserSettings(userList[i].email, nickname, bio);

                // Create display text with status and bio
                QString headerText =
                    userList[i].name + " - " + userList[i].status;
                if (!bio.isEmpty()) {
                    headerText += " (" + bio + ")";
                }

                chatHeader->setText(headerText);
                
                // If the user is online and we're looking at their chat, mark messages as read
                if (isOnline) {
                    markMessagesAsRead(currentUserId);
                }
            }

            qDebug() << "Detected online status change for" << userList[i].name
                     << "to" << (isOnline ? "Online" : "Offline");
        }
    }

    // Print diagnostic message if status changed
    if (statusChanged) {
        qDebug() << "Online status changes detected - updating UI";
    } else {
        qDebug() << "Status poll - no changes detected";
    }

    // Update all list items, completely rebuilding the list
    if (needsUiUpdate) {
        // Completely rebuild the user list to ensure all users are visible
        updateUsersList();
    }

    // Always check current user's online status and update checkbox
    Client *client = srv->getCurrentClient();
    if (client) {
        QString userId = client->getUserId();
        bool isOnline = srv->isUserOnline(userId);

        // Update checkbox without triggering signal
        if (onlineStatusCheckbox &&
            onlineStatusCheckbox->isChecked() != isOnline) {
            onlineStatusCheckbox->blockSignals(true);
            onlineStatusCheckbox->setChecked(isOnline);
            onlineStatusCheckbox->blockSignals(false);
            qDebug() << "Updated online status checkbox to"
                     << (isOnline ? "checked" : "unchecked");
        }

        // Update profile avatar and status indicator
        updateProfileAvatar();
    }
    
    // Update read receipts in current chat if visible
    if (contentStack->currentIndex() == 0 && currentUserId >= 0) {
        updateReadReceipts();
    }
}

// Story-related implementations
void ChatPage::createStory() {
    // Check if we have an active user
    Client *client = server::getInstance()->getCurrentClient();
    if (!client) {
        QMessageBox::warning(this, "Error",
                             "You must be logged in to create a story.");
        return;
    }

    // Open file dialog to select image
    QString filePath = QFileDialog::getOpenFileName(
        this, tr("Select Story Image"), QDir::homePath(),
        tr("Image Files (*.png *.jpg *.jpeg *.bmp)"));

    if (filePath.isEmpty()) {
        return; // User canceled
    }

    // Load the selected image
    QPixmap originalImage(filePath);
    if (originalImage.isNull()) {
        QMessageBox::warning(this, tr("Error"),
                             tr("Failed to load the selected image."));
        return;
    }

    // Resize if too large (max 800x800)
    QPixmap scaledImage;
    if (originalImage.width() > 800 || originalImage.height() > 800) {
        scaledImage = originalImage.scaled(800, 800, Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation);
    } else {
        scaledImage = originalImage;
    }

    // Ask for a caption
    bool ok;
    QString caption = QInputDialog::getText(
        this, tr("Add Caption"), tr("Enter a caption for your story:"),
        QLineEdit::Normal, "", &ok);

    if (!ok) {
        return; // User canceled
    }

    // Create directory for stories if it doesn't exist
    QDir dir;
    if (!dir.exists("../db/stories")) {
        dir.mkpath("../db/stories");
    }

    // Save the image with a unique filename
    QString userId = client->getUserId();
    QString timestamp =
        QString::number(QDateTime::currentDateTime().toSecsSinceEpoch());
    QString storyPath = "../db/stories/" + userId + "_" + timestamp + ".png";

    if (!scaledImage.save(storyPath, "PNG")) {
        QMessageBox::warning(this, tr("Error"),
                             tr("Failed to save the story image."));
        return;
    }

    // Add story to server
    QString storyId =
        server::getInstance()->addStory(userId, storyPath, caption);

    if (storyId.isEmpty()) {
        QMessageBox::warning(this, tr("Error"),
                             tr("Failed to create story. Please try again."));
        // Clean up the image file
        QFile::remove(storyPath);
        return;
    }

    // Refresh stories
    loadStories();

    QMessageBox::information(
        this, tr("Success"),
        tr("Your story has been created and will be visible for 24 hours."));
}

void ChatPage::viewStory(const StoryInfo &story) {
    // Check if we have an active user
    Client *client = server::getInstance()->getCurrentClient();
    if (!client) {
        QMessageBox::warning(this, "Error",
                             "You must be logged in to view stories.");
        return;
    }

    // Show the story in a dialog
    showStoryDialog(story);

    // Mark the story as viewed
    QString storyId =
        story.userId + "_" + story.timestamp.toString("yyyyMMdd_hhmmss");
    server::getInstance()->markStoryAsViewed(storyId, client->getUserId());

    // Refresh stories to update UI
    refreshStories();
}

void ChatPage::loadStories() {
    // Check if we have an active user
    Client *client = server::getInstance()->getCurrentClient();
    if (!client) {
        return;
    }

    // Clear current stories
    userStories.clear();

    // Get all stories from server
    QVector<StoryData> stories = server::getInstance()->getStories();

    // Convert to StoryInfo objects
    for (const StoryData &storyData : stories) {
        StoryInfo info;
        info.userId = storyData.userId;

        // Get user's nickname
        QString nickname, bio;
        if (server::getInstance()->getUserSettings(storyData.userId, nickname,
                                                   bio)) {
            info.username = nickname;
        } else {
            info.username =
                storyData.userId.split('@')[0]; // Fallback to email username
        }

        info.imagePath = storyData.imagePath;
        info.caption = storyData.caption;
        info.timestamp = storyData.timestamp;
        info.viewed = server::getInstance()->hasViewedStory(
            storyData.id, client->getUserId());

        userStories.append(info);
    }

    // Update UI
    refreshStories();

    // Schedule next update
    if (storiesTimer == nullptr) {
        storiesTimer = new QTimer(this);
        connect(storiesTimer, &QTimer::timeout, this, &ChatPage::loadStories);
        storiesTimer->start(60000); // Refresh every minute
    }
}

void ChatPage::refreshStories() {
    // Clear existing story widgets
    clearStoryWidgets();

    // If no stories are available, show the "no stories" label
    QLabel *noStoriesLabel = findChild<QLabel *>("noStoriesLabel");
    if (noStoriesLabel) {
        noStoriesLabel->setVisible(userStories.isEmpty());
    }

    // Add story circles to the top row
    for (const StoryInfo &story : userStories) {
        QWidget *storyCircle = createStoryCircle(story);
        storyWidgetsLayout->addWidget(storyCircle);
    }

    // Add story feed items
    for (const StoryInfo &story : userStories) {
        QWidget *feedItem = createStoryFeedItem(story);
        storiesFeedLayout->addWidget(feedItem);
    }
}

void ChatPage::deleteStory(const QString &storyId) {
    // Check if we have an active user
    Client *client = server::getInstance()->getCurrentClient();
    if (!client) {
        QMessageBox::warning(this, "Error",
                             "You must be logged in to delete a story.");
        return;
    }

    // Confirm deletion
    QMessageBox::StandardButton confirm = QMessageBox::question(
        this, "Delete Story", "Are you sure you want to delete this story?",
        QMessageBox::Yes | QMessageBox::No);

    if (confirm != QMessageBox::Yes) {
        return;
    }

    // Delete the story from server
    if (server::getInstance()->deleteStory(storyId)) {
        // Refresh stories to update UI
        loadStories();
        QMessageBox::information(this, "Success",
                                 "Story deleted successfully.");
    } else {
        QMessageBox::warning(this, "Error",
                             "Failed to delete story. Please try again.");
    }
}

QWidget *ChatPage::createStoryCircle(const StoryInfo &story) {
    QWidget *storyWidget = new QWidget;
    storyWidget->setFixedSize(80, 100);
    storyWidget->setCursor(Qt::PointingHandCursor);
    QVBoxLayout *storyLayout = new QVBoxLayout(storyWidget);
    storyLayout->setAlignment(Qt::AlignCenter);

    // Create circle with border color indicating viewed status
    QLabel *storyCircle = new QLabel();
    storyCircle->setFixedSize(70, 70);
    storyCircle->setAlignment(Qt::AlignCenter);

    // Load user avatar or use first letter
    QString avatarPath = server::getInstance()->getUserAvatar(story.userId);
    if (!avatarPath.isEmpty() && QFile::exists(avatarPath)) {
        QPixmap avatar(avatarPath);

        // Scale and crop to circle
        QPixmap scaledAvatar =
            avatar.scaled(storyCircle->width() - 4, // Smaller to show border
                          storyCircle->height() - 4, Qt::KeepAspectRatio,
                          Qt::SmoothTransformation);

        // Create a circular mask
        QPixmap circularAvatar(scaledAvatar.size());
        circularAvatar.fill(Qt::transparent);

        QPainter painter(&circularAvatar);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        QPainterPath path;
        path.addEllipse(0, 0, scaledAvatar.width(), scaledAvatar.height());
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, scaledAvatar);

        storyCircle->setPixmap(circularAvatar);
    } else {
        // Use first letter of username
        storyCircle->setText(story.username.left(1).toUpper());
    }

    // Set border color based on viewed status
    QString borderColor = story.viewed ? "#6e6e8e" : "#4e4e9e";
    storyCircle->setStyleSheet(QString(R"(
        background-color: #23233a;
        border: 3px solid %1;
        border-radius: 35px;
        color: white;
        font-size: 28px;
        font-weight: bold;
    )")
                                   .arg(borderColor));

    QLabel *usernameText = new QLabel(story.username);
    usernameText->setStyleSheet("color: white; font-size: 12px;");
    usernameText->setAlignment(Qt::AlignCenter);

    storyLayout->addWidget(storyCircle, 0, Qt::AlignCenter);
    storyLayout->addWidget(usernameText, 0, Qt::AlignCenter);

    // Connect click event
    storyWidget->installEventFilter(this);
    storyWidget->setProperty("storyType", "circle");
    storyWidget->setProperty("storyId",
                             story.userId + "_" +
                                 story.timestamp.toString("yyyyMMdd_hhmmss"));
    storyWidget->setProperty("storyData", QVariant::fromValue(story));

    return storyWidget;
}

QWidget *ChatPage::createStoryFeedItem(const StoryInfo &story) {
    QWidget *feedItem = new QWidget;
    feedItem->setFixedHeight(100);
    feedItem->setCursor(Qt::PointingHandCursor);
    QHBoxLayout *itemLayout = new QHBoxLayout(feedItem);
    itemLayout->setContentsMargins(10, 10, 10, 10);

    // User avatar
    QLabel *userAvatar = new QLabel();
    userAvatar->setFixedSize(50, 50);
    userAvatar->setAlignment(Qt::AlignCenter);

    // Load user avatar or use first letter
    QString avatarPath = server::getInstance()->getUserAvatar(story.userId);
    if (!avatarPath.isEmpty() && QFile::exists(avatarPath)) {
        QPixmap avatar(avatarPath);

        // Scale and crop to circle
        QPixmap scaledAvatar =
            avatar.scaled(userAvatar->width(), userAvatar->height(),
                          Qt::KeepAspectRatio, Qt::SmoothTransformation);

        // Create a circular mask
        QPixmap circularAvatar(scaledAvatar.size());
        circularAvatar.fill(Qt::transparent);

        QPainter painter(&circularAvatar);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        QPainterPath path;
        path.addEllipse(0, 0, scaledAvatar.width(), scaledAvatar.height());
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, scaledAvatar);

        userAvatar->setPixmap(circularAvatar);
    } else {
        // Use first letter of username
        userAvatar->setText(story.username.left(1).toUpper());
    }

    // Set border color based on viewed status
    QString borderColor = story.viewed ? "#6e6e8e" : "#4e4e9e";
    userAvatar->setStyleSheet(QString(R"(
        background-color: #23233a;
        border: 2px solid %1;
        border-radius: 25px;
        color: white;
        font-size: 20px;
        font-weight: bold;
    )")
                                  .arg(borderColor));

    // Story details
    QWidget *detailsWidget = new QWidget;
    QVBoxLayout *detailsLayout = new QVBoxLayout(detailsWidget);
    detailsLayout->setContentsMargins(10, 0, 0, 0);
    detailsLayout->setSpacing(5);

    QLabel *usernameLabel = new QLabel(story.username);
    usernameLabel->setStyleSheet(
        "color: white; font-size: 16px; font-weight: bold;");

    QLabel *timestampLabel =
        new QLabel(story.timestamp.toString("yyyy-MM-dd hh:mm"));
    timestampLabel->setStyleSheet("color: #6e6e8e; font-size: 12px;");

    QLabel *captionLabel =
        new QLabel(story.caption.length() > 40 ? story.caption.left(40) + "..."
                                               : story.caption);
    captionLabel->setStyleSheet("color: white; font-size: 14px;");

    // Get viewer count
    int viewerCount = server::getInstance()->getStoryViewerCount(
        story.userId + "_" + story.timestamp.toString("yyyyMMdd_hhmmss"));
    QLabel *viewerCountLabel =
        new QLabel(QString("👁️ %1 views").arg(viewerCount));
    viewerCountLabel->setStyleSheet("color: #6e6e8e; font-size: 12px;");

    detailsLayout->addWidget(usernameLabel);
    detailsLayout->addWidget(timestampLabel);
    detailsLayout->addWidget(captionLabel);
    detailsLayout->addWidget(viewerCountLabel);

    // Preview image
    QLabel *previewImage = new QLabel();
    previewImage->setFixedSize(60, 60);
    previewImage->setAlignment(Qt::AlignCenter);
    previewImage->setStyleSheet(
        "background-color: #2e2e3e; border-radius: 5px;");

    // Load story image if available
    if (!story.imagePath.isEmpty() && QFile::exists(story.imagePath)) {
        QPixmap image(story.imagePath);
        QPixmap scaledImage =
            image.scaled(previewImage->width(), previewImage->height(),
                         Qt::KeepAspectRatio, Qt::SmoothTransformation);
        previewImage->setPixmap(scaledImage);
    } else {
        previewImage->setText("📷");
        previewImage->setStyleSheet(
            "color: white; font-size: 24px; background-color: #2e2e3e; "
            "border-radius: 5px;");
    }

    itemLayout->addWidget(userAvatar);
    itemLayout->addWidget(detailsWidget, 1);
    itemLayout->addWidget(previewImage);

    // Style the feed item
    feedItem->setStyleSheet(R"(
        background-color: #23233a;
        border-radius: 10px;
    )");

    // Connect click event
    feedItem->installEventFilter(this);
    feedItem->setProperty("storyType", "feed");
    feedItem->setProperty("storyId",
                          story.userId + "_" +
                              story.timestamp.toString("yyyyMMdd_hhmmss"));
    feedItem->setProperty("storyData", QVariant::fromValue(story));

    return feedItem;
}

void ChatPage::showStoryDialog(const StoryInfo &story) {
    QDialog *storyDialog = new QDialog(this);
    storyDialog->setWindowTitle("Story - " + story.username);
    storyDialog->setFixedSize(600, 700);
    storyDialog->setStyleSheet("background-color: #12121a;");

    QVBoxLayout *dialogLayout = new QVBoxLayout(storyDialog);

    // Header with user info
    QWidget *headerWidget = new QWidget;
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);

    QLabel *userAvatar = new QLabel();
    userAvatar->setFixedSize(40, 40);
    userAvatar->setAlignment(Qt::AlignCenter);

    // Load user avatar or use first letter
    QString avatarPath = server::getInstance()->getUserAvatar(story.userId);
    if (!avatarPath.isEmpty() && QFile::exists(avatarPath)) {
        QPixmap avatar(avatarPath);

        // Scale and crop to circle
        QPixmap scaledAvatar =
            avatar.scaled(userAvatar->width(), userAvatar->height(),
                          Qt::KeepAspectRatio, Qt::SmoothTransformation);

        // Create a circular mask
        QPixmap circularAvatar(scaledAvatar.size());
        circularAvatar.fill(Qt::transparent);

        QPainter painter(&circularAvatar);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        QPainterPath path;
        path.addEllipse(0, 0, scaledAvatar.width(), scaledAvatar.height());
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, scaledAvatar);

        userAvatar->setPixmap(circularAvatar);
    } else {
        // Use first letter of username
        userAvatar->setText(story.username.left(1).toUpper());
    }
    userAvatar->setStyleSheet(R"(
        background-color: #23233a;
        border: 2px solid #4e4e9e;
        border-radius: 20px;
        color: white;
        font-size: 16px;
        font-weight: bold;
    )");

    QWidget *userInfoWidget = new QWidget;
    QVBoxLayout *userInfoLayout = new QVBoxLayout(userInfoWidget);
    userInfoLayout->setContentsMargins(0, 0, 0, 0);
    userInfoLayout->setSpacing(2);

    QLabel *usernameLabel = new QLabel(story.username);
    usernameLabel->setStyleSheet(
        "color: white; font-size: 16px; font-weight: bold;");

    QLabel *timestampLabel =
        new QLabel(story.timestamp.toString("yyyy-MM-dd hh:mm"));
    timestampLabel->setStyleSheet("color: #6e6e8e; font-size: 12px;");

    userInfoLayout->addWidget(usernameLabel);
    userInfoLayout->addWidget(timestampLabel);

    headerLayout->addWidget(userAvatar);
    headerLayout->addWidget(userInfoWidget, 1);

    // Remove timer label that was counting down from 30 seconds

    // Story image
    QLabel *storyImage = new QLabel();
    storyImage->setAlignment(Qt::AlignCenter);
    storyImage->setStyleSheet(
        "background-color: #2e2e3e; border-radius: 10px;");

    // Load story image if available
    if (!story.imagePath.isEmpty() && QFile::exists(story.imagePath)) {
        QPixmap image(story.imagePath);
        QPixmap scaledImage = image.scaled(580, 400, Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation);
        storyImage->setPixmap(scaledImage);
    } else {
        storyImage->setText("Image not available");
        storyImage->setStyleSheet(
            "color: white; font-size: 20px; background-color: #2e2e3e; "
            "border-radius: 10px;");
    }

    // Caption
    QLabel *captionLabel = new QLabel(story.caption);
    captionLabel->setStyleSheet(
        "color: white; font-size: 16px; padding: 10px;");
    captionLabel->setWordWrap(true);
    captionLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    // Get viewers info
    QString storyId =
        story.userId + "_" + story.timestamp.toString("yyyyMMdd_hhmmss");
    QSet<QString> viewers = server::getInstance()->getStoryViewers(storyId);

    // Don't include current user in viewers if they are the story creator
    Client *client = server::getInstance()->getCurrentClient();
    if (client && client->getUserId() == story.userId &&
        viewers.contains(client->getUserId())) {
        viewers.remove(client->getUserId());
    }

    // Create viewers section
    QWidget *viewersWidget = new QWidget();
    QVBoxLayout *viewersLayout = new QVBoxLayout(viewersWidget);
    viewersLayout->setContentsMargins(10, 10, 10, 10);

    // Viewers header with count
    QLabel *viewersHeader =
        new QLabel(QString("👁️ Viewers (%1)").arg(viewers.size()));
    viewersHeader->setStyleSheet(
        "color: white; font-size: 16px; font-weight: bold; margin-top: 10px;");
    viewersLayout->addWidget(viewersHeader);

    // Viewer list with scrollable area
    QScrollArea *viewersScrollArea = new QScrollArea();
    viewersScrollArea->setWidgetResizable(true);
    viewersScrollArea->setMaximumHeight(100);
    viewersScrollArea->setStyleSheet(
        "background-color: #23233a; border-radius: 10px; border: none;");

    QWidget *viewersListWidget = new QWidget();
    QVBoxLayout *viewersListLayout = new QVBoxLayout(viewersListWidget);
    viewersListLayout->setSpacing(5);

    // Add each viewer to the list
    QSet<QString>::const_iterator it;
    for (it = viewers.constBegin(); it != viewers.constEnd(); ++it) {
        QString viewerId = *it;

        // Skip current user if they're the story creator
        if (client && client->getUserId() == story.userId &&
            viewerId == client->getUserId()) {
            continue;
        }

        // Get viewer's nickname
        QString nickname, bio;
        if (server::getInstance()->getUserSettings(viewerId, nickname, bio)) {
            // Create viewer item
            QWidget *viewerItem = new QWidget();
            QHBoxLayout *viewerLayout = new QHBoxLayout(viewerItem);
            viewerLayout->setContentsMargins(5, 5, 5, 5);

            // Viewer avatar or first letter
            QLabel *viewerAvatar = new QLabel();
            viewerAvatar->setFixedSize(24, 24);
            viewerAvatar->setAlignment(Qt::AlignCenter);

            QString viewerAvatarPath =
                server::getInstance()->getUserAvatar(viewerId);
            if (!viewerAvatarPath.isEmpty() &&
                QFile::exists(viewerAvatarPath)) {
                QPixmap avatar(viewerAvatarPath);
                QPixmap scaledAvatar = avatar.scaled(
                    viewerAvatar->width(), viewerAvatar->height(),
                    Qt::KeepAspectRatio, Qt::SmoothTransformation);

                // Create circular mask
                QPixmap circularAvatar(scaledAvatar.size());
                circularAvatar.fill(Qt::transparent);

                QPainter painter(&circularAvatar);
                painter.setRenderHint(QPainter::Antialiasing);
                painter.setRenderHint(QPainter::SmoothPixmapTransform);

                QPainterPath path;
                path.addEllipse(0, 0, scaledAvatar.width(),
                                scaledAvatar.height());
                painter.setClipPath(path);
                painter.drawPixmap(0, 0, scaledAvatar);

                viewerAvatar->setPixmap(circularAvatar);
            } else {
                viewerAvatar->setText(nickname.left(1).toUpper());
            }

            viewerAvatar->setStyleSheet(R"(
                background-color: #2e2e4e;
                border-radius: 12px;
                color: white;
                font-size: 12px;
                font-weight: bold;
            )");

            // Viewer name
            QLabel *viewerName = new QLabel(nickname);
            viewerName->setStyleSheet("color: white; font-size: 14px;");

            // Timestamp (if available)
            QDateTime viewTime =
                server::getInstance()->getStoryViewTime(storyId, viewerId);
            QLabel *viewTimeLabel = new QLabel();
            if (viewTime.isValid()) {
                viewTimeLabel->setText(viewTime.toString("hh:mm"));
                viewTimeLabel->setStyleSheet(
                    "color: #6e6e8e; font-size: 12px;");
            }

            viewerLayout->addWidget(viewerAvatar);
            viewerLayout->addWidget(viewerName, 1);
            viewerLayout->addWidget(viewTimeLabel);

            viewersListLayout->addWidget(viewerItem);
        }
    }

    // If no viewers, show a message
    if (viewers.isEmpty()) {
        QLabel *noViewersLabel = new QLabel("No viewers yet");
        noViewersLabel->setStyleSheet(
            "color: #6e6e8e; font-size: 14px; padding: 10px;");
        noViewersLabel->setAlignment(Qt::AlignCenter);
        viewersListLayout->addWidget(noViewersLabel);
    }

    // Add a stretch to push everything to the top
    viewersListLayout->addStretch();

    viewersScrollArea->setWidget(viewersListWidget);
    viewersLayout->addWidget(viewersScrollArea);

    dialogLayout->addWidget(headerWidget);
    dialogLayout->addWidget(storyImage, 1);
    dialogLayout->addWidget(captionLabel);
    dialogLayout->addWidget(viewersWidget);

    // Close button
    QPushButton *closeButton = new QPushButton("Close");
    closeButton->setStyleSheet(R"(
        background-color: #4e4e9e;
        color: white;
        border-radius: 10px;
        padding: 10px;
        font-size: 16px;
    )");
    connect(closeButton, &QPushButton::clicked, storyDialog, &QDialog::accept);

    // Delete button (only for own stories) - fix the client redeclaration
    if (client && client->getUserId() == story.userId) {
        QPushButton *deleteButton = new QPushButton("Delete Story");
        deleteButton->setStyleSheet(R"(
            background-color: #E15554;
            color: white;
            border-radius: 10px;
            padding: 10px;
            font-size: 16px;
        )");

        connect(
            deleteButton, &QPushButton::clicked, [this, storyDialog, story]() {
                QString storyId = story.userId + "_" +
                                  story.timestamp.toString("yyyyMMdd_hhmmss");
                storyDialog->accept(); // Close dialog first
                deleteStory(storyId);
            });

        QHBoxLayout *buttonsLayout = new QHBoxLayout();
        buttonsLayout->addWidget(deleteButton);
        buttonsLayout->addWidget(closeButton);
        dialogLayout->addLayout(buttonsLayout);
    } else {
        dialogLayout->addWidget(closeButton);
    }

    // Remove the StoryTimer that was handling countdown and auto-closing

    // Show the dialog (blocks until dialog is closed)
    storyDialog->exec();
}

void ChatPage::clearStoryWidgets() {
    // Clear story circles
    QLayoutItem *item;
    while ((item = storyWidgetsLayout->takeAt(1)) !=
           nullptr) { // Start at 1 to keep the "Add Story" button
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    // Clear story feed items (skip the "no stories" label)
    QLabel *noStoriesLabel = findChild<QLabel *>("noStoriesLabel");

    // First remove and keep the no stories label if it exists
    if (noStoriesLabel && noStoriesLabel->parent()) {
        storiesFeedLayout->removeWidget(noStoriesLabel);
    }

    // Clear all items
    while ((item = storiesFeedLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    // Add back the no stories label
    if (noStoriesLabel) {
        storiesFeedLayout->addWidget(noStoriesLabel);
    }
}

// Add a new method to load blocked users
void ChatPage::loadBlockedUsers() {
    Client *client = server::getInstance()->getCurrentClient();
    if (!client) {
        return;
    }

    QString userId = client->getUserId();

    // Get the list of blocked users from the server
    QVector<QString> blockedUsers =
        server::getInstance()->getBlockedUsers(userId);

    // Clear existing UI elements
    QLayoutItem *item;
    while ((item = blockedUsersLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    // Check if we have any blocked users
    if (blockedUsers.isEmpty()) {
        QLabel *noBlockedUsersLabel =
            new QLabel("You haven't blocked any users yet.");
        noBlockedUsersLabel->setStyleSheet(
            "color: #6e6e8e; font-size: 14px; padding: 10px;");
        noBlockedUsersLabel->setAlignment(Qt::AlignCenter);
        noBlockedUsersLabel->setObjectName("noBlockedUsersLabel");
        blockedUsersLayout->addWidget(noBlockedUsersLabel);
        return;
    }

    // Add each blocked user to the list
    for (const QString &blockedUserId : blockedUsers) {
        // Create a widget for this blocked user
        QWidget *blockedUserWidget = new QWidget();
        QHBoxLayout *blockedUserLayout = new QHBoxLayout(blockedUserWidget);
        blockedUserLayout->setContentsMargins(5, 5, 5, 5);

        // Get user info
        QString nickname, bio;
        server::getInstance()->getUserSettings(blockedUserId, nickname, bio);

        // If no nickname, use the userId instead
        if (nickname.isEmpty()) {
            if (blockedUserId.contains("@")) {
                nickname =
                    blockedUserId.split("@")[0]; // Use first part of email
            } else {
                nickname = blockedUserId;
            }
        }

        // Create the user avatar
        QLabel *userAvatar = new QLabel();
        userAvatar->setFixedSize(30, 30);
        userAvatar->setAlignment(Qt::AlignCenter);

        // Try to load user's avatar
        QString avatarPath =
            server::getInstance()->getUserAvatar(blockedUserId);
        if (!avatarPath.isEmpty() && QFile::exists(avatarPath)) {
            QPixmap avatar(avatarPath);
            QPixmap scaledAvatar = avatar.scaled(30, 30, Qt::KeepAspectRatio,
                                                 Qt::SmoothTransformation);

            // Create circular mask
            QPixmap circularAvatar(scaledAvatar.size());
            circularAvatar.fill(Qt::transparent);

            QPainter painter(&circularAvatar);
            painter.setRenderHint(QPainter::Antialiasing);

            QPainterPath path;
            path.addEllipse(0, 0, scaledAvatar.width(), scaledAvatar.height());
            painter.setClipPath(path);
            painter.drawPixmap(0, 0, scaledAvatar);

            userAvatar->setPixmap(circularAvatar);
        } else {
            // Use first letter as avatar
            userAvatar->setText(nickname.left(1).toUpper());
            userAvatar->setStyleSheet(R"(
                background-color: #E15554;
                color: white;
                border-radius: 15px;
                font-weight: bold;
                font-size: 14px;
            )");
        }

        // Create the username label
        QLabel *usernameLabel = new QLabel(nickname);
        usernameLabel->setStyleSheet("color: white; font-size: 14px;");

        // Create the unblock button
        QPushButton *unblockButton = new QPushButton("Unblock");
        unblockButton->setStyleSheet(R"(
            background-color: #4e4e9e;
            color: white;
            border-radius: 4px;
            padding: 5px 10px;
            font-size: 12px;
        )");
        unblockButton->setCursor(Qt::PointingHandCursor);

        // Connect the unblock button
        connect(unblockButton, &QPushButton::clicked,
                [this, blockedUserId]() { unblockUser(blockedUserId); });

        // Add widgets to layout
        blockedUserLayout->addWidget(userAvatar);
        blockedUserLayout->addWidget(usernameLabel, 1);
        blockedUserLayout->addWidget(unblockButton);

        // Add the widget to the list
        blockedUsersLayout->addWidget(blockedUserWidget);
    }

    // Add a stretch at the end
    blockedUsersLayout->addStretch();
}

// Add a method to unblock a user
void ChatPage::unblockUser(const QString &userId) {
    Client *client = server::getInstance()->getCurrentClient();
    if (!client) {
        return;
    }

    // Confirm unblock
    QMessageBox::StandardButton reply =
        QMessageBox::question(this, "Unblock User",
                              "Are you sure you want to unblock this user? "
                              "They will be able to contact you again.",
                              QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // Unblock the user through the server
        bool success = server::getInstance()->unblockUserForClient(
            client->getUserId(), userId);

        if (success) {
            QMessageBox::information(this, "User Unblocked",
                                     "User has been unblocked successfully.");

            // Reload the blocked users list
            loadBlockedUsers();

            // Reload users list to show the unblocked user
            loadUsersFromDatabase();
        } else {
            QMessageBox::warning(this, "Error",
                                 "Failed to unblock user. Please try again.");
        }
    }
}

// Add a new method to load blocked users into the pane
void ChatPage::loadBlockedUsersPane() {
    Client *client = server::getInstance()->getCurrentClient();
    if (!client) {
        return;
    }

    QString userId = client->getUserId();

    // Get the list of blocked users from the server
    QVector<QString> blockedUsers =
        server::getInstance()->getBlockedUsers(userId);

    // Clear existing UI elements
    QLayoutItem *item;
    while ((item = blockedUsersLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    // If no blocked users, show a message
    if (blockedUsers.isEmpty()) {
        QLabel *noBlockedUsersLabel =
            new QLabel("You haven't blocked any users yet.");
        noBlockedUsersLabel->setStyleSheet(
            "color: #6e6e8e; font-size: 16px; padding: 20px;");
        noBlockedUsersLabel->setAlignment(Qt::AlignCenter);
        noBlockedUsersLabel->setObjectName("noBlockedUsersLabel");
        blockedUsersLayout->addWidget(noBlockedUsersLabel);
        return;
    }

    // For each blocked user, create an entry in the list
    for (const QString &blockedUserId : blockedUsers) {
        // Create and configure the list item widget
        QWidget *blockedUserWidget = new QWidget();
        blockedUserWidget->setStyleSheet(
            "background-color: #2a2a3a; border-radius: 10px; margin-bottom: "
            "8px;");
        blockedUserWidget->setFixedHeight(60);

        QHBoxLayout *blockedUserLayout = new QHBoxLayout(blockedUserWidget);
        blockedUserLayout->setContentsMargins(10, 5, 10, 5);

        // Get user details from user map instead of a nonexistent method
        QString nickname, bio;
        server::getInstance()->getUserSettings(blockedUserId, nickname, bio);

        // Use first part of email if no nickname is available
        if (nickname.isEmpty()) {
            if (blockedUserId.contains("@")) {
                nickname =
                    blockedUserId.split("@")[0]; // Use first part of email
            } else {
                nickname = blockedUserId;
            }
        }

        // Create the user avatar
        QLabel *userAvatar = new QLabel();
        userAvatar->setFixedSize(40, 40);
        userAvatar->setAlignment(Qt::AlignCenter);

        // Try to load user's avatar
        QString avatarPath =
            server::getInstance()->getUserAvatar(blockedUserId);
        if (!avatarPath.isEmpty() && QFile::exists(avatarPath)) {
            QPixmap avatar(avatarPath);
            QPixmap scaledAvatar = avatar.scaled(40, 40, Qt::KeepAspectRatio,
                                                 Qt::SmoothTransformation);

            // Create circular mask
            QPixmap circularAvatar(scaledAvatar.size());
            circularAvatar.fill(Qt::transparent);

            QPainter painter(&circularAvatar);
            painter.setRenderHint(QPainter::Antialiasing);

            QPainterPath path;
            path.addEllipse(0, 0, scaledAvatar.width(), scaledAvatar.height());
            painter.setClipPath(path);
            painter.drawPixmap(0, 0, scaledAvatar);

            userAvatar->setPixmap(circularAvatar);
        } else {
            // Use first letter as avatar
            userAvatar->setText(nickname.left(1).toUpper());
            userAvatar->setStyleSheet(R"(
                background-color: #E15554;
                color: white;
                border-radius: 20px;
                font-weight: bold;
                font-size: 16px;
            )");
        }

        // Create the username label
        QLabel *usernameLabel = new QLabel(nickname);
        usernameLabel->setStyleSheet("color: white; font-size: 16px;");

        // Create the unblock button
        QPushButton *unblockButton = new QPushButton("Unblock");
        unblockButton->setStyleSheet(R"(
            background-color: #4e4e9e;
            color: white;
            border-radius: 6px;
            padding: 6px 12px;
            font-size: 14px;
        )");
        unblockButton->setCursor(Qt::PointingHandCursor);

        // Connect the unblock button
        connect(unblockButton, &QPushButton::clicked,
                [this, blockedUserId]() { unblockUser(blockedUserId); });

        // Add widgets to layout
        blockedUserLayout->addWidget(userAvatar);
        blockedUserLayout->addWidget(usernameLabel, 1);
        blockedUserLayout->addWidget(unblockButton);

        // Add the widget to the list
        blockedUsersLayout->addWidget(blockedUserWidget);
    }
}

// Add to the end of the file before the last closing brace
void ChatPage::showCreateGroupDialog() {
    // Create a dialog for creating a new group
    QDialog *groupDialog = new QDialog(this);
    groupDialog->setWindowTitle("Create New Group");
    groupDialog->setFixedSize(450, 600);
    groupDialog->setStyleSheet("background-color: #1e1e2e;");

    QVBoxLayout *dialogLayout = new QVBoxLayout(groupDialog);
    dialogLayout->setContentsMargins(20, 20, 20, 20);
    dialogLayout->setSpacing(15);

    // Group name input
    QLabel *nameLabel = new QLabel("Group Name:");
    nameLabel->setStyleSheet("color: white; font-size: 16px;");

    QLineEdit *nameInput = new QLineEdit();
    nameInput->setPlaceholderText("Enter group name...");
    nameInput->setStyleSheet(R"(
        background-color: #2e2e3e;
        color: white;
        border-radius: 8px;
        padding: 10px;
        border: none;
        font-size: 15px;
    )");
    nameInput->setFixedHeight(40);

    // Available users list with checkboxes
    QLabel *usersLabel = new QLabel("Select Members:");
    usersLabel->setStyleSheet("color: white; font-size: 16px;");

    QListWidget *userSelectionList = new QListWidget();
    userSelectionList->setStyleSheet(R"(
        QListWidget {
            background-color: #2e2e3e;
            border-radius: 8px;
            color: white;
            padding: 10px;
            border: none;
        }
        QListWidget::item {
            border-bottom: 1px solid #3e3e4e;
            padding: 5px;
        }
        QListWidget::item:selected {
            background-color: #4e4e9e;
        }
        QCheckBox {
            color: white;
            font-size: 15px;
            margin-left: 5px;
        }
    )");

    // Populate the list with available users
    Client *client = server::getInstance()->getCurrentClient();
    if (client) {
        // Get blocked users to exclude them
        QVector<QString> blockedUsers = server::getInstance()->getBlockedUsers(client->getUserId());
        QSet<QString> blockedUsersSet;
        for (const QString &blocked : blockedUsers) {
            blockedUsersSet.insert(blocked);
        }

        // Add all users except current user and blocked users
        for (const UserInfo &user : userList) {
            // Skip blocked users
            if (blockedUsersSet.contains(user.email)) {
                continue;
            }

            // Create list item
            QListWidgetItem *item = new QListWidgetItem(userSelectionList);
            
            // Create main widget for the item
            QWidget *itemWidget = new QWidget();
            QHBoxLayout *itemLayout = new QHBoxLayout(itemWidget);
            itemLayout->setContentsMargins(5, 5, 5, 5);
            itemLayout->setSpacing(8);
            
            // Create avatar
            QLabel *avatar = new QLabel();
            avatar->setFixedSize(36, 36);
            avatar->setAlignment(Qt::AlignCenter);
            
            // Check if user has a custom avatar
            QString avatarPath = server::getInstance()->getUserAvatar(user.email);
            if (!avatarPath.isEmpty() && QFile::exists(avatarPath)) {
                QPixmap userAvatar(avatarPath);
                if (!userAvatar.isNull()) {
                    // Scale and crop the avatar to a circle
                    QPixmap scaledAvatar = userAvatar.scaled(
                        avatar->width(), avatar->height(), 
                        Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    
                    QPixmap circularAvatar(scaledAvatar.size());
                    circularAvatar.fill(Qt::transparent);
                    
                    QPainter painter(&circularAvatar);
                    painter.setRenderHint(QPainter::Antialiasing);
                    painter.setRenderHint(QPainter::SmoothPixmapTransform);
                    
                    QPainterPath path;
                    path.addEllipse(0, 0, scaledAvatar.width(), scaledAvatar.height());
                    painter.setClipPath(path);
                    painter.drawPixmap(0, 0, scaledAvatar);
                    painter.end();
                    
                    avatar->setPixmap(circularAvatar);
                    avatar->setStyleSheet("border-radius: 18px; border: 1px solid #23233a;");
                } else {
                    // Fallback to letter avatar
                    avatar->setText(user.name.left(1).toUpper());
                    avatar->setStyleSheet(R"(
                        background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #6a82fb, stop:1 #fc5c7d);
                        color: white;
                        border-radius: 18px;
                        font-weight: bold;
                        font-size: 16px;
                        border: 1px solid #23233a;
                    )");
                }
            } else {
                // Use text-based avatar
                avatar->setText(user.name.left(1).toUpper());
                avatar->setStyleSheet(R"(
                    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #6a82fb, stop:1 #fc5c7d);
                    color: white;
                    border-radius: 18px;
                    font-weight: bold;
                    font-size: 16px;
                    border: 1px solid #23233a;
                )");
            }
            
            // Name and status layout 
            QWidget *textWidget = new QWidget();
            QVBoxLayout *textLayout = new QVBoxLayout(textWidget);
            textLayout->setContentsMargins(0, 0, 0, 0);
            textLayout->setSpacing(2);
            
            QLabel *nameLabel = new QLabel(user.name);
            nameLabel->setStyleSheet("color: white; font-size: 15px; font-weight: 500;");
            
            QLabel *statusLabel = new QLabel(user.isOnline ? "Online" : "Offline");
            statusLabel->setStyleSheet(
                QString("color: %1; font-size: 12px;")
                .arg(user.isOnline ? "#4CAF50" : "#9E9E9E")
            );
            
            textLayout->addWidget(nameLabel);
            textLayout->addWidget(statusLabel);
            
            // Create checkbox directly in the main layout, adjacent to text
            QCheckBox *checkbox = new QCheckBox();
            checkbox->setProperty("userId", user.email);
            checkbox->setFixedSize(24, 24);
            checkbox->setStyleSheet(R"(
                QCheckBox {
                    margin-left: 0px;
                }
                QCheckBox::indicator {
                    width: 20px;
                    height: 20px;
                }
                QCheckBox::indicator:unchecked {
                    border: 2px solid #6a82fb;
                    border-radius: 4px;
                    background-color: #2e2e3e;
                }
                QCheckBox::indicator:checked {
                    border: 2px solid #6a82fb;
                    border-radius: 4px;
                    background-color: #6a82fb;
                    image: url(resources/checkmark.svg);
                }
            )");
            
            // Add everything to main layout
            itemLayout->addWidget(avatar);
            itemLayout->addWidget(textWidget, 1); // stretch factor 1
            itemLayout->addWidget(checkbox);
            
            // Set the item widget and size
            item->setSizeHint(QSize(userSelectionList->width() - 20, 60)); // slight padding
            userSelectionList->setItemWidget(item, itemWidget);
        }
    }

    // Create group button
    QPushButton *createButton = new QPushButton("Create Group");
    createButton->setStyleSheet(R"(
        background-color: #4d6eaa;
        color: white;
        border-radius: 8px;
        padding: 12px;
        border: none;
        font-size: 15px;
        font-weight: bold;
    )");

    // Cancel button
    QPushButton *cancelButton = new QPushButton("Cancel");
    cancelButton->setStyleSheet(R"(
        background-color: #E15554;
        color: white;
        border-radius: 8px;
        padding: 12px;
        border: none;
        font-size: 15px;
    )");

    // Connect buttons
    connect(cancelButton, &QPushButton::clicked, groupDialog, &QDialog::reject);
    connect(createButton, &QPushButton::clicked,
            [this, groupDialog, nameInput, userSelectionList]() {
                QString groupName = nameInput->text().trimmed();
                if (groupName.isEmpty()) {
                    QMessageBox::warning(groupDialog, "Error",
                                        "Please enter a group name");
                    return;
                }

                // Collect selected members
                QStringList members;
                for (int i = 0; i < userSelectionList->count(); i++) {
                    QListWidgetItem *item = userSelectionList->item(i);
                    QWidget *widget = userSelectionList->itemWidget(item);
                    QCheckBox *checkbox = widget->findChild<QCheckBox *>();

                    if (checkbox && checkbox->isChecked()) {
                        QString userId = checkbox->property("userId").toString();
                        if (!userId.isEmpty()) {
                            members.append(userId);
                        }
                    }
                }

                if (members.isEmpty()) {
                    QMessageBox::warning(groupDialog, "Error",
                                        "Please select at least one member");
                    return;
                }

                // Create the group
                createNewGroup(groupName, members);
                groupDialog->accept();
            });

    // Add buttons to horizontal layout
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(createButton);

    // Add all widgets to the dialog
    dialogLayout->addWidget(nameLabel);
    dialogLayout->addWidget(nameInput);
    dialogLayout->addWidget(usersLabel);
    dialogLayout->addWidget(userSelectionList);
    dialogLayout->addLayout(buttonLayout);

    // Show the dialog
    groupDialog->exec();
}

void ChatPage::createNewGroup(const QString &groupName, const QStringList &members) {
    Client *client = server::getInstance()->getCurrentClient();
    if (!client) {
        QMessageBox::warning(this, "Error",
                             "You must be logged in to create a group.");
        return;
    }

    QString currentUserId = client->getUserId();

    // Create a unique group ID
    QString groupId =
        "group_" +
        QString::number(QDateTime::currentDateTime().toSecsSinceEpoch()) + "_" +
        QString::number(QRandomGenerator::global()->bounded(10000));

    // Create the group info
    GroupInfo newGroup;
    newGroup.groupId = groupId;
    newGroup.name = groupName;
    newGroup.adminId = currentUserId;
    newGroup.members = members;
    newGroup.hasMessages = false;
    newGroup.lastMessage = "";
    newGroup.lastSeen = "";
    newGroup.hasCustomImage = false;

    // Add current user to members if not already there
    if (!newGroup.members.contains(currentUserId)) {
        newGroup.members.append(currentUserId);
    }

    // Save group to database
    if (!saveGroupToDatabase(newGroup)) {
        QMessageBox::warning(this, "Error",
                             "Failed to create group. Please try again.");
        return;
    }

    // Add group to our local list
    groupList.append(newGroup);

    // Initialize messages vector for this group
    groupMessages.resize(groupList.size());

    // Update UI
    updateGroupsList();

    // Let the user know group was created
    QMessageBox::information(
        this, "Success", "Group \"" + groupName + "\" created successfully!");

    // Create rooms for all members to see the group
    for (const QString &memberId : newGroup.members) {
        // Create a room for this member if they're not the current user
        if (memberId != currentUserId) {
            // Instead of using server->addGroupForUser, create the group
            // directory structure Create user's group directory
            QString userGroupDir = "../db/users/" + memberId + "/groups";
            QDir dir;
            if (!dir.exists(userGroupDir)) {
                dir.mkpath(userGroupDir);
            }

            // Save group info to a file for this user
            QFile groupFile(userGroupDir + "/" + groupId + ".txt");
            if (groupFile.open(QIODevice::WriteOnly)) {
                QTextStream stream(&groupFile);
                stream << "GroupID: " << groupId << "\n";
                stream << "Name: " << groupName << "\n";
                stream << "AdminID: " << currentUserId << "\n";
                stream << "Members: ";
                for (int i = 0; i < newGroup.members.size(); ++i) {
                    stream << newGroup.members[i];
                    if (i < newGroup.members.size() - 1) {
                        stream << ",";
                    }
                }
                stream << "\n";
                groupFile.close();

                qDebug() << "Added group info to user's directory:" << memberId;
            }
        }
    }
}

QListWidgetItem *ChatPage::createGroupListItem(const GroupInfo &group, int index) {
    QWidget *itemWidget = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(itemWidget);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(16);

    // Create a group avatar
    QLabel *avatar = new QLabel();
    avatar->setFixedSize(44, 44);
    avatar->setAlignment(Qt::AlignCenter);

    // Group avatar is either a custom image or a circle with the first letter
    if (group.hasCustomImage && !group.groupImage.isNull()) {
        // Scale and crop to circle
        QPixmap scaledAvatar = group.groupImage.scaled(
            avatar->width(), avatar->height(), Qt::KeepAspectRatio,
            Qt::SmoothTransformation);

        // Create a circular mask
        QPixmap circularAvatar(scaledAvatar.size());
        circularAvatar.fill(Qt::transparent);

        QPainter painter(&circularAvatar);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        QPainterPath path;
        path.addEllipse(0, 0, scaledAvatar.width(), scaledAvatar.height());
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, scaledAvatar);

        avatar->setPixmap(circularAvatar);
    } else {
        // Use first letter of group name
        avatar->setText(group.name.left(1).toUpper());

        // Use a distinct gradient for groups
        QString gradientColors =
            "stop:0 #13547a, stop:1 #80d0c7"; // Blue-green gradient for groups

        avatar->setStyleSheet(QString(R"(
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, %1);
            color: white;
            border-radius: 22px;
            font-weight: bold;
            font-size: 22px;
            border: 2px solid #23233a;
        )")
                                  .arg(gradientColors));
    }

    // Create the text section with group name and member count
    QWidget *textContainer = new QWidget;
    QVBoxLayout *textLayout = new QVBoxLayout(textContainer);
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(2);

    // Group name with 👥 icon to indicate it's a group
    QLabel *nameLabel = new QLabel("🗣️" + group.name);
    nameLabel->setStyleSheet(
        "color: white; font-size: 17px; font-weight: 600;");
    textLayout->addWidget(nameLabel);

    // Member count
    QLabel *memberCountLabel =
        new QLabel(QString("%1 members").arg(group.members.size()));
    memberCountLabel->setStyleSheet("color: #9E9E9E; font-size: 12px;");
    textLayout->addWidget(memberCountLabel);

    // Last message if it exists
    if (!group.lastMessage.isEmpty()) {
        QLabel *lastMsgLabel = new QLabel(group.lastMessage);
        lastMsgLabel->setStyleSheet("color: #a0a0a0; font-size: 12px;");
        textLayout->addWidget(lastMsgLabel);
    }

    layout->addWidget(avatar);
    layout->addWidget(textContainer, 1);

    QListWidgetItem *item = new QListWidgetItem;
    item->setSizeHint(itemWidget->sizeHint());
    item->setData(Qt::UserRole, index);
    item->setData(Qt::UserRole + 2, true); // Mark as group (vs user)

    // Store the widget pointer
    item->setData(Qt::UserRole + 1, QVariant::fromValue(itemWidget));

    // Make the item widget accept context menu events
    itemWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(itemWidget, &QWidget::customContextMenuRequested,
            [this, index](const QPoint &pos) { showGroupOptions(index); });

    return item;
}

void ChatPage::updateGroupChatArea(int index) {
    qDebug() << "Updating group chat area for index:" << index;

    if (index < 0 || index >= groupList.size())
        return;

    // Update header with group info
    const GroupInfo &group = groupList[index];
    chatHeader->setText("👥 " + group.name + " (" +
                        QString::number(group.members.size()) + " members)");

    // Clear existing messages from UI
    QLayoutItem *item;
    while ((item = messageLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
    messageLayout->addStretch(); // Re-add stretch to push messages up

    // Load messages for this group
    currentGroupId = index;
    
    // Ensure this group has a messages vector initialized
    if (currentGroupId >= groupMessages.size()) {
        groupMessages.resize(currentGroupId + 1);
    }
    
    // Load messages
    loadGroupMessagesForCurrentGroup();

    // Add messages to UI - now we're sure that messages are loaded
    const QList<MessageInfo> &messages = groupMessages[currentGroupId];
    for (const MessageInfo &msg : messages) {
        addGroupMessageToUI(msg.text, msg.sender, msg.timestamp);
    }

    // Scroll to bottom
    QScrollBar *vScrollBar = messageArea->verticalScrollBar();
    vScrollBar->setValue(vScrollBar->maximum());
}

void ChatPage::loadGroupsFromDatabase() {
    qDebug() << "Loading groups from database...";
    Client *client = server::getInstance()->getCurrentClient();
    if (!client) {
        qDebug() << "No current client found!";
        return;
    }

    // Clear existing groups
    groupList.clear();

    QString userId = client->getUserId();
    QString groupsDir = "../db/users/" + userId + "/groups";

    QDir dir(groupsDir);
    if (!dir.exists()) {
        dir.mkpath(groupsDir);
        return; // No groups yet
    }

    // Get all group files
    QStringList groupFiles =
        dir.entryList(QStringList() << "*.txt", QDir::Files);

    for (const QString &groupFile : groupFiles) {
        QFile file(groupsDir + "/" + groupFile);
        if (file.open(QIODevice::ReadOnly)) {
            QTextStream stream(&file);
            QString groupData = stream.readAll();
            file.close();

            QStringList lines = groupData.split("\n");
            if (lines.size() >= 4) {
                GroupInfo group;

                // Parse each line for the group properties
                for (const QString &line : lines) {
                    if (line.startsWith("GroupID: ")) {
                        group.groupId = line.mid(9).trimmed();
                    } else if (line.startsWith("Name: ")) {
                        group.name = line.mid(6).trimmed();
                    } else if (line.startsWith("AdminID: ")) {
                        group.adminId = line.mid(9).trimmed();
                    } else if (line.startsWith("Members: ")) {
                        QString membersStr = line.mid(9).trimmed();
                        group.members = membersStr.split(",");
                    }
                }

                // Check if group has messages
                QString messagesFile =
                    "../db/groups/" + group.groupId + "/messages/messages.txt";
                QFile msgFile(messagesFile);
                group.hasMessages = msgFile.exists();

                // Get last message if available
                if (group.hasMessages &&
                    msgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    // Read the last line to get the most recent message
                    QString lastLine;
                    QTextStream in(&msgFile);
                    while (!in.atEnd()) {
                        QString line = in.readLine();
                        if (!line.isEmpty()) {
                            lastLine = line;
                        }
                    }
                    msgFile.close();

                    if (!lastLine.isEmpty()) {
                        QStringList parts = lastLine.split("|");
                        if (parts.size() >= 3) {
                            group.lastMessage =
                                parts[2]; // The message text is the third part
                            group.lastSeen = "Recent";
                        }
                    }
                }

                // Try to load group image
                loadGroupImage(group);

                // Add to list
                groupList.append(group);

                qDebug() << "Loaded group:" << group.name << "with"
                         << group.members.size() << "members";
            }
        }
    }

    // Initialize messages vector for all groups
    groupMessages.resize(groupList.size());

    qDebug() << "Total groups loaded:" << groupList.size();
}

void ChatPage::showGroupOptions(int groupIndex) {
    if (groupIndex < 0 || groupIndex >= groupList.size())
        return;

    const GroupInfo &group = groupList[groupIndex];
    Client *client = server::getInstance()->getCurrentClient();

    if (!client)
        return;

    QString currentUserId = client->getUserId();
    bool isAdmin = (group.adminId == currentUserId);

    QMenu *menu = new QMenu(this);

    // Add members option (admin only)
    if (isAdmin) {
        QAction *addMemberAction = menu->addAction("Add Members");
        connect(addMemberAction, &QAction::triggered,
                [this, groupIndex]() { addMemberToGroup(groupIndex); });

        menu->addSeparator();
    }

    // Leave group option (for non-admins)
    if (!isAdmin) {
        QAction *leaveAction = menu->addAction("Leave Group");
        connect(leaveAction, &QAction::triggered,
                [this, groupIndex]() { leaveGroup(groupIndex); });
    }

    // Delete group option (admin only)
    if (isAdmin) {
        QAction *deleteAction = menu->addAction("Delete Group");
        connect(deleteAction, &QAction::triggered,
                [this, groupIndex]() { deleteGroup(groupIndex); });
    }

    menu->exec(QCursor::pos());
    delete menu;
}

// Group member management
void ChatPage::addMemberToGroup(int groupIndex) {
    if (groupIndex < 0 || groupIndex >= groupList.size())
        return;

    GroupInfo &group = groupList[groupIndex];
    Client *client = server::getInstance()->getCurrentClient();

    if (!client || client->getUserId() != group.adminId) {
        QMessageBox::warning(this, "Error",
                             "Only the group admin can add members.");
        return;
    }

    // Create a dialog to select users to add
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Add Members to " + group.name);
    dialog->setFixedSize(500, 600); // Increase size for better visibility
    dialog->setStyleSheet("background-color: #1e1e2e;");

    QVBoxLayout *dialogLayout = new QVBoxLayout(dialog);
    dialogLayout->setContentsMargins(25, 25, 25, 25); // Increase margins
    dialogLayout->setSpacing(20); // More spacing between elements

    // Header
    QLabel *headerLabel = new QLabel("Select users to add:");
    headerLabel->setStyleSheet(
        "color: white; font-size: 18px; font-weight: bold;"); // Larger text

    // User list with checkboxes
    QListWidget *userList = new QListWidget();
    userList->setStyleSheet(R"(
        QListWidget {
            background-color: #2e2e3e;
            border-radius: 8px;
            color: white;
            padding: 15px;  /* More padding */
            border: none;
        }
        QListWidget::item {
            padding: 12px;  /* More padding per item */
            border-bottom: 1px solid #3e3e4e;
            min-height: 40px;  /* Ensure minimum height */
        }
        QListWidget::item:selected {
            background-color: #4e4e9e;
        }
        QCheckBox {
            color: white;
            font-size: 16px;  /* Larger font for better readability */
            spacing: 10px;   /* More space between checkbox and text */
        }
    )");

    // Get blocked users to exclude them
    QVector<QString> blockedUsers =
        server::getInstance()->getBlockedUsers(client->getUserId());
    QSet<QString> blockedUsersSet;
    for (const QString &blocked : blockedUsers) {
        blockedUsersSet.insert(blocked);
    }

    // Create a set of existing members for quick lookup
    QSet<QString> existingMembers;
    for (const QString &member : group.members) {
        existingMembers.insert(member);
    }

    // Add users who are not already in the group
    bool hasEligibleUsers = false;
    for (const UserInfo &user : this->userList) {
        // Skip blocked users and existing members
        if (blockedUsersSet.contains(user.email) ||
            existingMembers.contains(user.email)) {
            continue;
        }

        hasEligibleUsers = true;
        QListWidgetItem *item = new QListWidgetItem(userList);
        QWidget *itemWidget = new QWidget();
        QHBoxLayout *itemLayout = new QHBoxLayout(itemWidget);
        itemLayout->setContentsMargins(10, 10, 10, 10); // More padding
        itemLayout->setSpacing(15); // More spacing between elements

        // Create checkbox with the user's name
        QCheckBox *checkbox = new QCheckBox(user.name);
        checkbox->setProperty("userId", user.email);

        // Optional: Add extra user info
        QLabel *emailLabel = new QLabel("(" + user.email + ")");
        emailLabel->setStyleSheet("color: #a0a0a0; font-size: 13px;");

        // Add checkbox and email to layout
        QVBoxLayout *userInfoLayout = new QVBoxLayout();
        userInfoLayout->setContentsMargins(0, 0, 0, 0);
        userInfoLayout->setSpacing(5);
        userInfoLayout->addWidget(checkbox);
        userInfoLayout->addWidget(emailLabel);

        itemLayout->addLayout(userInfoLayout, 1);

        // Set a minimum size to ensure item is large enough
        itemWidget->setMinimumHeight(60);

        item->setSizeHint(itemWidget->sizeHint());
        userList->setItemWidget(item, itemWidget);
    }

    // Buttons
    QPushButton *addButton = new QPushButton("Add Selected Users");
    addButton->setStyleSheet(R"(
        background-color: #4d6eaa;
        color: white;
        border-radius: 8px;
        padding: 12px;
        border: none;
        font-size: 15px;
        font-weight: bold;
    )");

    QPushButton *cancelButton = new QPushButton("Cancel");
    cancelButton->setStyleSheet(R"(
        background-color: #E15554;
        color: white;
        border-radius: 8px;
        padding: 12px;
        border: none;
        font-size: 15px;
    )");

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(addButton);

    // Add widgets to dialog
    dialogLayout->addWidget(headerLabel);
    dialogLayout->addWidget(userList);
    dialogLayout->addLayout(buttonLayout);

    // Connect buttons
    connect(cancelButton, &QPushButton::clicked, dialog, &QDialog::reject);

    // If no eligible users, show a message and disable the add button
    if (!hasEligibleUsers) {
        QLabel *noUsersLabel =
            new QLabel("No eligible users to add. All users are either already "
                        "in the group or blocked.");
        noUsersLabel->setStyleSheet(
            "color: white; font-size: 14px; padding: 20px; background-color: "
            "#2e2e3e; border-radius: 8px;");
        noUsersLabel->setWordWrap(true);
        noUsersLabel->setAlignment(Qt::AlignCenter);

        // Remove the user list and add the message
        dialogLayout->removeWidget(userList);
        delete userList;
        dialogLayout->insertWidget(1, noUsersLabel);

        addButton->setEnabled(false);
        addButton->setStyleSheet(R"(
            background-color: #6e6e8e;
            color: white;
            border-radius: 8px;
            padding: 12px;
            border: none;
            font-size: 15px;
            font-weight: bold;
        )");
    }

    // Connect add button to add selected users
    connect(
        addButton, &QPushButton::clicked,
        [this, dialog, userList, &group, groupIndex]() {
            QStringList addedUsers;

            // Collect selected users
            for (int i = 0; i < userList->count(); i++) {
                QListWidgetItem *item = userList->item(i);
                QWidget *widget = userList->itemWidget(item);
                QCheckBox *checkbox = widget->findChild<QCheckBox *>();

                if (checkbox && checkbox->isChecked()) {
                    QString userId = checkbox->property("userId").toString();
                    if (!userId.isEmpty() && !group.members.contains(userId)) {
                        group.members.append(userId);
                        addedUsers.append(userId);
                    }
                }
            }

            if (addedUsers.isEmpty()) {
                QMessageBox::information(dialog, "No Changes",
                                        "No users were selected to add.");
                return;
            }

            // Update the group in the database
            if (saveGroupToDatabase(group)) {
                // Add group to the new members' files
                Client *client = server::getInstance()->getCurrentClient();
                QString currentUserId = client ? client->getUserId() : "";

                for (const QString &memberId : addedUsers) {
                    // Create user's group directory
                    QString userGroupDir =
                        "../db/users/" + memberId + "/groups";
                    QDir dir;
                    if (!dir.exists(userGroupDir)) {
                        dir.mkpath(userGroupDir);
                    }

                    // Save group info to a file for this user
                    QFile groupFile(userGroupDir + "/" + group.groupId +
                                    ".txt");
                    if (groupFile.open(QIODevice::WriteOnly)) {
                        QTextStream stream(&groupFile);
                        stream << "GroupID: " << group.groupId << "\n";
                        stream << "Name: " << group.name << "\n";
                        stream << "AdminID: " << currentUserId << "\n";
                        stream << "Members: ";
                        for (int i = 0; i < group.members.size(); ++i) {
                            stream << group.members[i];
                            if (i < group.members.size() - 1) {
                                stream << ",";
                            }
                        }
                        stream << "\n";
                        groupFile.close();

                        qDebug() << "Added group info to user's directory:"
                                << memberId;
                    }
                }

                // Update UI if this is the current group
                if (isInGroupChat && currentGroupId == groupIndex) {
                    updateGroupChatArea(groupIndex);
                }

                QMessageBox::information(
                    dialog, "Success",
                    QString("Added %1 member(s) to the group.")
                        .arg(addedUsers.size()));
                dialog->accept();
            } else {
                QMessageBox::warning(
                    dialog, "Error",
                    "Failed to update group. Please try again.");
            }
        });

    // Show dialog
    dialog->exec();
}

void ChatPage::removeMemberFromGroup(int groupIndex, const QString &memberId) {
    if (groupIndex < 0 || groupIndex >= groupList.size())
        return;

    GroupInfo &group = groupList[groupIndex];
    Client *client = server::getInstance()->getCurrentClient();

    if (!client || (client->getUserId() != group.adminId &&
                    client->getUserId() != memberId)) {
        QMessageBox::warning(this, "Error",
                            "Only the group admin can remove members.");
        return;
    }

    // Confirm removal
    QMessageBox::StandardButton confirm = QMessageBox::question(
        this, "Remove Member",
        "Are you sure you want to remove this member from the group?",
        QMessageBox::Yes | QMessageBox::No);

    if (confirm != QMessageBox::Yes) {
        return;
    }

    // Cannot remove the admin
    if (memberId == group.adminId) {
        QMessageBox::warning(this, "Error",
                            "The group admin cannot be removed.");
        return;
    }

    // Remove member from the list
    group.members.removeOne(memberId);

    // Update the group in the database
    if (saveGroupToDatabase(group)) {
        // Remove group from the member's files
        QString userGroupFile =
            "../db/users/" + memberId + "/groups/" + group.groupId + ".txt";
        QFile::remove(userGroupFile);

        // Update UI if this is the current group
        if (isInGroupChat && currentGroupId == groupIndex) {
            updateGroupChatArea(groupIndex);
        }

        QMessageBox::information(this, "Success",
                                "Member has been removed from the group.");
    } else {
        QMessageBox::warning(this, "Error",
                            "Failed to update group. Please try again.");
    }
}

void ChatPage::leaveGroup(int groupIndex) {
    if (groupIndex < 0 || groupIndex >= groupList.size())
        return;

    GroupInfo &group = groupList[groupIndex];
    Client *client = server::getInstance()->getCurrentClient();

    if (!client)
        return;

    QString currentUserId = client->getUserId();

    // Confirm leaving
    QMessageBox::StandardButton confirm = QMessageBox::question(
        this, "Leave Group", "Are you sure you want to leave this group?",
        QMessageBox::Yes | QMessageBox::No);

    if (confirm != QMessageBox::Yes) {
        return;
    }

    // If this user is the admin, ask if they want to delete the group
    if (currentUserId == group.adminId) {
        QMessageBox::StandardButton adminConfirm = QMessageBox::question(
            this, "Delete Group",
            "You are the admin of this group. Leaving will delete the group "
            "for all members. Continue?",
            QMessageBox::Yes | QMessageBox::No);

        if (adminConfirm == QMessageBox::Yes) {
            deleteGroup(groupIndex);
            return;
        } else {
            return; // Don't leave if they don't want to delete
        }
    }

    // Remove user from the group members
    group.members.removeOne(currentUserId);

    // Update the group in the database
    if (saveGroupToDatabase(group)) {
        // Remove group from user's files
        QString userGroupFile = "../db/users/" + currentUserId + "/groups/" +
                                group.groupId + ".txt";
        QFile::remove(userGroupFile);

        // Remove group from our list
        groupList.removeAt(groupIndex);

        // If messages vector exists for this group, remove it
        if (groupIndex < groupMessages.size()) {
            groupMessages.removeAt(groupIndex);
        }

        // Reset current group if we were viewing this group
        if (isInGroupChat && currentGroupId == groupIndex) {
            isInGroupChat = false;
            currentGroupId = -1;
            chatHeader->setText("Select a user");

            // Clear messages
            QLayoutItem *item;
            while ((item = messageLayout->takeAt(0)) != nullptr) {
                if (item->widget()) {
                    delete item->widget();
                }
                delete item;
            }
            messageLayout->addStretch();
        } else if (isInGroupChat && currentGroupId > groupIndex) {
            // Adjust currentGroupId if needed
            currentGroupId--;
        }

        // Update UI
        updateUsersList();

        QMessageBox::information(this, "Success", "You have left the group.");
    } else {
        QMessageBox::warning(this, "Error",
                            "Failed to leave group. Please try again.");
    }
}

void ChatPage::deleteGroup(int groupIndex) {
    if (groupIndex < 0 || groupIndex >= groupList.size())
        return;

    GroupInfo &group = groupList[groupIndex];
    Client *client = server::getInstance()->getCurrentClient();

    if (!client || client->getUserId() != group.adminId) {
        QMessageBox::warning(this, "Error",
                            "Only the group admin can delete the group.");
        return;
    }

    // Confirm deletion
    QMessageBox::StandardButton confirm =
        QMessageBox::question(this, "Delete Group",
                            "Are you sure you want to delete this group? "
                            "This action cannot be undone.",
                            QMessageBox::Yes | QMessageBox::No);

    if (confirm != QMessageBox::Yes) {
        return;
    }

    // Remove group files for all members
    for (const QString &memberId : group.members) {
        QString userGroupFile =
            "../db/users/" + memberId + "/groups/" + group.groupId + ".txt";
        QFile::remove(userGroupFile);
    }

    // Remove group messages
    QString groupMessagesDir = "../db/groups/" + group.groupId;
    QDir dir(groupMessagesDir);
    if (dir.exists()) {
        dir.removeRecursively();
    }

    // Remove group from our list
    groupList.removeAt(groupIndex);

    // If messages vector exists for this group, remove it
    if (groupIndex < groupMessages.size()) {
        groupMessages.removeAt(groupIndex);
    }

    // Reset current group if we were viewing this group
    if (isInGroupChat && currentGroupId == groupIndex) {
        isInGroupChat = false;
        currentGroupId = -1;
        chatHeader->setText("Select a user");

        // Clear messages
        QLayoutItem *item;
        while ((item = messageLayout->takeAt(0)) != nullptr) {
            if (item->widget()) {
                delete item->widget();
            }
            delete item;
        }
        messageLayout->addStretch();
    } else if (isInGroupChat && currentGroupId > groupIndex) {
        // Adjust currentGroupId if needed
        currentGroupId--;
    }

    // Update UI
    updateUsersList();

    QMessageBox::information(this, "Success", "Group has been deleted.");
}

// Helper functions for groups
void ChatPage::updateGroupsList() {
    // This function is not needed separately since updateUsersList handles both
    // users and groups We'll just call the updateUsersList function
    updateUsersList();
}

void ChatPage::loadGroupMessagesForCurrentGroup() {
    qDebug() << "Loading messages for group index:" << currentGroupId;

    if (currentGroupId < 0 || currentGroupId >= groupList.size())
        return;

    Client *client = server::getInstance()->getCurrentClient();
    if (!client)
        return;

    const GroupInfo &group = groupList[currentGroupId];

    // Clear existing messages to prevent duplication
    // We assume groupMessages has already been resized properly by the caller
    groupMessages[currentGroupId].clear();

    // Load messages from the group's messages file
    QFile file("../db/groups/" + group.groupId + "/messages/messages.txt");
    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList parts = line.split("|");

            if (parts.size() >= 3) {
                MessageInfo msgInfo;
                msgInfo.sender = parts[0];
                msgInfo.timestamp =
                    QDateTime::fromString(parts[1], Qt::ISODate);
                msgInfo.text = parts[2];
                msgInfo.isFromMe = (msgInfo.sender == client->getUserId());

                groupMessages[currentGroupId].append(msgInfo);
            }
        }
        file.close();
    }

    qDebug() << "Loaded" << groupMessages[currentGroupId].size()
             << "messages for group" << group.groupId;
}

bool ChatPage::saveGroupToDatabase(const GroupInfo &group) {
    // Create groups directory if it doesn't exist
    QDir dir;
    if (!dir.exists("../db/groups")) {
        dir.mkpath("../db/groups");
    }

    if (!dir.exists("../db/groups/" + group.groupId)) {
        dir.mkpath("../db/groups/" + group.groupId);
    }

    // Update group info in each member's directory
    for (const QString &memberId : group.members) {
        QString userGroupDir = "../db/users/" + memberId + "/groups";
        if (!dir.exists(userGroupDir)) {
            dir.mkpath(userGroupDir);
        }

        // Save group info to a file for this user
        QFile groupFile(userGroupDir + "/" + group.groupId + ".txt");
        if (groupFile.open(QIODevice::WriteOnly)) {
            QTextStream stream(&groupFile);
            stream << "GroupID: " << group.groupId << "\n";
            stream << "Name: " << group.name << "\n";
            stream << "AdminID: " << group.adminId << "\n";
            stream << "Members: ";
            for (int i = 0; i < group.members.size(); ++i) {
                stream << group.members[i];
                if (i < group.members.size() - 1) {
                    stream << ",";
                }
            }
            stream << "\n";
            groupFile.close();
        } else {
            qDebug() << "Failed to save group info for user:" << memberId;
            return false;
        }
    }

    return true;
}

bool ChatPage::loadGroupImage(GroupInfo &group) {
    // Check if group has a custom image
    QString imagePath = "../db/groups/" + group.groupId + "/image.png";
    QFile imageFile(imagePath);

    if (imageFile.exists()) {
        QPixmap image(imagePath);
        if (!image.isNull()) {
            group.groupImage = image;
            group.hasCustomImage = true;
            return true;
        }
    }

    group.hasCustomImage = false;
    return false;
}

void ChatPage::addGroupMessageToUI(const QString &text, const QString &senderId,
                                    const QDateTime &timestamp) {
    QWidget *bubbleRow = new QWidget();
    QHBoxLayout *rowLayout = new QHBoxLayout(bubbleRow);
    rowLayout->setContentsMargins(10, 3, 10, 3);

    Client *client = server::getInstance()->getCurrentClient();
    bool isFromMe = client && senderId == client->getUserId();

    // Get sender's name
    QString senderName = senderId;
    QString nickname, bio;
    if (server::getInstance()->getUserSettings(senderId, nickname, bio)) {
        if (!nickname.isEmpty()) {
            senderName = nickname;
        } else {
            // Extract username from email if possible
            if (senderId.contains("@")) {
                senderName = senderId.split("@").first();
            }
        }
    }

    // Create message content widget
    QWidget *bubbleContent = new QWidget();
    QVBoxLayout *contentLayout = new QVBoxLayout(bubbleContent);
    contentLayout->setContentsMargins(12, 8, 12, 8);
    contentLayout->setSpacing(5);

    // Show sender name at the top of the message
    QLabel *senderLabel = new QLabel(senderName);
    senderLabel->setStyleSheet(
        QString("color: %1; font-size: 12px; font-weight: bold;")
            .arg(isFromMe ? "#7a5ca3" : "#4d6eaa"));
    contentLayout->addWidget(senderLabel);

    // Create message text and timestamp
    QLabel *messageText = new QLabel(text);
    messageText->setWordWrap(true);
    messageText->setTextFormat(Qt::TextFormat::PlainText);
    messageText->setStyleSheet("color: #000000; font-size: 14px;");

    QString timeStr = timestamp.toString("hh:mm");
    QLabel *timestampLabel = new QLabel(timeStr);
    timestampLabel->setStyleSheet(
        "font-size: 10px; color: #666666; margin: 0px 4px;");
    timestampLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // Add text and timestamp in a horizontal layout
    QHBoxLayout *textLayout = new QHBoxLayout();
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(5);
    textLayout->addWidget(messageText);
    textLayout->addWidget(timestampLabel, 0, Qt::AlignBottom);

    contentLayout->addLayout(textLayout);

    // Style the bubble
    QString bubbleColor = isFromMe ? "#DCF8C6" : "#FFFFFF";
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

    bubbleContent->setMaximumWidth(500);
    bubbleContent->setProperty("isFromMe", isFromMe);
    bubbleContent->setProperty("messageText", text);
    bubbleContent->setProperty("timestamp", timestamp);
    bubbleContent->setProperty("sender", senderId);
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

// New method to mark messages as read
void ChatPage::markMessagesAsRead(int userIndex) {
    if (userIndex < 0 || userIndex >= userList.size()) {
        return;
    }

    Client *client = server::getInstance()->getCurrentClient();
    if (!client) {
        return;
    }

    QString senderId = client->getUserId();
    QString recipientId = userList[userIndex].email;
    QString roomId = Room::generateRoomId(senderId, recipientId);
    Room *room = client->getRoom(roomId);
            
    if (room) {
        bool messagesMarkedAsRead = false;
        
        // Get all messages and mark unread ones from the other user as read
        QList<Message> messages = room->getMessages();
        QList<Message> updatedMessages;
        
        for (Message msg : messages) {
            // If this message is from the other user and not read yet
            if (msg.getSender() == recipientId && !msg.getReadStatus()) {
                msg.markAsRead();
                messagesMarkedAsRead = true;
            }
            updatedMessages.append(msg);
        }
        
        if (messagesMarkedAsRead) {
            // Update the room with the modified messages
            room->setMessages(updatedMessages);
            
            // Save the changes to the server and persist
            server::getInstance()->setMessageListFromVector(roomId, room->getMessagesAsVector());
            room->saveMessages();
            
            qDebug() << "Marked messages as read in room:" << roomId;
        }
    }
}

// Add a method to update read receipts for sent messages
void ChatPage::updateReadReceipts() {
    if (currentUserId < 0 || currentUserId >= userList.size()) {
        return;
    }
    
    Client *client = server::getInstance()->getCurrentClient();
    if (!client) {
        return;
    }
    
    QString senderId = client->getUserId();
    QString recipientId = userList[currentUserId].email;
    QString roomId = Room::generateRoomId(senderId, recipientId);
    Room *room = client->getRoom(roomId);
    
    if (!room) {
        return;
    }
    
    // Get all message widgets in current chat
    for (int i = 0; i < messageLayout->count(); i++) {
        QLayoutItem *item = messageLayout->itemAt(i);
        if (!item || !item->widget()) {
            continue;
        }
        
        QWidget *bubbleRow = item->widget();
        QList<QWidget*> bubbles = bubbleRow->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
        
        for (QWidget *bubble : bubbles) {
            // Check if this is a sent message
            if (bubble->property("isFromMe").toBool()) {
                QDateTime timestamp = bubble->property("timestamp").toDateTime();
                bool currentReadStatus = bubble->property("isRead").toBool();
                
                // Find this message in the room to check its current read status
                QList<Message> messages = room->getMessages();
                for (const Message &msg : messages) {
                    if (msg.getTimestamp() == timestamp && msg.getSender() == senderId) {
                        // If read status has changed, update the UI
                        if (msg.getReadStatus() != currentReadStatus) {
                            // Find the read receipt label
                            QList<QLabel*> labels = bubble->findChildren<QLabel*>();
                            for (QLabel *label : labels) {
                                if (label->text() == "✓✓") {
                                    // Update read receipt appearance
                                    if (msg.getReadStatus()) {
                                        label->setStyleSheet("font-size: 12px; color: #4169E1;"); // Blue for read
                                    } else {
                                        label->setStyleSheet("font-size: 12px; color: #A0A0A0;"); // Gray for delivered
                                    }
                                    break;
                                }
                            }
                            
                            // Update the property
                            bubble->setProperty("isRead", msg.getReadStatus());
                        }
                        break;
                    }
                }
            }
        }
    }
}

// Add these missing method implementations after refreshOnlineStatus()
void ChatPage::forceRefreshOnlineStatus() {
    // Force an immediate refresh of online status for all users
    QApplication::setOverrideCursor(Qt::WaitCursor);

    // Call the refresh method directly
    refreshOnlineStatus();

    QApplication::restoreOverrideCursor();

    // Give visual feedback in the chat header if a chat is open
    if (currentUserId >= 0 && currentUserId < userList.size()) {
        QString originalText = chatHeader->text();
        chatHeader->setText("Refreshing online status...");

        // Reset the header after a short delay
        QTimer::singleShot(1000, this, [this, originalText]() {
            chatHeader->setText(originalText);
        });
    }

    qDebug() << "Forced refresh of online status for all users";
}

void ChatPage::startStatusUpdateTimer() {
    if (onlineStatusTimer) {
        if (!onlineStatusTimer->isActive()) {
            onlineStatusTimer->start(3000); // 3 seconds
            qDebug() << "Started online status timer";
        }
    }
}

void ChatPage::stopStatusUpdateTimer() {
    if (onlineStatusTimer) {
        if (onlineStatusTimer->isActive()) {
            onlineStatusTimer->stop();
            qDebug() << "Stopped online status timer";
        }
    }
}

void ChatPage::updateProfileAvatar() {
    if (!profileAvatar)
        return;

    Client *client = server::getInstance()->getCurrentClient();
    if (!client)
        return;

    QString userId = client->getUserId();

    // Always reload the avatar from disk to ensure we get the latest version
    userSettings.hasCustomAvatar = false; // Force reload
    loadAvatarImage();

    // If we have a custom avatar, use it
    if (userSettings.hasCustomAvatar && !userSettings.avatarImage.isNull()) {
        // Create a circular label with the avatar image
        int size = profileAvatar->width();
        QPixmap scaledAvatar = userSettings.avatarImage.scaled(
            size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        // Create a new circular mask
        QPixmap circularAvatar(scaledAvatar.size());
        circularAvatar.fill(Qt::transparent);

        QPainter painter(&circularAvatar);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        QPainterPath path;
        path.addEllipse(0, 0, scaledAvatar.width(), scaledAvatar.height());
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, scaledAvatar);
        painter.end();

        // Set the avatar image
        profileAvatar->setPixmap(circularAvatar);
        profileAvatar->setStyleSheet(
            "border-radius: 25px; border: 2px solid #23233a;");
        qDebug() << "Updated profile avatar to show custom image for user:"
                 << userId;
    } else {
        // Fallback to text-based avatar if no image available
        QString nickname, bio;

        // Get current user's nickname
        if (server::getInstance()->getUserSettings(userId, nickname, bio) &&
            !nickname.isEmpty()) {
            // Use the first letter of nickname if available
            profileAvatar->setText(nickname.left(1).toUpper());
        } else {
            // Fallback to username
            profileAvatar->setText(client->getUsername().left(1).toUpper());
        }

        // Style the text-based avatar
        QString gradientColors =
            "stop:0 #4d6eaa, stop:1 #7a5ca3"; // Blue to purple gradient for
                                              // current user
        profileAvatar->setStyleSheet(QString(R"(
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, %1);
            color: white;
            border-radius: 25px;
            font-weight: bold;
            font-size: 22px;
            border: 2px solid #23233a;
        )")
                                         .arg(gradientColors));

        qDebug() << "Updated profile avatar to show text-based avatar for user:"
                 << userId;
    }

    // Also update the status indicator
    bool isOnline = server::getInstance()->isUserOnline(userId);
    if (profileStatusIndicator) {
        profileStatusIndicator->setStyleSheet(
            QString(R"(
            background-color: %1;
            border-radius: 6px;
            border: 1px solid #23233a;
        )")
                .arg(isOnline
                         ? "#4CAF50"
                         : "#9E9E9E")); // Green for online, gray for offline
    }
}

bool ChatPage::loadAvatarImage() {
    Client *client = server::getInstance()->getCurrentClient();
    if (!client) {
        qDebug() << "Cannot load avatar: No active client";
        return false;
    }

    QString userId = client->getUserId();
    QString nickname, bio, avatarPath;

    // Get avatar path from server
    if (server::getInstance()->getUserSettings(userId, nickname, bio,
                                               avatarPath) &&
        !avatarPath.isEmpty()) {
        // Create a fresh QPixmap instance each time to prevent sharing
        QPixmap avatar(avatarPath);

        if (!avatar.isNull()) {
            // Make a deep copy of the pixmap to ensure we have a completely new
            // instance
            userSettings.avatarImage = QPixmap(avatar);
            userSettings.hasCustomAvatar = true;
            qDebug() << "Loaded avatar image from:" << avatarPath
                     << "for user:" << userId;
            return true;
        } else {
            qDebug() << "Failed to load avatar image from:" << avatarPath
                     << "for user:" << userId;
        }
    }

    // If no avatar or loading failed
    userSettings.hasCustomAvatar = false;
    userSettings.avatarImage = QPixmap(); // Clear any existing avatar
    qDebug() << "No avatar image found for user:" << userId;
    return false;
}

bool ChatPage::saveAvatarImage(const QPixmap &image) {
    if (image.isNull()) {
        qDebug() << "Cannot save null avatar image";
        return false;
    }

    Client *client = server::getInstance()->getCurrentClient();
    if (!client) {
        qDebug() << "Cannot save avatar: No active client";
        return false;
    }

    QString userId = client->getUserId();

    // Clean up old avatar files for this user
    QDir avatarsDir("../db/avatars");
    if (avatarsDir.exists()) {
        // Get all files matching userId prefix
        QStringList nameFilters;
        nameFilters << userId + "_*.png" << userId + "_*.jpg"
                    << userId + "_*.jpeg" << userId + "_*.bmp";
        QStringList oldAvatars = avatarsDir.entryList(nameFilters, QDir::Files);

        qDebug() << "Found" << oldAvatars.size() << "old avatar files for user"
                 << userId;

        // Delete old avatar files
        for (const QString &oldFile : oldAvatars) {
            if (avatarsDir.remove(oldFile)) {
                qDebug() << "Deleted old avatar file:" << oldFile;
            } else {
                qDebug() << "Failed to delete old avatar file:" << oldFile;
            }
        }
    }
    
    // Create avatars directory if it doesn't exist
    QDir dir;
    if (!dir.exists("../db/avatars")) {
        dir.mkpath("../db/avatars");
    }

    // Create a unique filename based on userId and timestamp
    QString avatarFilename =
        "../db/avatars/" + userId + "_" +
        QString::number(QDateTime::currentDateTime().toSecsSinceEpoch()) +
        ".png";

    // Save image to file
    if (image.save(avatarFilename, "PNG")) {
        // Update server data with new avatar path
        if (server::getInstance()->updateUserAvatar(userId, avatarFilename)) {
            // Update local data
            userSettings.avatarImage = image;
            userSettings.hasCustomAvatar = true;

            qDebug() << "Avatar saved to:" << avatarFilename;
            return true;
        } else {
            // Remove the file if server update failed
            QFile::remove(avatarFilename);
            qDebug() << "Failed to update avatar path in server";
        }
    } else {
        qDebug() << "Failed to save avatar image to:" << avatarFilename;
    }
    
    return false;
}

void ChatPage::changeAvatar() {
    // Open file dialog to select image
    QString filePath = QFileDialog::getOpenFileName(
        this, tr("Select Avatar Image"), QDir::homePath(),
        tr("Image Files (*.png *.jpg *.jpeg *.bmp)"));

    if (filePath.isEmpty()) {
        return; // User canceled
    }

    // Load the selected image
    QPixmap originalImage(filePath);
    if (originalImage.isNull()) {
        QMessageBox::warning(this, tr("Error"),
                             tr("Failed to load the selected image."));
        return;
    }

    // Resize if too large (max 200x200)
    QPixmap scaledImage;
    if (originalImage.width() > 200 || originalImage.height() > 200) {
        scaledImage = originalImage.scaled(200, 200, Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation);
    } else {
        scaledImage = originalImage;
    }

    // Make the image square with a circular mask
    int size = qMin(scaledImage.width(), scaledImage.height());
    QPixmap squareImage(size, size);
    squareImage.fill(Qt::transparent);

    QPainter painter(&squareImage);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // Create circular mask
    QPainterPath path;
    path.addEllipse(0, 0, size, size);
    painter.setClipPath(path);

    // Draw the image centered in the square
    int x = (size - scaledImage.width()) / 2;
    int y = (size - scaledImage.height()) / 2;
    painter.drawPixmap(x, y, scaledImage);
    painter.end();

    // Save the processed image
    if (saveAvatarImage(squareImage)) {
        // Update profile avatar
        updateProfileAvatar();

        // Update avatar preview in settings
        if (avatarPreview) {
            QPixmap previewAvatar = squareImage.scaled(
                avatarPreview->width(), avatarPreview->height(),
                Qt::KeepAspectRatio, Qt::SmoothTransformation);
            avatarPreview->setPixmap(previewAvatar);
            avatarPreview->setStyleSheet(
                "border-radius: 50px; border: 2px solid #23233a;");
        }

        // Force an immediate UI refresh by updating users list
        QMessageBox::information(this, tr("Success"),
                                 tr("Avatar has been updated successfully."));

        // Schedule multiple refreshes to ensure all UI elements are updated
        QTimer::singleShot(100, this, &ChatPage::forceRefreshOnlineStatus);
        QTimer::singleShot(500, this, &ChatPage::forceRefreshOnlineStatus);
        QTimer::singleShot(1000, this, &ChatPage::forceRefreshOnlineStatus);
    } else {
        QMessageBox::warning(this, tr("Error"),
                             tr("Failed to save the avatar image."));
    }
}
