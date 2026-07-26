#ifndef MYSCHEDULE_USER_H
#define MYSCHEDULE_USER_H

/*
 * 文件职责：
 * 声明用户账号管理接口，负责被 CLI 和 Qt GUI 调用来完成注册、登录、
 * 用户存在性检查以及密码哈希相关处理。
 *
 * 主要功能定位：
 * - 用户注册：搜索“功能：用户注册”
 * - 用户登录：搜索“功能：用户登录”
 * - 密码处理：搜索“功能：密码哈希”
 */

#include "storage.h"

#include <map>
#include <string>

/**
 * @brief 用户账号管理类
 *
 * 内存中保存从 users.txt 读取的账号记录，注册和登录时统一使用
 * Storage 读写用户文件，不向调用层暴露盐值或密码哈希细节。
 */
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

    // 用户名限制在字母、数字、下划线和连字符，避免参与文件路径时产生歧义。
    static bool isValidUsername(const std::string& username);
    static std::string generateSalt();
    static std::string hashPassword(const std::string& password, const std::string& salt);
};

#endif
