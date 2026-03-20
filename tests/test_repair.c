#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sqlite3.h>
#include "../includes/repair.h"
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

sqlite3* setup_repair_test_db() {
    sqlite3* db;
    int rc = sqlite3_open(":memory:", &db);
    assert(rc == SQLITE_OK);
    
    const char* sql = 
        "CREATE TABLE workshops ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "address TEXT NOT NULL"
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
        "passport_number TEXT,"
        "owner_address TEXT"
        ");"
        
        "CREATE TABLE repair_types ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL"
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
    
    const char* data_sql = 
        "INSERT INTO workshops (address) VALUES "
        "('Мастерская 1'), ('Мастерская 2');"
        
        "INSERT INTO mechanics (name, specialization, workshop_id) VALUES "
        "('Мастер 1', 'Кузовные работы', 1),"
        "('Мастер 2', 'Двигатель', 1),"
        "('Мастер 3', 'Электрика', 2);"
        
        "INSERT INTO cars (license_plate, brand, model, year, owner_name, passport_number, owner_address) VALUES "
        "('CAR001', 'Toyota', 'Corolla', 2020, 'Владелец 1', 'PASS001', 'Адрес 1'),"
        "('CAR002', 'BMW', 'X5', 2021, 'Владелец 2', 'PASS002', 'Адрес 2'),"
        "('CAR003', 'Audi', 'A4', 2022, 'Владелец 3', 'PASS003', 'Адрес 3');"
        
        "INSERT INTO repair_types (name) VALUES "
        "('ТО'), ('Двигатель'), ('Кузовные работы'), ('Электрика');"
        
        "INSERT INTO repairs (workshop_id, mechanic_id, car_license, repair_type_id, "
        "start_date, end_date, cost, status) VALUES "
        "(1, 1, 'CAR001', 1, '2026-03-01', '2026-03-01', 100.0, 'completed'),"
        "(1, 1, 'CAR001', 2, '2026-03-02', '2026-03-03', 200.0, 'completed'),"
        "(1, 2, 'CAR002', 1, '2026-03-01', '2026-03-01', 100.0, 'completed'),"
        "(1, 2, 'CAR002', 3, '2026-03-04', NULL, 300.0, 'in_progress'),"
        "(2, 3, 'CAR003', 1, '2026-03-05', '2026-03-05', 100.0, 'completed'),"
        "(2, 3, 'CAR003', 4, '2026-03-06', NULL, 150.0, 'in_progress');";
        
    rc = db_execute(db, data_sql);
    assert(rc == SQLITE_OK);
    
    return db;
}

// ТЕСТ 1: Получение всех ремонтов
void test_repair_get_all() {
    printf("\n--- Тест 1: Получение всех ремонтов ---\n");
    
    sqlite3* db = setup_repair_test_db();
    
    int count;
    Repair* repairs = repair_get_all(db, &count);
    
    assert_test(repairs != NULL, "Ремонты получены");
    assert_test(count == 6, "Количество ремонтов = 6");
    
    free(repairs);
    sqlite3_close(db);
}

// ТЕСТ 2: Получение ремонта по ID
void test_repair_get_by_id() {
    printf("\n--- Тест 2: Получение ремонта по ID ---\n");
    
    sqlite3* db = setup_repair_test_db();
    
    Repair* r = repair_get_by_id(db, 1);
    assert_test(r != NULL, "Ремонт с ID 1 найден");
    if (r) {
        assert_test(r->id == 1, "ID совпадает");
        assert_test(r->workshop_id == 1, "workshop_id = 1");
        assert_test(r->mechanic_id == 1, "mechanic_id = 1");
        assert_test(strcmp(r->car_license, "CAR001") == 0, "car_license = CAR001");
        assert_test(r->repair_type_id == 1, "repair_type_id = 1");
        assert_test(r->cost == 100.0, "cost = 100.0");
        assert_test(strcmp(r->status, "completed") == 0, "status = completed");
        free(r);
    }
    
    r = repair_get_by_id(db, 999);
    assert_test(r == NULL, "Несуществующий ремонт не найден");
    
    sqlite3_close(db);
}

// ТЕСТ 3: Добавление ремонта
void test_repair_insert() {
    printf("\n--- Тест 3: Добавление ремонта ---\n");
    
    sqlite3* db = setup_repair_test_db();
    
    Repair r;
    memset(&r, 0, sizeof(Repair));
    r.workshop_id = 1;
    r.mechanic_id = 1;
    strcpy(r.car_license, "CAR001");
    r.repair_type_id = 4;
    strcpy(r.start_date, "2026-03-10");
    r.cost = 250.0;
    strcpy(r.status, "in_progress");
    r.end_date[0] = '\0';
    
    int rc = repair_insert(db, &r);
    assert_test(rc == SQLITE_OK, "Ремонт добавлен");
    assert_test(r.id > 0, "Присвоен ID > 0");
    
    // Проверяем через отдельный запрос
    char check_sql[256];
    snprintf(check_sql, sizeof(check_sql),
        "SELECT workshop_id, mechanic_id, cost FROM repairs WHERE id = %d;", r.id);
    
    sqlite3_stmt* stmt;
    int found = 0;
    if (sqlite3_prepare_v2(db, check_sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            found = 1;
            int ws = sqlite3_column_int(stmt, 0);
            int mech = sqlite3_column_int(stmt, 1);
            float cost = (float)sqlite3_column_double(stmt, 2);
            assert_test(ws == 1, "workshop_id = 1");
            assert_test(mech == 1, "mechanic_id = 1");
            assert_test(cost == 250.0, "cost = 250.0");
        }
        sqlite3_finalize(stmt);
    }
    assert_test(found == 1, "Данные сохранены в БД");
    
    sqlite3_close(db);
}

// ТЕСТ 4: Обновление ремонта
void test_repair_update() {
    printf("\n--- Тест 4: Обновление ремонта ---\n");
    
    sqlite3* db = setup_repair_test_db();
    
    Repair* r = repair_get_by_id(db, 1);
    assert_test(r != NULL, "Ремонт найден");
    
    if (r) {
        r->cost = 150.0;
        strcpy(r->status, "in_progress");
        
        int rc = repair_update(db, r);
        assert_test(rc == SQLITE_OK, "Ремонт обновлен");
        
        Repair* updated = repair_get_by_id(db, 1);
        assert_test(updated != NULL, "Обновленный ремонт найден");
        if (updated) {
            assert_test(updated->cost == 150.0, "cost обновлен на 150.0");
            assert_test(strcmp(updated->status, "in_progress") == 0, "status обновлен на in_progress");
            free(updated);
        }
        free(r);
    }
    
    sqlite3_close(db);
}

// ТЕСТ 5: Завершение ремонта
void test_repair_complete() {
    printf("\n--- Тест 5: Завершение ремонта ---\n");
    
    sqlite3* db = setup_repair_test_db();
    
    int rc = repair_complete(db, 4, "2026-03-15");
    assert_test(rc == SQLITE_OK, "Ремонт завершен");
    
    Repair* r = repair_get_by_id(db, 4);
    assert_test(r != NULL, "Ремонт найден");
    if (r) {
        assert_test(strcmp(r->status, "completed") == 0, "status = completed");
        assert_test(strcmp(r->end_date, "2026-03-15") == 0, "end_date = 2026-03-15");
        free(r);
    }
    
    sqlite3_close(db);
}

// ТЕСТ 6: Удаление ремонта
void test_repair_delete() {
    printf("\n--- Тест 6: Удаление ремонта ---\n");
    
    sqlite3* db = setup_repair_test_db();
    
    int rc = repair_delete(db, 6);
    assert_test(rc == SQLITE_OK, "Ремонт удален");
    
    Repair* r = repair_get_by_id(db, 6);
    assert_test(r == NULL, "Ремонт больше не существует");
    
    sqlite3_close(db);
}

// ТЕСТ 7: Получение ремонтов по статусу
void test_repair_get_by_status() {
    printf("\n--- Тест 7: Получение ремонтов по статусу ---\n");
    
    sqlite3* db = setup_repair_test_db();
    
    int count;
    Repair* repairs = repair_get_by_status(db, "completed", &count);
    
    assert_test(repairs != NULL, "Завершенные ремонты получены");
    assert_test(count == 4, "Найдено 4 завершенных ремонта");
    if (repairs) {
        for (int i = 0; i < count; i++) {
            char test_name[64];
            snprintf(test_name, sizeof(test_name), "Ремонт %d имеет статус completed", repairs[i].id);
            assert_test(strcmp(repairs[i].status, "completed") == 0, test_name);
        }
        free(repairs);
    }
    
    repairs = repair_get_by_status(db, "in_progress", &count);
    assert_test(repairs != NULL, "Ремонты в процессе получены");
    assert_test(count == 2, "Найдено 2 ремонта в процессе");
    if (repairs) {
        free(repairs);
    }
    
    sqlite3_close(db);
}

// ТЕСТ 8: Получение статистики по ремонтам за период
void test_repair_get_stats() {
    printf("\n--- Тест 8: Получение статистики по ремонтам за период ---\n");
    
    sqlite3* db = setup_repair_test_db();
    
    int type_counts[10] = {0};
    float total_revenue = 0;
    
    int rc = repair_get_stats_by_period(db, 1, "2026-03-01", "2026-03-31", 
                                         type_counts, 10, &total_revenue);
    
    assert_test(rc == 0, "Статистика получена");
    assert_test(type_counts[0] == 2, "ТО: 2 ремонта");
    assert_test(type_counts[1] == 1, "Двигатель: 1 ремонт");
    assert_test(type_counts[2] == 1, "Кузовные работы: 1 ремонт");
    assert_test(total_revenue == 700.0, "Общая выручка = 700.0");
    
    sqlite3_close(db);
}

// ТЕСТ 9: Получение информации о мастере
void test_repair_get_mechanic() {
    printf("\n--- Тест 9: Получение информации о мастере ---\n");
    
    sqlite3* db = setup_repair_test_db();
    
    Mechanic* m = repair_get_mechanic(db, 1);
    assert_test(m != NULL, "Мастер для ремонта 1 найден");
    if (m) {
        assert_test(m->id == 1, "ID мастера = 1");
        assert_test(strcmp(m->name, "Мастер 1") == 0, "Имя мастера = Мастер 1");
        free(m);
    }
    
    sqlite3_close(db);
}

// ТЕСТ 10: Получение информации об автомобиле
void test_repair_get_car() {
    printf("\n--- Тест 10: Получение информации об автомобиле ---\n");
    
    sqlite3* db = setup_repair_test_db();
    
    Car* c = repair_get_car(db, 1);
    assert_test(c != NULL, "Автомобиль для ремонта 1 найден");
    if (c) {
        assert_test(strcmp(c->license_plate, "CAR001") == 0, "Госномер = CAR001");
        assert_test(strcmp(c->brand, "Toyota") == 0, "Марка = Toyota");
        free(c);
    }
    
    sqlite3_close(db);
}

int main() {
    printf("\n========================================\n");
    printf("ТЕСТИРОВАНИЕ МОДУЛЯ РЕМОНТОВ\n");
    printf("========================================\n");
    
    test_repair_get_all();
    test_repair_get_by_id();
    test_repair_insert();
    test_repair_update();
    test_repair_complete();
    test_repair_delete();
    test_repair_get_by_status();
    test_repair_get_stats();
    test_repair_get_mechanic();
    test_repair_get_car();
    
    printf("\n========================================\n");
    printf("ИТОГИ ТЕСТИРОВАНИЯ\n");
    printf("========================================\n");
    printf("Пройдено: %d/%d тестов\n", tests_passed, tests_total);
    printf("========================================\n");
    
    return (tests_passed == tests_total) ? 0 : 1;
}