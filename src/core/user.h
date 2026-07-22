#ifndef MYSCHEDULE_USER_H
#define MYSCHEDULE_USER_H

#include "storage.h"

#include <map>
#include <string>

class UserManager {
public:
    UserManager();

    bool loadSucceeded(std::string& message) const;
    bool userExists(const std::string& username) const;
    bool login(const std::string& username, const std::string& password, std::string& message);
    bool registerUser(const std::string& username, const std::string& password, std::string& message);

private:
    std::map<std::string, UserRecord> users_;
    std::string loadError_;

    static bool isValidUsername(const std::string& username);
    static std::string generateSalt();
    static std::string hashPassword(const std::string& password, const std::string& salt);
};

#endif
