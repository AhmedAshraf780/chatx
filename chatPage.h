#ifndef CHATPAGE_H
#define CHATPAGE_H

#include <QWidget>
#include <QVector>
#include <QMap>
#include <QDateTime>
#include <qboxlayout.h>

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
    QString status;
    QString lastMessage;
    QString lastSeen;
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

private slots:
    void filterUsers();
    void handleUserSelected(int row);
    void sendMessage();

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
    
    // Data
    QVector<UserInfo> userList;
    QMap<int, QVector<MessageInfo>> userMessages;
    int currentUserId;

    // Setup methods
    void createNavigationPanel(QHBoxLayout *mainLayout);
    void createUsersPanel(QHBoxLayout *mainLayout);
    void createChatArea(QHBoxLayout *mainLayout);
    void setupDummyData();
    
    // Helper methods
    void updateUsersList();
    void updateChatArea(int userId);
    void addMessageToUI(const QString &text, bool isFromMe, const QDateTime &timestamp);
    void showMessageOptions(QWidget *bubble);
    
    // Event handling
    bool eventFilter(QObject *obj, QEvent *event) override;
};

#endif // CHATPAGE_H
