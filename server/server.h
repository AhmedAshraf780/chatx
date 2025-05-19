// server.h
#ifndef SERVER_H
#define SERVER_H

#include <QMap>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QDebug>
#include <QVector>
#include <QPair>
#include <QDateTime>
#include <QDir>
#include <QSet>
#include "../client/client.h"
#include <QList>

struct UserData {
    QString username;
    QString password;
    QString bio;
    QString nickname;
    bool isOnline;
    QDateTime lastStatusChange; // Track when online status last changed
    QString avatarPath;         // Path to user's avatar image
};

// Story data structure for server-side storage
struct StoryData {
    QString id;           // Unique ID for the story (userId_timestamp)
    QString userId;       // User who created the story
    QString imagePath;    // Path to the story image
    QString caption;      // Caption text
    QDateTime timestamp;  // When the story was created
    QSet<QString> viewers; // Set of userIds who have viewed this story
    QMap<QString, QDateTime> viewTimes; // Map of userId -> time when they viewed the story
};

class server {
private:
    // Private constructor so it can't be called externally
    server();
    
    // Delete copy constructor and assignment operator
    server(const server&) = delete;
    server& operator=(const server&) = delete;
    
    // The single instance
    static server* instance;
    
    // Data storage
    QMap<QString, UserData> userMap;                      // Email -> UserData
    QMap<QString, Client*> clients;                       // Email -> Client pointer
    QMap<QString, QVector<QString>> userContacts;         // UserId -> Contact list
    QMap<QString, QMap<QString, Room*>> userRooms;        // UserId -> (RoomId -> Room*)
    QMap<QString, QList<Message>> roomMessages;          // RoomId -> Messages (changed from QStack to QList)
    QVector<StoryData> stories;                           // All stories
    QMap<QString, QVector<QString>> blockedUsers;           // Add this line for blocked users
    
    Client* currentClient;  // Currently logged in client

    // File loading and saving
    void loadUsersAccounts();
    void saveUsersAccounts();
    void loadUserContacts(const QString& userId);
    void saveUserContacts(const QString& userId);
    void loadClientData(Client* client);
    void saveClientData(Client* client);
    void migrateRoomFiles();
    void loadAllData();
    void saveAllData();
    void createDefaultSettingsFiles();
    void loadStories();  // Load stories from disk
    void saveStories();  // Save stories to disk
    void updateRoomReferencesInUserFiles(const QString &oldRoomId, const QString &newRoomId); // Update room references in user files
    void fixUserContactsFile(const QString &userId); // Fix room references in a user's contact file

public:
    // Destructor
    ~server();
    
    // Method to access the singleton instance
    static server* getInstance();
    
    // Validation methods
    bool isValidEmail(const QString& email);
    bool isValidPassword(const QString& password);
    
    // Authentication methods
    bool checkUser(const QString& email, const QString& password);
    void addUser(const QString& email, const QString& password);
    bool registerUser(const QString& username, const QString& email, const QString& password, const QString& confirmPassword, QString& errorMessage);
    bool resetPassword(const QString& email, const QString& newPassword, const QString& confirmPassword, QString& errorMessage);
    bool changePassword(const QString& userId, const QString& currentPassword, const QString& newPassword, const QString& confirmPassword, QString& errorMessage);
    
    // Client management
    Client* loginUser(const QString& email, const QString& password);
    void logoutUser();
    Client* getCurrentClient() const { return currentClient; }
    QVector<QPair<QString, QString>> getAllUsers() const;
    QString getUsernameById(const QString &email) const;
    bool deleteUser(const QString &userId);
    
    // User settings management
    bool updateUserSettings(const QString &userId, const QString &nickname, const QString &bio);
    bool getUserSettings(const QString &userId, QString &nickname, QString &bio, QString &avatarPath);
    bool getUserSettings(const QString &userId, QString &nickname, QString &bio); // Old version for backward compatibility
    bool setUserOnlineStatus(const QString &userId, bool isOnline);
    bool isUserOnline(const QString &userId) const;
    
    // Avatar management
    bool updateUserAvatar(const QString &userId, const QString &avatarPath);
    QString getUserAvatar(const QString &userId) const;
    
    // Client and contact management
    Client* getClient(const QString &userId) const { 
        if (clients.contains(userId)) {
            return clients[userId]; 
        }
        return nullptr;
    }
    
    bool hasClient(const QString &userId) const { 
        bool exists = clients.contains(userId); 
        qDebug() << "Checking if client exists - UserID:" << userId << "Result:" << (exists ? "Found" : "Not found");
        return exists;
    }
    
    // Contact management
    bool addContactForUser(const QString &userId, const QString &contactId);
    bool hasContactForUser(const QString &userId, const QString &contactId);
    
    // Room management
    bool addRoomToUser(const QString &userId, Room *room);
    bool hasRoomForUser(const QString &userId, const QString &roomId);
    
    // Story management
    QString addStory(const QString &userId, const QString &imagePath, const QString &caption);
    bool deleteStory(const QString &storyId);
    bool markStoryAsViewed(const QString &storyId, const QString &viewerId);
    QVector<StoryData> getStories() const { return stories; }
    QVector<StoryData> getStoriesForUser(const QString &userId) const;
    StoryData getStory(const QString &storyId) const;
    bool hasViewedStory(const QString &storyId, const QString &userId) const;
    void cleanupOldStories(); // Remove stories older than 30 seconds (previously 24 hours)
    int getStoryViewerCount(const QString &storyId) const;
    QSet<QString> getStoryViewers(const QString &storyId) const;
    QDateTime getStoryViewTime(const QString &storyId, const QString &viewerId) const;
    
    // Data persistence
    void saveAllClientsData();
    
    // Message management
    void updateRoomMessages(const QString &roomId, const QVector<Message> &messages);
    QVector<Message> getRoomMessages(const QString &roomId) const;

    // Application shutdown handler
    void shutdown() {
        // Save current client data and logout
        saveAllClientsData();
        saveStories();
        logoutUser();
    }

    // Add new method to block user
    bool blockUserForClient(const QString &clientId, const QString &userToBlock);

    // Add method to unblock user
    bool unblockUserForClient(const QString &clientId, const QString &userToUnblock);

    // Add method to check if a user is blocked
    bool isUserBlocked(const QString &clientId, const QString &userToCheck) const;

    // Add method to get blocked users
    QVector<QString> getBlockedUsers(const QString &clientId) const;

    // Add new messages to room list (changed from stack)
    void addMessageToRoom(const QString &roomId, const Message &message);
    
    // Convert between QList and QVector for compatibility (changed from QStack)
    QVector<Message> getMessageVectorFromList(const QString &roomId) const;
    void setMessageListFromVector(const QString &roomId, const QVector<Message> &messages);

    // Repair methods
    void repairInconsistentRoomFiles(); // Repair room files that might use nicknames instead of emails
};

#endif // SERVER_H
