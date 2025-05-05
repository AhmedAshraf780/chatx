// server.h
#ifndef SERVER_H
#define SERVER_H

#include <QMap>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QDebug>

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
    QMap<QString, QString> userMap;

public:
    // Destructor
    ~server();
    
    // Method to access the singleton instance
    static server* getInstance();
    
    // Your methods
    void loadUsersAccounts();
    void saveUsersAccounts();
    bool checkUser(const QString &email, const QString &password);
    
    // Method to add a new user
    void addUser(const QString &email, const QString &password);
};

#endif // SERVER_H
