// server.cpp
#include "server.h"

// Initialize static member
server* server::instance = nullptr;

server::server() {
    loadUsersAccounts();
}

server* server::getInstance() {
    // Create the instance if it doesn't exist
    if (instance == nullptr) {
        instance = new server();
    }
    return instance;
}

void server::loadUsersAccounts() {
    QFile file("./db/users_credentials.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file for reading";
        return;
    }
    
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList parts = line.split(',');
        if (parts.size() == 2) {
            QString email = parts[0].trimmed();
            QString password = parts[1].trimmed();
            userMap.insert(email, password);
        }
    }
    file.close();
}

void server::saveUsersAccounts() {
    QFile file("./db/users_credentials.txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file for writing";
        return;
    }
    
    QTextStream out(&file);
    QMapIterator<QString, QString> i(userMap);
    while (i.hasNext()) {
        i.next();
        out << i.key() << "," << i.value() << "\n";
    }
    
    file.close();
}

bool server::checkUser(const QString &email, const QString &password) {
    if (userMap.contains(email)) {
        if (userMap.value(email) == password) {
            return true;
        }
    }
    return false;
}

void server::addUser(const QString &email, const QString &password) {
    userMap.insert(email, password);
    // Optionally save immediately
    saveUsersAccounts();
}

server::~server() {
    saveUsersAccounts();
    // Optional cleanup
    // instance = nullptr;
}
