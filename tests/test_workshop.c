#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sqlite3.h>
#include "../includes/workshop.h"
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

// Создание тестовой БД с данными
sqlite3* setup_workshop_test_db() {
    sqlite3* db;
    int rc = sqlite3_open(":memory:", &db);
    assert(rc == SQLITE_OK);
    
    const char* sql = 
        "CREATE TABLE workshops ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "address TEXT NOT NULL,"
        "phone TEXT,"
        "car_brands TEXT"
        ");"
        
        "CREATE TABLE mechanics ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL,"
        "specialization TEXT,"
        "workshop_id INTEGER"
        ");"
        
        "CREATE TABLE cars ("
        "license_plate TEXT PRIMARY KEY,"
        "brand TEXT NOT NULL,"
        "model TEXT,"
        "year INTEGER,"
        "owner_name TEXT NOT NULL,"
        "passport_number TEXT UNIQUE,"
        "owner_address TEXT"
        ");"
        
        "CREATE TABLE repair_types ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL,"
        "category TEXT,"
        "base_price REAL"
        ");"
        
        "CREATE TABLE repairs ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "workshop_id INTEGER NOT NULL,"
        "mechanic_id INTEGER NOT NULL,"
        "car_license TEXT NOT NULL,"
        "repair_type_id INTEGER NOT NULL,"
        "start_date TEXT NOT NULL,"
        "end_date TEXT,"
        "cost REAL NOT NULL,"
        "status TEXT DEFAULT 'in_progress',"
        "FOREIGN KEY (workshop_id) REFERENCES workshops(id),"
        "FOREIGN KEY (mechanic_id) REFERENCES mechanics(id),"
        "FOREIGN KEY (car_license) REFERENCES cars(license_plate),"
        "FOREIGN KEY (repair_type_id) REFERENCES repair_types(id)"
        ");";
    
    rc = db_execute(db, sql);
    assert(rc == SQLITE_OK);
    
    // Добавляем тестовые данные
    const char* data_sql = 
        "INSERT INTO workshops (address, phone, car_brands) VALUES "
        "('ул. Ленина, 1', '+375 17 111-11-11', 'Toyota, Honda'),"
        "('ул. Пушкина, 2', '+375 17 222-22-22', 'BMW, Audi'),"
        "('пр. Независимости, 3', '+375 17 333-33-33', 'Renault, Peugeot');"
        
        "INSERT INTO mechanics (name, specialization, workshop_id) VALUES "
        "('Иванов', 'Кузовные работы', 1),"
        "('Петров', 'Двигатель', 1),"
        "('Сидоров', 'Электрика', 2);"
        
        "INSERT INTO cars (license_plate, brand, model, year, owner_name, passport_number, owner_address) VALUES "
        "('1234AB', 'Toyota', 'Corolla', 2020, 'Иванов Иван', 'AB123456', 'ул. Ленина, 10'),"
        "('5678CD', 'BMW', 'X5', 2021, 'Петров Петр', 'CD789012', 'ул. Пушкина, 20'),"
        "('9012EF', 'Renault', 'Logan', 2019, 'Сидоров Сидор', 'EF345678', 'ул. Гагарина, 30');"
        
        "INSERT INTO repair_types (name, base_price) VALUES "
        "('Замена масла', 50.00),"
        "('Замена тормозов', 120.00),"
        "('Диагностика', 30.00);"
        
        "INSERT INTO repairs (workshop_id, mechanic_id, car_license, repair_type_id, "
        "start_date, end_date, cost, status) VALUES "
        "(1, 1, '1234AB', 1, '2026-03-01', '2026-03-01', 50.00, 'completed'),"
        "(1, 2, '1234AB', 2, '2026-03-02', '2026-03-02', 120.00, 'completed'),"
        "(1, 1, '1234AB', 3, '2026-03-03', NULL, 30.00, 'in_progress'),"
        "(2, 3, '5678CD', 1, '2026-03-01', '2026-03-01', 50.00, 'completed'),"
        "(2, 3, '5678CD', 2, '2026-03-02', '2026-03-03', 120.00, 'completed'),"
        "(3, 3, '9012EF', 1, '2026-03-04', NULL, 50.00, 'in_progress');";
    
    rc = db_execute(db, data_sql);
    assert(rc == SQLITE_OK);
    
    return db;
}

// ТЕСТ 1: Получение всех мастерских
void test_workshop_get_all() {
    printf("\n--- Тест 1: Получение всех мастерских ---\n");
    
    sqlite3* db = setup_workshop_test_db();
    
    int count;
    Workshop* workshops = workshop_get_all(db, &count);
    
    assert_test(workshops != NULL, "Мастерские получены");
    assert_test(count == 3, "Количество мастерских = 3");
    if (workshops) {
        assert_test(strcmp(workshops[0].address, "ул. Ленина, 1") == 0, "Адрес 1 корректный");
        assert_test(strcmp(workshops[1].address, "ул. Пушкина, 2") == 0, "Адрес 2 корректный");
        assert_test(strcmp(workshops[2].address, "пр. Независимости, 3") == 0, "Адрес 3 корректный");
        free(workshops);
    }
    
    sqlite3_close(db);
}

// ТЕСТ 2: Получение мастерской по ID
void test_workshop_get_by_id() {
    printf("\n--- Тест 2: Получение мастерской по ID ---\n");
    
    sqlite3* db = setup_workshop_test_db();
    
    Workshop* w = workshop_get_by_id(db, 1);
    assert_test(w != NULL, "Мастерская с ID 1 найдена");
    if (w) {
        assert_test(w->id == 1, "ID совпадает");
        assert_test(strcmp(w->address, "ул. Ленина, 1") == 0, "Адрес совпадает");
        free(w);
    }
    
    w = workshop_get_by_id(db, 999);
    assert_test(w == NULL, "Несуществующая мастерская не найдена");
    
    sqlite3_close(db);
}

// ТЕСТ 3: Добавление мастерской
void test_workshop_insert() {
    printf("\n--- Тест 3: Добавление мастерской ---\n");
    
    sqlite3* db = setup_workshop_test_db();
    
    Workshop w;
    memset(&w, 0, sizeof(Workshop));
    strcpy(w.address, "Новая мастерская");
    strcpy(w.phone, "+375 29 999-99-99");
    strcpy(w.car_brands, "Mazda, Subaru");
    
    int rc = workshop_insert(db, &w);
    assert_test(rc == SQLITE_OK, "Мастерская добавлена");
    assert_test(w.id > 0, "Присвоен ID > 0");
    
    Workshop* inserted = workshop_get_by_id(db, w.id);
    assert_test(inserted != NULL, "Мастерская найдена в БД");
    if (inserted) {
        assert_test(strcmp(inserted->address, w.address) == 0, "Адрес сохранен");
        assert_test(strcmp(inserted->phone, w.phone) == 0, "Телефон сохранен");
        assert_test(strcmp(inserted->car_brands, w.car_brands) == 0, "Марки сохранены");
        free(inserted);
    }
    
    sqlite3_close(db);
}

// ТЕСТ 4: Обновление мастерской
void test_workshop_update() {
    printf("\n--- Тест 4: Обновление мастерской ---\n");
    
    sqlite3* db = setup_workshop_test_db();
    
    Workshop* w = workshop_get_by_id(db, 1);
    assert_test(w != NULL, "Мастерская найдена");
    
    if (w) {
        strcpy(w->phone, "+375 17 888-88-88");
        strcpy(w->car_brands, "Toyota, Honda, Lexus");
        
        int rc = workshop_update(db, w);
        assert_test(rc == SQLITE_OK, "Мастерская обновлена");
        
        Workshop* updated = workshop_get_by_id(db, 1);
        assert_test(updated != NULL, "Обновленная мастерская найдена");
        if (updated) {
            assert_test(strcmp(updated->phone, "+375 17 888-88-88") == 0, "Телефон обновлен");
            assert_test(strcmp(updated->car_brands, "Toyota, Honda, Lexus") == 0, "Марки обновлены");
            free(updated);
        }
        free(w);
    }
    
    sqlite3_close(db);
}

// ТЕСТ 5: Удаление мастерской
void test_workshop_delete() {
    printf("\n--- Тест 5: Удаление мастерской ---\n");
    
    sqlite3* db = setup_workshop_test_db();
    
    // Пытаемся удалить мастерскую с мастерами
    int rc = workshop_delete(db, 1);
    assert_test(rc == SQLITE_CONSTRAINT, "Запрет удаления мастерской с мастерами");
    
    // Добавляем пустую мастерскую
    Workshop w;
    memset(&w, 0, sizeof(Workshop));
    strcpy(w.address, "Пустая мастерская");
    workshop_insert(db, &w);
    int new_id = w.id;
    
    // Удаляем пустую мастерскую
    rc = workshop_delete(db, new_id);
    assert_test(rc == SQLITE_OK, "Пустая мастерская удалена");
    
    Workshop* deleted = workshop_get_by_id(db, new_id);
    assert_test(deleted == NULL, "Мастерская больше не существует");
    
    sqlite3_close(db);
}

// ТЕСТ 6: Получение мастеров мастерской
void test_workshop_get_mechanics() {
    printf("\n--- Тест 6: Получение мастеров мастерской ---\n");
    
    sqlite3* db = setup_workshop_test_db();
    
    int count;
    Mechanic* mechanics = workshop_get_mechanics(db, 1, &count);
    
    assert_test(mechanics != NULL, "Мастера получены");
    assert_test(count == 2, "Найдено 2 мастера");
    if (mechanics && count >= 2) {
        assert_test(strcmp(mechanics[0].name, "Иванов") == 0, "Первый мастер - Иванов");
        assert_test(strcmp(mechanics[1].name, "Петров") == 0, "Второй мастер - Петров");
        free(mechanics);
    }
    
    mechanics = workshop_get_mechanics(db, 999, &count);
    assert_test(mechanics == NULL, "Несуществующая мастерская не имеет мастеров");
    assert_test(count == 0, "Количество мастеров = 0");
    
    sqlite3_close(db);
}

// ТЕСТ 7: Получение ремонтов мастерской за период
void test_workshop_get_repairs_by_period() {
    printf("\n--- Тест 7: Получение ремонтов за период ---\n");
    
    sqlite3* db = setup_workshop_test_db();
    
    int count;
    Repair* repairs = workshop_get_repairs_by_period(db, 1, "2026-03-01", "2026-03-02", &count);
    
    assert_test(repairs != NULL, "Ремонты получены");
    assert_test(count == 2, "Найдено 2 ремонта за 01-02.03");
    if (repairs) {
        assert_test(repairs[0].cost == 50.0 || repairs[0].cost == 120.0, "Стоимость ремонта корректна");
        free(repairs);
    }
    
    repairs = workshop_get_repairs_by_period(db, 1, "2025-01-01", "2025-01-31", &count);
    assert_test(repairs == NULL, "Пустой период возвращает NULL");
    assert_test(count == 0, "Количество ремонтов = 0");
    
    sqlite3_close(db);
}

// ТЕСТ 8: Получение выручки мастерской
void test_workshop_get_revenue() {
    printf("\n--- Тест 8: Получение выручки мастерской ---\n");
    
    sqlite3* db = setup_workshop_test_db();
    
    float revenue = workshop_get_revenue(db, 1, "2026-03-01", "2026-03-31");
    assert_test(revenue == 50.0 + 120.0, "Выручка мастерской 1 = 170.00");
    
    revenue = workshop_get_revenue(db, 2, "2026-03-01", "2026-03-31");
    assert_test(revenue == 50.0 + 120.0, "Выручка мастерской 2 = 170.00");
    
    revenue = workshop_get_revenue(db, 3, "2026-03-01", "2026-03-31");
    assert_test(revenue == 0.0, "Выручка мастерской 3 = 0.00");
    
    sqlite3_close(db);
}

// ТЕСТ 9: Получение самой активной мастерской
void test_workshop_get_most_active() {
    printf("\n--- Тест 9: Получение самой активной мастерской ---\n");
    
    sqlite3* db = setup_workshop_test_db();
    
    Workshop* w = workshop_get_most_active(db);
    assert_test(w != NULL, "Активная мастерская найдена");
    if (w) {
        assert_test(w->id == 1 || w->id == 2, "ID активной мастерской = 1 или 2");
        free(w);
    }
    
    sqlite3_close(db);
}

int main() {
    printf("\n========================================\n");
    printf("ТЕСТИРОВАНИЕ МОДУЛЯ МАСТЕРСКИХ\n");
    printf("========================================\n");
    
    test_workshop_get_all();
    test_workshop_get_by_id();
    test_workshop_insert();
    test_workshop_update();
    test_workshop_delete();
    test_workshop_get_mechanics();
    test_workshop_get_repairs_by_period();
    test_workshop_get_revenue();
    test_workshop_get_most_active();
    
    printf("\n========================================\n");
    printf("ИТОГИ ТЕСТИРОВАНИЯ\n");
    printf("========================================\n");
    printf("Пройдено: %d/%d тестов\n", tests_passed, tests_total);
    printf("========================================\n");
    
    return (tests_passed == tests_total) ? 0 : 1;
}