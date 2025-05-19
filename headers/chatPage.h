#ifndef CHATPAGE_H
#define CHATPAGE_H

#include <QWidget>
#include <QVector>
#include <QPair>
#include <QDateTime>
#include <QBoxLayout>
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
#include <QCheckBox>
#include <QTimer>
#include <QApplication>
#include <QDialog>
#include <QFileDialog>
#include <QMetaType>
#include "../server/server.h"

// Forward declarations
class QVBoxLayout;
class QHBoxLayout;
class QScrollArea;
class QLineEdit;
class QLabel;
class QListWidget;
class QStackedWidget;
class QPushButton;
class QEvent;
class QTextEdit;
class QCheckBox;
class QTimer;

// User info structure
struct UserInfo {
    QString name;
    QString email;
    QString status;
    QString lastMessage;
    QString lastSeen;
    bool isContact;     // Flag to indicate if this is a contact
    bool hasMessages;   // Flag to indicate if there are messages with this user
    bool isOnline;      // Flag to indicate if user is online
};

// User settings struct
struct UserSettings {
    QString nickname;
    QString bio;
    bool isOnline;
    QPixmap avatarImage;       // User's avatar image
    bool hasCustomAvatar;      // Flag indicating if user has a custom avatar
};

// Message Structure
struct MessageInfo {
    QString text;
    bool isFromMe;
    QDateTime timestamp;
    QString sender;    // Add sender field for group messages
};

// Story Structure
struct StoryInfo {
    QString userId;     // ID of the user who created the story
    QString username;   // Username of the creator
    QString imagePath;  // Path to the story image
    QString caption;    // Caption for the story
    QDateTime timestamp; // When the story was created
    bool viewed;        // Whether the current user has viewed this story
};

// Register StoryInfo with Qt's meta-object system
Q_DECLARE_METATYPE(StoryInfo)

// Add group chat related structures after other structures
struct GroupInfo {
    QString groupId;
    QString name;
    QString adminId;
    QStringList members;
    bool hasMessages;
    QString lastMessage;
    QString lastSeen;
    QPixmap groupImage;
    bool hasCustomImage;
};

class ChatPage : public QWidget {
    Q_OBJECT

public:
    ChatPage(QWidget *parent = nullptr);
    ~ChatPage();
    void loadUsersFromDatabase();  // Moved to public section
    void forceRefreshOnlineStatus(); // Force immediate refresh of online status
    void startStatusUpdateTimer(); // Start the online status timer
    void stopStatusUpdateTimer();  // Stop the online status timer

private slots:
    void logout(); // Method to handle logout
    void saveUserSettings(); // Method to save user settings
    void changePassword(); // Method to handle password changes
    void onlineStatusChanged(int state); // Handle online/offline toggle
    void refreshOnlineStatus(); // Periodically refresh online status of users
    void updateUsersList();
    void filterUsers();
    void handleUserSelected(int row);
    void sendMessage();
    void changeAvatar(); // Method to handle avatar change request
    void displayLatestMessage(int userIndex); // Display the latest message using stack's top function
    
    // User interaction slots
    void showMessageOptions(QWidget *bubble);
    void showUserOptions(int userIndex); // Add option menu for user list items
    void removeUserFromContacts(int userIndex); // Remove a user from contacts
    void blockUser(int userIndex); // Block a user
    void addToContacts(int userId); // Add a user to contacts
    void loadBlockedUsers(); // Load blocked users list
    void unblockUser(const QString &userId); // Unblock a user
    
    // Story-related slots
    void createStory(); // Create a new story
    void viewStory(const StoryInfo &story); // View a specific story
    void loadStories(); // Load all available stories
    void refreshStories(); // Refresh the stories list
    void deleteStory(const QString &storyId); // Delete a story
    void clearStoryWidgets(); // Clear all story widgets
    void loadBlockedUsersPane(); // Load blocked users into the pane
    
    // Group chat slots
    void showCreateGroupDialog();
    void createNewGroup(const QString &groupName, const QStringList &members);
    void showGroupOptions(int groupIndex);
    void addMemberToGroup(int groupIndex);
    void removeMemberFromGroup(int groupIndex, const QString &memberId);
    void leaveGroup(int groupIndex);
    void deleteGroup(int groupIndex);
    
    // Message read status methods
    void markMessagesAsRead(int userIndex); // Mark messages from other user as read
    void updateReadReceipts(); // Update read receipt indicators in UI

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
    QWidget *usersSidebar; // Add reference to users sidebar widget
    
    // Settings UI Elements
    QLineEdit *nicknameEdit;
    QTextEdit *bioEdit;
    QCheckBox *onlineStatusCheckbox;
    QLabel *avatarPreview; // Avatar preview in settings
    
    // Password change UI elements
    QLineEdit *currentPasswordEdit;
    QLineEdit *newPasswordEdit;
    QLineEdit *confirmPasswordEdit;
    
    // Profile UI Elements
    QLabel *profileAvatar;
    QLabel *profileStatusIndicator;
    
    // Stories UI Elements
    QHBoxLayout *storyWidgetsLayout; // Layout for story circles at the top
    QVBoxLayout *storiesFeedLayout;  // Layout for story feed items
    QVector<StoryInfo> userStories;  // Vector to store all stories
    
    // Timer for refreshing online status
    QTimer *onlineStatusTimer;
    QTimer *storiesTimer; // Timer for refreshing stories

    // Data
    QVector<UserInfo> userList;
    QVector<QVector<MessageInfo>> userMessages;
    int currentUserId;
    bool isSearching;  // Flag to indicate if we're in search mode
    UserSettings userSettings; // Store user settings

    // Blocked users UI elements
    QVBoxLayout *blockedUsersLayout; // Layout for blocked users list

    // Group chat member variables
    QVector<GroupInfo> groupList;
    QVector<QList<MessageInfo>> groupMessages;
    int currentGroupId;
    bool isInGroupChat;

    // Setup methods
    void createNavigationPanel(QHBoxLayout *mainLayout);
    void createUsersPanel(QHBoxLayout *mainLayout);
    void createChatArea(QHBoxLayout *mainLayout);
    void updateChatArea(int userId);
    void updateGroupChatArea(int groupId); // Add method declaration for group chat area update
    void addMessageToUI(const QString &text, bool isFromMe, const QDateTime &timestamp);
    bool eventFilter(QObject *obj, QEvent *event) override;
    void loadMessagesForCurrentUser();
    void loadUserSettings(); // Load user settings from storage
    void updateUserStatus(const QString &userId, bool isOnline); // Update user's online status
    QListWidgetItem* createUserListItem(const UserInfo &user, int index);
    void updateProfileAvatar(); // Update the profile avatar with current user's info
    bool saveAvatarImage(const QPixmap &image); // Save avatar image to file
    bool loadAvatarImage(); // Load avatar image from file
    
    // Story helper methods
    QWidget* createStoryCircle(const StoryInfo &story); // Create a story circle UI element
    QWidget* createStoryFeedItem(const StoryInfo &story); // Create a story feed item UI element
    void showStoryDialog(const StoryInfo &story); // Show a dialog with the full story
    
    // Group chat methods
    QListWidgetItem *createGroupListItem(const GroupInfo &group, int index);
    void loadGroupMessagesForCurrentGroup();
    void addGroupMessageToUI(const QString &text, const QString &sender, const QDateTime &timestamp);
    void loadGroupsFromDatabase();
    void updateGroupsList();
    bool saveGroupToDatabase(const GroupInfo &group);
    bool loadGroupImage(GroupInfo &group);
};

#endif // CHATPAGE_H
