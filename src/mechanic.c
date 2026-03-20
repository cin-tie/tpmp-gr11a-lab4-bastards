#include "mechanic.h"
#include <stdlib.h>
#include <string.h>

// Получить всех мастеров
Mechanic* mechanic_get_all(sqlite3* db, int* count) {
    if (!db || !count) return NULL;
    
    const char* count_sql = "SELECT COUNT(*) FROM mechanics;";
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
    
    Mechanic* mechanics = (Mechanic*)malloc(sizeof(Mechanic) * total);
    if (!mechanics) return NULL;
    
    const char* sql = "SELECT id, name, specialization, workshop_id FROM mechanics ORDER BY id;";
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < total) {
        mechanics[i].id = sqlite3_column_int(stmt, 0);
        strcpy(mechanics[i].name, (const char*)sqlite3_column_text(stmt, 1));
        strcpy(mechanics[i].specialization, (const char*)sqlite3_column_text(stmt, 2));
        mechanics[i].workshop_id = sqlite3_column_int(stmt, 3);
        i++;
    }
    sqlite3_finalize(stmt);
    
    *count = total;
    return mechanics;
}

// Получить мастера по ID
Mechanic* mechanic_get_by_id(sqlite3* db, int id) {
    if (!db || id <= 0) return NULL;
    
    char sql[256];
    snprintf(sql, sizeof(sql),
        "SELECT id, name, specialization, workshop_id FROM mechanics WHERE id = %d;", id);
    
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

// Добавить мастера
int mechanic_insert(sqlite3* db, Mechanic* mechanic) {
    if (!db || !mechanic) return SQLITE_ERROR;
    
    char* safe_name = db_escape_string(mechanic->name);
    char* safe_spec = db_escape_string(mechanic->specialization);
    
    char sql[1024];
    snprintf(sql, sizeof(sql),
        "INSERT INTO mechanics (name, specialization, workshop_id) "
        "VALUES ('%s', '%s', %d);",
        safe_name, safe_spec, mechanic->workshop_id);
    
    free(safe_name);
    free(safe_spec);
    
    int rc = db_execute(db, sql);
    if (rc == SQLITE_OK) {
        mechanic->id = db_last_insert_rowid(db);
    }
    
    return rc;
}

// Обновить мастера
int mechanic_update(sqlite3* db, Mechanic* mechanic) {
    if (!db || !mechanic || mechanic->id <= 0) return SQLITE_ERROR;
    
    char* safe_name = db_escape_string(mechanic->name);
    char* safe_spec = db_escape_string(mechanic->specialization);
    
    char sql[1024];
    snprintf(sql, sizeof(sql),
        "UPDATE mechanics SET name = '%s', specialization = '%s', workshop_id = %d "
        "WHERE id = %d;",
        safe_name, safe_spec, mechanic->workshop_id, mechanic->id);
    
    free(safe_name);
    free(safe_spec);
    
    return db_execute(db, sql);
}

// Удалить мастера
int mechanic_delete(sqlite3* db, int id) {
    if (!db || id <= 0) return SQLITE_ERROR;
    
    // Проверяем, есть ли связанные ремонты
    char check_sql[256];
    snprintf(check_sql, sizeof(check_sql),
        "SELECT COUNT(*) FROM repairs WHERE mechanic_id = %d;", id);
    
    sqlite3_stmt* stmt;
    int has_repairs = 0;
    
    if (sqlite3_prepare_v2(db, check_sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            has_repairs = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    
    if (has_repairs > 0) {
        return SQLITE_CONSTRAINT; // Нельзя удалить, есть ремонты
    }
    
    char sql[256];
    snprintf(sql, sizeof(sql), "DELETE FROM mechanics WHERE id = %d;", id);
    return db_execute(db, sql);
}

// Получить ремонты мастера
Repair* mechanic_get_repairs(sqlite3* db, int mechanic_id, int* count) {
    if (!db || !count || mechanic_id <= 0) {
        if (count) *count = 0;
        return NULL;
    }
    
    char count_sql[256];
    snprintf(count_sql, sizeof(count_sql),
        "SELECT COUNT(*) FROM repairs WHERE mechanic_id = %d;", mechanic_id);
    
    sqlite3_stmt* stmt;
    int total = 0;
    
    // Получаем количество записей
    if (sqlite3_prepare_v2(db, count_sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            total = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    
    *count = total;
    
    if (total == 0) {
        return NULL;  // Нет ремонтов, возвращаем NULL
    }
    
    // Выделяем память под результат
    Repair* repairs = (Repair*)malloc(sizeof(Repair) * total);
    if (!repairs) {
        return NULL;
    }
    
    // ОБНУЛЯЕМ ПАМЯТЬ - ЭТО ВАЖНО!
    memset(repairs, 0, sizeof(Repair) * total);
    
    char sql[1024];
    snprintf(sql, sizeof(sql),
        "SELECT id, workshop_id, mechanic_id, car_license, repair_type_id, "
        "start_date, end_date, cost, status FROM repairs "
        "WHERE mechanic_id = %d ORDER BY start_date DESC;", mechanic_id);
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        free(repairs);
        return NULL;
    }
    
    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < total) {
        repairs[i].id = sqlite3_column_int(stmt, 0);
        repairs[i].workshop_id = sqlite3_column_int(stmt, 1);
        repairs[i].mechanic_id = sqlite3_column_int(stmt, 2);
        
        const char* license = (const char*)sqlite3_column_text(stmt, 3);
        if (license) {
            strncpy(repairs[i].car_license, license, sizeof(repairs[i].car_license) - 1);
            repairs[i].car_license[sizeof(repairs[i].car_license) - 1] = '\0';
        }
        
        repairs[i].repair_type_id = sqlite3_column_int(stmt, 4);
        
        const char* start = (const char*)sqlite3_column_text(stmt, 5);
        if (start) {
            strncpy(repairs[i].start_date, start, sizeof(repairs[i].start_date) - 1);
            repairs[i].start_date[sizeof(repairs[i].start_date) - 1] = '\0';
        }
        
        const char* end = (const char*)sqlite3_column_text(stmt, 6);
        if (end) {
            strncpy(repairs[i].end_date, end, sizeof(repairs[i].end_date) - 1);
            repairs[i].end_date[sizeof(repairs[i].end_date) - 1] = '\0';
        } else {
            repairs[i].end_date[0] = '\0';
        }
        
        repairs[i].cost = (float)sqlite3_column_double(stmt, 7);
        
        const char* status = (const char*)sqlite3_column_text(stmt, 8);
        if (status) {
            strncpy(repairs[i].status, status, sizeof(repairs[i].status) - 1);
            repairs[i].status[sizeof(repairs[i].status) - 1] = '\0';
        }
        
        i++;
    }
    sqlite3_finalize(stmt);
    
    return repairs;
}

// Получить ремонты мастера за период
Repair* mechanic_get_repairs_by_period(sqlite3* db, int mechanic_id, 
                                        const char* start_date, const char* end_date, 
                                        int* count) {
    if (!db || !count || mechanic_id <= 0 || !start_date || !end_date) return NULL;
    
    char count_sql[512];
    snprintf(count_sql, sizeof(count_sql),
        "SELECT COUNT(*) FROM repairs "
        "WHERE mechanic_id = %d AND start_date >= '%s' AND start_date <= '%s';",
        mechanic_id, start_date, end_date);
    
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
        "WHERE mechanic_id = %d AND start_date >= '%s' AND start_date <= '%s' "
        "ORDER BY start_date;",
        mechanic_id, start_date, end_date);
    
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

// Получить статистику по мастеру (количество ремонтов по видам)
int mechanic_get_stats_by_type(sqlite3* db, int mechanic_id, int* type_counts, int type_count) {
    if (!db || mechanic_id <= 0 || !type_counts || type_count <= 0) return -1;
    
    // Инициализируем массив нулями
    for (int i = 0; i < type_count; i++) {
        type_counts[i] = 0;
    }
    
    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT repair_type_id, COUNT(*) as cnt FROM repairs "
        "WHERE mechanic_id = %d GROUP BY repair_type_id;", mechanic_id);
    
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int type_id = sqlite3_column_int(stmt, 0);
            int cnt = sqlite3_column_int(stmt, 1);
            if (type_id > 0 && type_id <= type_count) {
                type_counts[type_id - 1] = cnt;
            }
        }
        sqlite3_finalize(stmt);
    }
    
    return 0;
}

// Получить мастерскую мастера
Workshop* mechanic_get_workshop(sqlite3* db, int mechanic_id) {
    if (!db || mechanic_id <= 0) return NULL;
    
    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT w.id, w.address, w.phone, w.car_brands "
        "FROM workshops w "
        "JOIN mechanics m ON w.id = m.workshop_id "
        "WHERE m.id = %d;", mechanic_id);
    
    sqlite3_stmt* stmt;
    Workshop* workshop = NULL;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            workshop = (Workshop*)malloc(sizeof(Workshop));
            if (workshop) {
                memset(workshop, 0, sizeof(Workshop));
                workshop->id = sqlite3_column_int(stmt, 0);
                
                const char* addr = (const char*)sqlite3_column_text(stmt, 1);
                if (addr) strncpy(workshop->address, addr, sizeof(workshop->address) - 1);
                
                const char* ph = (const char*)sqlite3_column_text(stmt, 2);
                if (ph) strncpy(workshop->phone, ph, sizeof(workshop->phone) - 1);
                
                const char* brands = (const char*)sqlite3_column_text(stmt, 3);
                if (brands) strncpy(workshop->car_brands, brands, sizeof(workshop->car_brands) - 1);
            }
        }
        sqlite3_finalize(stmt);
    }
    
    return workshop;
}