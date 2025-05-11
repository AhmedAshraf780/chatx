// server.cpp
#include "server.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QRegExp>
#include <QTextStream>
#include <qglobal.h>

// Initialize static member
server *server::instance = nullptr;

server::server() : currentClient(nullptr) { loadUsersAccounts(); }

server *server::getInstance() {
    // Create the instance if it doesn't exist
    if (instance == nullptr) {
        instance = new server();
    }
    return instance;
}

void server::loadUsersAccounts() {
    QString filePath = "../db/users_credentials.txt";
    QFile file(filePath);
    qDebug() << "Loading users from:" << filePath;

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file for reading:" << file.errorString();
        return;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList parts = line.split(',');
        if (parts.size() == 3) {
            QString email = parts[0].trimmed();
            QString username = parts[1].trimmed();
            QString password = parts[2].trimmed();
            UserData userData;
            userData.username = username;
            userData.password = password;
            userMap.insert(email, userData);
            qDebug() << "Loaded user:" << email << username;
        }
    }
    file.close();
    qDebug() << "Total users loaded:" << userMap.size();
}

void server::saveUsersAccounts() {
    QString filePath = "../db/users_credentials.txt";
    QFile file(filePath);
    qDebug() << "Saving users to:" << filePath;

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file for writing:" << file.errorString();
        return;
    }

    QTextStream out(&file);
    QMapIterator<QString, UserData> i(userMap);
    while (i.hasNext()) {
        i.next();
        out << i.key() << "," << i.value().username << "," << i.value().password
            << "\n";
        qDebug() << "Saved user:" << i.key() << i.value().username;
    }

    file.close();
    qDebug() << "Total users saved:" << userMap.size();
}

bool server::checkUser(const QString &email, const QString &password) {
    if (!isValidEmail(email)) {
        return false;
    }
    if (userMap.contains(email)) {
        if (userMap.value(email).password == password) {
            return true;
        }
    }
    return false;
}

void server::addUser(const QString &email, const QString &password) {
    UserData userData;
    userData.password = password;
    userMap.insert(email, userData);
    saveUsersAccounts();
}

bool server::isValidEmail(const QString &email) {
    // Basic email validation
    QRegExp emailRegex("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    return emailRegex.exactMatch(email);
}

bool server::isValidPassword(const QString &password) {
    // Password must be at least 4 characters
    return password.length() >= 4;
}

bool server::registerUser(const QString &username, const QString &email,
                          const QString &password,
                          const QString &confirmPassword,
                          QString &errorMessage) {
    // Validate email format
    if (!isValidEmail(email)) {
        errorMessage =
            "Please enter a valid email address (e.g., user@example.com)";
        return false;
    }

    // Check if email already exists
    if (userMap.contains(email)) {
        errorMessage = "This email is already registered. Please use a "
                       "different email or try logging in.";
        return false;
    }

    // Validate username
    if (username.trimmed().isEmpty()) {
        errorMessage = "Please enter a nickname";
        return false;
    }

    // Validate password
    if (!isValidPassword(password)) {
        errorMessage = "Password must be at least 4 characters long";
        return false;
    }

    // Check if passwords match
    if (password != confirmPassword) {
        errorMessage = "Passwords do not match. Please make sure both "
                       "passwords are identical.";
        return false;
    }

    // Create new user
    UserData userData;
    userData.username = username;
    userData.password = password;
    userMap.insert(email, userData);

    // Save to file
    saveUsersAccounts();
    return true;
}

bool server::resetPassword(const QString &email, const QString &newPassword,
                           const QString &confirmPassword,
                           QString &errorMessage) {
    // Check if email exists
    if (!isValidEmail(email)) {
        errorMessage =
            "Please enter a valid email address (e.g., user@example.com)";
        return false;
    }

    if (!userMap.contains(email)) {
        errorMessage = "No account found with this email address. Please check "
                       "your email or sign up.";
        return false;
    }

    // Validate new password
    if (!isValidPassword(newPassword)) {
        errorMessage = "Password must be at least 4 characters long";
        return false;
    }

    // Check if passwords match
    if (newPassword != confirmPassword) {
        errorMessage = "Passwords do not match. Please make sure both "
                       "passwords are identical.";
        return false;
    }

    // Update the password
    userMap[email].password = newPassword;

    // Save changes to file
    saveUsersAccounts();
    return true;
}

QVector<QPair<QString, QString>> server::getAllUsers() const {
    QVector<QPair<QString, QString>> users;
    QMapIterator<QString, UserData> i(userMap);
    while (i.hasNext()) {
        i.next();
        users.append(qMakePair(i.key(), i.value().username));
    }
    return users;
}

Client *server::loginUser(const QString &email, const QString &password) {
    if (!checkUser(email, password)) {
        return nullptr;
    }

    // Create new client if doesn't exist
    if (!clients.contains(email)) {
        Client *newClient = new Client(email, userMap[email].username, email);
        clients[email] = newClient;
        loadClientData(newClient);
    }

    currentClient = clients[email];
    return currentClient;
}

void server::logoutUser() {
    if (currentClient) {
        saveClientData(currentClient);
        currentClient = nullptr;
    }
}

void server::loadClientData(Client *client) {
    // Create necessary directories
    QDir dir;
    dir.mkpath("../db/users");
    dir.mkpath("../db/rooms");

    // Load user's data file
    QString filename = "../db/users/" + client->getUserId() + ".txt";
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);

        // Load contacts and rooms
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("CONTACT:")) {
                QString contactId = line.mid(8);
                client->addContact(contactId);
            } else if (line.startsWith("ROOM:")) {
                QStringList parts = line.mid(5).split('|');
                if (parts.size() >= 2) {
                    QString roomId = parts[0];
                    QString roomName = parts[1];

                    // Create room if it doesn't exist
                    Room *room = client->getRoom(roomId);
                    if (!room) {
                        // Extract the other user's ID from the room name
                        QStringList userIds = roomName.split("_");
                        QString otherUserId = userIds[0] == client->getUserId() ? userIds[1] : userIds[0];
                        room = client->createRoom(otherUserId);
                    }
                }
            }
        }
    }

    // Load all rooms with messages
    QVector<Room *> rooms = client->getAllRooms();
    for (Room *room : rooms) {
        // Use a map to track unique messages
        QMap<QString, Message> uniqueMessages;
        
        // Load room messages
        QString roomFile = "../db/rooms/" + room->getRoomId() + ".txt";
        QFile roomDataFile(roomFile);
        if (roomDataFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream roomIn(&roomDataFile);
            while (!roomIn.atEnd()) {
                QString msgLine = roomIn.readLine().trimmed();
                if (!msgLine.isEmpty()) {
                    Message msg = Message::fromString(msgLine);
                    // Use timestamp and sender as unique key
                    QString key = msg.getTimestamp().toString(Qt::ISODate) + "_" + msg.getSender();
                    uniqueMessages[key] = msg;
                }
            }
            roomDataFile.close();
        }
        
        // Clear existing messages to prevent duplicates
        room->clearMessages();
        
        // Add unique messages to room
        for (const Message &msg : uniqueMessages.values()) {
            room->addMessage(msg);
        }
    }

    // Also check for rooms where this user is a participant but not in their contacts
    QDir roomsDir("../db/rooms");
    QStringList roomFiles = roomsDir.entryList(QDir::Files);
    for (const QString &roomFile : roomFiles) {
        QString roomId = roomFile;
        if (roomId.endsWith(".txt")) {
            roomId.chop(4); // Remove .txt extension
        }
        
        // Skip rooms we already loaded
        if (client->getRoom(roomId)) {
            continue;
        }
        
        QFile file(roomsDir.filePath(roomFile));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            bool hasMessages = false;
            QString otherUserId;

            // Check messages to see if this user is a participant
            while (!in.atEnd()) {
                QString msgLine = in.readLine().trimmed();
                if (!msgLine.isEmpty()) {
                    Message msg = Message::fromString(msgLine);
                    // If this user is either sender or receiver, create the room
                    if (msg.getSender() == client->getUserId()) {
                        hasMessages = true;
                        
                        // Extract room name to get other user ID
                        QStringList parts = roomId.split("_");
                        for (const QString &part : parts) {
                            if (part != client->getUserId()) {
                                otherUserId = part;
                                break;
                            }
                        }
                        break;
                    }
                }
            }

            if (hasMessages && !otherUserId.isEmpty()) {
                // Create room if it doesn't exist
                Room *room = client->getRoomWithUser(otherUserId);
                if (!room) {
                    room = client->createRoom(otherUserId);
                }

                // Load all messages
                QMap<QString, Message> uniqueMessages;
                file.seek(0); // Go back to start of file
                
                while (!in.atEnd()) {
                    QString msgLine = in.readLine().trimmed();
                    if (!msgLine.isEmpty()) {
                        Message msg = Message::fromString(msgLine);
                        QString key = msg.getTimestamp().toString(Qt::ISODate) + "_" + msg.getSender();
                        uniqueMessages[key] = msg;
                    }
                }
                
                // Add unique messages to room
                for (const Message &msg : uniqueMessages.values()) {
                    room->addMessage(msg);
                }
            }
            file.close();
        }
    }
}

void server::saveClientData(Client *client) {
    // Create necessary directories
    QDir dir;
    dir.mkpath("../db/users");
    dir.mkpath("../db/rooms");

    // Save user's data file
    QString filename = "../db/users/" + client->getUserId() + ".txt";
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);

        // Save contacts
        QVector<QString> contacts = client->getContacts();
        for (const QString &contactId : contacts) {
            out << "CONTACT:" << contactId << "\n";
        }

        // Save rooms
        QVector<Room *> rooms = client->getAllRooms();
        for (Room *room : rooms) {
            out << "ROOM:" << room->getRoomId() << "|" << room->getName() << "\n";

            // Save room messages to a single file
            // We'll save a temporary map of messages by timestamp to avoid duplicates
            QMap<QString, Message> uniqueMessages;
            
            // First load existing messages to avoid duplicates
            QString roomFile = "../db/rooms/" + room->getRoomId() + ".txt";
            QFile roomDataFile(roomFile);
            if (roomDataFile.exists() && roomDataFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream roomIn(&roomDataFile);
                while (!roomIn.atEnd()) {
                    QString msgLine = roomIn.readLine().trimmed();
                    if (!msgLine.isEmpty()) {
                        Message msg = Message::fromString(msgLine);
                        // Use timestamp and sender as a unique key
                        QString key = msg.getTimestamp().toString(Qt::ISODate) + "_" + msg.getSender();
                        uniqueMessages[key] = msg;
                    }
                }
                roomDataFile.close();
            }
            
            // Add new messages, overwriting any with the same timestamp and sender
            QVector<Message> newMessages = room->getMessages();
            for (const Message &msg : newMessages) {
                QString key = msg.getTimestamp().toString(Qt::ISODate) + "_" + msg.getSender();
                uniqueMessages[key] = msg;
            }
            
            // Now write all unique messages back to file
            if (roomDataFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream roomOut(&roomDataFile);
                for (const Message &msg : uniqueMessages.values()) {
                    roomOut << msg.toString() << "\n";
                }
                roomDataFile.close();
            }
        }
    }
}

void server::saveAllClientsData() {
    for (Client *client : clients) {
        saveClientData(client);
    }
}

server::~server() {
    saveAllClientsData();
    // Clean up clients
    for (Client *client : clients) {
        delete client;
    }
    clients.clear();
    currentClient = nullptr;
    saveUsersAccounts();
}
