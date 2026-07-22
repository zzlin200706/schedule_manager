#include "user.h"

#include <iomanip>
#include <random>
#include <sstream>

namespace {
std::string toHex(unsigned long long value) {
    std::ostringstream output;
    output << std::hex << std::setw(16) << std::setfill('0') << value;
    return output.str();
}
}

UserManager::UserManager() {
    users_ = Storage::loadUsers(loadError_);
}

bool UserManager::loadSucceeded(std::string& message) const {
    if (loadError_.empty()) {
        return true;
    }
    message = loadError_;
    return false;
}

bool UserManager::userExists(const std::string& username) const {
    return users_.find(username) != users_.end();
}

bool UserManager::login(const std::string& username, const std::string& password, std::string& message) {
    if (!loadSucceeded(message)) {
        return false;
    }
    const auto it = users_.find(username);
    if (it == users_.end()) {
        message = "Error: user not found.";
        return false;
    }

    const std::string candidateHash = hashPassword(password, it->second.salt);
    if (candidateHash != it->second.passwordHash) {
        message = "Error: username or password is incorrect.";
        return false;
    }

    message = "Login successful.";
    return true;
}

bool UserManager::registerUser(const std::string& username, const std::string& password, std::string& message) {
    if (!loadSucceeded(message)) {
        return false;
    }
    if (!isValidUsername(username)) {
        message = "Error: username may only contain letters, digits, underscore, or hyphen.";
        return false;
    }
    if (password.empty()) {
        message = "Error: password cannot be empty.";
        return false;
    }
    if (userExists(username)) {
        message = "Error: username already exists.";
        return false;
    }

    const std::string salt = generateSalt();
    users_[username] = UserRecord{salt, hashPassword(password, salt)};

    std::string errorMessage;
    if (!Storage::saveUsers(users_, errorMessage)) {
        users_.erase(username);
        message = errorMessage;
        return false;
    }

    message = "User created successfully.";
    return true;
}

bool UserManager::isValidUsername(const std::string& username) {
    if (username.empty()) {
        return false;
    }

    for (unsigned char ch : username) {
        const bool allowed =
            (ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') ||
            ch == '_' || ch == '-';
        if (!allowed) {
            return false;
        }
    }
    return true;
}

std::string UserManager::generateSalt() {
    std::random_device device;
    std::mt19937_64 generator(device());
    return toHex(generator());
}

std::string UserManager::hashPassword(const std::string& password, const std::string& salt) {
    constexpr unsigned long long kOffsetBasis = 1469598103934665603ULL;
    constexpr unsigned long long kPrime = 1099511628211ULL;
    unsigned long long hash = kOffsetBasis;
    const std::string merged = salt + ":" + password;
    for (unsigned char ch : merged) {
        hash ^= static_cast<unsigned long long>(ch);
        hash *= kPrime;
    }
    return toHex(hash);
}
