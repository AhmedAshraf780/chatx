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

    // Load users from database
    loadUsersFromDatabase();

    // Show contacts by default and select the first contact
    updateUsersList();
    if (!userList.isEmpty()) {
        for (int i = 0; i < userList.size(); ++i) {
            if (userList[i].isContact) {
                usersListWidget->setCurrentRow(i);
                currentUserId = i;
                updateChatArea(i);
                break;
            }
        }
    }
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
    QLabel *settingsLabel = new QLabel("Settings Feature Coming Soon");
    settingsLabel->setAlignment(Qt::AlignCenter);
    settingsLabel->setStyleSheet("color: white; font-size: 24px;");
    settingsLayout->addWidget(settingsLabel);
    contentStack->addWidget(settingsView);

    mainLayout->addWidget(contentStack);
    mainLayout->setStretch(2, 1); // Make chat area take remaining space
}

void ChatPage::loadUsersFromDatabase() {
    qDebug() << "Loading users from database...";
    Client *currentClient = server::getInstance()->getCurrentClient();
    if (!currentClient) {
        qDebug() << "No current client found!";
        return;
    }

    userList.clear();
    userMessages.clear();

    // Get all users from the server
    QVector<QPair<QString, QString>> allUsers =
        server::getInstance()->getAllUsers();
    qDebug() << "Retrieved users from server:" << allUsers.size();

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
        userInfo.status = "Offline"; // Default status
        userInfo.lastMessage = "";
        userInfo.lastSeen = "";
        userInfo.isContact = currentClient->hasContact(email);
        userList.append(userInfo);
        qDebug() << "Added user to list:" << username << email;
    }

    qDebug() << "Total users loaded:" << userList.size();

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
        if (!isSearching && !user.isContact)
            continue;
        if (isSearching && !user.name.toLower().contains(searchText) &&
            !user.email.toLower().contains(searchText))
            continue;

        QListWidgetItem *item = new QListWidgetItem;
        item->setSizeHint(QSize(0, 60)); // consistent height
        item->setData(Qt::UserRole, i);

        usersListWidget->addItem(item);

        // Create the custom widget
        QWidget *itemWidget = new QWidget;
        QHBoxLayout *layout = new QHBoxLayout(itemWidget);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(16);

        QLabel *avatar = new QLabel(user.name.left(1).toUpper());
        avatar->setFixedSize(44, 44);
        avatar->setAlignment(Qt::AlignCenter);
        avatar->setStyleSheet(R"(
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                       stop:0 #6a82fb, stop:1 #fc5c7d);
            color: white;
            border-radius: 22px;
            font-weight: bold;
            font-size: 22px;
            border: 2px solid #23233a;
        )");

        QLabel *nameLabel = new QLabel(user.name);
        nameLabel->setStyleSheet(
            "color: white; font-size: 17px; font-weight: 600;");

        layout->addWidget(avatar);
        layout->addWidget(nameLabel);
        layout->addStretch();

        usersListWidget->setItemWidget(item, itemWidget);
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
    chatHeader->setText(user.name + " - " + user.status);

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
            currentClient->addContact(userList[userId].name);
            userList[userId].isContact = true;
            updateUsersList();
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

    QString targetUsername = userList[currentUserId].name;
    Room *room = client->getRoomWithUser(targetUsername);
    if (!room) {
        room = client->createRoom(targetUsername);
        if (!userList[currentUserId].isContact) {
            userList[currentUserId].isContact = true;
            client->addContact(targetUsername);
        }
    }

    // Add the message to the file and in-memory structure
    room->addMessage(msg);

    // Update the last message and UI
    userList[currentUserId].lastMessage = messageText;
    userList[currentUserId].lastSeen = "Just now";
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

    QLabel *avatar = new QLabel(user.name.left(1).toUpper());
    avatar->setFixedSize(44, 44);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setStyleSheet(R"(
        background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                   stop:0 #6a82fb, stop:1 #fc5c7d);
        color: white;
        border-radius: 22px;
        font-weight: bold;
        font-size: 22px;
        border: 2px solid #23233a;
    )");

    QLabel *nameLabel = new QLabel(user.name);
    nameLabel->setStyleSheet(
        "color: white; font-size: 17px; font-weight: 600;");

    layout->addWidget(avatar);
    layout->addWidget(nameLabel);
    layout->addStretch();

    QListWidgetItem *item = new QListWidgetItem;
    item->setSizeHint(itemWidget->sizeHint());
    item->setData(Qt::UserRole, index);
    item->setData(Qt::UserRole + 1, QVariant::fromValue<void *>(itemWidget));
    return item;
}

void ChatPage::loadMessagesForCurrentUser() {
    qDebug() << "Loading messages for user index:" << currentUserId;

    if (currentUserId < 0 || currentUserId >= userList.size())
        return;

    Client *client = server::getInstance()->getCurrentClient();
    if (!client)
        return;

    QString targetUsername = userList[currentUserId].name;
    Room *room = client->getRoomWithUser(targetUsername);
    if (!room)
        return;

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
             << "messages for user" << targetUsername;
}
