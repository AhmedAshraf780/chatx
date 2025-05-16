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
#include <QIcon>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMap>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPixmap>
#include <QPointer>
#include <QProgressDialog>
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
        }
    }

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

    // Set up timer to refresh online status more frequently (every 3 seconds)
    onlineStatusTimer = new QTimer(this);
    connect(onlineStatusTimer, &QTimer::timeout, this,
            &ChatPage::refreshOnlineStatus);
    onlineStatusTimer->start(3000); // 3 seconds instead of 10

    // Make sure profile avatar is properly initialized
    QTimer::singleShot(500, this, &ChatPage::updateProfileAvatar);

    // In the constructor after creating the navigation panel
    // Add this after contentStack is initialized

    // Set up various pages in contentStack
    contentStack->setCurrentIndex(0); // Start with chats view

    // Load initial data
    loadUsersFromDatabase();
    loadGroupsFromDatabase();

    // Initialize navigation history with starting state
    NavigationEntry initialEntry;
    initialEntry.state = NavigationState::CHATS;
    initialEntry.identifier = "";
    navigationHistory.push_back(initialEntry);

    // After loading users in the constructor
    loadUsersFromDatabase();
    loadGroupsFromDatabase();
    updateUsersList();  // Make sure this is called first
    updateGroupsList(); // This will show both users and groups
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

    // Create each navigation button with appropriate icon
    for (int i = 0; i < navItems.size(); i++) {
        QPushButton *navBtn = new QPushButton();
        navBtn->setToolTip(navItems[i]);
        navBtn->setFixedSize(50, 50);

        // Set appropriate icon based on the button type
        if (navItems[i] == "Calls") {
            // Use the call.svg icon
            QPixmap pixmap("/home/loki/Ashraf/progZone/projects/chatXGroup/"
                           "resources/call.svg");
            if (!pixmap.isNull()) {
                QIcon icon(pixmap);
                navBtn->setIcon(icon);
                navBtn->setIconSize(QSize(30, 30));
            } else {
                navBtn->setText("🔊"); // Fallback emoji
            }
        } else if (navItems[i] == "Blocked") {
            // Use the blocked.svg icon
            QPixmap pixmap("/home/loki/Ashraf/progZone/projects/chatXGroup/"
                           "resources/blocked.svg");
            if (!pixmap.isNull()) {
                QIcon icon(pixmap);
                navBtn->setIcon(icon);
                navBtn->setIconSize(QSize(30, 30));
            } else {
                navBtn->setText("⚠️"); // Fallback emoji
            }
        } else if (navItems[i] == "Chat") {
            // Use an emoji for Chat
            navBtn->setText("💬");
        } else if (navItems[i] == "Stories") {
            // Use an emoji for Stories
            navBtn->setText("📷");
        } else if (navItems[i] == "Settings") {
            // Use an emoji for Settings
            navBtn->setText("⚙️");
        }

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
        connect(navBtn, &QPushButton::clicked, [=]() {
            // Update active state of all buttons
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

            // Directly handle navigation based on the button index
            switch (i) {
                case 0: // Chat view
                    contentStack->setCurrentIndex(0);
                    usersSidebar->show();
                    loadUsersFromDatabase();
                    loadGroupsFromDatabase();
                    updateGroupsList(); // Make sure to call this to show groups
                    break;
                case 1: // Stories view
                    contentStack->setCurrentIndex(1);
                    usersSidebar->hide();
                    loadStories();
                    break;
                case 2: // Calls view - skip for now
                    break;
                case 3: // Blocked users view
                    contentStack->setCurrentIndex(3);
                    usersSidebar->hide();
                    loadBlockedUsersPane();
                    break;
                case 4: // Settings view
                    contentStack->setCurrentIndex(2);
                    usersSidebar->hide();
                    loadUserSettings();
                    break;
                default:
                    break;
            }
        });
    }

    navLayout->addStretch();

    // Add logout button at the bottom
    QPushButton *logoutBtn = new QPushButton();
    logoutBtn->setToolTip("Logout");
    logoutBtn->setFixedSize(50, 50);

    // Use the logout SVG icon
    QPixmap logoutPixmap(
        "/home/loki/Ashraf/progZone/projects/chatXGroup/resources/logout.svg");
    if (!logoutPixmap.isNull()) {
        QIcon logoutIcon(logoutPixmap);
        logoutBtn->setIcon(logoutIcon);
        logoutBtn->setIconSize(QSize(30, 30));
    } else {
        logoutBtn->setText("✖️"); // Fallback emoji
    }

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

    sendButton = new QPushButton("Send");
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
    buttonsLayout->addWidget(deleteAccountButton);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(confirmButton);
    settingsLayout->addLayout(buttonsLayout);

    // Add a stretch at the end to push everything to the top
    settingsLayout->addStretch();

    // Connect signals to slots
    connect(confirmButton, &QPushButton::clicked, this,
            &ChatPage::saveUserSettings);
    connect(deleteAccountButton, &QPushButton::clicked, this,
            &ChatPage::deleteUserAccount);

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
    }

    // Navigate back to login page (index 0)
    QStackedWidget *mainStack = qobject_cast<QStackedWidget *>(parent());
    if (mainStack) {
        mainStack->setCurrentIndex(0);
    }
}

void ChatPage::loadUsersFromDatabase() {
    Client *currentClient = server::getInstance()->getCurrentClient();
    if (!currentClient) {
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

    if (allUsers.isEmpty()) {
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
                if (!room->getMessages().isEmpty()) {
                    usersWithMessages.insert(participant);
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
        QString email = user.first;
        QString username = user.second;

        // Skip current user and blocked users
        if (email == currentClient->getUserId() ||
            blockedUsersSet.contains(email)) {
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
        } else {
            regularUsers.append(userInfo);
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

    // Ensure userMessages is initialized for every user
    userMessages.resize(userList.size());

    // IMPORTANT: Clear the list widget BEFORE adding new items
    usersListWidget->clear();

    // Update the UI with the loaded users - this populates the user list widget
    updateUsersList();

    // Debug: Check if the list widget has items now

    // Start the status timer if it's not already running
    startStatusUpdateTimer();
}

void ChatPage::updateUsersList() {

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

    // If we didn't add any users but have users in our list, add them all
    if (countAdded == 0 && !userList.isEmpty()) {

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

/*
 * Contributor: Hossam
 * Function: handleUserSelected
 * Description: Handles selection of a user or group in the sidebar.
 * Displays the chat area for the selected item.
 */
void ChatPage::handleUserSelected(int row) {
    QListWidgetItem *item = usersListWidget->item(row);
    if (!item) return;
    
    // Check if this is a group
    bool isGroup = item->data(Qt::UserRole + 2).toBool();
    
    // Check if this is the "Create Group" special item
    bool isSpecial = item->data(Qt::UserRole + 3).toBool();
    
    if (isSpecial) {
        // Show create group dialog
        showCreateGroupDialog();
        return;
    }
    
    if (isGroup) {
        // Get group index
        int groupIndex = item->data(Qt::UserRole).toInt();
        
        // Update the chat area with group info
        updateGroupChatArea(groupIndex);
    } else {
        // Get user index
        int userIndex = item->data(Qt::UserRole).toInt();
        
        // Update the chat area with user info
        updateChatArea(userIndex);
    }
}

void ChatPage::updateChatArea(int index) {

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

        } else {
        }

        // Notify all group members about the new message
        for (const QString &memberId : group.members) {
            if (memberId != senderId) {
                // Instead of using server->notifyGroupMessage, just log the
                // notification This would be implemented in a real app with
                // proper notifications

                // If the member is online, we could queue a message for them
                // directly For now, this is just a placeholder for the real
                // implementation
            }
        }

        // Update the UI
        updateGroupChatArea(currentGroupId);
    } else {
        // Sending a direct message to a user
        if (currentUserId < 0 || currentUserId >= userList.size()) {
            return;
        }

        Message msg(messageText, senderId);

        // Use email instead of username for consistent room creation
        QString targetEmail = userList[currentUserId].email;

        // If we have an email, use it. Otherwise fall back to name (for
        // backward compatibility)
        QString targetId =
            targetEmail.isEmpty() ? userList[currentUserId].name : targetEmail;

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

        // Make sure the message is also added to the server's in-memory
        // structure
        server *srv = server::getInstance();
        QVector<Message> roomMsgs = room->getMessages();
        srv->updateRoomMessages(room->getRoomId(), roomMsgs);

        // IMPORTANT: Add the room to the recipient's rooms in the server's
        // in-memory structure
        QString roomId = room->getRoomId();

        // First, check if a Client object exists for the recipient
        if (!srv->hasClient(targetId)) {
            // The target user isn't logged in, so add the room to their data
            // manually

            // 1. Make sure the contact list contains the sender
            srv->addContactForUser(targetId, senderId);

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
                if (!targetClient->hasContact(senderId)) {
                    targetClient->addContact(senderId);
                }

                // Add room if needed
                if (!targetClient->getRoom(roomId)) {
                    // Create a room with the same data
                    Room *recipientRoom = new Room(room->getName());
                    recipientRoom->setMessages(roomMsgs);
                    targetClient->addRoom(recipientRoom);
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
    
    // Handle clicks on profile avatar
    if (obj->objectName() == "profileAvatarContainer" && 
        event->type() == QEvent::MouseButtonPress) {
        // Directly show settings page
        contentStack->setCurrentIndex(2);
        usersSidebar->hide();
        loadUserSettings();
        
        // Update navigation buttons
        for (int j = 0; j < navButtons.size(); j++) {
            navButtons[j]->setStyleSheet(
                QString(R"(
                background-color: %1;
                border-radius: 12px;
                color: white;
                font-size: 30px;
            )") 
                    .arg(j == 4 ? "#4d6eaa" : "#2e2e3e")); // Highlight Settings button
        }
        
        return true;
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
                            QVector<Message> messages = room->getMessages();
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
                            QVector<Message> updatedMsgs = room->getMessages();

                            // Update messages in server
                            srv->updateRoomMessages(roomId, updatedMsgs);
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
    if (userIndex < 0 || userIndex >= userList.size()) {
        return;
    }

    UserInfo user = userList[userIndex];
    QMenu *contextMenu = new QMenu(this);

    // Add "Add to contacts" option if not a contact already
    if (!user.isContact) {
        QAction *addContactAction = new QAction("Add to contacts", this);
        connect(addContactAction, &QAction::triggered, [this, userIndex]() {
            addToContacts(userIndex);
        });
        contextMenu->addAction(addContactAction);
    } else {
        // Add "Remove from contacts" option
        QAction *removeContactAction = new QAction("Remove from contacts", this);
        connect(removeContactAction, &QAction::triggered, [this, userIndex]() {
            // Create confirmation dialog
            QDialog *confirmDialog = new QDialog(this);
            confirmDialog->setWindowTitle("Confirm Remove");
            
            QVBoxLayout *layout = new QVBoxLayout(confirmDialog);
            QLabel *label = new QLabel("Are you sure you want to remove " + userList[userIndex].name + " from your contacts?");
            layout->addWidget(label);
            
            QHBoxLayout *buttonLayout = new QHBoxLayout();
            QPushButton *cancelBtn = new QPushButton("Cancel");
            QPushButton *confirmBtn = new QPushButton("Remove");
            confirmBtn->setStyleSheet("background-color: #E15554; color: white;");
            
            buttonLayout->addWidget(cancelBtn);
            buttonLayout->addWidget(confirmBtn);
            layout->addLayout(buttonLayout);
            
            connect(cancelBtn, &QPushButton::clicked, confirmDialog, &QDialog::reject);
            connect(confirmBtn, &QPushButton::clicked, [this, userIndex, confirmDialog]() {
                removeUserFromContacts(userIndex);
                confirmDialog->accept();
            });
            
            // Use dialog stack to manage this dialog
            openDialog(confirmDialog);
        });
        contextMenu->addAction(removeContactAction);
    }

    // Add "Block user" option
    QAction *blockUserAction = new QAction("Block user", this);
    connect(blockUserAction, &QAction::triggered, [this, userIndex]() {
        // Create confirmation dialog
        QDialog *confirmDialog = new QDialog(this);
        confirmDialog->setWindowTitle("Confirm Block");
        
        QVBoxLayout *layout = new QVBoxLayout(confirmDialog);
        QLabel *label = new QLabel("Are you sure you want to block " + userList[userIndex].name + "? They will not be able to message you.");
        layout->addWidget(label);
        
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        QPushButton *cancelBtn = new QPushButton("Cancel");
        QPushButton *confirmBtn = new QPushButton("Block");
        confirmBtn->setStyleSheet("background-color: #E15554; color: white;");
        
        buttonLayout->addWidget(cancelBtn);
        buttonLayout->addWidget(confirmBtn);
        layout->addLayout(buttonLayout);
        
        connect(cancelBtn, &QPushButton::clicked, confirmDialog, &QDialog::reject);
        connect(confirmBtn, &QPushButton::clicked, [this, userIndex, confirmDialog]() {
            blockUser(userIndex);
            confirmDialog->accept();
        });
        
        // Use dialog stack to manage this dialog
        openDialog(confirmDialog);
    });
    contextMenu->addAction(blockUserAction);

    // Show the context menu
    contextMenu->exec(QCursor::pos());
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

    if (currentUserId < 0 || currentUserId >= userList.size())
        return;

    Client *client = server::getInstance()->getCurrentClient();
    if (!client)
        return;

    // Use email instead of username for consistent room lookup
    QString targetEmail = userList[currentUserId].email;
    QString targetName = userList[currentUserId].name;

    // If we have an email, use it. Otherwise fall back to name (for backward
    // compatibility)
    QString targetId = targetEmail.isEmpty() ? targetName : targetEmail;

    Room *room = client->getRoomWithUser(targetId);
    if (!room) {
        // Try with name as fallback
        if (!targetEmail.isEmpty()) {
            room = client->getRoomWithUser(targetName);
        }

        if (!room) {
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
}

void ChatPage::loadUserSettings() {
    Client *client = server::getInstance()->getCurrentClient();
    if (!client) {
        return;
    }

    QString userId = client->getUserId();

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
}

void ChatPage::saveUserSettings() {
    Client *client = server::getInstance()->getCurrentClient();
    if (!client) {
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

        // Save settings to server
        bool nicknameBioSuccess = server::getInstance()->updateUserSettings(
            userId, userSettings.nickname, userSettings.bio);
        bool onlineSuccess = server::getInstance()->setUserOnlineStatus(
            userId, userSettings.isOnline);

        if (nicknameBioSuccess && onlineSuccess) {

            // Update profile avatar
            updateProfileAvatar();

            // If bio or nickname changed, force UI refresh
            if (newNickname != userSettings.nickname ||
                newBio != userSettings.bio) {
                // Force refresh the UI to show updated bio in chat headers
                refreshOnlineStatus();
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

void ChatPage::deleteUserAccount() {
    // Ask for confirmation
    QMessageBox::StandardButton confirm =
        QMessageBox::question(this, "Delete Account",
                              "Are you sure you want to delete your account? "
                              "This action cannot be undone.",
                              QMessageBox::Yes | QMessageBox::No);

    if (confirm == QMessageBox::Yes) {
        server *srv = server::getInstance();
        Client *client = srv->getCurrentClient();
        if (!client) {
            QMessageBox::warning(this, "Error",
                                 "Cannot delete account: No active user");
            return;
        }

        // Store userId before deletion since client will be deleted
        QString userId = client->getUserId();

        try {
            // First, clear all UI-related resources
            if (onlineStatusTimer) {
                onlineStatusTimer->stop();
                onlineStatusTimer->deleteLater();
                onlineStatusTimer = nullptr;
            }

            // Reset various UI states
            userList.clear();
            userMessages.clear();
            groupList.clear();
            groupMessages.clear();
            currentUserId = -1;
            currentGroupId = -1;
            isInGroupChat = false;

            // Clear chat area to prevent any UI access during deletion
            chatHeader->setText("Deleting account...");
            messageInput->setEnabled(false);
            sendButton->setEnabled(false);
            usersListWidget->setEnabled(false);

            // Clear all widgets from chat area
            QLayoutItem *item;
            while ((item = messageLayout->takeAt(0)) != nullptr) {
                if (item->widget()) {
                    delete item->widget();
                }
                delete item;
            }
            messageLayout->addStretch();

            // Process all direct chat rooms related to this user
            QDir roomsDir("../db/rooms");
            QStringList roomFiles = roomsDir.entryList(QDir::Files);

            // First, empty all room files to prevent crashes during Room object
            // deletion
            for (const QString &roomFile : roomFiles) {
                if (roomFile.contains(userId) && roomFile.endsWith(".txt")) {
                    QString roomPath = "../db/rooms/" + roomFile;

                    // Create an empty version of the file so any attempts to
                    // read it won't crash
                    QFile file(roomPath);
                    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        file.close();
                    }
                }
            }

            // Process all group chats this user is part of
            QDir userGroupsDir("../db/users/" + userId + "/groups");
            QStringList groupFiles;
            if (userGroupsDir.exists()) {
                groupFiles = userGroupsDir.entryList(QDir::Files);
            }

            // For each group, update the members list to remove this user
            for (const QString &groupFile : groupFiles) {
                if (groupFile.endsWith(".txt")) {
                    QString groupId = groupFile;
                    groupId.chop(4); // Remove .txt extension

                    // Load group info to update members
                    QFile file("../db/users/" + userId + "/groups/" +
                               groupFile);
                    QString groupAdminId;
                    QStringList groupMembers;
                    QString groupName;

                    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        QTextStream in(&file);
                        while (!in.atEnd()) {
                            QString line = in.readLine();
                            if (line.startsWith("AdminID:")) {
                                groupAdminId = line.mid(8).trimmed();
                            } else if (line.startsWith("Members:")) {
                                QString memberList = line.mid(8).trimmed();
                                groupMembers = memberList.split(",");
                            } else if (line.startsWith("Name:")) {
                                groupName = line.mid(5).trimmed();
                            }
                        }
                        file.close();
                    }

                    // If this user is the admin, delete the entire group
                    if (groupAdminId == userId) {
                        // Remove group directory
                        QDir groupDir("../db/groups/" + groupId);
                        if (groupDir.exists()) {
                            groupDir.removeRecursively();
                        }

                        // Remove group from all members
                        for (const QString &memberId : groupMembers) {
                            if (memberId != userId) {
                                QFile::remove("../db/users/" + memberId +
                                              "/groups/" + groupId + ".txt");
                            }
                        }
                    } else {
                        // Just remove this user from the group members
                        groupMembers.removeAll(userId);

                        // Update group info for all other members
                        for (const QString &memberId : groupMembers) {
                            QFile memberGroupFile("../db/users/" + memberId +
                                                  "/groups/" + groupId +
                                                  ".txt");
                            if (memberGroupFile.open(QIODevice::WriteOnly |
                                                     QIODevice::Text)) {
                                QTextStream out(&memberGroupFile);
                                out << "GroupID: " << groupId << "\n";
                                out << "Name: " << groupName << "\n";
                                out << "AdminID: " << groupAdminId << "\n";
                                out << "Members: ";
                                for (int i = 0; i < groupMembers.size(); ++i) {
                                    out << groupMembers[i];
                                    if (i < groupMembers.size() - 1) {
                                        out << ",";
                                    }
                                }
                                out << "\n";
                                memberGroupFile.close();
                            }
                        }
                    }
                }
            }

            // Use a progress dialog to show that deletion is in progress
            QProgressDialog progress("Deleting account...", "Cancel", 0, 100,
                                     this);
            progress.setWindowModality(Qt::WindowModal);
            progress.setValue(30);
            progress.setCancelButton(nullptr); // No cancel button
            qApp->processEvents(); // Process events to show the dialog

            // Delete user from server (this handles removing them from
            // in-memory structures)
            bool success = srv->deleteUser(userId);
            progress.setValue(60);
            qApp->processEvents();

            if (success) {
                // Clean up additional user resources
                // Remove user directory and all its contents
                QDir userDir("../db/users/" + userId);
                if (userDir.exists()) {
                    userDir.removeRecursively();
                }

                // Remove settings file
                QString settingsFile =
                    "../db/settings/" + userId + "_settings.txt";
                QFile::remove(settingsFile);

                // Remove any blocked users file
                QString blockedFile = "../db/users/" + userId + "_blocked.txt";
                QFile::remove(blockedFile);

                // Remove avatar if exists
                QString avatarFile = "../db/avatars/" + userId + ".png";
                QFile::remove(avatarFile);

                progress.setValue(80);
                qApp->processEvents();

                // Remove user from all contacts lists of other users
                QDir allUsersDir("../db/users");
                QStringList userDirs =
                    allUsersDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                for (const QString &otherUser : userDirs) {
                    QString contactsFile = "../db/users/" + otherUser + ".txt";
                    QFile file(contactsFile);
                    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        // Read the file
                        QStringList lines;
                        QTextStream in(&file);
                        while (!in.atEnd()) {
                            QString line = in.readLine();
                            // Skip contact lines referencing the deleted user
                            if (!line.startsWith("CONTACT:" + userId)) {
                                lines.append(line);
                            }
                        }
                        file.close();

                        // Write back without the deleted user
                        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                            QTextStream out(&file);
                            for (const QString &line : lines) {
                                out << line << "\n";
                            }
                            file.close();
                        }
                    }
                }

                // Clean up private chat rooms - now just delete the files since
                // we've already emptied them
                for (const QString &roomFile : roomFiles) {
                    // Check if room involves the deleted user
                    if (roomFile.contains(userId) &&
                        roomFile.endsWith(".txt")) {
                        QString roomPath = "../db/rooms/" + roomFile;
                        QFile::remove(roomPath);
                    }
                }

                // Remove stories created by this user
                QDir storiesDir("../db/stories");
                QStringList storyFiles = storiesDir.entryList(QDir::Files);
                for (const QString &storyFile : storyFiles) {
                    if (storyFile.startsWith(userId) &&
                        (storyFile.endsWith(".meta") ||
                         storyFile.endsWith(".jpg") ||
                         storyFile.endsWith(".png"))) {
                        QString storyPath = "../db/stories/" + storyFile;
                        QFile::remove(storyPath);
                    }
                }

                // Clean up credentials file manually to ensure user is removed
                QString credentialsPath =
                    "../db/users_credentials/credentials.txt";
                QFile credFile(credentialsPath);
                if (credFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QStringList lines;
                    QTextStream in(&credFile);
                    while (!in.atEnd()) {
                        QString line = in.readLine();
                        if (!line.startsWith(userId + ",")) {
                            lines.append(line);
                        }
                    }
                    credFile.close();

                    if (credFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        QTextStream out(&credFile);
                        for (const QString &line : lines) {
                            out << line << "\n";
                        }
                        credFile.close();
                    }
                }

                progress.setValue(100);
                qApp->processEvents();

                QMessageBox::information(
                    this, "Account Deleted",
                    "Your account has been deleted successfully.");

                // Use a custom logout approach that doesn't try to restore
                // status Clear UI without restoring online status
                chatHeader->setText("Select a user");
                usersListWidget->clear();

                // Navigate back to login page
                QStackedWidget *mainStack =
                    qobject_cast<QStackedWidget *>(parent());
                if (mainStack) {
                    mainStack->setCurrentIndex(0);
                }
            } else {
                progress.close();
                QMessageBox::warning(
                    this, "Error",
                    "Failed to delete account. Please try again later.");
            }
        } catch (const std::exception &e) {
            QMessageBox::critical(
                this, "Error",
                QString("An error occurred during account deletion: %1")
                    .arg(e.what()));
        }
    }
}

void ChatPage::onlineStatusChanged(int state) {
    bool isOnline = (state == Qt::Checked);

    Client *client = server::getInstance()->getCurrentClient();
    if (!client) {
        return;
    }

    QString userId = client->getUserId();
    userSettings.isOnline = isOnline;

    // Show a small indicator that the change is being processed
    QApplication::setOverrideCursor(Qt::WaitCursor);

    // Update the server with the new status immediately
    bool success = server::getInstance()->setUserOnlineStatus(userId, isOnline);

    if (success) {

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
            }
        }
    }

    // Print diagnostic message if status changed
    if (statusChanged) {
    } else {
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
        }

        // Update profile avatar and status indicator
        updateProfileAvatar();
    }
}

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
}

void ChatPage::startStatusUpdateTimer() {
    if (onlineStatusTimer) {
        if (!onlineStatusTimer->isActive()) {
            onlineStatusTimer->start(3000); // 3 seconds
        }
    }
}

void ChatPage::stopStatusUpdateTimer() {
    if (onlineStatusTimer) {
        if (onlineStatusTimer->isActive()) {
            onlineStatusTimer->stop();
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
            return true;
        }
    }

    // If no avatar or loading failed
    userSettings.hasCustomAvatar = false;
    userSettings.avatarImage = QPixmap(); // Clear any existing avatar
    return false;
}

bool ChatPage::saveAvatarImage(const QPixmap &image) {
    if (image.isNull()) {
        return false;
    }

    Client *client = server::getInstance()->getCurrentClient();
    if (!client) {
        return false;
    }

    QString userId = client->getUserId();

    // Clean up old avatar files for this user
    QDir avatarsDir("../db/avatars");
    if (avatarsDir.exists()) {
        // Get all files matching userId prefix
        QStringList nameFilters;
        QStringList oldAvatars = avatarsDir.entryList(nameFilters, QDir::Files);

        // Delete old avatar files
        for (const QString &oldFile : oldAvatars) {
            if (avatarsDir.remove(oldFile)) {
            } else {
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

            return true;
        } else {
            // Remove the file if server update failed
            QFile::remove(avatarFilename);
        }
    } else {
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

        // Only add stories from other users or your own unviewed stories 
        // to prevent clutter
        if (storyData.userId != client->getUserId() || !info.viewed) {
            userStories.append(info);
        }
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
    QString storyId =
        story.userId + "_" + story.timestamp.toString("yyyyMMdd_hhmmss");

    // Get viewers info and exclude creator if viewing their own story
    QSet<QString> viewers = server::getInstance()->getStoryViewers(storyId);
    
    // Check if current user is the story owner
    Client *client = server::getInstance()->getCurrentClient();
    bool isStoryOwner = (client && client->getUserId() == story.userId);
    
    // Don't count current user in viewers if they are the story creator
    int viewerCount = viewers.size();
    if (isStoryOwner && viewers.contains(client->getUserId())) {
        viewerCount--;
    }

    detailsLayout->addWidget(usernameLabel);
    detailsLayout->addWidget(timestampLabel);
    detailsLayout->addWidget(captionLabel);
    
    // Only add viewer count label if current user is the story owner
    if (isStoryOwner) {
        QLabel *viewerCountLabel =
            new QLabel(QString("👁️ %1 views").arg(viewerCount));
        viewerCountLabel->setStyleSheet("color: #6e6e8e; font-size: 12px;");
        detailsLayout->addWidget(viewerCountLabel);
    }

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

    // Only create and add viewers widget if the current user is the story owner
    bool isStoryOwner = (client && client->getUserId() == story.userId);
    
    // Create viewers widget
    QWidget *viewersWidget = nullptr;
    
    if (isStoryOwner) {
        // Create viewers section
        viewersWidget = new QWidget();
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
    } else { // Remove this entire else block
        // For non-owners, we don't show any viewer information at all
    }

    dialogLayout->addWidget(headerWidget);
    dialogLayout->addWidget(storyImage, 1);
    dialogLayout->addWidget(captionLabel);
    
    // Only add viewers widget if it was created (for the story owner)
    if (viewersWidget) {
        dialogLayout->addWidget(viewersWidget);
    }

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
    openDialog(storyDialog);
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
    // Clear any previously selected members
    QStringList selectedMembers;
    
    // Create dialog
    QDialog *groupDialog = new QDialog(this);
    groupDialog->setWindowTitle("Create Group");
    groupDialog->setMinimumSize(400, 500);
    groupDialog->setStyleSheet("background-color: #12121a; color: white;");
    
    QVBoxLayout *dialogLayout = new QVBoxLayout(groupDialog);
    dialogLayout->setSpacing(15);
    
    // Group name input
    QLabel *nameLabel = new QLabel("Group Name:");
    QLineEdit *nameInput = new QLineEdit();
    nameInput->setStyleSheet(R"(
        QLineEdit {
            background-color: #1d1d2b;
            border-radius: 8px;
            padding: 10px;
            color: white;
            border: 1px solid #3d3d4b;
        }
        QLineEdit:focus {
            border: 1px solid #4d6eaa;
        }
    )");
    
    // Users list
    QLabel *usersLabel = new QLabel("Select Members:");
    QListWidget *userSelectionList = new QListWidget();
    userSelectionList->setStyleSheet(R"(
        QListWidget {
            background-color: #1d1d2b;
            border-radius: 8px;
            padding: 5px;
            color: white;
            border: 1px solid #3d3d4b;
        }
        QListWidget::item {
            border-bottom: 1px solid #2d2d3b;
            padding: 5px;
        }
        QListWidget::item:selected {
            background-color: #4d6eaa;
        }
    )");
    
    userSelectionList->setSelectionMode(QAbstractItemView::NoSelection);
    
    // Add users to the list
    Client *client = server::getInstance()->getCurrentClient();
    if (client) {
        QVector<UserInfo> allUsers = userList;
        
        // Get blocked users to exclude them
        QVector<QString> blockedUsers = server::getInstance()->getBlockedUsers(client->getUserId());
        
        for (const UserInfo &user : allUsers) {
            // Skip self and blocked users
            if (user.email == client->getUserId() || blockedUsers.contains(user.email)) {
                continue;
            }
            
            // Create list item
            QListWidgetItem *item = new QListWidgetItem();
            userSelectionList->addItem(item);
            
            // Create widget for the item
            QWidget *itemWidget = new QWidget();
            QHBoxLayout *itemLayout = new QHBoxLayout(itemWidget);
            itemLayout->setContentsMargins(5, 5, 5, 5);
            itemLayout->setSpacing(8);

            // Create avatar
            QLabel *avatar = new QLabel();
            avatar->setFixedSize(36, 36);
            avatar->setAlignment(Qt::AlignCenter);
            avatar->setStyleSheet("background-color: #4d6eaa; border-radius: 18px;");
            
            // Get avatar path or use first letter
            QString avatarPath = server::getInstance()->getUserAvatar(user.email);
            if (!avatarPath.isEmpty() && QFile::exists(avatarPath)) {
                QPixmap userAvatar(avatarPath);
                avatar->setPixmap(userAvatar.scaled(36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            } else {
                avatar->setText(user.name.left(1).toUpper());
                avatar->setStyleSheet("background-color: #4d6eaa; border-radius: 18px; color: white; font-weight: bold;");
            }
            
            // Create checkbox with username
            QCheckBox *checkbox = new QCheckBox(user.name);
            checkbox->setProperty("email", user.email);
            
            // Use a direct connection instead of a lambda
            connect(checkbox, &QCheckBox::toggled, groupDialog, [&selectedMembers, email=user.email](bool checked) {
                if (checked) {
                    if (!selectedMembers.contains(email)) {
                        selectedMembers.append(email);
                    }
                } else {
                    selectedMembers.removeAll(email);
                }
            });
            
            // Add to layout
            itemLayout->addWidget(avatar);
            itemLayout->addWidget(checkbox, 1);
            itemWidget->setLayout(itemLayout);
            
            // Set the widget for this item
            userSelectionList->setItemWidget(item, itemWidget);
        }
    }

    // Create and cancel buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);

    QPushButton *cancelButton = new QPushButton("Cancel");
    cancelButton->setStyleSheet(R"(
        QPushButton {
            background-color: #E15554;
            color: white;
            border-radius: 8px;
            padding: 10px 20px;
            font-size: 15px;
        }
        QPushButton:hover {
            background-color: #D14545;
        }
    )");
    
    QPushButton *createButton = new QPushButton("Create Group");
    createButton->setStyleSheet(R"(
        QPushButton {
            background-color: #4d6eaa;
            color: white;
            border-radius: 8px;
            padding: 10px 20px;
            font-size: 15px;
        }
        QPushButton:hover {
            background-color: #3d5e9a;
        }
    )");
    
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(createButton);
    
    // Add widgets to dialog layout
    dialogLayout->addWidget(nameLabel);
    dialogLayout->addWidget(nameInput);
    dialogLayout->addWidget(usersLabel);
    dialogLayout->addWidget(userSelectionList);
    dialogLayout->addLayout(buttonLayout);
    
    // Connect button signals
    connect(cancelButton, &QPushButton::clicked, groupDialog, &QDialog::reject);
    
    connect(createButton, &QPushButton::clicked, [this, nameInput, &selectedMembers, groupDialog, dialogLayout]() {
        QString groupName = nameInput->text().trimmed();
        if (groupName.isEmpty()) {
            // Show error message
            QLabel *errorLabel = new QLabel("Please enter a group name");
            errorLabel->setStyleSheet("color: #E15554;");
            dialogLayout->insertWidget(2, errorLabel);
            QTimer::singleShot(3000, errorLabel, &QLabel::deleteLater);
            return;
        }
        
        if (selectedMembers.isEmpty()) {
            // Show error message
            QLabel *errorLabel = new QLabel("Please select at least one member");
            errorLabel->setStyleSheet("color: #E15554;");
            dialogLayout->insertWidget(4, errorLabel);
            QTimer::singleShot(3000, errorLabel, &QLabel::deleteLater);
            return;
        }
        
        // Create the group - make a safe copy of the members
        QStringList membersCopy = selectedMembers;
        // Double-check to make sure the copy is valid
        if (membersCopy.isEmpty()) {
            membersCopy.append(server::getInstance()->getCurrentClient()->getUserId());
        }
        
        createNewGroup(groupName, membersCopy);
        groupDialog->accept();
    });
    
    // Show as a modal dialog instead of using our stack
    groupDialog->exec();
}

/*
 * Contributor: Hossam
 * Function: navigateToState
 * Description: Implements navigation history tracking using std::list.
 * This is part of the college requirement to demonstrate STL containers.
 * Pushes the current state to navigation history and moves to a new state.
 */
void ChatPage::navigateToState(NavigationState state, const QString &identifier) {
    // Create a navigation entry for the current state before navigating
    if (!navigationHistory.empty()) {
        // Don't add duplicate entries
        NavigationEntry &currentEntry = navigationHistory.back();
        if (currentEntry.state == state && 
            currentEntry.identifier == identifier.toStdString()) {
            return;
        }
    }
    
    // Create a new entry
    NavigationEntry entry;
    entry.state = state;
    entry.identifier = identifier.toStdString();
    
    // Add to navigation history
    navigationHistory.push_back(entry);
    
    // Implement the actual navigation logic based on the state
    switch (state) {
        case NavigationState::CHATS:
            contentStack->setCurrentIndex(0); // Chats page
            break;
            
        case NavigationState::STORIES:
            contentStack->setCurrentIndex(1); // Stories page
            loadStories();
            break;
            
        case NavigationState::SETTINGS:
            contentStack->setCurrentIndex(2); // Settings page
            loadUserSettings();
            break;
            
        case NavigationState::BLOCKED_USERS:
            contentStack->setCurrentIndex(3); // Blocked users page
            loadBlockedUsersPane();
            break;
            
        case NavigationState::CHAT_DETAIL:
            contentStack->setCurrentIndex(0); // Back to chats page
            // Find and select the user with the given identifier
            for (int i = 0; i < userList.size(); i++) {
                if (userList[i].email == identifier) {
                    handleUserSelected(i);
                    break;
                }
            }
            break;
            
        case NavigationState::GROUP_DETAIL:
            contentStack->setCurrentIndex(0); // Back to chats page
            // Find and select the group with the given identifier
            for (int i = 0; i < groupList.size(); i++) {
                if (groupList[i].groupId == identifier) {
                    updateGroupChatArea(i);
                    break;
                }
            }
            break;
            
        default:
            break;
    }
}

/*
 * Contributor: Hossam
 * Function: goBack
 * Description: Implements navigation history tracking using std::list.
 * This is part of the college requirement to demonstrate STL containers.
 * Pops the current state from navigation history and returns to the previous state.
 */
void ChatPage::goBack() {
    if (navigationHistory.size() <= 1) {
        // If we're at the root or have no history, do nothing
        return;
    }
    
    // Remove the current state
    navigationHistory.pop_back();
    
    // Get the previous state
    NavigationEntry prevEntry = navigationHistory.back();
    
    // Remove this entry as well, as we'll add it back with navigateToState
    navigationHistory.pop_back();
    
    // Navigate to the previous state
    navigateToState(prevEntry.state, QString::fromStdString(prevEntry.identifier));
}

/*
 * Contributor: Hossam
 * Function: openDialog
 * Description: Implements dialog management using std::stack.
 * This is part of the college requirement to demonstrate STL containers.
 * Pushes a new dialog onto the stack and shows it.
 */
void ChatPage::openDialog(QDialog* dialog) {
    if (!dialog) return;
    
    // Connect the dialog's finished signal to closeTopDialog
    connect(dialog, &QDialog::finished, this, &ChatPage::closeTopDialog);
    
    // Create a shared_ptr to manage the dialog
    std::shared_ptr<QDialog> dialogPtr(dialog);
    
    // Push onto the stack
    dialogStack.push(dialogPtr);
    
    // Show the dialog
    dialog->show();
}

/*
 * Contributor: Hossam
 * Function: closeTopDialog
 * Description: Implements dialog management using std::stack.
 * This is part of the college requirement to demonstrate STL containers.
 * Pops the top dialog from the stack when it's closed.
 */
void ChatPage::closeTopDialog() {
    if (!dialogStack.empty()) {
        // Get the dialog from the stack
        std::shared_ptr<QDialog> dialogPtr = dialogStack.top();
        dialogStack.pop();
        
        // Close the dialog
        if (dialogPtr) {
            dialogPtr->reject();
        }
    }
}

/*
 * Contributor: Hossam
 * Function: loadGroupsFromDatabase
 * Description: Loads group chat data from the database.
 * Initializes the group list and messages in the UI.
 */
void ChatPage::loadGroupsFromDatabase() {
    // Clear existing groups
    groupList.clear();
    groupMessages.clear();
    
    // Get current client
    Client *client = server::getInstance()->getCurrentClient();
    if (!client) return;
    
    QString userId = client->getUserId();
    
    // Create groups directory if it doesn't exist
    QDir dir;
    dir.mkpath("../db/groups");
    
    // Check for existing groups
    QDir groupsDir("../db/groups");
    QStringList groupDirs = groupsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    
    // For each group directory, check if the current user is a member
    for (const QString &groupId : groupDirs) {
        QString membersPath = "../db/groups/" + groupId + "/members.txt";
        QFile membersFile(membersPath);
        
        if (membersFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&membersFile);
            bool isUserMember = false;
            QString adminId;
            QStringList members;
            
            // Read all members
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (line.startsWith("ADMIN:")) {
                    adminId = line.mid(6).trimmed();
                    members.append(adminId);
                    if (adminId == userId) {
                        isUserMember = true;
                    }
                } else if (!line.isEmpty()) {
                    members.append(line);
                    if (line == userId) {
                        isUserMember = true;
                    }
                }
            }
            membersFile.close();
            
            // If user is a member, load the group
            if (isUserMember) {
                // Read group info
                QString infoPath = "../db/groups/" + groupId + "/info.txt";
                QFile infoFile(infoPath);
                
                GroupInfo group;
                group.groupId = groupId;
                group.name = "Group " + groupId; // Default name
                group.adminId = adminId.isEmpty() ? userId : adminId;
                group.members = members;
                group.hasMessages = false;
                group.lastMessage = "";
                group.lastSeen = "";
                group.hasCustomImage = false;
                
                if (infoFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QTextStream infoIn(&infoFile);
                    
                    while (!infoIn.atEnd()) {
                        QString line = infoIn.readLine().trimmed();
                        if (line.startsWith("NAME:")) {
                            group.name = line.mid(5).trimmed();
                        } else if (line.startsWith("IMAGE:")) {
                            group.hasCustomImage = true;
                            QString imagePath = line.mid(6).trimmed();
                            if (!imagePath.isEmpty() && QFile::exists(imagePath)) {
                                group.groupImage = QPixmap(imagePath);
                            }
                        }
                    }
                    infoFile.close();
                }
                
                // Load messages
                QString messagesPath = "../db/groups/" + groupId + "/messages/messages.txt";
                QFile messagesFile(messagesPath);
                
                QVector<MessageInfo> messages;
                
                if (messagesFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QTextStream msgIn(&messagesFile);
                    
                    while (!msgIn.atEnd()) {
                        QString line = msgIn.readLine().trimmed();
                        if (!line.isEmpty()) {
                            QStringList parts = line.split("|");
                            if (parts.size() >= 3) {
                                MessageInfo msg;
                                msg.sender = parts[0];
                                msg.timestamp = QDateTime::fromString(parts[1], Qt::ISODate);
                                msg.text = parts[2];
                                msg.isFromMe = (msg.sender == userId);
                                
                                messages.append(msg);
                                
                                // Update last message info
                                group.hasMessages = true;
                                group.lastMessage = msg.text;
                                
                                // Calculate "last seen" text
                                QDateTime now = QDateTime::currentDateTime();
                                qint64 secsAgo = msg.timestamp.secsTo(now);
                                
                                if (secsAgo < 60) {
                                    group.lastSeen = "Just now";
                                } else if (secsAgo < 3600) {
                                    group.lastSeen = QString::number(secsAgo / 60) + " min ago";
                                } else if (secsAgo < 86400) {
                                    group.lastSeen = QString::number(secsAgo / 3600) + " hours ago";
                                } else {
                                    group.lastSeen = msg.timestamp.toString("MMM d");
                                }
                            }
                        }
                    }
                    messagesFile.close();
                }
                
                // Add group to list
                groupList.append(group);
                
                // Add messages to group messages
                groupMessages.append(messages);
            }
        }
    }
    
    // If we're starting fresh, create sample groups
    if (groupList.isEmpty()) {
        // Create a sample group with the current user if needed
        GroupInfo sampleGroup;
        sampleGroup.groupId = QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch());
        sampleGroup.name = "My Group Chat";
        sampleGroup.adminId = userId;
        sampleGroup.members.append(userId);
        
        // Try to add one other user if available
        for (const UserInfo &user : userList) {
            if (user.email != userId) {
                sampleGroup.members.append(user.email);
                break;
            }
        }
        
        // Only add if we have more than just ourselves
        if (sampleGroup.members.size() > 1) {
            groupList.append(sampleGroup);
            groupMessages.append(QVector<MessageInfo>());
            
            // Save this group to database
            saveGroupToDatabase(sampleGroup);
        }
    }

    qDebug() << "Loaded" << groupList.size() << "groups";
    updateGroupsList();
    updateUsersList();
}

/*
 * Contributor: Hossam
 * Function: createGroupListItem
 * Description: Creates a UI list item for a group chat.
 * Part of the UI rendering for group chats in the sidebar.
 */
QListWidgetItem* ChatPage::createGroupListItem(const GroupInfo &group, int index) {
    QListWidgetItem *item = new QListWidgetItem;
    item->setData(Qt::UserRole, index);
    item->setData(Qt::UserRole + 2, true); // Mark as group
    
    // Create widget for item
    QWidget *widget = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(8);
    
    // Create avatar for the group
    QLabel *avatar = new QLabel();
    avatar->setFixedSize(40, 40);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setStyleSheet("background-color: #4d6eaa; border-radius: 20px; color: white; font-weight: bold;");
    
    if (group.hasCustomImage && !group.groupImage.isNull()) {
        QPixmap scaledPixmap = group.groupImage.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        avatar->setPixmap(scaledPixmap);
    } else {
        avatar->setText(group.name.left(1).toUpper());
    }
    
    // Create info widget (name, last message, etc.)
    QWidget *infoWidget = new QWidget();
    QVBoxLayout *infoLayout = new QVBoxLayout(infoWidget);
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setSpacing(2);
    
    // Group name
    QLabel *nameLabel = new QLabel(group.name);
    nameLabel->setStyleSheet("color: white; font-weight: bold;");
    
    // Last message with time
    QWidget *lastMessageWidget = new QWidget();
    QHBoxLayout *lastMessageLayout = new QHBoxLayout(lastMessageWidget);
    lastMessageLayout->setContentsMargins(0, 0, 0, 0);
    lastMessageLayout->setSpacing(5);
    
    QLabel *lastMessageLabel = new QLabel(group.hasMessages ? group.lastMessage : "No messages yet");
    lastMessageLabel->setStyleSheet("color: #9e9e9e; font-size: 12px;");
    lastMessageLabel->setMaximumWidth(120);
    
    QLabel *timeLabel = new QLabel(group.hasMessages ? group.lastSeen : "");
    timeLabel->setStyleSheet("color: #9e9e9e; font-size: 11px;");
    timeLabel->setAlignment(Qt::AlignRight);
    
    lastMessageLayout->addWidget(lastMessageLabel);
    lastMessageLayout->addWidget(timeLabel, 0, Qt::AlignRight);
    
    infoLayout->addWidget(nameLabel);
    infoLayout->addWidget(lastMessageWidget);
    
    // Members count badge
    QLabel *memberCount = new QLabel(QString::number(group.members.size()));
    memberCount->setFixedSize(24, 24);
    memberCount->setAlignment(Qt::AlignCenter);
    memberCount->setStyleSheet("background-color: #2e2e4e; color: white; border-radius: 12px; font-size: 11px;");
    
    // Add to layout
    layout->addWidget(avatar);
    layout->addWidget(infoWidget, 1);
    layout->addWidget(memberCount);
    
    // Set appropriate size
    item->setSizeHint(QSize(usersListWidget->width(), 60));
    
    // Store widget in item
    item->setData(Qt::UserRole + 1, QVariant::fromValue(widget));
    
    return item;
}

/*
 * Contributor: Hossam
 * Function: saveGroupToDatabase
 * Description: Saves a group to the database.
 * Creates necessary directories and files for group data persistence.
 */
bool ChatPage::saveGroupToDatabase(const GroupInfo &group) {
    // Create directories if they don't exist
    QDir dir;
    dir.mkpath("../db/groups/" + group.groupId);
    dir.mkpath("../db/groups/" + group.groupId + "/messages");
    
    // Save group info
    QString infoPath = "../db/groups/" + group.groupId + "/info.txt";
    QFile infoFile(infoPath);
    
    if (infoFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&infoFile);
        out << "NAME:" << group.name << "\n";
        
        // Save image path if available
        if (group.hasCustomImage && !group.groupImage.isNull()) {
            QString imagePath = "../db/groups/" + group.groupId + "/image.png";
            group.groupImage.save(imagePath, "PNG");
            out << "IMAGE:" << imagePath << "\n";
        }
        
        infoFile.close();
    }
    
    // Save members
    QString membersPath = "../db/groups/" + group.groupId + "/members.txt";
    QFile membersFile(membersPath);
    
    if (membersFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&membersFile);
        
        // First write admin
        out << "ADMIN:" << group.adminId << "\n";
        
        // Then write other members
        for (const QString &member : group.members) {
            if (member != group.adminId) {
                out << member << "\n";
            }
        }
        
        membersFile.close();
    }
}

/*
 * Contributor: Hossam
 * Function: updateGroupChatArea
 * Description: Updates the chat area when a group is selected.
 * Displays the group name and messages in the main chat window.
 */
void ChatPage::updateGroupChatArea(int index) {
    if (index < 0 || index >= groupList.size())
        return;
    
    const GroupInfo &group = groupList[index];
    
    // Update header with group info
    QString headerText = "Group: " + group.name + " (" + QString::number(group.members.size()) + " members)";
    chatHeader->setText(headerText);
    
    // Clear existing messages
    QLayoutItem *item;
    while ((item = messageLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
    messageLayout->addStretch(); // Re-add stretch to push messages up
    
    // Add messages to UI
    if (index < groupMessages.size()) {
        const QVector<MessageInfo> &messages = groupMessages[index];
        
        for (const MessageInfo &msg : messages) {
            addGroupMessageToUI(msg.text, msg.sender, msg.timestamp);
        }
    }
    
    // Add a "show options" button to the header
    if (!chatHeader->actions().isEmpty()) {
        // Remove existing actions first
        for (QAction *action : chatHeader->actions()) {
            chatHeader->removeAction(action);
        }
    }
    
    QAction *showOptionsAction = new QAction("📝", this);
    showOptionsAction->setToolTip("Group Options");
    connect(showOptionsAction, &QAction::triggered, [this, index]() {
        showGroupOptions(index);
    });
    
    chatHeader->addAction(showOptionsAction);
    
    // Scroll to bottom
    QScrollBar *vScrollBar = messageArea->verticalScrollBar();
    vScrollBar->setValue(vScrollBar->maximum());
}

/*
 * Contributor: Hossam
 * Function: addGroupMessageToUI
 * Description: Adds a message to the group chat UI.
 * Shows sender name and formats the message appropriately.
 */
void ChatPage::addGroupMessageToUI(const QString &text, const QString &sender, const QDateTime &timestamp) {
    if (text.isEmpty())
        return;
    
    // Get current client
    Client *client = server::getInstance()->getCurrentClient();
    if (!client)
        return;
    
    // Check if this is from current user
    bool isFromMe = (sender == client->getUserId());
    
    // Get sender's display name
    QString senderName = sender;
    // Try to get a friendly name from the server
    QString nickname, bio;
    if (server::getInstance()->getUserSettings(sender, nickname, bio) && !nickname.isEmpty()) {
        senderName = nickname;
    } else {
        // Try to find in user list
        for (const UserInfo &user : userList) {
            if (user.email == sender) {
                senderName = user.name;
                break;
            }
        }
    }
    
    // Create message bubble
    QWidget *bubble = new QWidget();
    bubble->setObjectName("messageBubble");
    bubble->setProperty("fromMe", isFromMe);
    
    QVBoxLayout *bubbleLayout = new QVBoxLayout(bubble);
    bubbleLayout->setContentsMargins(10, 8, 10, 8);
    bubbleLayout->setSpacing(2);
    
    // Add sender name if not from me
    if (!isFromMe) {
        QLabel *senderLabel = new QLabel(senderName);
        senderLabel->setStyleSheet("color: #65a9e0; font-weight: bold; font-size: 12px;");
        bubbleLayout->addWidget(senderLabel);
    }
    
    // Message text
    QLabel *textLabel = new QLabel(text);
    textLabel->setWordWrap(true);
    textLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    textLabel->setCursor(Qt::IBeamCursor);
    textLabel->setStyleSheet("color: white;");
    
    // Time
    QLabel *timeLabel = new QLabel(timestamp.toString("hh:mm"));
    timeLabel->setStyleSheet("color: rgba(255,255,255,0.5); font-size: 10px;");
    timeLabel->setAlignment(Qt::AlignRight);
    
    bubbleLayout->addWidget(textLabel);
    bubbleLayout->addWidget(timeLabel);
    
    // Style the bubble based on sender
    if (isFromMe) {
        bubble->setStyleSheet(R"(
            #messageBubble {
                background-color: #4d6eaa;
                border-radius: 15px;
                border-top-right-radius: 5px;
                padding: 5px;
            }
        )");
    } else {
        bubble->setStyleSheet(R"(
            #messageBubble {
                background-color: #3e3e5e;
                border-radius: 15px;
                border-top-left-radius: 5px;
                padding: 5px;
            }
        )");
    }
    
    // Create container for alignment
    QWidget *container = new QWidget();
    QHBoxLayout *containerLayout = new QHBoxLayout(container);
    containerLayout->setContentsMargins(10, 0, 10, 0);
    
    // Add the bubble to the left or right depending on sender
    if (isFromMe) {
        containerLayout->addStretch();
        containerLayout->addWidget(bubble);
    } else {
        containerLayout->addWidget(bubble);
        containerLayout->addStretch();
    }
    
    // Add the container to the message area
    messageLayout->insertWidget(messageLayout->count() - 1, container);
}

/*
 * Contributor: Hossam
 * Function: addMemberToGroup
 * Description: Adds a new member to an existing group.
 * Opens a dialog to select a user and adds them to the group.
 */
void ChatPage::addMemberToGroup(int groupIndex) {
    if (groupIndex < 0 || groupIndex >= groupList.size())
        return;
    
    // Simple implementation to add the first available user who is not already in the group
    GroupInfo &group = groupList[groupIndex];
    Client *client = server::getInstance()->getCurrentClient();
    if (!client) return;
    
    // Get blocked users
    QVector<QString> blockedUsers = server::getInstance()->getBlockedUsers(client->getUserId());
    QSet<QString> blockedUsersSet(blockedUsers.begin(), blockedUsers.end());
    
    // Create a set of existing members for fast lookup
    QSet<QString> existingMembers(group.members.begin(), group.members.end());
    
    // Find a user to add
    for (const UserInfo &user : userList) {
        if (!existingMembers.contains(user.email) && !blockedUsersSet.contains(user.email)) {
            // Add user to the group
            group.members.append(user.email);
            
            // Save updated group to database
            saveGroupToDatabase(group);
            
            // Update UI
            updateGroupChatArea(groupIndex);
            updateUsersList();
            
            return;
        }
    }
}

/*
 * Contributor: Hossam
 * Function: removeMemberFromGroup
 * Description: Removes a member from a group chat.
 * Updates the group membership list and saves changes to database.
 */
void ChatPage::removeMemberFromGroup(int groupIndex, const QString &memberId) {
    if (groupIndex < 0 || groupIndex >= groupList.size())
        return;
    
    GroupInfo &group = groupList[groupIndex];
    
    // Cannot remove admin
    if (memberId == group.adminId)
        return;
    
    // Remove the member
    group.members.removeOne(memberId);
    
    // Save updated group to database
    saveGroupToDatabase(group);
    
    // Update UI
    updateGroupChatArea(groupIndex);
    updateUsersList();
}

/*
 * Contributor: Hossam
 * Function: leaveGroup
 * Description: Allows current user to leave a group chat.
 * Removes the user from the group's member list and updates UI.
 */
void ChatPage::leaveGroup(int groupIndex) {
    if (groupIndex < 0 || groupIndex >= groupList.size())
        return;
    
    // Get current user
    Client *client = server::getInstance()->getCurrentClient();
    if (!client) return;
    
    QString userId = client->getUserId();
    GroupInfo &group = groupList[groupIndex];
    
    // If user is admin and there are other members, don't allow leaving
    if (userId == group.adminId && group.members.size() > 1) {
        // Show message that admin can't leave the group
        QMessageBox::warning(this, "Cannot Leave Group", 
                            "You are the admin of this group. You must make someone else the admin or delete the group.");
        return;
    }
    
    // Remove the user from the group
    group.members.removeOne(userId);
    
    // If group is now empty, delete it
    if (group.members.isEmpty()) {
        deleteGroup(groupIndex);
        return;
    }
    
    // If user was admin, assign admin role to first remaining member
    if (userId == group.adminId && !group.members.isEmpty()) {
        group.adminId = group.members.first();
    }
    
    // Save updated group to database
    saveGroupToDatabase(group);
    
    // Remove group from user's view
    groupList.removeAt(groupIndex);
    if (groupIndex < groupMessages.size()) {
        groupMessages.removeAt(groupIndex);
    }
    
    // Return to main chat view
    navigateToState(NavigationState::CHATS);
    
    // Update UI
    updateUsersList();
}

/*
 * Contributor: Hossam
 * Function: deleteGroup
 * Description: Deletes a group chat entirely.
 * Removes the group from the database and updates the UI.
 */
void ChatPage::deleteGroup(int groupIndex) {
    if (groupIndex < 0 || groupIndex >= groupList.size())
        return;
    
    GroupInfo &group = groupList[groupIndex];
    
    // Get current user
    Client *client = server::getInstance()->getCurrentClient();
    if (!client) return;
    
    QString userId = client->getUserId();
    
    // Only admin can delete the group
    if (userId != group.adminId && !group.members.isEmpty()) {
        return;
    }
    
    // Delete group directory
    QDir groupDir("../db/groups/" + group.groupId);
    if (groupDir.exists()) {
        groupDir.removeRecursively();
    }
    
    // Remove from our lists
    groupList.removeAt(groupIndex);
    if (groupIndex < groupMessages.size()) {
        groupMessages.removeAt(groupIndex);
    }
    
    // Return to main chat view
    navigateToState(NavigationState::CHATS);
    
    // Update UI
    updateUsersList();
}

/*
 * Contributor: Hossam
 * Function: createNewGroup
 * Description: Creates a new group chat with the specified name and members.
 * Adds the group to the groupList and updates the UI.
 */
void ChatPage::createNewGroup(const QString &groupName, const QStringList &members) {
    if (groupName.isEmpty())
        return;
    
    // Get current client
    Client *client = server::getInstance()->getCurrentClient();
    if (!client)
        return;
    
    QString userId = client->getUserId();
    
    // Create new group
    GroupInfo group;
    group.name = groupName;
    group.groupId = QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch());
    group.adminId = userId;
    
    // Add current user if not already in members list
    if (!members.contains(userId)) {
        group.members = members;
        group.members.append(userId);
    } else {
        group.members = members;
    }
    
    group.hasMessages = false;
    group.lastMessage = "";
    group.lastSeen = "";
    group.hasCustomImage = false;
    
    // Add group to list
    groupList.append(group);
    
    // Add empty messages vector
    groupMessages.append(QVector<MessageInfo>());
    
    // Create necessary directories for the group
    QDir dir;
    dir.mkpath("../db/groups/" + group.groupId);
    dir.mkpath("../db/groups/" + group.groupId + "/messages");
    
    // Save to database
    saveGroupToDatabase(group);
    
    // Update UI
    updateGroupsList();
    
    // Add a welcome message
    if (groupMessages.size() == groupList.size()) {
        // Create welcome message
        MessageInfo welcomeMsg;
        welcomeMsg.text = "Welcome to the group '" + groupName + "'!";
        welcomeMsg.sender = userId;
        welcomeMsg.isFromMe = true;
        welcomeMsg.timestamp = QDateTime::currentDateTime();
        
        // Add to messages
        groupMessages.last().append(welcomeMsg);
        
        // Save message to file
        QString messagesPath = "../db/groups/" + group.groupId + "/messages/messages.txt";
        QFile messagesFile(messagesPath);
        
        if (messagesFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&messagesFile);
            out << userId << "|"
                << welcomeMsg.timestamp.toString(Qt::ISODate) << "|"
                << welcomeMsg.text << "\n";
            messagesFile.close();
        }
        
        // Update group info
        groupList.last().hasMessages = true;
        groupList.last().lastMessage = welcomeMsg.text;
        groupList.last().lastSeen = "Just now";
    }
    
    // Select the new group in the UI
    updateGroupChatArea(groupList.size() - 1);
    
    // Output debug info
    qDebug() << "Created new group:" << groupName << "with" << group.members.size() << "members";
    qDebug() << "Total groups now:" << groupList.size();
}

/*
 * Contributor: Hossam
 * Function: showGroupOptions
 * Description: Displays options menu for a group chat.
 * Shows options like add member, leave group, etc.
 */
void ChatPage::showGroupOptions(int groupIndex) {
    if (groupIndex < 0 || groupIndex >= groupList.size())
        return;
    
    GroupInfo &group = groupList[groupIndex];
    
    // Create dialog
    QDialog *optionsDialog = new QDialog(this);
    optionsDialog->setWindowTitle("Group Options: " + group.name);
    optionsDialog->setFixedSize(400, 450);
    optionsDialog->setStyleSheet("background-color: #1e1e2e;");
    
    QVBoxLayout *dialogLayout = new QVBoxLayout(optionsDialog);
    dialogLayout->setContentsMargins(20, 20, 20, 20);
    dialogLayout->setSpacing(15);
    
    // Group name and info
    QLabel *groupNameLabel = new QLabel("Group Name: " + group.name);
    groupNameLabel->setStyleSheet("color: white; font-size: 18px; font-weight: bold;");
    
    QLabel *membersCountLabel = new QLabel("Members: " + QString::number(group.members.size()));
    membersCountLabel->setStyleSheet("color: #9e9e9e; font-size: 14px;");
    
    // List of members
    QLabel *membersLabel = new QLabel("Members:");
    membersLabel->setStyleSheet("color: white; font-size: 16px;");
    
    QListWidget *membersList = new QListWidget();
    membersList->setStyleSheet(R"(
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
    )");
    
    // Current client
    Client *client = server::getInstance()->getCurrentClient();
    if (!client)
        return;
    
    QString currentUserId = client->getUserId();
    bool isAdmin = (group.adminId == currentUserId);
    
    // Populate members list
    for (const QString &memberId : group.members) {
        // Get user info
        QString nickname, bio;
        if (server::getInstance()->getUserSettings(memberId, nickname, bio)) {
            QListWidgetItem *item = new QListWidgetItem(nickname);
            item->setData(Qt::UserRole, memberId);
            
            // Highlight admin
            if (memberId == group.adminId) {
                item->setText(nickname + " (Admin)");
                item->setForeground(QBrush(QColor("#4CAF50")));
            }
            
            // Highlight current user
            if (memberId == currentUserId) {
                item->setText(item->text() + " (You)");
                item->setForeground(QBrush(QColor("#65a9e0")));
            }
            
            membersList->addItem(item);
        } else {
            // Fallback if user settings can't be retrieved
            QListWidgetItem *item = new QListWidgetItem(memberId);
            item->setData(Qt::UserRole, memberId);
            membersList->addItem(item);
        }
    }
    
    // Add buttons for group actions
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);
    
    QPushButton *closeButton = new QPushButton("Close");
    closeButton->setStyleSheet(R"(
        QPushButton {
            background-color: #2e2e3e;
            color: white;
            border-radius: 8px;
            padding: 10px 20px;
            font-size: 15px;
        }
        QPushButton:hover {
            background-color: #3e3e4e;
        }
    )");
    
    // Add member button (admin only)
    QPushButton *addMemberButton = new QPushButton("Add Member");
    addMemberButton->setStyleSheet(R"(
        QPushButton {
            background-color: #4d6eaa;
            color: white;
            border-radius: 8px;
            padding: 10px 20px;
            font-size: 15px;
        }
        QPushButton:hover {
            background-color: #3d5e9a;
        }
    )");
    addMemberButton->setEnabled(isAdmin);
    
    // Leave/Delete group button
    QPushButton *leaveButton = new QPushButton(isAdmin ? "Delete Group" : "Leave Group");
    leaveButton->setStyleSheet(R"(
        QPushButton {
            background-color: #E15554;
            color: white;
            border-radius: 8px;
            padding: 10px 20px;
            font-size: 15px;
        }
        QPushButton:hover {
            background-color: #D14545;
        }
    )");
    
    buttonLayout->addWidget(closeButton);
    buttonLayout->addWidget(addMemberButton);
    buttonLayout->addWidget(leaveButton);
    
    // Add context menu for removing members (admin only)
    if (isAdmin) {
        membersList->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(membersList, &QListWidget::customContextMenuRequested, [this, membersList, groupIndex](const QPoint &pos) {
            QListWidgetItem *selectedItem = membersList->itemAt(pos);
            if (selectedItem) {
                QString memberId = selectedItem->data(Qt::UserRole).toString();
                
                // Don't show remove option for admin
                if (memberId == groupList[groupIndex].adminId)
                    return;
                
                QMenu *contextMenu = new QMenu(this);
                QAction *removeAction = new QAction("Remove from Group", this);
                
                connect(removeAction, &QAction::triggered, [this, groupIndex, memberId]() {
                    removeMemberFromGroup(groupIndex, memberId);
                });
                
                contextMenu->addAction(removeAction);
                contextMenu->exec(membersList->mapToGlobal(pos));
            }
        });
    }
    
    // Add widgets to dialog layout
    dialogLayout->addWidget(groupNameLabel);
    dialogLayout->addWidget(membersCountLabel);
    dialogLayout->addWidget(membersLabel);
    dialogLayout->addWidget(membersList);
    dialogLayout->addLayout(buttonLayout);
    
    // Connect buttons
    connect(closeButton, &QPushButton::clicked, optionsDialog, &QDialog::accept);
    
    connect(addMemberButton, &QPushButton::clicked, [this, optionsDialog, groupIndex]() {
        optionsDialog->accept();
        addMemberToGroup(groupIndex);
    });
    
    connect(leaveButton, &QPushButton::clicked, [this, optionsDialog, groupIndex, isAdmin]() {
        optionsDialog->accept();
        if (isAdmin) {
            deleteGroup(groupIndex);
        } else {
            leaveGroup(groupIndex);
        }
    });
    
    // Show dialog
    openDialog(optionsDialog);
}

/*
 * Contributor: Hossam
 * Function: updateGroupsList
 * Description: Updates the user list to show both users and groups.
 * This function ensures groups are displayed alongside users in the sidebar.
 */
void ChatPage::updateGroupsList() {
    // First clear the list
    usersListWidget->clear();
    
    // Add the "Create Group" item at the top
    QListWidgetItem *createGroupItem = new QListWidgetItem();
    createGroupItem->setData(Qt::UserRole + 2, false); // Not a group
    createGroupItem->setData(Qt::UserRole + 3, true);  // Special item
    usersListWidget->addItem(createGroupItem);
    
    // Create widget for the "Create Group" item
    QWidget *createGroupWidget = new QWidget();
    QHBoxLayout *createGroupLayout = new QHBoxLayout(createGroupWidget);
    createGroupLayout->setContentsMargins(10, 5, 10, 5);
    
    QLabel *plusIcon = new QLabel("+");
    plusIcon->setFixedSize(40, 40);
    plusIcon->setAlignment(Qt::AlignCenter);
    plusIcon->setStyleSheet("background-color: #4d6eaa; border-radius: 20px; color: white; font-size: 24px; font-weight: bold;");
    
    QLabel *createGroupLabel = new QLabel("Create New Group");
    createGroupLabel->setStyleSheet("color: white; font-size: 16px;");
    
    createGroupLayout->addWidget(plusIcon);
    createGroupLayout->addWidget(createGroupLabel);
    
    usersListWidget->setItemWidget(createGroupItem, createGroupWidget);
    
    // Add existing groups
    for (int i = 0; i < groupList.size(); i++) {
        QListWidgetItem *item = createGroupListItem(groupList[i], i);
        usersListWidget->addItem(item);
    }
    
    // Then add users as we normally do
    for (int i = 0; i < userList.size(); i++) {
        QListWidgetItem *item = createUserListItem(userList[i], i);
        usersListWidget->addItem(item);
    }
    
    // Update UI after adding items
    usersListWidget->update();
}
