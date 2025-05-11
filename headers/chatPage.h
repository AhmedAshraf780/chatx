#ifndef CHATPAGE_H
#define CHATPAGE_H

#include <QWidget>
#include <QVector>
#include <QPair>
#include <QDateTime>
#include <qboxlayout.h>
#include <QListWidget>
#include <QLineEdit>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QColor>
#include <QScrollBar>
#include <QStackedWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QSizePolicy>
#include <QEvent>
#include <QMouseEvent>
#include <QPalette>
#include <QPixmap>
#include <QTextEdit>
#include <QListWidgetItem>
#include <QDebug>
#include "../server/server.h"

class QVBoxLayout;
class QHBoxLayout;
class QScrollArea;
class QLineEdit;
class QLabel;
class QListWidget;
class QStackedWidget;
class QPushButton;
class QEvent;

// Forward declaration of UserInfo
struct UserInfo {
    QString name;
    QString email;  // Add this field if not present
    QString status;
    QString lastMessage;
    QString lastSeen;
    bool isContact;  // Flag to indicate if this is a contact
};

// Message Structure
struct MessageInfo {
    QString text;
    bool isFromMe;
    QDateTime timestamp;
};

class ChatPage : public QWidget {
    Q_OBJECT

public:
    explicit ChatPage(QWidget *parent = nullptr);
    ~ChatPage();
    void loadUsersFromDatabase();  // Moved to public section

private:
    // UI Elements
    QListWidget *usersListWidget;
    QLineEdit *searchInput;
    QLineEdit *messageInput;
    QScrollArea *messageArea;
    QVBoxLayout *messageLayout;
    QLabel *chatHeader;
    QStackedWidget *contentStack;
    QVector<QPushButton*> navButtons;
    QListWidgetItem* createUserListItem(const UserInfo &user, int index);

    // Data
    QVector<UserInfo> userList;
    QVector<QVector<MessageInfo>> userMessages;
    int currentUserId;
    bool isSearching;  // Flag to indicate if we're in search mode

    // Setup methods
    void createNavigationPanel(QHBoxLayout *mainLayout);
    void createUsersPanel(QHBoxLayout *mainLayout);
    void createChatArea(QHBoxLayout *mainLayout);
    void updateUsersList();
    void filterUsers();
    void handleUserSelected(int row);
    void updateChatArea(int userId);
    void sendMessage();
    void addMessageToUI(const QString &text, bool isFromMe, const QDateTime &timestamp);
    bool eventFilter(QObject *obj, QEvent *event) override;
    void showMessageOptions(QWidget *bubble);
    void addToContacts(int userId);  // New method to add a user to contacts
    void loadMessagesForCurrentUser();

};

#endif // CHATPAGE_H
