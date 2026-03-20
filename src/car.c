#include "car.h"
#include <stdlib.h>
#include <string.h>

// Получить все автомобили
Car* car_get_all(sqlite3* db, int* count) {
    if (!db || !count) return NULL;
    
    const char* count_sql = "SELECT COUNT(*) FROM cars;";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, count_sql, -1, &stmt, NULL) != SQLITE_OK) {
        return NULL;
    }
    
    int total = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        total = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    
    if (total == 0) {
        *count = 0;
        return NULL;
    }
    
    Car* cars = (Car*)malloc(sizeof(Car) * total);
    if (!cars) return NULL;
    
    const char* sql = "SELECT license_plate, brand, model, year, owner_name, "
                        "passport_number, owner_address FROM cars ORDER BY brand;";
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < total) {
        strcpy(cars[i].license_plate, (const char*)sqlite3_column_text(stmt, 0));
        strcpy(cars[i].brand, (const char*)sqlite3_column_text(stmt, 1));
        strcpy(cars[i].model, (const char*)sqlite3_column_text(stmt, 2));
        cars[i].year = sqlite3_column_int(stmt, 3);
        strcpy(cars[i].owner_name, (const char*)sqlite3_column_text(stmt, 4));
        strcpy(cars[i].passport_number, (const char*)sqlite3_column_text(stmt, 5));
        strcpy(cars[i].owner_address, (const char*)sqlite3_column_text(stmt, 6));
        i++;
    }
    sqlite3_finalize(stmt);
    
    *count = total;
    return cars;
}

// Получить автомобиль по госномеру
Car* car_get_by_license(sqlite3* db, const char* license_plate) {
    if (!db || !license_plate) return NULL;
    
    char* safe_plate = db_escape_string(license_plate);
    
    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT license_plate, brand, model, year, owner_name, "
        "passport_number, owner_address FROM cars WHERE license_plate = '%s';",
        safe_plate);
    
    free(safe_plate);
    
    sqlite3_stmt* stmt;
    Car* car = NULL;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            car = (Car*)malloc(sizeof(Car));
            strcpy(car->license_plate, (const char*)sqlite3_column_text(stmt, 0));
            strcpy(car->brand, (const char*)sqlite3_column_text(stmt, 1));
            strcpy(car->model, (const char*)sqlite3_column_text(stmt, 2));
            car->year = sqlite3_column_int(stmt, 3);
            strcpy(car->owner_name, (const char*)sqlite3_column_text(stmt, 4));
            strcpy(car->passport_number, (const char*)sqlite3_column_text(stmt, 5));
            strcpy(car->owner_address, (const char*)sqlite3_column_text(stmt, 6));
        }
        sqlite3_finalize(stmt);
    }
    
    return car;
}

// Получить автомобили по марке
Car* car_get_by_brand(sqlite3* db, const char* brand, int* count) {
    if (!db || !brand || !count) return NULL;
    
    char* safe_brand = db_escape_string(brand);
    
    char count_sql[512];
    snprintf(count_sql, sizeof(count_sql),
        "SELECT COUNT(*) FROM cars WHERE brand = '%s';", safe_brand);
    
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, count_sql, -1, &stmt, NULL) != SQLITE_OK) {
        free(safe_brand);
        return NULL;
    }
    
    int total = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        total = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    
    if (total == 0) {
        free(safe_brand);
        *count = 0;
        return NULL;
    }
    
    Car* cars = (Car*)malloc(sizeof(Car) * total);
    if (!cars) {
        free(safe_brand);
        return NULL;
    }
    
    char sql[1024];
    snprintf(sql, sizeof(sql),
        "SELECT license_plate, brand, model, year, owner_name, "
        "passport_number, owner_address FROM cars WHERE brand = '%s' ORDER BY model;",
        safe_brand);
    
    free(safe_brand);
    
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < total) {
        strcpy(cars[i].license_plate, (const char*)sqlite3_column_text(stmt, 0));
        strcpy(cars[i].brand, (const char*)sqlite3_column_text(stmt, 1));
        strcpy(cars[i].model, (const char*)sqlite3_column_text(stmt, 2));
        cars[i].year = sqlite3_column_int(stmt, 3);
        strcpy(cars[i].owner_name, (const char*)sqlite3_column_text(stmt, 4));
        strcpy(cars[i].passport_number, (const char*)sqlite3_column_text(stmt, 5));
        strcpy(cars[i].owner_address, (const char*)sqlite3_column_text(stmt, 6));
        i++;
    }
    sqlite3_finalize(stmt);
    
    *count = total;
    return cars;
}

// Получить автомобили по владельцу
Car* car_get_by_owner(sqlite3* db, const char* owner_name, int* count) {
    if (!db || !owner_name || !count) return NULL;
    
    char* safe_owner = db_escape_string(owner_name);
    
    char count_sql[512];
    snprintf(count_sql, sizeof(count_sql),
        "SELECT COUNT(*) FROM cars WHERE owner_name LIKE '%%%s%%';", safe_owner);
    
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, count_sql, -1, &stmt, NULL) != SQLITE_OK) {
        free(safe_owner);
        return NULL;
    }
    
    int total = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        total = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    
    if (total == 0) {
        free(safe_owner);
        *count = 0;
        return NULL;
    }
    
    Car* cars = (Car*)malloc(sizeof(Car) * total);
    if (!cars) {
        free(safe_owner);
        return NULL;
    }
    
    char sql[1024];
    snprintf(sql, sizeof(sql),
        "SELECT license_plate, brand, model, year, owner_name, "
        "passport_number, owner_address FROM cars "
        "WHERE owner_name LIKE '%%%s%%' ORDER BY brand;",
        safe_owner);
    
    free(safe_owner);
    
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < total) {
        strcpy(cars[i].license_plate, (const char*)sqlite3_column_text(stmt, 0));
        strcpy(cars[i].brand, (const char*)sqlite3_column_text(stmt, 1));
        strcpy(cars[i].model, (const char*)sqlite3_column_text(stmt, 2));
        cars[i].year = sqlite3_column_int(stmt, 3);
        strcpy(cars[i].owner_name, (const char*)sqlite3_column_text(stmt, 4));
        strcpy(cars[i].passport_number, (const char*)sqlite3_column_text(stmt, 5));
        strcpy(cars[i].owner_address, (const char*)sqlite3_column_text(stmt, 6));
        i++;
    }
    sqlite3_finalize(stmt);
    
    *count = total;
    return cars;
}

// Добавить новый автомобиль
int car_insert(sqlite3* db, Car* car) {
    if (!db || !car) return SQLITE_ERROR;
    
    char* safe_plate = db_escape_string(car->license_plate);
    char* safe_brand = db_escape_string(car->brand);
    char* safe_model = db_escape_string(car->model);
    char* safe_owner = db_escape_string(car->owner_name);
    char* safe_passport = db_escape_string(car->passport_number);
    char* safe_address = db_escape_string(car->owner_address);
    
    char sql[2048];
    snprintf(sql, sizeof(sql),
        "INSERT INTO cars (license_plate, brand, model, year, owner_name, "
        "passport_number, owner_address) VALUES ('%s', '%s', '%s', %d, '%s', '%s', '%s');",
        safe_plate, safe_brand, safe_model, car->year, 
        safe_owner, safe_passport, safe_address);
    
    free(safe_plate);
    free(safe_brand);
    free(safe_model);
    free(safe_owner);
    free(safe_passport);
    free(safe_address);
    
    return db_execute(db, sql);
}

// Обновить информацию об автомобиле
int car_update(sqlite3* db, Car* car) {
    if (!db || !car || !car->license_plate[0]) return SQLITE_ERROR;
    
    char* safe_plate = db_escape_string(car->license_plate);
    char* safe_brand = db_escape_string(car->brand);
    char* safe_model = db_escape_string(car->model);
    char* safe_owner = db_escape_string(car->owner_name);
    char* safe_passport = db_escape_string(car->passport_number);
    char* safe_address = db_escape_string(car->owner_address);
    
    char sql[2048];
    snprintf(sql, sizeof(sql),
        "UPDATE cars SET brand = '%s', model = '%s', year = %d, "
        "owner_name = '%s', passport_number = '%s', owner_address = '%s' "
        "WHERE license_plate = '%s';",
        safe_brand, safe_model, car->year, safe_owner, safe_passport, safe_address, safe_plate);
    
    free(safe_plate);
    free(safe_brand);
    free(safe_model);
    free(safe_owner);
    free(safe_passport);
    free(safe_address);
    
    return db_execute(db, sql);
}

// Удалить автомобиль
int car_delete(sqlite3* db, const char* license_plate) {
    if (!db || !license_plate) return SQLITE_ERROR;
    
    // Проверяем, есть ли связанные ремонты
    char* safe_plate = db_escape_string(license_plate);
    
    char check_sql[512];
    snprintf(check_sql, sizeof(check_sql),
        "SELECT COUNT(*) FROM repairs WHERE car_license = '%s';", safe_plate);
    
    sqlite3_stmt* stmt;
    int has_repairs = 0;
    
    if (sqlite3_prepare_v2(db, check_sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            has_repairs = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    
    if (has_repairs > 0) {
        free(safe_plate);
        return SQLITE_CONSTRAINT;
    }
    
    char sql[512];
    snprintf(sql, sizeof(sql), "DELETE FROM cars WHERE license_plate = '%s';", safe_plate);
    
    free(safe_plate);
    return db_execute(db, sql);
}

// Получить историю ремонтов автомобиля
Repair* car_get_repair_history(sqlite3* db, const char* license_plate, int* count) {
    if (!db || !license_plate || !count) return NULL;
    
    char* safe_plate = db_escape_string(license_plate);
    
    char count_sql[512];
    snprintf(count_sql, sizeof(count_sql),
        "SELECT COUNT(*) FROM repairs WHERE car_license = '%s';", safe_plate);
    
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, count_sql, -1, &stmt, NULL) != SQLITE_OK) {
        free(safe_plate);
        return NULL;
    }
    
    int total = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        total = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    
    if (total == 0) {
        free(safe_plate);
        *count = 0;
        return NULL;
    }
    
    Repair* repairs = (Repair*)malloc(sizeof(Repair) * total);
    if (!repairs) {
        free(safe_plate);
        return NULL;
    }
    
    char sql[1024];
    snprintf(sql, sizeof(sql),
        "SELECT id, workshop_id, mechanic_id, car_license, repair_type_id, "
        "start_date, end_date, cost, status FROM repairs "
        "WHERE car_license = '%s' ORDER BY start_date DESC;", safe_plate);
    
    free(safe_plate);
    
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < total) {
        repairs[i].id = sqlite3_column_int(stmt, 0);
        repairs[i].workshop_id = sqlite3_column_int(stmt, 1);
        repairs[i].mechanic_id = sqlite3_column_int(stmt, 2);
        strcpy(repairs[i].car_license, (const char*)sqlite3_column_text(stmt, 3));
        repairs[i].repair_type_id = sqlite3_column_int(stmt, 4);
        strcpy(repairs[i].start_date, (const char*)sqlite3_column_text(stmt, 5));
        strcpy(repairs[i].end_date, (const char*)sqlite3_column_text(stmt, 6));
        repairs[i].cost = (float)sqlite3_column_double(stmt, 7);
        strcpy(repairs[i].status, (const char*)sqlite3_column_text(stmt, 8));
        i++;
    }
    sqlite3_finalize(stmt);
    
    *count = total;
    return repairs;
}

// Проверить существование автомобиля
int car_exists(sqlite3* db, const char* license_plate) {
    if (!db || !license_plate) return 0;
    
    char* safe_plate = db_escape_string(license_plate);
    
    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT COUNT(*) FROM cars WHERE license_plate = '%s';", safe_plate);
    
    free(safe_plate);
    
    sqlite3_stmt* stmt;
    int exists = 0;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            exists = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    
    return exists > 0;
}