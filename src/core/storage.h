#ifndef MYSCHEDULE_STORAGE_H
#define MYSCHEDULE_STORAGE_H

#include <map>
#include <string>

struct UserRecord {
    std::string salt;
    std::string passwordHash;
};

class Storage {
public:
    static bool ensureDataDir(std::string& errorMessage);
    static std::map<std::string, UserRecord> loadUsers(std::string& errorMessage);
    static bool saveUsers(const std::map<std::string, UserRecord>& users, std::string& errorMessage);
    static std::string taskFilePathForUser(const std::string& username);

private:
    static std::string rootDir();
    static std::string dataDir();
    static std::string usersPath();
    static std::string tasksPath(const std::string& username);
};

#endif
