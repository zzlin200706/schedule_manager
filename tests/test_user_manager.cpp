#include "storage.h"
#include "user.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unistd.h>

namespace fs = std::filesystem;

namespace {
bool expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "Test failure: " << message << '\n';
        return false;
    }
    return true;
}
}

int main() {
    char directoryTemplate[] = "/tmp/myschedule-user-test-XXXXXX";
    char* directory = mkdtemp(directoryTemplate);
    if (directory == nullptr) {
        std::cerr << "Test failure: unable to create temporary data directory.\n";
        return 1;
    }
    const fs::path dataDirectory(directory);
    setenv("MYSCHEDULE_DATA_DIR", dataDirectory.c_str(), 1);

    std::string message;
    UserManager users;
    bool passed = expect(users.loadSucceeded(message), "initial user storage should load") &&
                  expect(users.registerUser("alice", "same-password", message), "alice should register") &&
                  expect(users.userExists("alice"), "alice should exist after registration") &&
                  expect(!users.registerUser("alice", "same-password", message), "duplicate usernames should fail") &&
                  expect(!users.registerUser("bad name", "same-password", message), "invalid usernames should fail") &&
                  expect(!users.registerUser("empty", "", message), "empty passwords should fail");

    UserManager reloadedUsers;
    passed = expect(reloadedUsers.login("alice", "same-password", message), "correct password should log in") &&
             expect(!reloadedUsers.login("alice", "wrong-password", message), "wrong password should fail") &&
             expect(reloadedUsers.registerUser("bob", "same-password", message), "bob should register") &&
             passed;

    std::string storageError;
    const auto records = Storage::loadUsers(storageError);
    std::ifstream userFile(dataDirectory / "users.txt");
    const std::string storedContents((std::istreambuf_iterator<char>(userFile)), std::istreambuf_iterator<char>());

    passed = expect(storageError.empty(), "saved users should reload") &&
             expect(records.size() == 2U, "two users should be stored") &&
             expect(records.at("alice").salt != records.at("bob").salt, "users should have distinct salts") &&
             expect(records.at("alice").passwordHash != records.at("bob").passwordHash,
                    "the same password should produce distinct stored hashes") &&
             expect(storedContents.find("same-password") == std::string::npos,
                    "users.txt must not contain the plaintext password") &&
             expect(Storage::taskFilePathForUser("alice") != Storage::taskFilePathForUser("bob"),
                    "task file paths should be isolated by user") &&
             expect(fs::path(Storage::taskFilePathForUser("alice")).filename() == "tasks_alice.txt",
                    "alice task file path should use her username") &&
             passed;

    fs::remove_all(dataDirectory);
    unsetenv("MYSCHEDULE_DATA_DIR");
    return passed ? 0 : 1;
}
