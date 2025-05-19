// server.cpp
#include "server.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QRegExp>
#include <QStack>
#include <QTextStream>
#include <qglobal.h>

// Initialize static member
server *server::instance = nullptr;

server::server() : currentClient(nullptr) { loadAllData(); }

server *server::getInstance() {
    // Create the instance if it doesn't exist
    if (instance == nullptr) {
        instance = new server();
    }
    return instance;
}

void server::loadAllData() {
    // Qobesy

    qDebug() << "Loading all data from files...";

    // Create necessary directories
    QDir dir;
    dir.mkpath("../db/users_credentials");
    dir.mkpath("../db/settings");
    dir.mkpath("../db/rooms");
    dir.mkpath("../db/users");
    dir.mkpath("../db/avatars");

    // Load user accounts
    loadUsersAccounts();

    // Create default settings files for all users if they don't exist
    createDefaultSettingsFiles();

    // Load user settings
    QDir settingsDir("../db/settings");
    QStringList settingsFiles = settingsDir.entryList(QDir::Files);

    for (const QString &settingsFile : settingsFiles) {
        if (settingsFile.endsWith("_settings.txt")) {
            // Extract email from filename
            QString email = settingsFile;
            email.chop(13); // Remove "_settings.txt"

            if (userMap.contains(email)) {
                QString settingsPath = "../db/settings/" + settingsFile;
                QFile file(settingsPath);

                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QTextStream in(&file);

                    while (!in.atEnd()) {
                        QString line = in.readLine();

                        if (line.startsWith("NICKNAME:")) {
                            userMap[email].nickname = line.mid(9).trimmed();
                        } else if (line.startsWith("BIO:")) {
                            userMap[email].bio = line.mid(4).trimmed();
                        } else if (line.startsWith("ONLINE:")) {
                            QString onlineValue = line.mid(7).trimmed();
                            userMap[email].isOnline = (onlineValue == "true");
                        } else if (line.startsWith("LAST_CHANGE:")) {
                            QString lastChangeStr = line.mid(12).trimmed();
                            userMap[email].lastStatusChange =
                                QDateTime::fromString(lastChangeStr,
                                                      Qt::ISODate);
                            if (!userMap[email].lastStatusChange.isValid()) {
                                userMap[email].lastStatusChange =
                                    QDateTime::currentDateTime();
                            }
                        } else if (line.startsWith("AVATAR:")) {
                            userMap[email].avatarPath = line.mid(7).trimmed();
                            qDebug() << "Loaded avatar path for user:" << email
                                     << "-" << userMap[email].avatarPath;
                        }
                    }

                    file.close();
                    qDebug() << "Loaded settings for user:" << email;
                }
            }
        }
    }

    // Migrate any inconsistent room files
    migrateRoomFiles();
    
    // Repair any room files that might use nicknames instead of emails
    repairInconsistentRoomFiles();

    // Load room messages
    QDir roomsDir("../db/rooms");
    QStringList roomFiles = roomsDir.entryList(QDir::Files);
    qDebug() << "Loading" << roomFiles.size() << "room files...";

    for (const QString &roomFile : roomFiles) {
        QString roomId = roomFile;
        if (roomId.endsWith(".txt")) {
            roomId.chop(4); // Remove .txt extension
        }

        // Open room file and load messages
        QFile file(roomsDir.filePath(roomFile));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QList<Message> messageList;

            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (!line.isEmpty()) {
                    Message msg = Message::fromString(line);
                    messageList.append(msg);
                }
            }

            file.close();
            
            roomMessages[roomId] = messageList;
            qDebug() << "Loaded" << messageList.size() << "messages for room" << roomId;
        }
    }

    // Load user data (contacts, rooms, settings)
    QDir usersDir("../db/users");
    QStringList userFiles = usersDir.entryList(QDir::Files);
    qDebug() << "Loading" << userFiles.size() << "user files...";

    for (const QString &userFile : userFiles) {
        QString userId = userFile;
        if (userId.endsWith(".txt")) {
            userId.chop(4); // Remove .txt extension
        }
        // qobesy

        loadUserContacts(userId);
    }

    qDebug() << "All data loaded successfully.";
}

void server::saveAllData() {
    qDebug() << "Saving all data to files...";

    // Create necessary directories
    QDir dir;
    dir.mkpath("../db/users_credentials");
    dir.mkpath("../db/settings");
    dir.mkpath("../db/rooms");
    dir.mkpath("../db/users");

    // Save user accounts - save to users_credentials directory
    QString credentialsPath = "../db/users_credentials/credentials.txt";
    QFile credFile(credentialsPath);

    if (credFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&credFile);
        QMapIterator<QString, UserData> i(userMap);
        while (i.hasNext()) {
            i.next();
            out << i.key() << "," << i.value().username << ","
                << i.value().password << "\n";
            qDebug() << "Saved user credentials:" << i.key()
                     << i.value().username;
        }
        credFile.close();
        qDebug() << "Total user credentials saved:" << userMap.size();
    } else {
        qDebug() << "Failed to save user credentials:"
                 << credFile.errorString();
    }

    // Save user settings - each user gets their own settings file
    QMapIterator<QString, UserData> userIter(userMap);
    while (userIter.hasNext()) {
        userIter.next();

        QString email = userIter.key();
        const UserData &userData = userIter.value();

        QString settingsPath = "../db/settings/" + email + "_settings.txt";
        QFile settingsFile(settingsPath);

        if (settingsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&settingsFile);
            out << "NICKNAME:" << userData.nickname << "\n";
            out << "BIO:" << userData.bio << "\n";
            out << "ONLINE:" << (userData.isOnline ? "true" : "false") << "\n";

            // Save avatar path if available
            if (!userData.avatarPath.isEmpty()) {
                out << "AVATAR:" << userData.avatarPath << "\n";
            }

            settingsFile.close();
            qDebug() << "Saved settings for user:" << email;
        } else {
            qDebug() << "Failed to save settings for user:" << email;
        }
    }

    // Save room messages
    QMapIterator<QString, QList<Message>> roomIt(roomMessages);
    while (roomIt.hasNext()) {
        roomIt.next();

        QString roomId = roomIt.key();
        const QList<Message> &messages = roomIt.value();

        QFile file("../db/rooms/" + roomId + ".txt");
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            
            // Write messages in chronological order
            for (const Message &msg : messages) {
                out << msg.toString() << "\n";
            }

            file.close();
            qDebug() << "Saved" << messages.size() << "messages for room" << roomId;
        } else {
            qDebug() << "Failed to save messages for room" << roomId;
        }
    }

    // Save user data (contacts, rooms)
    QMapIterator<QString, QVector<QString>> userIt(userContacts);
    while (userIt.hasNext()) {
        userIt.next();

        QString userId = userIt.key();
        saveUserContacts(userId);
    }

    qDebug() << "All data saved successfully.";
}

void server::loadUsersAccounts() {
    QString filePath = "../db/users_credentials/credentials.txt";
    QFile file(filePath);
    qDebug() << "Loading users from:" << filePath;

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file for reading:" << file.errorString();
        return;
    }

    userMap.clear();
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList parts = line.split(',');
        if (parts.size() >= 3) {
            QString email = parts[0].trimmed();
            QString username = parts[1].trimmed();
            QString password = parts[2].trimmed();

            UserData userData;
            userData.username = username;
            userData.password = password;
            userData.nickname = username; // Default nickname is username
            userData.bio = "";            // Default empty bio
            userData.isOnline = false;    // Default offline status

            userMap.insert(email, userData);
            qDebug() << "Loaded user:" << email << username;
        }
    }
    file.close();
    qDebug() << "Total users loaded:" << userMap.size();
}

void server::loadUserContacts(const QString &userId) {
    // Load the user file
    QString filename = "../db/users/" + userId + ".txt";
    QFile file(filename);

    QVector<QString> contacts;
    QMap<QString, Room *> rooms;

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);

        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();

            if (line.startsWith("CONTACT:")) {
                QString contactId = line.mid(8);
                contacts.append(contactId);
                qDebug() << "Loaded contact for" << userId << ":" << contactId;
            } else if (line.startsWith("ROOM:")) {
                QStringList parts = line.mid(5).split('|');
                if (parts.size() >= 2) {
                    QString roomId = parts[0];
                    QString roomName = parts[1];

                    // Create room and add it to the rooms map
                    Room *room = new Room(roomName);
                    room->setRoomId(roomId);
                    rooms[roomId] = room;

                    qDebug() << "Loaded room for" << userId << ":" << roomId
                             << roomName;
                }
            } else if (line.startsWith("NICKNAME:")) {
                QString nickname = line.mid(9).trimmed();
                if (userMap.contains(userId)) {
                    userMap[userId].nickname = nickname;
                    qDebug()
                        << "Loaded nickname for" << userId << ":" << nickname;
                }
            } else if (line.startsWith("BIO:")) {
                QString bio = line.mid(4).trimmed();
                if (userMap.contains(userId)) {
                    userMap[userId].bio = bio;
                    qDebug() << "Loaded bio for" << userId << ":" << bio;
                }
            }
        }

        file.close();
    }

    userContacts[userId] = contacts;
    userRooms[userId] = rooms;

    // Also load blocked users if available
    QString blockedPath = "../db/users/" + userId + "_blocked.txt";
    QFile blockedFile(blockedPath);

    if (blockedFile.exists() &&
        blockedFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QVector<QString> blocked;
        QTextStream in(&blockedFile);

        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty()) {
                blocked.append(line);
                qDebug() << "Loaded blocked user for" << userId << ":" << line;
            }
        }

        blockedFile.close();
        blockedUsers[userId] = blocked;
    }
}

void server::saveUserContacts(const QString &userId) {
    QString filename = "../db/users/" + userId + ".txt";
    QFile file(filename);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);

        // Save contacts
        if (userContacts.contains(userId)) {
            const QVector<QString> &contacts = userContacts[userId];
            for (const QString &contactId : contacts) {
                out << "CONTACT:" << contactId << "\n";
            }
        }

        // Save rooms
        if (userRooms.contains(userId)) {
            const QMap<QString, Room *> &rooms = userRooms[userId];
            QMapIterator<QString, Room *> i(rooms);
            while (i.hasNext()) {
                i.next();
                out << "ROOM:" << i.key() << "|" << i.value()->getName()
                    << "\n";
            }
        }

        // Save nickname and bio
        if (userMap.contains(userId)) {
            const UserData &userData = userMap[userId];
            if (!userData.nickname.isEmpty()) {
                out << "NICKNAME:" << userData.nickname << "\n";
            }
            if (!userData.bio.isEmpty()) {
                out << "BIO:" << userData.bio << "\n";
            }
        }

        file.close();
        qDebug() << "Saved user data for" << userId;
    } else {
        qDebug() << "Failed to save user data for" << userId << ":"
                 << file.errorString();
    }
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
    userData.username =
        email.split('@')[0]; // Default username is part before @
    userData.password = password;
    userData.nickname = userData.username;
    userData.bio = "";
    userData.isOnline = false;
    userData.lastStatusChange = QDateTime::currentDateTime();

    userMap.insert(email, userData);
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
    userData.nickname = username;
    userData.bio = "";
    userData.isOnline = false;
    userData.lastStatusChange = QDateTime::currentDateTime();

    // Add user to in-memory map
    userMap.insert(email, userData);

    // Save to credentials file immediately
    saveUsersAccounts();

    // Create default settings file for the new user
    QString settingsPath = "../db/settings/" + email + "_settings.txt";
    QFile settingsFile(settingsPath);
    if (settingsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&settingsFile);
        out << "NICKNAME:" << username << "\n";
        out << "BIO:Welcome to my profile!\n"; // Default bio
        out << "ONLINE:false\n";               // Default to offline
        settingsFile.close();
        qDebug() << "Created settings file for new user:" << email;
    }

    qDebug() << "Registered new user:" << email << username;
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
    return true;
}

QVector<QPair<QString, QString>> server::getAllUsers() const {
    QVector<QPair<QString, QString>> users;
    QMapIterator<QString, UserData> i(userMap);
    while (i.hasNext()) {
        i.next();
        users.append(qMakePair(i.key(), i.value().nickname));
    }
    return users;
}

Client *server::loginUser(const QString &email, const QString &password) {
    if (!checkUser(email, password)) {
        return nullptr;
    }

    // Read current online status before doing anything else
    bool wasOnline = userMap[email].isOnline;

    // Create new client if doesn't exist
    if (!clients.contains(email)) {
        // IMPORTANT: Always use email as the userId to ensure consistency
        Client *newClient = new Client(email, userMap[email].username, email);
        clients[email] = newClient;
        loadClientData(newClient);
        qDebug() << "Created new client with userId/email:" << email
                 << "and username:" << userMap[email].username;
    }

    currentClient = clients[email];

    // Preserve the previous online status instead of forcing offline
    qDebug() << "User" << email << "logged in with status preserved as"
             << (wasOnline ? "online" : "offline");

    return currentClient;
}

void server::logoutUser() {
    if (currentClient) {
        // Get the user ID before we reset currentClient
        QString userId = currentClient->getUserId();
        bool currentStatus = false;

        // Remember the current online status
        if (userMap.contains(userId)) {
            currentStatus = userMap[userId].isOnline;
        }

        // Save client data
        saveClientData(currentClient);

        // Clear currentClient without changing online status
        currentClient = nullptr;

        qDebug() << "User" << userId << "logged out. Status preserved as"
                 << (currentStatus ? "online" : "offline");
    }
}

void server::loadClientData(Client *client) {
    if (!client)
        return;

    QString userId = client->getUserId();

    // Load client data from our in-memory structures
    if (userContacts.contains(userId)) {
        // Add contacts
        const QVector<QString> &contacts = userContacts[userId];
        for (const QString &contactId : contacts) {
            client->addContact(contactId);
        }
    }

    // Load rooms
    if (userRooms.contains(userId)) {
        const QMap<QString, Room *> &rooms = userRooms[userId];
        QMapIterator<QString, Room *> i(rooms);
        while (i.hasNext()) {
            i.next();
            Room *room = i.value();

            // Add room to client
            client->addRoom(room);

            // Add messages to room
            if (roomMessages.contains(room->getRoomId())) {
                const QList<Message> &messages =
                    roomMessages[room->getRoomId()];
                room->setMessages(messages);
            }
        }
    }
}

void server::saveClientData(Client *client) {
    if (!client)
        return;

    QString userId = client->getUserId();

    // Save contacts
    QVector<QString> contacts = client->getContacts();
    userContacts[userId] = contacts;

    // Save rooms
    QMap<QString, Room *> rooms;
    QVector<Room *> clientRooms = client->getAllRooms();

    for (Room *room : clientRooms) {
        rooms[room->getRoomId()] = room;

        // Save room messages
        const QList<Message> &messages = room->getMessages();
        roomMessages[room->getRoomId()] = messages;
    }

    userRooms[userId] = rooms;
}

void server::saveAllClientsData() {
    for (Client *client : clients) {
        saveClientData(client);
    }
}

bool server::deleteUser(const QString &userId) {
    // Check if the user exists in our userMap
    if (!userMap.contains(userId)) {
        qDebug() << "User" << userId << "not found for deletion";
        return false;
    }

    qDebug() << "Deleting user account:" << userId;

    // Remove from clients map and delete the client object
    if (clients.contains(userId)) {
        Client *clientToDelete = clients[userId];

        // If this is the current client, set to null first
        if (currentClient == clientToDelete) {
            currentClient = nullptr;
        }

        // Delete the client object
        delete clientToDelete;
        clients.remove(userId);
    }

    // Remove all user data from in-memory structures
    userMap.remove(userId);
    userContacts.remove(userId);

    // Remove rooms associated with this user and their messages
    if (userRooms.contains(userId)) {
        QMap<QString, Room *> &rooms = userRooms[userId];
        QMapIterator<QString, Room *> i(rooms);
        while (i.hasNext()) {
            i.next();
            QString roomId = i.key();
            delete i.value();            // Delete Room object
            roomMessages.remove(roomId); // Remove messages
        }

        userRooms.remove(userId);
    }

    qDebug() << "User" << userId << "deleted successfully";
    return true;
}

bool server::updateUserSettings(const QString &userId, const QString &nickname,
                                const QString &bio) {
    if (!userMap.contains(userId)) {
        qDebug() << "User" << userId << "not found for updating settings";
        return false;
    }

    // Save the current avatar path before updating other settings
    QString avatarPath = userMap[userId].avatarPath;

    userMap[userId].nickname = nickname;
    userMap[userId].bio = bio;

    // Save settings immediately to ensure persistence
    QString settingsPath = "../db/settings/" + userId + "_settings.txt";
    QFile settingsFile(settingsPath);

    if (settingsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&settingsFile);
        out << "NICKNAME:" << nickname << "\n";
        out << "BIO:" << bio << "\n";
        out << "ONLINE:" << (userMap[userId].isOnline ? "true" : "false")
            << "\n";

        // Preserve the avatar path in the settings file
        if (!avatarPath.isEmpty()) {
            out << "AVATAR:" << avatarPath << "\n";
        }

        settingsFile.close();
        qDebug() << "Saved settings for user:" << userId << "with avatar path:"
                 << (!avatarPath.isEmpty() ? avatarPath : "none");
    }

    qDebug() << "Updated settings for user" << userId
             << "- Nickname:" << nickname << "Bio:" << bio;
    return true;
}

bool server::getUserSettings(const QString &userId, QString &nickname,
                             QString &bio) {
    // Call the new version with a dummy avatar path
    QString dummyPath;
    return getUserSettings(userId, nickname, bio, dummyPath);
}

bool server::getUserSettings(const QString &userId, QString &nickname,
                             QString &bio, QString &avatarPath) {
    qDebug() << "Getting settings for user ID:" << userId;
    
    if (!userMap.contains(userId)) {
        qDebug() << "ERROR: User" << userId << "not found in user map";
        return false;
    }

    nickname = userMap[userId].nickname;
    bio = userMap[userId].bio;
    avatarPath = userMap[userId].avatarPath;

    qDebug() << "Retrieved settings - User:" << userId 
             << "Nickname:" << nickname 
             << "Bio:" << (bio.isEmpty() ? "[empty]" : "[set]")
             << "Avatar:" << (avatarPath.isEmpty() ? "[none]" : avatarPath);
    
    return true;
}

bool server::setUserOnlineStatus(const QString &userId, bool isOnline) {
    if (!userMap.contains(userId)) {
        qDebug() << "User" << userId << "not found for updating online status";
        return false;
    }

    // Save the current avatar path before updating status
    QString avatarPath = userMap[userId].avatarPath;

    // Only update if status actually changes
    if (userMap[userId].isOnline != isOnline) {
        userMap[userId].isOnline = isOnline;
        userMap[userId].lastStatusChange = QDateTime::currentDateTime();
        qDebug() << "Set user" << userId << "online status to"
                 << (isOnline ? "online" : "offline") << "at"
                 << userMap[userId].lastStatusChange.toString();
    }

    // Immediately save the settings to ensure persistence
    QString settingsPath = "../db/settings/" + userId + "_settings.txt";
    QFile settingsFile(settingsPath);

    if (settingsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&settingsFile);
        out << "NICKNAME:" << userMap[userId].nickname << "\n";
        out << "BIO:" << userMap[userId].bio << "\n";
        out << "ONLINE:" << (isOnline ? "true" : "false") << "\n";
        out << "LAST_CHANGE:"
            << userMap[userId].lastStatusChange.toString(Qt::ISODate) << "\n";

        // Preserve the avatar path in the settings file
        if (!avatarPath.isEmpty()) {
            out << "AVATAR:" << avatarPath << "\n";
        }

        settingsFile.close();
        qDebug() << "Saved online status for user:" << userId
                 << "with avatar path:"
                 << (!avatarPath.isEmpty() ? avatarPath : "none");
    }

    return true;
}

bool server::isUserOnline(const QString &userId) const {
    if (!userMap.contains(userId)) {
        return false;
    }

    return userMap[userId].isOnline;
}

server::~server() {
    // Save all data before shutting down
    saveAllData();

    // Clean up clients
    for (Client *client : clients) {
        delete client;
    }
    clients.clear();
    currentClient = nullptr;
}

// Migrate inconsistent room files (unchanged)
void server::migrateRoomFiles() {
    qDebug() << "Checking for inconsistent room files to migrate...";

    // Create rooms directory if it doesn't exist
    QDir dir;
    dir.mkpath("../db/rooms");

    // Get all room files
    QDir roomsDir("../db/rooms");
    QStringList roomFiles = roomsDir.entryList(QDir::Files);

    QStringList migratedFiles;
    QStringList problemFiles;

    // First handle nickname vs email conflicts
    qDebug() << "Looking for nickname vs email conflicts...";

    // Group room files by user pairs (ignoring formatting differences)
    QMap<QString, QStringList> userPairToRoomIds;

    for (const QString &filename : roomFiles) {
        if (!filename.endsWith(".txt"))
            continue;

        QString roomId = filename;
        roomId.chop(4); // Remove .txt extension

        // Check if this is a valid room ID format
        if (!roomId.contains("_")) {
            problemFiles << roomId;
            continue;
        }

        QStringList users = roomId.split("_");
        if (users.size() != 2) {
            problemFiles << roomId;
            continue;
        }

        // Normalize users to emails for consistent matching
        QString user1 = users[0];
        QString user2 = users[1];

        // We must use the exact user IDs without any transformation
        // Email addresses should already be used properly from other fixes

        // Create a consistent key regardless of order
        QString userPairKey =
            user1 < user2 ? user1 + "+" + user2 : user2 + "+" + user1;

        if (!userPairToRoomIds.contains(userPairKey)) {
            userPairToRoomIds[userPairKey] = QStringList();
        }

        userPairToRoomIds[userPairKey].append(roomId);
    }

    // For any user pair with multiple room IDs, merge them
    QMapIterator<QString, QStringList> i(userPairToRoomIds);
    while (i.hasNext()) {
        i.next();

        if (i.value().size() > 1) {
            qDebug() << "Found conflict for users:" << i.key() << "with"
                     << i.value().size() << "room files";

            // Extract the canonical users from the key
            QStringList canonicalUsers = i.key().split("+");
            QString canonicalUser1 = canonicalUsers[0];
            QString canonicalUser2 = canonicalUsers[1];

            // Generate the correct room ID
            QString correctRoomId =
                Room::generateRoomId(canonicalUser1, canonicalUser2);
            QString correctPath = "../db/rooms/" + correctRoomId + ".txt";

            qDebug() << "Canonical room ID should be:" << correctRoomId;

            // Create the correct file if it doesn't exist
            if (!QFile::exists(correctPath)) {
                QFile::copy("../db/rooms/" + i.value().first() + ".txt",
                            correctPath);
            }

            // Merge all files into the canonical one
            for (const QString &roomId : i.value()) {
                if (roomId == correctRoomId)
                    continue; // Skip if it's the canonical one

                QString roomPath = "../db/rooms/" + roomId + ".txt";

                qDebug() << "Merging" << roomId << "into" << correctRoomId;

                QFile roomFile(roomPath);
                QFile correctFile(correctPath);

                if (roomFile.open(QIODevice::ReadOnly | QIODevice::Text) &&
                    correctFile.open(QIODevice::Append | QIODevice::Text)) {

                    QTextStream in(&roomFile);
                    QTextStream out(&correctFile);

                    // Copy all messages
                    while (!in.atEnd()) {
                        QString line = in.readLine();
                        if (!line.trimmed().isEmpty()) {
                            out << line << "\n";
                        }
                    }

                    roomFile.close();
                    correctFile.close();

                    // Delete the duplicate file
                    QFile::remove(roomPath);
                    migratedFiles << roomId + " -> " << correctRoomId;
                }
            }
        }
    }

    // Now update user files to reference the correct room IDs
    QDir usersDir("../db/users");
    QStringList userFiles = usersDir.entryList(QDir::Files);

    for (const QString &userFile : userFiles) {
        if (!userFile.endsWith(".txt"))
            continue;

        QString userId = userFile;
        userId.chop(4); // Remove .txt extension

        QString filePath = "../db/users/" + userFile;
        QFile file(filePath);

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

        QStringList lines;
        QTextStream in(&file);
        bool needsUpdate = false;

        while (!in.atEnd()) {
            QString line = in.readLine();

            if (line.startsWith("ROOM:")) {
                QStringList parts = line.mid(5).split('|');
                if (parts.size() >= 2) {
                    QString roomId = parts[0];
                    QString roomName = parts[1];

                    // Check if this room ID needs normalization
                    QStringList users = roomName.split("_");
                    if (users.size() == 2) {
                        QString user1 = users[0];
                        QString user2 = users[1];

                        // Use the exact user IDs without any transformation

                        QString correctRoomId =
                            Room::generateRoomId(user1, user2);
                        QString correctRoomName = user1 < user2
                                                      ? user1 + "_" + user2
                                                      : user2 + "_" + user1;

                        if (roomId != correctRoomId ||
                            roomName != correctRoomName) {
                            line =
                                "ROOM:" + correctRoomId + "|" + correctRoomName;
                            needsUpdate = true;
                            qDebug() << "Updated room reference:" << roomId
                                     << "->" << correctRoomId;
                        }
                    }
                }
            }

            lines.append(line);
        }

        file.close();

        if (needsUpdate) {
            // Write the updated file
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                for (const QString &line : lines) {
                    out << line << "\n";
                }
                file.close();
                qDebug() << "Updated room references in user file:" << userId;
            }
        }
    }

    if (!migratedFiles.isEmpty()) {
        qDebug() << "Migrated" << migratedFiles.size() << "room files:";
        for (const QString &info : migratedFiles) {
            qDebug() << "  " << info;
        }
    } else {
        qDebug() << "No room files needed migration.";
    }

    if (!problemFiles.isEmpty()) {
        qDebug() << "Problem files that couldn't be fixed automatically:";
        for (const QString &info : problemFiles) {
            qDebug() << "  " << info;
        }
    }
}

// Re-add saveUsersAccounts method for backward compatibility
void server::saveUsersAccounts() {
    QString filePath = "../db/users_credentials/credentials.txt";
    QFile file(filePath);
    qDebug() << "Saving users to:" << filePath;

    // Create the directory if it doesn't exist
    QDir dir("../db/users_credentials");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

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

void server::createDefaultSettingsFiles() {
    qDebug() << "Creating default settings files for all users...";

    // Create settings directory if it doesn't exist
    QDir dir;
    if (!dir.exists("../db/settings")) {
        dir.mkpath("../db/settings");
    }

    QMapIterator<QString, UserData> i(userMap);
    int filesCreated = 0;

    while (i.hasNext()) {
        i.next();

        QString email = i.key();
        const UserData &userData = i.value();

        // Check if settings file already exists
        QString settingsPath = "../db/settings/" + email + "_settings.txt";
        QFile settingsFile(settingsPath);

        if (!settingsFile.exists()) {
            // Create default settings file
            if (settingsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&settingsFile);

                // Use username as default nickname if nickname is empty
                QString nickname = userData.nickname.isEmpty()
                                       ? userData.username
                                       : userData.nickname;

                out << "NICKNAME:" << nickname << "\n";
                out << "BIO:Welcome to my profile!\n"; // Default bio
                out << "ONLINE:false\n";               // Default to offline

                settingsFile.close();
                filesCreated++;
                qDebug() << "Created default settings file for user:" << email;
            } else {
                qDebug() << "Failed to create settings file for user:" << email
                         << ":" << settingsFile.errorString();
            }
        }
    }

    qDebug() << "Created" << filesCreated << "default settings files.";
}

// Update the updateRoomMessages method to use QList
void server::updateRoomMessages(const QString &roomId,
                                const QVector<Message> &messages) {
    // Convert QVector to QList
    setMessageListFromVector(roomId, messages);
    qDebug() << "Updated in-memory messages for room:" << roomId << "with"
             << messages.size() << "messages";
}

// Update the getRoomMessages method to convert from QList to QVector
QVector<Message> server::getRoomMessages(const QString &roomId) const {
    return getMessageVectorFromList(roomId);
}

// New methods for QList implementation

// Add a message to a room's message list
void server::addMessageToRoom(const QString &roomId, const Message &message) {
    roomMessages[roomId].append(message);
    qDebug() << "Added new message to room:" << roomId;
}

// Convert a message list to a vector (for compatibility with existing code)
QVector<Message> server::getMessageVectorFromList(const QString &roomId) const {
    if (!roomMessages.contains(roomId)) {
        return QVector<Message>();
    }
    
    QVector<Message> messageVector;
    
    // Copy messages from list to vector (already in chronological order)
    for (const Message &msg : roomMessages[roomId]) {
        messageVector.append(msg);
    }
    
    return messageVector;
}

// Set a room's message list from a vector
void server::setMessageListFromVector(const QString &roomId, const QVector<Message> &messages) {
    QList<Message> messageList;
    
    // Add messages to list (already in chronological order)
    for (const Message &msg : messages) {
        messageList.append(msg);
    }
    
    roomMessages[roomId] = messageList;
}

bool server::addContactForUser(const QString &userId,
                               const QString &contactId) {
    if (!userMap.contains(userId)) {
        qDebug() << "User" << userId << "not found for adding contact";
        return false;
    }

    // Create contacts vector if it doesn't exist
    if (!userContacts.contains(userId)) {
        userContacts[userId] = QVector<QString>();
    }

    // Add the contact if it's not already there
    if (!userContacts[userId].contains(contactId)) {
        userContacts[userId].append(contactId);

        // Save to disk immediately
        saveUserContacts(userId);
        qDebug() << "Added contact" << contactId << "for user" << userId;
        return true;
    }

    return false; // Contact already existed
}

bool server::hasContactForUser(const QString &userId,
                               const QString &contactId) {
    if (!userContacts.contains(userId)) {
        return false;
    }

    return userContacts[userId].contains(contactId);
}

bool server::addRoomToUser(const QString &userId, Room *room) {
    if (!userMap.contains(userId) || !room) {
        qDebug() << "User" << userId
                 << "not found or invalid room for adding room";
        return false;
    }

    // Create rooms map if it doesn't exist
    if (!userRooms.contains(userId)) {
        userRooms[userId] = QMap<QString, Room *>();
    }

    // Add the room if it's not already there
    QString roomId = room->getRoomId();
    if (!userRooms[userId].contains(roomId)) {
        userRooms[userId][roomId] = room;

        // Also ensure the messages are in the server's message store
        roomMessages[roomId] = room->getMessages();

        // Save to disk immediately
        saveUserContacts(userId);
        qDebug() << "Added room" << roomId << "for user" << userId;
        return true;
    }

    return false; // Room already existed
}

bool server::hasRoomForUser(const QString &userId, const QString &roomId) {
    qDebug() << "Checking if user" << userId << "has room" << roomId;
    
    // First make sure the user exists in our data structure
    if (!userRooms.contains(userId)) {
        qDebug() << "User" << userId << "not found in userRooms map";
        return false;
    }
    
    // Check if the room exists for this user
    bool hasRoom = userRooms[userId].contains(roomId);
    qDebug() << "Result:" << (hasRoom ? "Room found" : "Room not found");
    
    return hasRoom;
}

bool server::updateUserAvatar(const QString &userId,
                              const QString &avatarPath) {
    if (!userMap.contains(userId)) {
        qDebug() << "User" << userId << "not found for updating avatar";
        return false;
    }

    userMap[userId].avatarPath = avatarPath;

    // Save settings immediately to ensure persistence
    QString settingsPath = "../db/settings/" + userId + "_settings.txt";
    QFile settingsFile(settingsPath);

    if (settingsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&settingsFile);
        out << "NICKNAME:" << userMap[userId].nickname << "\n";
        out << "BIO:" << userMap[userId].bio << "\n";
        out << "ONLINE:" << (userMap[userId].isOnline ? "true" : "false")
            << "\n";
        out << "AVATAR:" << avatarPath << "\n"; // Add avatar path to settings

        settingsFile.close();
        qDebug() << "Saved avatar path for user:" << userId
                 << " - Path:" << avatarPath;
    }

    return true;
}

QString server::getUserAvatar(const QString &userId) const {
    if (!userMap.contains(userId)) {
        qDebug() << "User" << userId << "not found for getting avatar";
        return QString();
    }

    return userMap[userId].avatarPath;
}

bool server::changePassword(const QString &userId,
                            const QString &currentPassword,
                            const QString &newPassword,
                            const QString &confirmPassword,
                            QString &errorMessage) {
    // Check if user exists
    if (!userMap.contains(userId)) {
        errorMessage = "User not found. Please try again.";
        qDebug() << "User" << userId << "not found for changing password";
        return false;
    }

    // Verify current password
    if (userMap[userId].password != currentPassword) {
        errorMessage = "Current password is incorrect. Please try again.";
        qDebug() << "Incorrect current password for user" << userId;
        return false;
    }

    // Validate new password
    if (!isValidPassword(newPassword)) {
        errorMessage = "New password must be at least 4 characters long";
        return false;
    }

    // Check if passwords match
    if (newPassword != confirmPassword) {
        errorMessage = "New passwords do not match. Please make sure both "
                       "passwords are identical.";
        return false;
    }

    // Update the password in memory
    userMap[userId].password = newPassword;

    // Save to credentials file immediately
    saveUsersAccounts();

    qDebug() << "Password changed successfully for user" << userId;
    return true;
}

// Story-related methods
void server::loadStories() {
    qDebug() << "Loading stories from disk...";

    // Create stories directory if it doesn't exist
    QDir dir;
    dir.mkpath("../db/stories");

    // Get all story metadata files
    QDir storiesDir("../db/stories");
    QStringList filters;
    filters << "*.meta";
    storiesDir.setNameFilters(filters);
    QStringList storyFiles = storiesDir.entryList(QDir::Files);

    // Clear current stories
    stories.clear();

    // Load each story
    for (const QString &storyFile : storyFiles) {
        QString storyId = storyFile;
        storyId.chop(5); // Remove ".meta" extension

        QFile file(storiesDir.filePath(storyFile));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);

            StoryData story;
            story.id = storyId;

            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();

                if (line.startsWith("USERID:")) {
                    story.userId = line.mid(7).trimmed();
                } else if (line.startsWith("IMAGE:")) {
                    story.imagePath = line.mid(6).trimmed();
                } else if (line.startsWith("CAPTION:")) {
                    story.caption = line.mid(8).trimmed();
                } else if (line.startsWith("TIMESTAMP:")) {
                    QString timestampStr = line.mid(10).trimmed();
                    story.timestamp =
                        QDateTime::fromString(timestampStr, Qt::ISODate);
                    if (!story.timestamp.isValid()) {
                        story.timestamp = QDateTime::currentDateTime();
                    }
                } else if (line.startsWith("VIEWER:")) {
                    // Parse viewer ID and view time
                    QString viewerInfo = line.mid(7).trimmed();
                    QStringList parts = viewerInfo.split('|');
                    QString viewerId = parts.first().trimmed();
                    QDateTime viewTime;

                    if (parts.size() > 1) {
                        viewTime = QDateTime::fromString(parts.last().trimmed(),
                                                         Qt::ISODate);
                    }

                    if (!viewerId.isEmpty()) {
                        story.viewers.insert(viewerId);
                        // Also store the view time if valid
                        if (viewTime.isValid()) {
                            story.viewTimes[viewerId] = viewTime;
                        } else {
                            story.viewTimes[viewerId] =
                                QDateTime::currentDateTime();
                        }
                    }
                }
            }

            file.close();

            // Only add valid stories that aren't expired
            if (!story.userId.isEmpty() && !story.imagePath.isEmpty() &&
                story.timestamp.isValid()) {

                // Check if story is expired (older than 1 hour, changed from 30
                // seconds)
                QDateTime now = QDateTime::currentDateTime();
                QDateTime expirationTime = story.timestamp.addSecs(
                    3600); // Changed from 30 to 3600 seconds (1 hour)

                if (now < expirationTime) {
                    stories.append(story);
                    qDebug()
                        << "Loaded story:" << story.id << "by" << story.userId;
                } else {
                    // Delete expired story files
                    QFile::remove(storiesDir.filePath(storyFile));
                    QFile::remove(story.imagePath);
                    qDebug() << "Removed expired story:" << story.id;
                }
            }
        }
    }

    qDebug() << "Loaded" << stories.size() << "stories";
}

void server::saveStories() {
    qDebug() << "Saving stories to disk...";

    // Create stories directory if it doesn't exist
    QDir dir;
    dir.mkpath("../db/stories");

    // Clean up old stories first
    cleanupOldStories();

    // Save each story
    for (const StoryData &story : stories) {
        QString metaPath = "../db/stories/" + story.id + ".meta";
        QFile file(metaPath);

        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);

            out << "USERID:" << story.userId << "\n";
            out << "IMAGE:" << story.imagePath << "\n";
            out << "CAPTION:" << story.caption << "\n";
            out << "TIMESTAMP:" << story.timestamp.toString(Qt::ISODate)
                << "\n";

            // Save viewers with view time
            QSetIterator<QString> i(story.viewers);
            while (i.hasNext()) {
                QString viewerId = i.next();
                QDateTime viewTime = story.viewTimes.value(viewerId);
                if (viewTime.isValid()) {
                    out << "VIEWER:" << viewerId << "|"
                        << viewTime.toString(Qt::ISODate) << "\n";
                } else {
                    out << "VIEWER:" << viewerId << "\n";
                }
            }

            file.close();
            qDebug() << "Saved story:" << story.id;
        } else {
            qDebug() << "Failed to save story:" << story.id;
        }
    }

    qDebug() << "Saved" << stories.size() << "stories";
}

QString server::addStory(const QString &userId, const QString &imagePath,
                         const QString &caption) {
    if (!userMap.contains(userId)) {
        qDebug() << "Cannot add story: User" << userId << "not found";
        return QString();
    }

    // Create a unique ID for the story
    QString storyId =
        userId + "_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");

    // Create a new story
    StoryData story;
    story.id = storyId;
    story.userId = userId;
    story.imagePath = imagePath;
    story.caption = caption;
    story.timestamp = QDateTime::currentDateTime();

    // Add to the stories list
    stories.append(story);

    // Save to disk
    saveStories();

    qDebug() << "Added new story:" << storyId << "by" << userId;
    return storyId;
}

bool server::deleteStory(const QString &storyId) {
    for (int i = 0; i < stories.size(); ++i) {
        if (stories[i].id == storyId) {
            // Get the image path before removing
            QString imagePath = stories[i].imagePath;

            // Remove from the list
            stories.remove(i);

            // Delete the meta file
            QString metaPath = "../db/stories/" + storyId + ".meta";
            QFile::remove(metaPath);

            // Delete the image file
            QFile::remove(imagePath);

            qDebug() << "Deleted story:" << storyId;

            // Save changes
            saveStories();
            return true;
        }
    }

    qDebug() << "Story not found for deletion:" << storyId;
    return false;
}

bool server::markStoryAsViewed(const QString &storyId,
                               const QString &viewerId) {
    for (int i = 0; i < stories.size(); ++i) {
        if (stories[i].id == storyId) {
            // Add viewer to the set
            stories[i].viewers.insert(viewerId);

            // Record the view time
            stories[i].viewTimes[viewerId] = QDateTime::currentDateTime();

            // Save changes
            saveStories();

            qDebug() << "Marked story" << storyId << "as viewed by" << viewerId
                     << "at" << stories[i].viewTimes[viewerId].toString();
            return true;
        }
    }

    qDebug() << "Story not found for marking as viewed:" << storyId;
    return false;
}

QVector<StoryData> server::getStoriesForUser(const QString &userId) const {
    QVector<StoryData> userStories;

    for (const StoryData &story : stories) {
        if (story.userId == userId) {
            userStories.append(story);
        }
    }

    return userStories;
}

StoryData server::getStory(const QString &storyId) const {
    for (const StoryData &story : stories) {
        if (story.id == storyId) {
            return story;
        }
    }

    return StoryData(); // Return empty story if not found
}

bool server::hasViewedStory(const QString &storyId,
                            const QString &userId) const {
    for (const StoryData &story : stories) {
        if (story.id == storyId) {
            return story.viewers.contains(userId);
        }
    }

    return false; // Story not found
}

void server::cleanupOldStories() {
    QDateTime now = QDateTime::currentDateTime();
    QVector<StoryData> validStories;
    QSet<QString> storiesToRemove; // Track stories that need to be deleted

    // First identify which stories to keep and which to remove
    for (const StoryData &story : stories) {
        // Check if story is expired (older than 1 hour, changed from 30
        // seconds)
        QDateTime expirationTime = story.timestamp.addSecs(
            3600); // Changed from 30 to 3600 seconds (1 hour)

        if (now < expirationTime) {
            // Story is still valid
            validStories.append(story);
        } else {
            // Mark for removal but don't delete files yet
            storiesToRemove.insert(story.id);
            qDebug() << "Marking story for cleanup:" << story.id;
        }
    }

    // Update the stories list first (important for thread safety)
    stories = validStories;

    // Now that the in-memory list is updated, we can safely delete the files
    for (const QString &storyId : storiesToRemove) {
        // Find the image path in case it's needed for deletion
        QString imagePath;
        for (const StoryData &story : validStories) {
            if (story.id == storyId) {
                imagePath = story.imagePath;
                break;
            }
        }

        // If we didn't find the image path in valid stories, it might be
        // available in the meta file, so try to read it
        if (imagePath.isEmpty()) {
            QString metaPath = "../db/stories/" + storyId + ".meta";
            QFile metaFile(metaPath);
            if (metaFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&metaFile);
                while (!in.atEnd()) {
                    QString line = in.readLine();
                    if (line.startsWith("IMAGE:")) {
                        imagePath = line.mid(6).trimmed();
                        break;
                    }
                }
                metaFile.close();
            }
        }

        // Now try to delete files safely
        QString metaPath = "../db/stories/" + storyId + ".meta";
        bool metaDeleted = QFile::remove(metaPath);

        if (!imagePath.isEmpty()) {
            bool imageDeleted = QFile::remove(imagePath);
            qDebug() << "Cleaned up expired story:" << storyId
                     << "- Meta file deleted:" << metaDeleted
                     << "- Image file deleted:" << imageDeleted
                     << "- Image path:" << imagePath;
        } else {
            qDebug() << "Cleaned up expired story:" << storyId
                     << "- Meta file deleted:" << metaDeleted
                     << "- Image file not found";
        }
    }
}

int server::getStoryViewerCount(const QString &storyId) const {
    for (const StoryData &story : stories) {
        if (story.id == storyId) {
            return story.viewers.size();
        }
    }
    return 0;
}

QSet<QString> server::getStoryViewers(const QString &storyId) const {
    for (const StoryData &story : stories) {
        if (story.id == storyId) {
            return story.viewers;
        }
    }
    return QSet<QString>();
}

QDateTime server::getStoryViewTime(const QString &storyId,
                                   const QString &viewerId) const {
    for (const StoryData &story : stories) {
        if (story.id == storyId) {
            return story.viewTimes.value(viewerId);
        }
    }
    return QDateTime(); // Return invalid datetime if not found
}

bool server::blockUserForClient(const QString &clientId,
                                const QString &userToBlock) {
    // Make sure client exists
    if (!userMap.contains(clientId)) {
        qDebug() << "Cannot block user: Client" << clientId << "not found";
        return false;
    }

    // Check if we need to create a new entry for blocked users
    if (!blockedUsers.contains(clientId)) {
        blockedUsers[clientId] = QVector<QString>();
    }

    // Check if already blocked
    if (blockedUsers[clientId].contains(userToBlock)) {
        qDebug() << "User" << userToBlock << "is already blocked by"
                 << clientId;
        return true; // Already blocked, so consider it a success
    }

    // Add to blocked list
    blockedUsers[clientId].append(userToBlock);
    qDebug() << "User" << userToBlock << "blocked by" << clientId;

    // Save to file
    QString blockedPath = "../db/users/" + clientId + "_blocked.txt";
    QFile file(blockedPath);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const QString &user : blockedUsers[clientId]) {
            out << user << "\n";
        }
        file.close();
        qDebug() << "Saved blocked users list for" << clientId;
    } else {
        qDebug() << "Failed to save blocked users for" << clientId;
        // Continue anyway as we've updated the in-memory structure
    }

    // Remove from contacts if present
    Client *client = getClient(clientId);
    if (client && client->hasContact(userToBlock)) {
        client->removeContact(userToBlock);
        qDebug() << "Removed" << userToBlock << "from contacts of" << clientId;
        saveUserContacts(clientId);
    }

    return true;
}

bool server::isUserBlocked(const QString &clientId,
                           const QString &userToCheck) const {
    if (!blockedUsers.contains(clientId)) {
        return false;
    }

    return blockedUsers[clientId].contains(userToCheck);
}

QVector<QString> server::getBlockedUsers(const QString &clientId) const {
    if (!blockedUsers.contains(clientId)) {
        return QVector<QString>();
    }

    return blockedUsers[clientId];
}

// Add the unblockUserForClient method implementation
bool server::unblockUserForClient(const QString &clientId,
                                  const QString &userToUnblock) {
    // Make sure client exists
    if (!userMap.contains(clientId)) {
        qDebug() << "Cannot unblock user: Client" << clientId << "not found";
        return false;
    }

    // Check if we have a blocked users list for this client
    if (!blockedUsers.contains(clientId)) {
        qDebug() << "User" << userToUnblock << "is not blocked by" << clientId
                 << "(no blocked users list)";
        return true; // No blocked users list means nothing to unblock
    }

    // Check if user is blocked
    if (!blockedUsers[clientId].contains(userToUnblock)) {
        qDebug() << "User" << userToUnblock << "is not blocked by" << clientId;
        return true; // Not blocked, so consider it a success
    }

    // Remove from blocked list
    blockedUsers[clientId].removeAll(userToUnblock);
    qDebug() << "User" << userToUnblock << "unblocked by" << clientId;

    // Save to file
    QString blockedPath = "../db/users/" + clientId + "_blocked.txt";
    QFile file(blockedPath);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const QString &user : blockedUsers[clientId]) {
            out << user << "\n";
        }
        file.close();
        qDebug() << "Saved updated blocked users list for" << clientId;
    } else {
        qDebug() << "Failed to save blocked users for" << clientId;
        // Continue anyway as we've updated the in-memory structure
    }

    return true;
}

void server::repairInconsistentRoomFiles() {
    qDebug() << "\n======== REPAIRING INCONSISTENT ROOM FILES ========";
    
    // Load all user emails and their corresponding nicknames
    QMap<QString, QString> nicknameToEmail;
    QMapIterator<QString, UserData> it(userMap);
    while (it.hasNext()) {
        it.next();
        QString email = it.key();
        QString nickname = it.value().nickname;
        if (!nickname.isEmpty()) {
            nicknameToEmail[nickname] = email;
            nicknameToEmail[nickname.toLower()] = email; // Also map lowercase version
            qDebug() << "Mapped nickname" << nickname << "to email" << email;
        }
    }
    
    // STEP 1: Scan and fix any existing room files with nickname-based IDs
    qDebug() << "Step 1: Scanning disk-based room files...";
    QDir roomsDir("../db/rooms");
    QStringList roomFiles = roomsDir.entryList(QDir::Files);
    
    for (const QString &roomFile : roomFiles) {
        if (!roomFile.endsWith(".txt"))
            continue;
        
        QString roomId = roomFile;
        roomId.chop(4); // Remove .txt extension
        
        QStringList parts = roomId.split("_");
        if (parts.size() != 2)
            continue;
        
        QString user1 = parts[0];
        QString user2 = parts[1];
        
        bool needsRepair = false;
        QString repairedUser1 = user1;
        QString repairedUser2 = user2;
        
        // Check if either part is a nickname and needs to be converted to email
        if (!user1.contains("@") && nicknameToEmail.contains(user1)) {
            repairedUser1 = nicknameToEmail[user1];
            needsRepair = true;
            qDebug() << "Will convert nickname" << user1 << "to email" << repairedUser1;
        }
        
        if (!user2.contains("@") && nicknameToEmail.contains(user2)) {
            repairedUser2 = nicknameToEmail[user2];
            needsRepair = true;
            qDebug() << "Will convert nickname" << user2 << "to email" << repairedUser2;
        }
        
        if (needsRepair) {
            // Generate the correct room ID using emails
            QString correctRoomId = Room::generateRoomId(repairedUser1, repairedUser2);
            QString oldPath = "../db/rooms/" + roomId + ".txt";
            QString newPath = "../db/rooms/" + correctRoomId + ".txt";
            
            qDebug() << "Repairing room file:" << roomId << "->" << correctRoomId;
            
            // Don't overwrite an existing correct file
            if (QFile::exists(newPath)) {
                // Merge content instead
                QFile oldFile(oldPath);
                QFile newFile(newPath);
                
                if (oldFile.open(QIODevice::ReadOnly | QIODevice::Text) &&
                    newFile.open(QIODevice::Append | QIODevice::Text)) {
                    
                    QTextStream in(&oldFile);
                    QTextStream out(&newFile);
                    
                    // Copy all messages
                    while (!in.atEnd()) {
                        QString line = in.readLine();
                        if (!line.trimmed().isEmpty()) {
                            out << line << "\n";
                        }
                    }
                    
                    oldFile.close();
                    newFile.close();
                    
                    // Delete the duplicate file
                    QFile::remove(oldPath);
                    qDebug() << "Merged and removed old room file:" << roomId;
                }
            } else {
                // Simply rename the file
                QFile::rename(oldPath, newPath);
                qDebug() << "Renamed room file:" << roomId << "->" << correctRoomId;
            }
            
            // Now update any user files that reference this room
            updateRoomReferencesInUserFiles(roomId, correctRoomId);
        }
    }
    
    // STEP 2: Verify that all expected room combinations exist, create if missing
    qDebug() << "Step 2: Verifying expected rooms exist...";
    
    // Get all user emails
    QStringList allEmails;
    QMapIterator<QString, UserData> emailIt(userMap);
    while (emailIt.hasNext()) {
        emailIt.next();
        allEmails.append(emailIt.key());
    }
    
    // Get existing room files
    QStringList existingRoomFiles;
    for (const QString &file : roomFiles) {
        if (file.endsWith(".txt")) {
            QString roomId = file;
            roomId.chop(4);
            existingRoomFiles.append(roomId);
        }
    }
    
    // For each user with contacts, verify room files exist
    for (const QString &userId : userContacts.keys()) {
        const QVector<QString> &contacts = userContacts[userId];
        
        for (const QString &contactId : contacts) {
            QString expectedRoomId = Room::generateRoomId(userId, contactId);
            QString roomPath = "../db/rooms/" + expectedRoomId + ".txt";
            
            // Check if this room file exists
            if (!QFile::exists(roomPath)) {
                qDebug() << "Missing room file for contacts:" << userId << "and" << contactId;
                qDebug() << "Creating room file:" << expectedRoomId;
                
                // Create the empty room file
                QFile file(roomPath);
                if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    file.close();
                    qDebug() << "Created empty room file:" << roomPath;
                }
            }
        }
    }
    
    // STEP 3: Update all user contact files to use correct room references
    qDebug() << "Step 3: Updating user contact files...";
    QDir usersDir("../db/users");
    QStringList userFiles = usersDir.entryList(QDir::Files);
    
    for (const QString &userFile : userFiles) {
        if (!userFile.endsWith(".txt"))
            continue;
        
        QString userId = userFile;
        userId.chop(4); // Remove .txt extension
        
        fixUserContactsFile(userId);
    }
    
    qDebug() << "Room repair operation completed\n";
}

void server::fixUserContactsFile(const QString &userId) {
    qDebug() << "Fixing user contacts file for:" << userId;
    
    QString filePath = "../db/users/" + userId + ".txt";
    QFile file(filePath);
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Could not open file for reading:" << filePath;
        return;
    }
    
    QStringList lines;
    QStringList contacts;
    QMap<QString, QString> roomMap; // roomId -> roomName
    bool needsUpdate = false;
    
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        
        if (line.startsWith("CONTACT:")) {
            QString contactId = line.mid(8).trimmed();
            contacts.append(contactId);
            lines.append(line);
        } else if (line.startsWith("ROOM:")) {
            QStringList parts = line.mid(5).split('|');
            if (parts.size() >= 2) {
                QString roomId = parts[0].trimmed();
                QString roomName = parts[1].trimmed();
                
                // Check if this room needs to be fixed
                QStringList roomUsers = roomName.split('_');
                if (roomUsers.size() == 2) {
                    QString user1 = roomUsers[0];
                    QString user2 = roomUsers[1];
                    
                    // Make sure both parts are valid emails
                    if (!user1.contains("@") || !user2.contains("@")) {
                        qDebug() << "Room needs repair:" << roomName;
                        
                        // Fix user1 and user2 if they're not emails
                        // This assumes contacts are using email addresses
                        if (contacts.contains(user1) && !user1.contains("@")) {
                            user1 = contacts.filter(QRegExp(".*" + user1 + ".*@.*")).value(0, user1);
                        }
                        
                        if (contacts.contains(user2) && !user2.contains("@")) {
                            user2 = contacts.filter(QRegExp(".*" + user2 + ".*@.*")).value(0, user2);
                        }
                        
                        QString correctRoomId = Room::generateRoomId(user1, user2);
                        QString correctRoomName = correctRoomId;
                        
                        // Updated line
                        line = "ROOM:" + correctRoomId + "|" + correctRoomName;
                        needsUpdate = true;
                        qDebug() << "Fixed room reference:" << roomId << "->" << correctRoomId;
                    }
                }
            }
            
            lines.append(line);
        } else {
            lines.append(line);
        }
    }
    
    file.close();
    
    // Write updated content if needed
    if (needsUpdate) {
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            for (const QString &line : lines) {
                out << line << "\n";
            }
            file.close();
            qDebug() << "Updated user contacts file:" << userId;
        } else {
            qDebug() << "Could not open file for writing:" << filePath;
        }
    }
}

void server::updateRoomReferencesInUserFiles(const QString &oldRoomId, const QString &newRoomId) {
    QDir usersDir("../db/users");
    QStringList userFiles = usersDir.entryList(QDir::Files);
    
    for (const QString &userFile : userFiles) {
        if (!userFile.endsWith(".txt"))
            continue;
        
        QString userId = userFile;
        userId.chop(4); // Remove .txt extension
        
        QString filePath = "../db/users/" + userFile;
        QFile file(filePath);
        
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }
        
        QStringList lines;
        QTextStream in(&file);
        bool needsUpdate = false;
        
        while (!in.atEnd()) {
            QString line = in.readLine();
            
            if (line.startsWith("ROOM:")) {
                QStringList parts = line.mid(5).split('|');
                if (parts.size() >= 2) {
                    QString roomId = parts[0];
                    QString roomName = parts[1];
                    
                    if (roomId == oldRoomId) {
                        line = "ROOM:" + newRoomId + "|" + newRoomId;
                        needsUpdate = true;
                        qDebug() << "Updated room reference in user file" << userId << ":" << oldRoomId << "->" << newRoomId;
                    }
                }
            }
            
            lines.append(line);
        }
        
        file.close();
        
        if (needsUpdate) {
            // Write the updated file
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                for (const QString &line : lines) {
                    out << line << "\n";
                }
                file.close();
                qDebug() << "Updated room references in user file:" << userId;
            }
        }
    }
}
