#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sqlite3.h>
#include "../includes/database.h"

#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define NC "\033[0m"

static int tests_passed = 0;
static int tests_total = 0;

void assert_test(int condition, const char* test_name) {
    tests_total++;
    if (condition) {
        tests_passed++;
        printf("  %s✓ %s%s\n", GREEN, test_name, NC);
    } else {
        printf("  %s✗ %s%s\n", RED, test_name, NC);
    }
}

// ТЕСТ 1: Инициализация БД в памяти
void test_db_init_memory() {
    printf("\n--- Тест 1: Инициализация БД в памяти ---\n");
    
    sqlite3* db = db_init(":memory:");
    assert_test(db != NULL, "БД в памяти создана");
    
    db_close(db);
}

// ТЕСТ 2: Инициализация БД в файле
void test_db_init_file() {
    printf("\n--- Тест 2: Инициализация БД в файле ---\n");
    
    // Удаляем если есть
    remove("test.db");
    
    sqlite3* db = db_init("test.db");
    assert_test(db != NULL, "БД в файле создана");
    
    db_close(db);
    
    // Проверяем, что файл создан
    FILE* f = fopen("test.db", "r");
    assert_test(f != NULL, "Файл test.db существует");
    if (f) fclose(f);
    
    remove("test.db");
}

// ТЕСТ 3: Выполнение SQL запроса
void test_db_execute() {
    printf("\n--- Тест 3: Выполнение SQL запроса ---\n");
    
    sqlite3* db = db_init(":memory:");
    
    const char* sql = "CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT);";
    int rc = db_execute(db, sql);
    assert_test(rc == SQLITE_OK, "Таблица создана");
    
    // Проверяем, что таблица существует
    const char* check_sql = "SELECT name FROM sqlite_master WHERE type='table' AND name='test';";
    sqlite3_stmt* stmt;
    int found = 0;
    
    if (sqlite3_prepare_v2(db, check_sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            found = 1;
        }
        sqlite3_finalize(stmt);
    }
    
    assert_test(found == 1, "Таблица действительно создана");
    
    db_close(db);
}

// ТЕСТ 4: Вставка данных
void test_db_insert() {
    printf("\n--- Тест 4: Вставка данных ---\n");
    
    sqlite3* db = db_init(":memory:");
    
    db_execute(db, "CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT);");
    
    int rc = db_execute(db, "INSERT INTO test (name) VALUES ('test1');");
    assert_test(rc == SQLITE_OK, "Данные вставлены");
    
    db_close(db);
}

// ТЕСТ 5: Получение последнего ID
void test_last_insert_rowid() {
    printf("\n--- Тест 5: Получение последнего ID ---\n");
    
    sqlite3* db = db_init(":memory:");
    
    db_execute(db, "CREATE TABLE test (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT);");
    db_execute(db, "INSERT INTO test (name) VALUES ('test1');");
    db_execute(db, "INSERT INTO test (name) VALUES ('test2');");
    db_execute(db, "INSERT INTO test (name) VALUES ('test3');");
    
    int last_id = db_last_insert_rowid(db);
    assert_test(last_id == 3, "Последний ID = 3");
    
    db_close(db);
}

// ТЕСТ 6: Экранирование простой строки
void test_escape_simple() {
    printf("\n--- Тест 6: Экранирование простой строки ---\n");
    
    char* escaped = db_escape_string("Hello World");
    assert_test(strcmp(escaped, "Hello World") == 0, "Простая строка без изменений");
    free(escaped);
}

// ТЕСТ 7: Экранирование строки с кавычкой
void test_escape_with_quote() {
    printf("\n--- Тест 7: Экранирование строки с кавычкой ---\n");
    
    char* escaped = db_escape_string("O'Reilly");
    assert_test(strcmp(escaped, "O''Reilly") == 0, "Кавычка экранирована");
    free(escaped);
}

// ТЕСТ 8: Экранирование строки с несколькими кавычками
void test_escape_multiple_quotes() {
    printf("\n--- Тест 8: Экранирование нескольких кавычек ---\n");
    
    char* escaped = db_escape_string("It's John's book");
    assert_test(strcmp(escaped, "It''s John''s book") == 0, "Все кавычки экранированы");
    free(escaped);
}

// ТЕСТ 9: Экранирование NULL строки
void test_escape_null() {
    printf("\n--- Тест 9: Экранирование NULL ---\n");
    
    char* escaped = db_escape_string(NULL);
    assert_test(escaped == NULL, "NULL возвращает NULL");
}

// ТЕСТ 10: Создание таблиц статистики
void test_create_stats_tables() {
    printf("\n--- Тест 10: Создание таблиц статистики ---\n");
    
    sqlite3* db = db_init(":memory:");
    
    int rc = db_create_stats_tables(db);
    assert_test(rc == SQLITE_OK, "Таблицы статистики созданы");
    
    // Проверяем, что таблицы существуют
    const char* check_sql = "SELECT name FROM sqlite_master WHERE type='table' AND name='mechanic_stats';";
    sqlite3_stmt* stmt;
    int found = 0;
    
    if (sqlite3_prepare_v2(db, check_sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            found = 1;
        }
        sqlite3_finalize(stmt);
    }
    
    assert_test(found == 1, "Таблица mechanic_stats создана");
    
    db_close(db);
}

// ТЕСТ 11: Создание триггеров
void test_create_triggers() {
    printf("\n--- Тест 11: Создание триггеров ---\n");
    
    sqlite3* db = db_init(":memory:");
    
    // Создаем необходимые таблицы
    db_execute(db, "CREATE TABLE mechanics (id INTEGER PRIMARY KEY, name TEXT);");
    db_execute(db, "CREATE TABLE repairs (id INTEGER, mechanic_id INTEGER, cost REAL, start_date TEXT);");
    db_create_stats_tables(db);
    
    int rc = db_create_triggers(db);
    assert_test(rc == SQLITE_OK, "Триггеры созданы");
    
    // Проверяем, что триггеры существуют
    const char* check_sql = "SELECT name FROM sqlite_master WHERE type='trigger' AND name='update_mechanic_stats_insert';";
    sqlite3_stmt* stmt;
    int found = 0;
    
    if (sqlite3_prepare_v2(db, check_sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            found = 1;
        }
        sqlite3_finalize(stmt);
    }
    
    assert_test(found == 1, "Триггер update_mechanic_stats_insert создан");
    
    db_close(db);
}

// ТЕСТ 12: Функция получения дневной статистики
void test_daily_stats() {
    printf("\n--- Тест 12: Получение дневной статистики ---\n");
    
    sqlite3* db = db_init(":memory:");
    
    // Создаем таблицы и данные
    const char* sql = 
        "CREATE TABLE repairs ("
        "id INTEGER PRIMARY KEY,"
        "workshop_id INTEGER,"
        "start_date TEXT,"
        "cost REAL"
        ");"
        
        "INSERT INTO repairs (workshop_id, start_date, cost) VALUES "
        "(1, '2026-03-01', 100.00),"
        "(1, '2026-03-01', 150.00),"
        "(1, '2026-03-02', 200.00),"
        "(2, '2026-03-01', 300.00);";
    
    db_execute(db, sql);
    
    int total_repairs;
    int rc = db_get_daily_stats(db, 1, "2026-03-01", &total_repairs);
    
    assert_test(rc == 0, "Функция выполнена успешно");
    assert_test(total_repairs == 2, "За 2026-03-01 в мастерской 1 найдено 2 ремонта");
    
    rc = db_get_daily_stats(db, 1, "2026-03-02", &total_repairs);
    assert_test(total_repairs == 1, "За 2026-03-02 в мастерской 1 найден 1 ремонт");
    
    rc = db_get_daily_stats(db, 2, "2026-03-01", &total_repairs);
    assert_test(total_repairs == 1, "За 2026-03-01 в мастерской 2 найден 1 ремонт");
    
    db_close(db);
}

int main() {
    printf("\n========================================\n");
    printf("ТЕСТИРОВАНИЕ МОДУЛЯ БАЗЫ ДАННЫХ\n");
    printf("========================================\n");
    
    test_db_init_memory();
    test_db_init_file();
    test_db_execute();
    test_db_insert();
    test_last_insert_rowid();
    test_escape_simple();
    test_escape_with_quote();
    test_escape_multiple_quotes();
    test_escape_null();
    test_create_stats_tables();
    test_create_triggers();
    test_daily_stats();
    
    printf("\n========================================\n");
    printf("ИТОГИ ТЕСТИРОВАНИЯ\n");
    printf("========================================\n");
    printf("Пройдено: %d/%d тестов\n", tests_passed, tests_total);
    printf("========================================\n");
    
    return (tests_passed == tests_total) ? 0 : 1;
}