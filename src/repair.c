#include "repair.h"
#include <stdlib.h>
#include <string.h>

// Получить все ремонты
// Получить все ремонты
Repair* repair_get_all(sqlite3* db, int* count) {
    if (!db || !count) return NULL;
    
    const char* count_sql = "SELECT COUNT(*) FROM repairs;";
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
    
    Repair* repairs = (Repair*)malloc(sizeof(Repair) * total);
    if (!repairs) return NULL;
    
    const char* sql = "SELECT id, workshop_id, mechanic_id, car_license, repair_type_id, "
                        "start_date, end_date, cost, status FROM repairs ORDER BY start_date DESC;";
    
    // ВАЖНО: проверяем результат prepare
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        free(repairs);
        return NULL;
    }
    
    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < total) {
        repairs[i].id = sqlite3_column_int(stmt, 0);
        repairs[i].workshop_id = sqlite3_column_int(stmt, 1);
        repairs[i].mechanic_id = sqlite3_column_int(stmt, 2);
        
        const char* car_license = (const char*)sqlite3_column_text(stmt, 3);
        if (car_license) {
            strncpy(repairs[i].car_license, car_license, sizeof(repairs[i].car_license) - 1);
            repairs[i].car_license[sizeof(repairs[i].car_license) - 1] = '\0';
        } else {
            repairs[i].car_license[0] = '\0';
        }
        
        repairs[i].repair_type_id = sqlite3_column_int(stmt, 4);
        
        const char* start_date = (const char*)sqlite3_column_text(stmt, 5);
        if (start_date) {
            strncpy(repairs[i].start_date, start_date, sizeof(repairs[i].start_date) - 1);
            repairs[i].start_date[sizeof(repairs[i].start_date) - 1] = '\0';
        } else {
            repairs[i].start_date[0] = '\0';
        }
        
        const char* end_date = (const char*)sqlite3_column_text(stmt, 6);
        if (end_date) {
            strncpy(repairs[i].end_date, end_date, sizeof(repairs[i].end_date) - 1);
            repairs[i].end_date[sizeof(repairs[i].end_date) - 1] = '\0';
        } else {
            repairs[i].end_date[0] = '\0';
        }
        
        repairs[i].cost = (float)sqlite3_column_double(stmt, 7);
        
        const char* status = (const char*)sqlite3_column_text(stmt, 8);
        if (status) {
            strncpy(repairs[i].status, status, sizeof(repairs[i].status) - 1);
            repairs[i].status[sizeof(repairs[i].status) - 1] = '\0';
        } else {
            strcpy(repairs[i].status, "unknown");
        }
        
        i++;
    }
    sqlite3_finalize(stmt);
    
    *count = total;
    return repairs;
}

// Получить ремонт по ID
Repair* repair_get_by_id(sqlite3* db, int id) {
    if (!db || id <= 0) return NULL;
    
    char sql[256];
    snprintf(sql, sizeof(sql),
        "SELECT id, workshop_id, mechanic_id, car_license, repair_type_id, "
        "start_date, end_date, cost, status FROM repairs WHERE id = %d;", id);
    
    sqlite3_stmt* stmt;
    Repair* repair = NULL;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            repair = (Repair*)malloc(sizeof(Repair));
            repair->id = sqlite3_column_int(stmt, 0);
            repair->workshop_id = sqlite3_column_int(stmt, 1);
            repair->mechanic_id = sqlite3_column_int(stmt, 2);
            strcpy(repair->car_license, (const char*)sqlite3_column_text(stmt, 3));
            repair->repair_type_id = sqlite3_column_int(stmt, 4);
            strcpy(repair->start_date, (const char*)sqlite3_column_text(stmt, 5));
            strcpy(repair->end_date, (const char*)sqlite3_column_text(stmt, 6));
            repair->cost = (float)sqlite3_column_double(stmt, 7);
            strcpy(repair->status, (const char*)sqlite3_column_text(stmt, 8));
        }
        sqlite3_finalize(stmt);
    }
    
    return repair;
}

// Добавить новый ремонт
int repair_insert(sqlite3* db, Repair* repair) {
    if (!db || !repair) return SQLITE_ERROR;
    
    char* safe_plate = db_escape_string(repair->car_license);
    char* safe_start = db_escape_string(repair->start_date);
    char* safe_end = db_escape_string(repair->end_date);
    char* safe_status = db_escape_string(repair->status);
    
    char sql[2048];
    
    if (repair->end_date[0] == '\0') {
        snprintf(sql, sizeof(sql),
            "INSERT INTO repairs (workshop_id, mechanic_id, car_license, repair_type_id, "
            "start_date, cost, status) VALUES (%d, %d, '%s', %d, '%s', %.2f, '%s');",
            repair->workshop_id, repair->mechanic_id, safe_plate, repair->repair_type_id,
            safe_start, repair->cost, safe_status);
    } else {
        snprintf(sql, sizeof(sql),
            "INSERT INTO repairs (workshop_id, mechanic_id, car_license, repair_type_id, "
            "start_date, end_date, cost, status) VALUES (%d, %d, '%s', %d, '%s', '%s', %.2f, '%s');",
            repair->workshop_id, repair->mechanic_id, safe_plate, repair->repair_type_id,
            safe_start, safe_end, repair->cost, safe_status);
    }
    
    free(safe_plate);
    free(safe_start);
    free(safe_end);
    free(safe_status);
    
    int rc = db_execute(db, sql);
    if (rc == SQLITE_OK) {
        repair->id = db_last_insert_rowid(db);
    }
    
    return rc;
}

// Обновить информацию о ремонте
int repair_update(sqlite3* db, Repair* repair) {
    if (!db || !repair || repair->id <= 0) return SQLITE_ERROR;
    
    char* safe_plate = db_escape_string(repair->car_license);
    char* safe_start = db_escape_string(repair->start_date);
    char* safe_end = db_escape_string(repair->end_date);
    char* safe_status = db_escape_string(repair->status);
    
    char sql[2048];
    snprintf(sql, sizeof(sql),
        "UPDATE repairs SET workshop_id = %d, mechanic_id = %d, car_license = '%s', "
        "repair_type_id = %d, start_date = '%s', end_date = '%s', cost = %.2f, status = '%s' "
        "WHERE id = %d;",
        repair->workshop_id, repair->mechanic_id, safe_plate, repair->repair_type_id,
        safe_start, safe_end, repair->cost, safe_status, repair->id);
    
    free(safe_plate);
    free(safe_start);
    free(safe_end);
    free(safe_status);
    
    return db_execute(db, sql);
}

// Удалить ремонт
int repair_delete(sqlite3* db, int id) {
    if (!db || id <= 0) return SQLITE_ERROR;
    
    char sql[256];
    snprintf(sql, sizeof(sql), "DELETE FROM repairs WHERE id = %d;", id);
    return db_execute(db, sql);
}

// Завершить ремонт
int repair_complete(sqlite3* db, int id, const char* end_date) {
    if (!db || id <= 0 || !end_date) return SQLITE_ERROR;
    
    char* safe_end = db_escape_string(end_date);
    
    char sql[512];
    snprintf(sql, sizeof(sql),
        "UPDATE repairs SET end_date = '%s', status = 'completed' WHERE id = %d;",
        safe_end, id);
    
    free(safe_end);
    return db_execute(db, sql);
}

// Получить ремонты по статусу
Repair* repair_get_by_status(sqlite3* db, const char* status, int* count) {
    if (!db || !status || !count) return NULL;
    
    char* safe_status = db_escape_string(status);
    
    char count_sql[512];
    snprintf(count_sql, sizeof(count_sql),
        "SELECT COUNT(*) FROM repairs WHERE status = '%s';", safe_status);
    
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, count_sql, -1, &stmt, NULL) != SQLITE_OK) {
        free(safe_status);
        return NULL;
    }
    
    int total = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        total = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    
    if (total == 0) {
        free(safe_status);
        *count = 0;
        return NULL;
    }
    
    Repair* repairs = (Repair*)malloc(sizeof(Repair) * total);
    if (!repairs) {
        free(safe_status);
        return NULL;
    }
    
    char sql[1024];
    snprintf(sql, sizeof(sql),
        "SELECT id, workshop_id, mechanic_id, car_license, repair_type_id, "
        "start_date, end_date, cost, status FROM repairs "
        "WHERE status = '%s' ORDER BY start_date;", safe_status);
    
    free(safe_status);
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        free(repairs);
        return NULL;
    }
    
    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < total) {
        repairs[i].id = sqlite3_column_int(stmt, 0);
        repairs[i].workshop_id = sqlite3_column_int(stmt, 1);
        repairs[i].mechanic_id = sqlite3_column_int(stmt, 2);
        
        const char* car_license = (const char*)sqlite3_column_text(stmt, 3);
        if (car_license) strcpy(repairs[i].car_license, car_license);
        else repairs[i].car_license[0] = '\0';
        
        repairs[i].repair_type_id = sqlite3_column_int(stmt, 4);
        
        const char* start_date = (const char*)sqlite3_column_text(stmt, 5);
        if (start_date) strcpy(repairs[i].start_date, start_date);
        else repairs[i].start_date[0] = '\0';
        
        const char* end_date = (const char*)sqlite3_column_text(stmt, 6);
        if (end_date) strcpy(repairs[i].end_date, end_date);
        else repairs[i].end_date[0] = '\0';
        
        repairs[i].cost = (float)sqlite3_column_double(stmt, 7);
        
        const char* status_val = (const char*)sqlite3_column_text(stmt, 8);
        if (status_val) strcpy(repairs[i].status, status_val);
        else strcpy(repairs[i].status, "unknown");
        
        i++;
    }
    sqlite3_finalize(stmt);
    
    *count = total;
    return repairs;
}

// Получить статистику по ремонтам за период
int repair_get_stats_by_period(sqlite3* db, int workshop_id, 
                                const char* start_date, const char* end_date,
                                int* type_counts, int type_count, float* total_revenue) {
    if (!db || workshop_id <= 0 || !start_date || !end_date || !type_counts || type_count <= 0) {
        return -1;
    }
    
    // Инициализируем массивы
    for (int i = 0; i < type_count; i++) {
        type_counts[i] = 0;
    }
    *total_revenue = 0.0;
    
    char sql[1024];
    snprintf(sql, sizeof(sql),
        "SELECT repair_type_id, COUNT(*) as cnt, SUM(cost) as revenue FROM repairs "
        "WHERE workshop_id = %d AND start_date >= '%s' AND start_date <= '%s' "
        "GROUP BY repair_type_id;",
        workshop_id, start_date, end_date);
    
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int type_id = sqlite3_column_int(stmt, 0);
            int cnt = sqlite3_column_int(stmt, 1);
            float revenue = (float)sqlite3_column_double(stmt, 2);
            
            if (type_id > 0 && type_id <= type_count) {
                type_counts[type_id - 1] = cnt;
            }
            *total_revenue += revenue;
        }
        sqlite3_finalize(stmt);
    }
    
    return 0;
}

// Получить информацию о мастере, выполнившем ремонт
Mechanic* repair_get_mechanic(sqlite3* db, int repair_id) {
    if (!db || repair_id <= 0) return NULL;
    
    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT m.id, m.name, m.specialization, m.workshop_id "
        "FROM mechanics m "
        "JOIN repairs r ON m.id = r.mechanic_id "
        "WHERE r.id = %d;", repair_id);
    
    sqlite3_stmt* stmt;
    Mechanic* mechanic = NULL;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            mechanic = (Mechanic*)malloc(sizeof(Mechanic));
            mechanic->id = sqlite3_column_int(stmt, 0);
            strcpy(mechanic->name, (const char*)sqlite3_column_text(stmt, 1));
            strcpy(mechanic->specialization, (const char*)sqlite3_column_text(stmt, 2));
            mechanic->workshop_id = sqlite3_column_int(stmt, 3);
        }
        sqlite3_finalize(stmt);
    }
    
    return mechanic;
}

// Получить информацию об автомобиле
Car* repair_get_car(sqlite3* db, int repair_id) {
    if (!db || repair_id <= 0) return NULL;
    
    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT c.license_plate, c.brand, c.model, c.year, c.owner_name, "
        "c.passport_number, c.owner_address "
        "FROM cars c "
        "JOIN repairs r ON c.license_plate = r.car_license "
        "WHERE r.id = %d;", repair_id);
    
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

// Получить ремонты по виду работ
Repair* repair_get_by_type(sqlite3* db, int repair_type_id, int* count) {
    if (!db || repair_type_id <= 0 || !count) return NULL;
    
    char count_sql[256];
    snprintf(count_sql, sizeof(count_sql),
        "SELECT COUNT(*) FROM repairs WHERE repair_type_id = %d;", repair_type_id);
    
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
    
    Repair* repairs = (Repair*)malloc(sizeof(Repair) * total);
    if (!repairs) return NULL;
    
    char sql[1024];
    snprintf(sql, sizeof(sql),
        "SELECT id, workshop_id, mechanic_id, car_license, repair_type_id, "
        "start_date, end_date, cost, status FROM repairs "
        "WHERE repair_type_id = %d ORDER BY start_date;", repair_type_id);
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        free(repairs);
        return NULL;
    }
    
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

// Получить информацию о мастерской по ремонту
Workshop* repair_get_workshop(sqlite3* db, int repair_id) {
    if (!db || repair_id <= 0) return NULL;
    
    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT w.id, w.address, w.phone, w.car_brands "
        "FROM workshops w "
        "JOIN repairs r ON w.id = r.workshop_id "
        "WHERE r.id = %d;", repair_id);
    
    sqlite3_stmt* stmt;
    Workshop* workshop = NULL;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            workshop = (Workshop*)malloc(sizeof(Workshop));
            workshop->id = sqlite3_column_int(stmt, 0);
            strcpy(workshop->address, (const char*)sqlite3_column_text(stmt, 1));
            strcpy(workshop->phone, (const char*)sqlite3_column_text(stmt, 2));
            strcpy(workshop->car_brands, (const char*)sqlite3_column_text(stmt, 3));
        }
        sqlite3_finalize(stmt);
    }
    
    return workshop;
}

// Получить информацию о виде работ по ремонту
RepairType* repair_get_type(sqlite3* db, int repair_id) {
    if (!db || repair_id <= 0) return NULL;
    
    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT rt.id, rt.name, rt.category, rt.base_price "
        "FROM repair_types rt "
        "JOIN repairs r ON rt.id = r.repair_type_id "
        "WHERE r.id = %d;", repair_id);
    
    sqlite3_stmt* stmt;
    RepairType* repair_type = NULL;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            repair_type = (RepairType*)malloc(sizeof(RepairType));
            repair_type->id = sqlite3_column_int(stmt, 0);
            strcpy(repair_type->name, (const char*)sqlite3_column_text(stmt, 1));
            strcpy(repair_type->category, (const char*)sqlite3_column_text(stmt, 2));
            repair_type->base_price = (float)sqlite3_column_double(stmt, 3);
        }
        sqlite3_finalize(stmt);
    }
    
    return repair_type;
}