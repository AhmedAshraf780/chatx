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
#include "../client/client.h"

struct UserData {
    QString username;
    QString password;
    QString bio;
    QString nickname;
    bool isOnline;
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
    QMap<QString, QVector<Message>> roomMessages;         // RoomId -> Messages
    
    Client* currentClient;  // Currently logged in client

    // File loading and saving
    void loadUsersAccounts();
    void saveUsersAccounts();
    void loadUserContacts(const QString& userId);
    void saveUserContacts(const QString& userId);
    void loadClientData(Client* client);
    void saveClientData(Client* client);
    void migrateRoomFiles();
<<<<<<< HEAD
=======
    void loadAllData();
    void saveAllData();
    void createDefaultSettingsFiles();
>>>>>>> bfd0fc2 (handle the settings)

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
    
    // Client management
    Client* loginUser(const QString& email, const QString& password);
    void logoutUser();
    Client* getCurrentClient() const { return currentClient; }
    QVector<QPair<QString, QString>> getAllUsers() const;
    QString getUsernameById(const QString &email) const;
    bool deleteUser(const QString &userId);
    
    // User settings management
    bool updateUserSettings(const QString &userId, const QString &nickname, const QString &bio);
    bool getUserSettings(const QString &userId, QString &nickname, QString &bio);
    bool setUserOnlineStatus(const QString &userId, bool isOnline);
    bool isUserOnline(const QString &userId) const;
    
    // Client and contact management
    bool hasClient(const QString &userId) const { return clients.contains(userId); }
    Client* getClient(const QString &userId) { return clients.value(userId, nullptr); }
    
    // Contact management
    bool addContactForUser(const QString &userId, const QString &contactId);
    bool hasContactForUser(const QString &userId, const QString &contactId);
    
    // Room management
    bool addRoomToUser(const QString &userId, Room *room);
    bool hasRoomForUser(const QString &userId, const QString &roomId);
    
    // Data persistence
    void saveAllClientsData();
    
    // Message management
    void updateRoomMessages(const QString &roomId, const QVector<Message> &messages);
    QVector<Message> getRoomMessages(const QString &roomId) const;
};

#endif // SERVER_H
