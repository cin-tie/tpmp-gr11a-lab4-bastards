#include "auth.h"
#include "database.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/sha.h>

// Вспомогательная функция для преобразования роли из строки
static UserRole role_from_string(const char* role_str) {
    if (strcmp(role_str, "admin") == 0) return ROLE_ADMIN;
    if (strcmp(role_str, "mechanic") == 0) return ROLE_MECHANIC;
    return ROLE_UNKNOWN;
}

// Вспомогательная функция для преобразования роли в строку
static const char* role_to_string(UserRole role) {
    switch(role) {
        case ROLE_ADMIN: return "admin";
        case ROLE_MECHANIC: return "mechanic";
        default: return "unknown";
    }
}

void auth_hash_password(const char* password, char* hash) {
    unsigned char sha256_hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)password, strlen(password), sha256_hash);
    
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(hash + (i * 2), "%02x", sha256_hash[i]);
    }
    hash[64] = '\0';
}

int auth_init(sqlite3* db){
        const char* sql = 
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "username TEXT UNIQUE NOT NULL,"
        "password_hash TEXT NOT NULL,"
        "role TEXT NOT NULL,"
        "mechanic_id INTEGER,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (mechanic_id) REFERENCES mechanics(id)"
        ");";
    
    return db_execute(db, sql);
}

User* auth_login(sqlite3* db, const char* username, const char* password){
    if(!db || !username || !password)
        return NULL;

    char hash[65];
    auth_hash_password(password, hash);

    // Экранируем username для защиты от SQL injection
    char* safe_username = db_escape_string(username);
    if (!safe_username) return NULL;

    char sql[512];
        snprintf(sql, sizeof(sql),
        "SELECT id, username, password_hash, role, mechanic_id "
        "FROM users WHERE username = '%s' AND password_hash = '%s';",
        safe_username, hash);
    
    free(safe_username);
    
    User* user = NULL;
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            user = (User*)malloc(sizeof(User));
            user->id = sqlite3_column_int(stmt, 0);
            strcpy(user->username, (const char*)sqlite3_column_text(stmt, 1));
            strcpy(user->password_hash, (const char*)sqlite3_column_text(stmt, 2));
            user->role = role_from_string((const char*)sqlite3_column_text(stmt, 3));
            user->mechanic_id = sqlite3_column_int(stmt, 4);
        }
        sqlite3_finalize(stmt);
    }
    
    return user;
}

int auth_check_permission(User* user, const char* operation) {
    if (!user) return 0;
    
    // Админ имеет доступ ко всему
    if (user->role == ROLE_ADMIN) return 1;
    
    // Мастер имеет ограниченный доступ
    if (user->role == ROLE_MECHANIC) {
        // Список операций, доступных мастеру
        if (strcmp(operation, "view_my_repairs") == 0) return 1;
        if (strcmp(operation, "add_repair") == 0) return 1;
        if (strcmp(operation, "update_repair_status") == 0) return 1;
        if (strcmp(operation, "view_car_info") == 0) return 1;
        // Остальные операции запрещены
    }
    
    return 0;
}

void auth_logout(User* user) {
    if (user) {
        free(user);
    }
}

int auth_create_user(sqlite3* db, const char* username, const char* password, 
                        const char* role, int mechanic_id) {
    if (!db || !username || !password || !role) return 0;
    
    char hash[65];
    auth_hash_password(password, hash);
    
    char* safe_username = db_escape_string(username);
    char* safe_role = db_escape_string(role);
    
    if (!safe_username || !safe_role) {
        free(safe_username);
        free(safe_role);
        return 0;
    }
    
    char sql[1024];
    if (mechanic_id > 0) {
        snprintf(sql, sizeof(sql),
            "INSERT INTO users (username, password_hash, role, mechanic_id) "
            "VALUES ('%s', '%s', '%s', %d);",
            safe_username, hash, safe_role, mechanic_id);
    } else {
        snprintf(sql, sizeof(sql),
            "INSERT INTO users (username, password_hash, role) "
            "VALUES ('%s', '%s', '%s');",
            safe_username, hash, safe_role);
    }
    
    free(safe_username);
    free(safe_role);
    
    int rc = db_execute(db, sql);
    return (rc == SQLITE_OK);
}