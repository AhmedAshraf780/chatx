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
#include "../client/client.h"

struct UserData {
    QString username;
    QString password;
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
    
    // Your data
    QMap<QString, UserData> userMap;
    QMap<QString, Client*> clients;  // Map of email to Client pointer
    Client* currentClient;  // Currently logged in client

    void loadUsersAccounts();
    void saveUsersAccounts();
    void loadClientData(Client* client);
    void saveClientData(Client* client);
    void migrateRoomFiles();

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
    
    // Data persistence
    void saveAllClientsData();
};

#endif // SERVER_H
