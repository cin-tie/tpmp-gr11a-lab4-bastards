#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sqlite3.h>
#include "../includes/auth.h"
#include "../includes/database.h"

#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define NC "\033[0m"

// Счетчики тестов
static int tests_passed = 0;
static int tests_total = 0;

// Вспомогательная функция для создания тестовой БД
sqlite3* create_test_db() {
    sqlite3* db;
    int rc = sqlite3_open(":memory:", &db);
    assert(rc == SQLITE_OK);
    
    // Создаем таблицу mechanics
    const char* sql = 
        "CREATE TABLE mechanics ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL,"
        "specialization TEXT,"
        "workshop_id INTEGER"
        ");"
        
        "INSERT INTO mechanics (name, workshop_id) VALUES "
        "('Test Mechanic 1', 1),"
        "('Test Mechanic 2', 1),"
        "('Test Mechanic 3', 2);";
    
    char* err_msg = NULL;
    rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    assert(rc == SQLITE_OK);
    
    return db;
}

// Функция для вывода результата теста
void assert_test(int condition, const char* test_name) {
    tests_total++;
    if (condition) {
        tests_passed++;
        printf("  %s✓ %s%s\n", GREEN, test_name, NC);
    } else {
        printf("  %s✗ %s%s\n", RED, test_name, NC);
    }
}

// ТЕСТ 1: Инициализация системы аутентификации
void test_auth_init() {
    printf("\n--- Тест 1: Инициализация auth_init ---\n");
    
    sqlite3* db = create_test_db();
    
    int rc = auth_init(db);
    assert_test(rc == SQLITE_OK, "auth_init вернул SQLITE_OK");
    
    // Проверяем, что таблица users создана
    const char* check_sql = "SELECT name FROM sqlite_master WHERE type='table' AND name='users';";
    sqlite3_stmt* stmt;
    int found = 0;
    
    if (sqlite3_prepare_v2(db, check_sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            found = 1;
        }
        sqlite3_finalize(stmt);
    }
    
    assert_test(found == 1, "Таблица users создана");
    
    sqlite3_close(db);
}

// ТЕСТ 2: Хеширование пароля
void test_password_hashing() {
    printf("\n--- Тест 2: Хеширование пароля ---\n");
    
    char hash[65];
    auth_hash_password("password123", hash);
    
    // SHA256 хеш всегда 64 символа
    assert_test(strlen(hash) == 64, "Длина хеша = 64 символа");
    
    // Одинаковые пароли дают одинаковый хеш
    char hash2[65];
    auth_hash_password("password123", hash2);
    assert_test(strcmp(hash, hash2) == 0, "Одинаковые пароли = одинаковые хеши");
    
    // Разные пароли дают разные хеши
    auth_hash_password("password456", hash2);
    assert_test(strcmp(hash, hash2) != 0, "Разные пароли = разные хеши");
    
    // Пустой пароль
    auth_hash_password("", hash);
    assert_test(strlen(hash) == 64, "Пустой пароль тоже хешируется");
}

// ТЕСТ 3: Создание пользователя (админ)
void test_create_admin_user() {
    printf("\n--- Тест 3: Создание администратора ---\n");
    
    sqlite3* db = create_test_db();
    auth_init(db);
    
    int rc = auth_create_user(db, "admin", "admin123", "admin", 0);
    assert_test(rc == 1, "Администратор создан");
    
    // Проверяем, что пользователь есть в БД
    const char* check_sql = "SELECT COUNT(*) FROM users WHERE username = 'admin';";
    sqlite3_stmt* stmt;
    int count = 0;
    
    if (sqlite3_prepare_v2(db, check_sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    
    assert_test(count == 1, "Администратор сохранен в БД");
    
    sqlite3_close(db);
}

// ТЕСТ 4: Создание пользователя (мастер)
void test_create_mechanic_user() {
    printf("\n--- Тест 4: Создание мастера ---\n");
    
    sqlite3* db = create_test_db();
    auth_init(db);
    
    int rc = auth_create_user(db, "mechanic1", "mech123", "mechanic", 1);
    assert_test(rc == 1, "Мастер создан");
    
    // Проверяем связь с mechanics
    const char* check_sql = 
        "SELECT u.username, m.name FROM users u "
        "JOIN mechanics m ON u.mechanic_id = m.id "
        "WHERE u.username = 'mechanic1';";
    
    sqlite3_stmt* stmt;
    int found = 0;
    
    if (sqlite3_prepare_v2(db, check_sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            found = 1;
        }
        sqlite3_finalize(stmt);
    }
    
    assert_test(found == 1, "Мастер связан с таблицей mechanics");
    
    sqlite3_close(db);
}

// ТЕСТ 5: Запрет дублирования логинов
void test_duplicate_username() {
    printf("\n--- Тест 5: Запрет дублирования логинов ---\n");
    
    sqlite3* db = create_test_db();
    auth_init(db);
    
    auth_create_user(db, "user1", "pass1", "admin", 0);
    int rc = auth_create_user(db, "user1", "pass2", "mechanic", 1);
    
    assert_test(rc == 0, "Дубликат логина отклонен");
    
    // Проверяем, что в БД только один пользователь
    const char* check_sql = "SELECT COUNT(*) FROM users WHERE username = 'user1';";
    sqlite3_stmt* stmt;
    int count = 0;
    
    if (sqlite3_prepare_v2(db, check_sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    
    assert_test(count == 1, "В БД только одна запись");
    
    sqlite3_close(db);
}

// ТЕСТ 6: Вход с правильными данными
void test_login_success() {
    printf("\n--- Тест 6: Успешный вход ---\n");
    
    sqlite3* db = create_test_db();
    auth_init(db);
    
    auth_create_user(db, "testuser", "testpass", "admin", 0);
    
    User* user = auth_login(db, "testuser", "testpass");
    
    assert_test(user != NULL, "Пользователь найден");
    if (user) {
        assert_test(strcmp(user->username, "testuser") == 0, "Логин совпадает");
        assert_test(user->role == ROLE_ADMIN, "Роль определена правильно");
        auth_logout(user);
    }
    
    sqlite3_close(db);
}

// ТЕСТ 7: Вход с неправильным паролем
void test_login_wrong_password() {
    printf("\n--- Тест 7: Неправильный пароль ---\n");
    
    sqlite3* db = create_test_db();
    auth_init(db);
    
    auth_create_user(db, "testuser", "correctpass", "admin", 0);
    
    User* user = auth_login(db, "testuser", "wrongpass");
    
    assert_test(user == NULL, "Вход отклонен при неправильном пароле");
    
    sqlite3_close(db);
}

// ТЕСТ 8: Вход с несуществующим логином
void test_login_nonexistent() {
    printf("\n--- Тест 8: Несуществующий логин ---\n");
    
    sqlite3* db = create_test_db();
    auth_init(db);
    
    auth_create_user(db, "testuser", "pass", "admin", 0);
    
    User* user = auth_login(db, "nonexistent", "pass");
    
    assert_test(user == NULL, "Вход отклонен при несуществующем логине");
    
    sqlite3_close(db);
}

// ТЕСТ 9: Проверка прав доступа администратора
void test_admin_permissions() {
    printf("\n--- Тест 9: Права администратора ---\n");
    
    sqlite3* db = create_test_db();
    auth_init(db);
    
    auth_create_user(db, "admin", "pass", "admin", 0);
    User* admin = auth_login(db, "admin", "pass");
    
    assert_test(auth_check_permission(admin, "any_operation") == 1, 
                "Админ имеет доступ к любой операции");
    assert_test(auth_check_permission(admin, "delete_workshop") == 1, 
                "Админ может удалять мастерские");
    assert_test(auth_check_permission(admin, "view_all_repairs") == 1, 
                "Админ может смотреть все ремонты");
    
    auth_logout(admin);
    sqlite3_close(db);
}

// ТЕСТ 10: Проверка прав доступа мастера
void test_mechanic_permissions() {
    printf("\n--- Тест 10: Права мастера ---\n");
    
    sqlite3* db = create_test_db();
    auth_init(db);
    
    auth_create_user(db, "mechanic", "pass", "mechanic", 1);
    User* mechanic = auth_login(db, "mechanic", "pass");
    
    // Разрешенные операции
    assert_test(auth_check_permission(mechanic, "view_my_repairs") == 1, 
                "Мастер может смотреть свои ремонты");
    assert_test(auth_check_permission(mechanic, "add_repair") == 1, 
                "Мастер может добавлять ремонты");
    assert_test(auth_check_permission(mechanic, "update_repair_status") == 1, 
                "Мастер может обновлять статус");
    assert_test(auth_check_permission(mechanic, "view_car_info") == 1, 
                "Мастер может смотреть информацию об авто");
    
    // Запрещенные операции
    assert_test(auth_check_permission(mechanic, "delete_workshop") == 0, 
                "Мастер НЕ может удалять мастерские");
    assert_test(auth_check_permission(mechanic, "view_all_repairs") == 0, 
                "Мастер НЕ может смотреть все ремонты");
    
    auth_logout(mechanic);
    sqlite3_close(db);
}

int main() {
    printf("\n========================================\n");
    printf("ТЕСТИРОВАНИЕ МОДУЛЯ АУТЕНТИФИКАЦИИ\n");
    printf("========================================\n");
    
    test_auth_init();
    test_password_hashing();
    test_create_admin_user();
    test_create_mechanic_user();
    test_duplicate_username();
    test_login_success();
    test_login_wrong_password();
    test_login_nonexistent();
    test_admin_permissions();
    test_mechanic_permissions();
    
    printf("\n========================================\n");
    printf("ИТОГИ ТЕСТИРОВАНИЯ\n");
    printf("========================================\n");
    printf("Пройдено: %d/%d тестов\n", tests_passed, tests_total);
    printf("========================================\n");
    
    return (tests_passed == tests_total) ? 0 : 1;
}