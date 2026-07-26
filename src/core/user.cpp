#include "user.h"

/*
 * 文件职责：
 * 实现用户注册、登录和密码哈希逻辑。账号数据通过 Storage 保存到
 * data/users.txt，文件中保存用户名、盐值和密码哈希，不保存明文密码。
 *
 * 主要功能定位：
 * - 功能：用户注册
 * - 功能：用户登录
 * - 功能：密码哈希
 */

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

// ==================== 功能：用户登录 ====================
bool UserManager::login(const std::string& username, const std::string& password, std::string& message) {
    if (!loadSucceeded(message)) {
        return false;
    }
    // 步骤 1：先按用户名查找账号记录；不存在时不继续验证密码。
    const auto it = users_.find(username);
    if (it == users_.end()) {
        message = "Error: user not found.";
        return false;
    }

    // 步骤 2：使用该用户保存的盐值重新计算哈希，与文件中的哈希比对。
    const std::string candidateHash = hashPassword(password, it->second.salt);
    if (candidateHash != it->second.passwordHash) {
        message = "Error: username or password is incorrect.";
        return false;
    }

    message = "Login successful.";
    return true;
}

// ==================== 功能：用户注册 ====================
bool UserManager::registerUser(const std::string& username, const std::string& password, std::string& message) {
    if (!loadSucceeded(message)) {
        return false;
    }
    // 步骤 1：注册失败的主要条件：用户名格式非法、密码为空、用户名已存在。
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

    // 步骤 2：为新用户生成盐值，并把“盐值 + 密码”的哈希保存到账号记录。
    const std::string salt = generateSalt();
    users_[username] = UserRecord{salt, hashPassword(password, salt)};

    std::string errorMessage;
    // 步骤 3：写入 users.txt；如果保存失败则撤销内存中的新增用户。
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

// ==================== 功能：密码哈希 ====================
std::string UserManager::hashPassword(const std::string& password, const std::string& salt) {
    constexpr unsigned long long kOffsetBasis = 1469598103934665603ULL;
    constexpr unsigned long long kPrime = 1099511628211ULL;
    unsigned long long hash = kOffsetBasis;
    // 盐值与密码用分隔符组合后参与 FNV-1a 风格哈希，结果用于注册保存和登录比对。
    const std::string merged = salt + ":" + password;
    for (unsigned char ch : merged) {
        hash ^= static_cast<unsigned long long>(ch);
        hash *= kPrime;
    }
    return toHex(hash);
}
