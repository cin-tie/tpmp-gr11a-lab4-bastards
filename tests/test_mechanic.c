#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sqlite3.h>
#include "../includes/mechanic.h"
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

sqlite3* setup_mechanic_test_db() {
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
        "workshop_id INTEGER,"
        "FOREIGN KEY (workshop_id) REFERENCES workshops(id)"
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
        "FOREIGN KEY (mechanic_id) REFERENCES mechanics(id),"
        "FOREIGN KEY (car_license) REFERENCES cars(license_plate),"
        "FOREIGN KEY (repair_type_id) REFERENCES repair_types(id)"
        ");";
    
    rc = db_execute(db, sql);
    assert(rc == SQLITE_OK);
    
    const char* workshops_sql = 
    "INSERT INTO workshops (id, address, phone, car_brands) VALUES "
    "(1, 'Мастерская 1', '+375 17 111-11-11', 'Toyota, Honda'),"
    "(2, 'Мастерская 2', '+375 17 222-22-22', 'BMW, Audi');";
    db_execute(db, workshops_sql);
    
    const char* mechanics_sql = 
        "INSERT INTO mechanics (id, name, specialization, workshop_id) VALUES "
        "(1, 'Иванов Иван', 'Кузовные работы', 1),"
        "(2, 'Петров Петр', 'Двигатель', 1),"
        "(3, 'Сидоров Сидор', 'Электрика', 1),"
        "(4, 'Козлов Николай', 'Ходовая часть', 2),"
        "(5, 'Новиков Андрей', 'ТО', 2);";
    db_execute(db, mechanics_sql);
    
    const char* cars_sql = 
        "INSERT INTO cars (license_plate, brand, model, year, owner_name, passport_number, owner_address) VALUES "
        "('CAR001', 'Toyota', 'Corolla', 2020, 'Владелец 1', 'PASS001', 'Адрес 1'),"
        "('CAR002', 'BMW', 'X5', 2021, 'Владелец 2', 'PASS002', 'Адрес 2');";
    db_execute(db, cars_sql);
    
    const char* types_sql = 
        "INSERT INTO repair_types (id, name) VALUES "
        "(1, 'ТО'), (2, 'Двигатель'), (3, 'Кузовные работы'), (4, 'Электрика');";
    db_execute(db, types_sql);
    
    const char* repairs_sql = 
        "INSERT INTO repairs (workshop_id, mechanic_id, car_license, repair_type_id, "
        "start_date, end_date, cost, status) VALUES "
        "(1, 1, 'CAR001', 3, '2026-03-01', '2026-03-02', 300.0, 'completed'),"
        "(1, 1, 'CAR002', 3, '2026-03-03', NULL, 350.0, 'in_progress'),"
        "(1, 2, 'CAR001', 2, '2026-03-01', '2026-03-01', 200.0, 'completed'),"
        "(1, 3, 'CAR001', 4, '2026-03-04', '2026-03-04', 100.0, 'completed'),"
        "(2, 4, 'CAR002', 1, '2026-03-02', '2026-03-02', 50.0, 'completed');";
    db_execute(db, repairs_sql);
    
    return db;
}

// ТЕСТ 1: Получение всех мастеров
void test_mechanic_get_all() {
    printf("\n--- Тест 1: Получение всех мастеров ---\n");
    
    sqlite3* db = setup_mechanic_test_db();
    
    int count;
    Mechanic* mechanics = mechanic_get_all(db, &count);
    
    assert_test(mechanics != NULL, "Мастера получены");
    assert_test(count == 5, "Количество мастеров = 5");
    if (mechanics && count >= 3) {
        assert_test(strcmp(mechanics[0].name, "Иванов Иван") == 0, "Первый мастер - Иванов");
        assert_test(strcmp(mechanics[1].name, "Петров Петр") == 0, "Второй мастер - Петров");
        assert_test(strcmp(mechanics[2].name, "Сидоров Сидор") == 0, "Третий мастер - Сидоров");
        free(mechanics);
    }
    
    sqlite3_close(db);
}

// ТЕСТ 2: Получение мастера по ID
void test_mechanic_get_by_id() {
    printf("\n--- Тест 2: Получение мастера по ID ---\n");
    
    sqlite3* db = setup_mechanic_test_db();
    
    Mechanic* m = mechanic_get_by_id(db, 1);
    assert_test(m != NULL, "Мастер с ID 1 найден");
    if (m) {
        assert_test(m->id == 1, "ID совпадает");
        assert_test(strcmp(m->name, "Иванов Иван") == 0, "Имя совпадает");
        assert_test(strcmp(m->specialization, "Кузовные работы") == 0, "Специализация совпадает");
        assert_test(m->workshop_id == 1, "workshop_id = 1");
        free(m);
    }
    
    m = mechanic_get_by_id(db, 999);
    assert_test(m == NULL, "Несуществующий мастер не найден");
    
    sqlite3_close(db);
}

// ТЕСТ 3: Добавление мастера
void test_mechanic_insert() {
    printf("\n--- Тест 3: Добавление мастера ---\n");
    
    sqlite3* db = setup_mechanic_test_db();
    
    Mechanic m;
    memset(&m, 0, sizeof(Mechanic));
    strcpy(m.name, "Новый Мастер");
    strcpy(m.specialization, "Тестовая специализация");
    m.workshop_id = 1;
    
    int rc = mechanic_insert(db, &m);
    assert_test(rc == SQLITE_OK, "Мастер добавлен");
    assert_test(m.id > 0, "Присвоен ID > 0");
    
    Mechanic* inserted = mechanic_get_by_id(db, m.id);
    assert_test(inserted != NULL, "Мастер найден в БД");
    if (inserted) {
        assert_test(strcmp(inserted->name, "Новый Мастер") == 0, "Имя сохранено");
        assert_test(strcmp(inserted->specialization, "Тестовая специализация") == 0, "Специализация сохранена");
        assert_test(inserted->workshop_id == 1, "workshop_id сохранен");
        free(inserted);
    }
    
    sqlite3_close(db);
}

// ТЕСТ 4: Обновление мастера
void test_mechanic_update() {
    printf("\n--- Тест 4: Обновление мастера ---\n");
    
    sqlite3* db = setup_mechanic_test_db();
    
    Mechanic* m = mechanic_get_by_id(db, 1);
    assert_test(m != NULL, "Мастер найден");
    
    if (m) {
        strcpy(m->specialization, "Новая специализация");
        m->workshop_id = 2;
        
        int rc = mechanic_update(db, m);
        assert_test(rc == SQLITE_OK, "Мастер обновлен");
        
        Mechanic* updated = mechanic_get_by_id(db, 1);
        assert_test(updated != NULL, "Обновленный мастер найден");
        if (updated) {
            assert_test(strcmp(updated->specialization, "Новая специализация") == 0, "Специализация обновлена");
            assert_test(updated->workshop_id == 2, "workshop_id обновлен");
            free(updated);
        }
        free(m);
    }
    
    sqlite3_close(db);
}

// ТЕСТ 5: Удаление мастера
void test_mechanic_delete() {
    printf("\n--- Тест 5: Удаление мастера ---\n");
    
    sqlite3* db = setup_mechanic_test_db();
    
    // Пытаемся удалить мастера с ремонтами
    int rc = mechanic_delete(db, 1);
    assert_test(rc == SQLITE_CONSTRAINT, "Запрет удаления мастера с ремонтами");
    
    // Добавляем мастера без ремонтов
    Mechanic m;
    memset(&m, 0, sizeof(Mechanic));
    strcpy(m.name, "Временный Мастер");
    m.workshop_id = 1;
    mechanic_insert(db, &m);
    int new_id = m.id;
    
    rc = mechanic_delete(db, new_id);
    assert_test(rc == SQLITE_OK, "Мастер без ремонтов удален");
    
    Mechanic* deleted = mechanic_get_by_id(db, new_id);
    assert_test(deleted == NULL, "Мастер больше не существует");
    
    sqlite3_close(db);
}

// ТЕСТ 6: Получение ремонтов мастера
void test_mechanic_get_repairs() {
    printf("\n--- Тест 6: Получение ремонтов мастера ---\n");
    
    sqlite3* db = setup_mechanic_test_db();
    
    int count;
    Repair* repairs = mechanic_get_repairs(db, 1, &count);
    
    assert_test(repairs != NULL, "Ремонты получены");
    assert_test(count == 2, "Мастер имеет 2 ремонта");
    if (repairs) {
        assert_test(repairs[0].mechanic_id == 1, "Первый ремонт принадлежит мастеру 1");
        assert_test(repairs[1].mechanic_id == 1, "Второй ремонт принадлежит мастеру 1");
        free(repairs);
    }
    
    sqlite3_close(db);
}

// ТЕСТ 7: Получение ремонтов мастера за период
void test_mechanic_get_repairs_by_period() {
    printf("\n--- Тест 7: Получение ремонтов за период ---\n");
    
    sqlite3* db = setup_mechanic_test_db();
    
    int count;
    Repair* repairs = mechanic_get_repairs_by_period(db, 1, "2026-03-01", "2026-03-01", &count);
    
    assert_test(repairs != NULL, "Ремонты за 01.03 получены");
    assert_test(count == 1, "Найден 1 ремонт за 01.03");
    if (repairs) {
        assert_test(repairs[0].cost == 300.0, "Стоимость ремонта = 300.0");
        free(repairs);
    }
    
    repairs = mechanic_get_repairs_by_period(db, 1, "2026-03-02", "2026-03-02", &count);
    assert_test(repairs == NULL, "Пустой период возвращает NULL");
    assert_test(count == 0, "Количество ремонтов = 0");
    
    sqlite3_close(db);
}

// ТЕСТ 8: Получение статистики по видам работ
void test_mechanic_get_stats_by_type() {
    printf("\n--- Тест 8: Получение статистики по видам работ ---\n");
    
    sqlite3* db = setup_mechanic_test_db();
    
    int type_counts[10] = {0};
    int rc = mechanic_get_stats_by_type(db, 1, type_counts, 10);
    
    assert_test(rc == 0, "Статистика получена");
    assert_test(type_counts[2] == 2, "Кузовные работы: 2 ремонта");
    
    memset(type_counts, 0, sizeof(type_counts));
    rc = mechanic_get_stats_by_type(db, 2, type_counts, 10);
    assert_test(rc == 0, "Статистика для мастера 2 получена");
    assert_test(type_counts[1] == 1, "Двигатель: 1 ремонт");
    
    sqlite3_close(db);
}

// ТЕСТ 9: Получение мастерской мастера
void test_mechanic_get_workshop() {
    printf("\n--- Тест 9: Получение мастерской мастера ---\n");
    
    sqlite3* db = setup_mechanic_test_db();
    
    // Получаем мастера 1
    Mechanic* m = mechanic_get_by_id(db, 1);
    assert_test(m != NULL, "Мастер 1 найден");
    
    if (m) {
        // Получаем мастерскую мастера
        Workshop* w = mechanic_get_workshop(db, 1);
        
        if (w) {
            assert_test(w->id == m->workshop_id, "ID мастерской совпадает");
            assert_test(w->id == 1, "Мастерская имеет ID 1");
            assert_test(strcmp(w->address, "Мастерская 1") == 0, "Адрес мастерской = 'Мастерская 1'");
            free(w);
        } else {
            // Если мастерская не найдена - тест не проходит
            assert_test(0, "Мастерская найдена");
        }
        free(m);
    }
    
    // Проверяем несуществующего мастера
    Workshop* w = mechanic_get_workshop(db, 999);
    assert_test(w == NULL, "Несуществующий мастер не имеет мастерской");
    
    sqlite3_close(db);
}

int main() {
    printf("\n========================================\n");
    printf("ТЕСТИРОВАНИЕ МОДУЛЯ МАСТЕРОВ\n");
    printf("========================================\n");
    
    test_mechanic_get_all();
    test_mechanic_get_by_id();
    test_mechanic_insert();
    test_mechanic_update();
    test_mechanic_delete();
    test_mechanic_get_repairs();
    test_mechanic_get_repairs_by_period();
    test_mechanic_get_stats_by_type();
    test_mechanic_get_workshop();
    
    printf("\n========================================\n");
    printf("ИТОГИ ТЕСТИРОВАНИЯ\n");
    printf("========================================\n");
    printf("Пройдено: %d/%d тестов\n", tests_passed, tests_total);
    printf("========================================\n");
    
    return (tests_passed == tests_total) ? 0 : 1;
}