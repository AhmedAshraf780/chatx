#include "chatPage.h"

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

ChatPage::ChatPage(QWidget *parent) : QWidget(parent), currentUserId(-1) {
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);



    // Navigation Panel (leftmost)
    createNavigationPanel(mainLayout);

    // Users Panel (second from left)
    createUsersPanel(mainLayout);

    // Chat Area (right side)
    createChatArea(mainLayout);

    // Setup demo content
    setupDummyData();
    updateUsersList();

    // Select first user by default
    if (!userList.isEmpty()) {
        currentUserId = 0;
        updateChatArea(currentUserId);
    }
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
                .arg(i == 0 ? "#4d6eaa" : "#2e2e3e"));  // Highlight Chat button

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
            }
        });
    }

    navLayout->addStretch();
    mainLayout->addWidget(navSidebar);
}

void ChatPage::createUsersPanel(QHBoxLayout *mainLayout) {
    // Users Sidebar (left side)
    QWidget *usersSidebar = new QWidget;
    usersSidebar->setFixedWidth(250);
    usersSidebar->setStyleSheet(
        "background-color: #1e1e2e; border-right: 1px solid #2e2e3e;");
    QVBoxLayout *usersLayout = new QVBoxLayout(usersSidebar);
    usersLayout->setContentsMargins(10, 10, 10, 10);

    // Search bar
    searchInput = new QLineEdit;
    searchInput->setPlaceholderText("Search users...");
    searchInput->setStyleSheet(R"(
        background-color: #2e2e3e;
        color: white;
        border-radius: 10px;
        padding: 8px;
        border: none;
    )");
    usersLayout->addWidget(searchInput);

    // Connect search signal
    connect(searchInput, &QLineEdit::textChanged, this, &ChatPage::filterUsers);

    // Users list widget
    usersListWidget = new QListWidget;
    usersListWidget->setStyleSheet(R"(
        QListWidget {
            background-color: transparent;
            border: none;
        }
        QListWidget::item {
            background-color: transparent;
            color: white;
            padding: 4px;
            border-radius: 6px;
        }
        QListWidget::item:hover {
            background-color: #2a2a3a;
        }
        QListWidget::item:selected {
            background-color: #3a3a4a;
        }
    )");
    usersLayout->addWidget(usersListWidget);

    // Connect user selection
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
    QLabel *settingsLabel = new QLabel("Settings Feature Coming Soon");
    settingsLabel->setAlignment(Qt::AlignCenter);
    settingsLabel->setStyleSheet("color: white; font-size: 24px;");
    settingsLayout->addWidget(settingsLabel);
    contentStack->addWidget(settingsView);

    mainLayout->addWidget(contentStack);
    mainLayout->setStretch(2, 1);  // Make chat area take remaining space
}

void ChatPage::setupDummyData() {
    // Add some dummy users
    userList.append(
        {{"Alice Smith", "Online", "Hey, how's it going?", "Just now"}});
    userList.append(
        {{"Bob Johnson", "Away", "Can we meet tomorrow?", "5 min ago"}});
    userList.append(
        {{"Charlie Brown", "Offline", "Check out this link", "Yesterday"}});
    userList.append(
        {{"Diana Prince", "Online", "Thanks for your help!", "2 hours ago"}});
    userList.append(
        {{"Edward Blake", "Busy", "Will call you later", "Yesterday"}});

    // Dummy messages for Alice
    userMessages[0].append({"Hey, how's it going?", false,
                            QDateTime::currentDateTime().addSecs(-3600)});
    userMessages[0].append({"I'm doing well, thanks! How about you?", true,
                            QDateTime::currentDateTime().addSecs(-3540)});
    userMessages[0].append({"Pretty good. Working on that project.", false,
                            QDateTime::currentDateTime().addSecs(-3500)});
    userMessages[0].append({"Make sure to send me the files when you're done.",
                            true, QDateTime::currentDateTime().addSecs(-3480)});

    // Dummy messages for Bob
    userMessages[1].append({"Can we meet tomorrow?", false,
                            QDateTime::currentDateTime().addSecs(-300)});
    userMessages[1].append({"Sure, what time works for you?", true,
                            QDateTime::currentDateTime().addSecs(-240)});

    // Dummy messages for Charlie
    userMessages[2].append({"Check out this link", false,
                            QDateTime::currentDateTime().addDays(-1)});

    // Dummy messages for Diana
    userMessages[3].append(
        {"I was having trouble with that issue you mentioned", false,
         QDateTime::currentDateTime().addSecs(-7200)});
    userMessages[3].append({"Here's what I found might help", true,
                            QDateTime::currentDateTime().addSecs(-7100)});
    userMessages[3].append({"Thanks for your help!", false,
                            QDateTime::currentDateTime().addSecs(-7000)});

    // Dummy messages for Edward
    userMessages[4].append({"Will call you later", false,
                            QDateTime::currentDateTime().addDays(-1)});
}

void ChatPage::updateUsersList() {
    usersListWidget->clear();

    QString searchText = searchInput->text().toLower();

    for (int i = 0; i < userList.size(); i++) {
        const UserInfo &user = userList[i];

        // Filter users based on search text
        if (!searchText.isEmpty() &&
            !user.name.toLower().contains(searchText) &&
            !user.lastMessage.toLower().contains(searchText)) {
            continue;
        }

        // Create user item widget
        QWidget *userWidget = new QWidget;
        QHBoxLayout *userLayout = new QHBoxLayout(userWidget);
        userLayout->setContentsMargins(8, 8, 8, 8);

        // Avatar placeholder
        QLabel *avatar = new QLabel(user.name.left(1));
        avatar->setFixedSize(40, 40);
        avatar->setAlignment(Qt::AlignCenter);
        avatar->setStyleSheet(R"(
            background-color: #4d6eaa;
            color: white;
            border-radius: 20px;
            font-weight: bold;
        )");

        // User info
        QWidget *infoWidget = new QWidget;
        QVBoxLayout *infoLayout = new QVBoxLayout(infoWidget);
        infoLayout->setContentsMargins(0, 0, 0, 0);
        infoLayout->setSpacing(2);

        QHBoxLayout *nameLayout = new QHBoxLayout;
        QLabel *nameLabel = new QLabel(user.name);
        nameLabel->setStyleSheet("color: white; font-weight: bold;");
        QLabel *timeLabel = new QLabel(user.lastSeen);
        timeLabel->setStyleSheet("color: #aaa; font-size: 10px;");
        nameLayout->addWidget(nameLabel);
        nameLayout->addStretch();
        nameLayout->addWidget(timeLabel);

        QLabel *msgLabel = new QLabel(user.lastMessage);
        msgLabel->setStyleSheet("color: #aaa; font-size: 11px;");
        msgLabel->setMaximumWidth(160);
        msgLabel->setWordWrap(true);

        infoLayout->addLayout(nameLayout);
        infoLayout->addWidget(msgLabel);

        userLayout->addWidget(avatar);
        userLayout->addWidget(infoWidget);

        // Add to list widget
        QListWidgetItem *item = new QListWidgetItem(usersListWidget);
        item->setSizeHint(userWidget->sizeHint());
        item->setData(Qt::UserRole, i);  // Store user index
        usersListWidget->setItemWidget(item, userWidget);
    }
}

void ChatPage::filterUsers() { updateUsersList(); }

void ChatPage::handleUserSelected(int row) {
    if (row >= 0) {
        QListWidgetItem *item = usersListWidget->item(row);
        int userId = item->data(Qt::UserRole).toInt();
        currentUserId = userId;
        updateChatArea(userId);
    }
}

void ChatPage::updateChatArea(int userId) {
    if (userId < 0 || userId >= userList.size()) return;

    // Update header
    const UserInfo &user = userList[userId];
    chatHeader->setText(user.name + " - " + user.status);

    // Clear existing messages
    QWidget *messageWidget = messageArea->widget();
    while (messageLayout->count() > 1) {
        QLayoutItem *item = messageLayout->takeAt(0);
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    // Add messages for this user
    const QVector<MessageInfo> &messages = userMessages[userId];
    for (const MessageInfo &msg : messages) {
        addMessageToUI(msg.text, msg.isFromMe, msg.timestamp);
    }

    // Scroll to bottom
    QScrollBar *vScrollBar = messageArea->verticalScrollBar();
    vScrollBar->setValue(vScrollBar->maximum());
}

void ChatPage::sendMessage() {
    if (currentUserId < 0 || messageInput->text().isEmpty()) return;

    QString text = messageInput->text();
    QDateTime timestamp = QDateTime::currentDateTime();

    // Add message to data structure
    userMessages[currentUserId].append({text, true, timestamp});

    // Update UI
    addMessageToUI(text, true, timestamp);

    // Update last message in user list
    userList[currentUserId].lastMessage = text;
    userList[currentUserId].lastSeen = "Just now";
    updateUsersList();

    // Clear input
    messageInput->clear();

    // Scroll to bottom
    QScrollBar *vScrollBar = messageArea->verticalScrollBar();
    vScrollBar->setValue(vScrollBar->maximum());
}

void ChatPage::addMessageToUI(const QString &text, bool isFromMe,
                              const QDateTime &timestamp) {
    QWidget *bubbleRow = new QWidget();
    QHBoxLayout *rowLayout = new QHBoxLayout(bubbleRow);
    rowLayout->setContentsMargins(0, 0, 0, 0);

    // Create message widget
    QLabel *messageText = new QLabel(text);
    messageText->setWordWrap(true);
    messageText->setTextFormat(Qt::TextFormat::RichText);
    messageText->setMaximumWidth(400);

    // Format timestamp
    QString timeStr = timestamp.toString("hh:mm");
    QLabel *timestampLabel = new QLabel(timeStr);
    timestampLabel->setStyleSheet(
        "font-size: 10px; color: #999; margin: 0px 4px;");
    timestampLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // Create bubble content
    QWidget *bubbleContent = new QWidget();
    QVBoxLayout *contentLayout = new QVBoxLayout(bubbleContent);
    contentLayout->setContentsMargins(12, 8, 12, 8);
    contentLayout->setSpacing(2);

    contentLayout->addWidget(messageText);

    QWidget *timestampContainer = new QWidget();
    QHBoxLayout *timestampLayout = new QHBoxLayout(timestampContainer);
    timestampLayout->setContentsMargins(0, 0, 0, 0);
    timestampLayout->addStretch();
    timestampLayout->addWidget(timestampLabel);
    contentLayout->addWidget(timestampContainer);

    // Styling
    QString bubbleColor = isFromMe ? "#4d6eaa" : "#33334d";
    bubbleContent->setStyleSheet(QString("background-color: %1;"
                                         "border-radius: 12px;"
                                         "color: white;")
                                     .arg(bubbleColor));

    bubbleContent->setMaximumWidth(450);

    // Make bubble clickable for edit/delete options
    bubbleContent->setProperty("isFromMe", isFromMe);
    bubbleContent->setProperty("messageText", text);
    bubbleContent->setProperty("timestamp", timestamp);
    bubbleContent->installEventFilter(this);
    bubbleContent->setCursor(Qt::PointingHandCursor);

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
                }
            }
        }
    });

    connect(deleteAction, &QAction::triggered, [this, bubble]() {
        QDateTime timestamp = bubble->property("timestamp").toDateTime();

        // Remove from UI
        QWidget *bubbleRow = bubble->parentWidget()->parentWidget();
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
            }
        }
    });

    menu->exec(QCursor::pos());
    delete menu;
}
