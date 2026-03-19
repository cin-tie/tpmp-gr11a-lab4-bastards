#include "workshop.h"
#include <stdlib.h>
#include <string.h>

// Callback для получения всех мастерских
static int callback_get_all(void* data, int argc, char** argv, char** col_name) {
    Workshop** result = (Workshop**)data;
    Workshop* w = &(*result)[0];
    
    w->id = atoi(argv[0]);
    strcpy(w->address, argv[1] ? argv[1] : "");
    strcpy(w->phone, argv[2] ? argv[2] : "");
    strcpy(w->car_brands, argv[3] ? argv[3] : "");
    
    (*result)++;
    return 0;
}

// Получить все мастерские
Workshop* workshop_get_all(sqlite3* db, int* count) {
    if (!db || !count) return NULL;
    
    const char* count_sql = "SELECT COUNT(*) FROM workshops;";
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
    
    Workshop* workshops = (Workshop*)malloc(sizeof(Workshop) * total);
    if (!workshops) return NULL;
    
    Workshop* ptr = workshops;
    const char* sql = "SELECT id, address, phone, car_brands FROM workshops ORDER BY id;";
    int rc = db_query(db, sql, callback_get_all, &ptr);
    
    if (rc != SQLITE_OK) {
        free(workshops);
        return NULL;
    }
    
    *count = total;
    return workshops;
}

// Получить мастерскую по ID
Workshop* workshop_get_by_id(sqlite3* db, int id) {
    if (!db || id <= 0) return NULL;
    
    char sql[256];
    snprintf(sql, sizeof(sql),
        "SELECT id, address, phone, car_brands FROM workshops WHERE id = %d;", id);
    
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

// Добавить мастерскую
int workshop_insert(sqlite3* db, Workshop* workshop) {
    if (!db || !workshop) return SQLITE_ERROR;
    
    char* safe_address = db_escape_string(workshop->address);
    char* safe_phone = db_escape_string(workshop->phone);
    char* safe_brands = db_escape_string(workshop->car_brands);
    
    char sql[1024];
    snprintf(sql, sizeof(sql),
        "INSERT INTO workshops (address, phone, car_brands) "
        "VALUES ('%s', '%s', '%s');",
        safe_address, safe_phone, safe_brands);
    
    free(safe_address);
    free(safe_phone);
    free(safe_brands);
    
    int rc = db_execute(db, sql);
    if (rc == SQLITE_OK) {
        workshop->id = db_last_insert_rowid(db);
    }
    
    return rc;
}

// Обновить мастерскую
int workshop_update(sqlite3* db, Workshop* workshop) {
    if (!db || !workshop || workshop->id <= 0) return SQLITE_ERROR;
    
    char* safe_address = db_escape_string(workshop->address);
    char* safe_phone = db_escape_string(workshop->phone);
    char* safe_brands = db_escape_string(workshop->car_brands);
    
    char sql[1024];
    snprintf(sql, sizeof(sql),
        "UPDATE workshops SET address = '%s', phone = '%s', car_brands = '%s' "
        "WHERE id = %d;",
        safe_address, safe_phone, safe_brands, workshop->id);
    
    free(safe_address);
    free(safe_phone);
    free(safe_brands);
    
    return db_execute(db, sql);
}

// Удалить мастерскую
int workshop_delete(sqlite3* db, int id) {
    if (!db || id <= 0) return SQLITE_ERROR;
    
    // Проверяем, есть ли связанные записи
    char check_sql[256];
    snprintf(check_sql, sizeof(check_sql),
        "SELECT COUNT(*) FROM mechanics WHERE workshop_id = %d;", id);
    
    sqlite3_stmt* stmt;
    int has_mechanics = 0;
    
    if (sqlite3_prepare_v2(db, check_sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            has_mechanics = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    
    if (has_mechanics > 0) {
        return SQLITE_CONSTRAINT; // Нельзя удалить, есть мастера
    }
    
    char sql[256];
    snprintf(sql, sizeof(sql), "DELETE FROM workshops WHERE id = %d;", id);
    return db_execute(db, sql);
}

// Получить мастеров мастерской
Mechanic* workshop_get_mechanics(sqlite3* db, int workshop_id, int* count) {
    if (!db || !count || workshop_id <= 0) return NULL;
    
    char count_sql[256];
    snprintf(count_sql, sizeof(count_sql),
        "SELECT COUNT(*) FROM mechanics WHERE workshop_id = %d;", workshop_id);
    
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
    
    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT id, name, specialization, workshop_id FROM mechanics "
        "WHERE workshop_id = %d ORDER BY id;", workshop_id);
    
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

// Получить ремонты мастерской за период
Repair* workshop_get_repairs_by_period(sqlite3* db, int workshop_id, 
                                       const char* start_date, const char* end_date, 
                                       int* count) {
    if (!db || !count || workshop_id <= 0 || !start_date || !end_date) return NULL;
    
    char count_sql[512];
    snprintf(count_sql, sizeof(count_sql),
        "SELECT COUNT(*) FROM repairs "
        "WHERE workshop_id = %d AND start_date >= '%s' AND start_date <= '%s';",
        workshop_id, start_date, end_date);
    
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
        "WHERE workshop_id = %d AND start_date >= '%s' AND start_date <= '%s' "
        "ORDER BY start_date;",
        workshop_id, start_date, end_date);
    
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

// Получить общую выручку мастерской за период
float workshop_get_revenue(sqlite3* db, int workshop_id, 
                           const char* start_date, const char* end_date) {
    if (!db || workshop_id <= 0 || !start_date || !end_date) return 0.0;
    
    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT COALESCE(SUM(cost), 0) FROM repairs "
        "WHERE workshop_id = %d AND start_date >= '%s' AND start_date <= '%s' "
        "AND status = 'completed';",
        workshop_id, start_date, end_date);
    
    sqlite3_stmt* stmt;
    float revenue = 0.0;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            revenue = (float)sqlite3_column_double(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    
    return revenue;
}

// Получить мастерскую с наибольшим количеством ремонтов
Workshop* workshop_get_most_active(sqlite3* db) {
    if (!db) return NULL;
    
    const char* sql = 
        "SELECT w.id, w.address, w.phone, w.car_brands, COUNT(r.id) as repair_count "
        "FROM workshops w "
        "LEFT JOIN repairs r ON w.id = r.workshop_id "
        "GROUP BY w.id "
        "ORDER BY repair_count DESC "
        "LIMIT 1;";
    
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