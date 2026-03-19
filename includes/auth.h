#ifndef AUTH_H
#define AUTH_H

#include <stdio.h>
#include <string.h>
#include <sqlite3.h>

// Роли пользователей
typedef enum {
    ROLE_ADMIN,
    ROLE_MECHANIC,
    ROLE_UNKNOWN
} UserRole;

// Структура пользователя
typedef struct {
    int id;
    char username[64];
    char password_hash[128];
    UserRole role;
    int mechanic_id;  // связь с таблицей mechanics (NULL для админа)
} User;

// Инициализация системы аутентификации
int auth_init(sqlite3* db);

// Вход в систему
User* auth_login(sqlite3* db, const char* username, const char* password);

// Проверка прав доступа
int auth_check_permission(User* user, const char* operation);

// Выход из системы
void auth_logout(User* user);

// Хеширование пароля
void auth_hash_password(const char* password, char* hash);

// Создание нового пользователя (только для админа)
int auth_create_user(sqlite3* db, const char* username, const char* password, 
                    const char* role, int mechanic_id);

#endif