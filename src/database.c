#include "database.h"

sqlite3* db_init(const char* db_name){
    sqlite3* db;
    int rc = sqlite3_open(db_name, &db);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return NULL;
    }
    
    return db;
}

void db_close(sqlite3* db){
    if(db){
        sqlite3_close(db);
    }
}

int db_execute(sqlite3* db, const char* sql) {
    char* err_msg = 0;
    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return rc;
    }
    
    return SQLITE_OK;
}

int db_query(sqlite3* db, const char* sql, int (*callback)(void*,int,char**,char**), void* data) {
    char* err_msg = 0;
    int rc = sqlite3_exec(db, sql, callback, data, &err_msg);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return rc;
    }
    
    return SQLITE_OK;
}

int db_last_insert_rowid(sqlite3* db) {
    return (int)sqlite3_last_insert_rowid(db);
}

char* db_escape_string(const char* str) {
    if (!str) return NULL;
    
    int len = strlen(str);
    char* escaped = malloc(len * 2 + 1);
    int j = 0;
    
    for (int i = 0; i < len; i++) {
        if (str[i] == '\'') {
            escaped[j++] = '\'';
            escaped[j++] = '\'';
        } else {
            escaped[j++] = str[i];
        }
    }
    escaped[j] = '\0';
    
    return escaped;
}

// Создание таблицы для статистики мастеров (для триггера)
int db_create_stats_tables(sqlite3* db) {
    const char* sql = 
        "CREATE TABLE IF NOT EXISTS mechanic_stats ("
        "mechanic_id INTEGER PRIMARY KEY,"
        "total_repairs INTEGER DEFAULT 0,"
        "total_revenue REAL DEFAULT 0.0,"
        "last_repair_date TEXT,"
        "FOREIGN KEY (mechanic_id) REFERENCES mechanics(id)"
        ");"
        
        "CREATE TABLE IF NOT EXISTS workshop_stats ("
        "workshop_id INTEGER PRIMARY KEY,"
        "total_repairs INTEGER DEFAULT 0,"
        "total_revenue REAL DEFAULT 0.0,"
        "last_repair_date TEXT,"
        "FOREIGN KEY (workshop_id) REFERENCES workshops(id)"
        ");";
    
    return db_execute(db, sql);
}

// Создание триггера для обновления статистики мастеров
int db_create_triggers(sqlite3* db) {
    const char* sql = 
        "CREATE TRIGGER IF NOT EXISTS update_mechanic_stats_insert "
        "AFTER INSERT ON repairs "
        "BEGIN "
        "    INSERT OR REPLACE INTO mechanic_stats (mechanic_id, total_repairs, total_revenue, last_repair_date) "
        "    SELECT mechanic_id, "
        "           COALESCE((SELECT total_repairs FROM mechanic_stats WHERE mechanic_id = NEW.mechanic_id), 0) + 1, "
        "           COALESCE((SELECT total_revenue FROM mechanic_stats WHERE mechanic_id = NEW.mechanic_id), 0) + NEW.cost, "
        "           NEW.start_date "
        "    WHERE NEW.mechanic_id IS NOT NULL; "
        "END;"
        
        "CREATE TRIGGER IF NOT EXISTS update_mechanic_stats_update "
        "AFTER UPDATE ON repairs "
        "BEGIN "
        "    UPDATE mechanic_stats SET "
        "        total_repairs = total_repairs + 1, "
        "        total_revenue = total_revenue + NEW.cost - OLD.cost, "
        "        last_repair_date = NEW.start_date "
        "    WHERE mechanic_id = NEW.mechanic_id; "
        "END;";
    
    return db_execute(db, sql);
}

// Функция для получения статистики по мастерской за период
int db_get_workshop_stats_by_period(sqlite3* db, int workshop_id, 
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
        "SELECT rt.id, COUNT(r.id) as cnt, COALESCE(SUM(r.cost), 0) as revenue "
        "FROM repair_types rt "
        "LEFT JOIN repairs r ON rt.id = r.repair_type_id "
        "    AND r.workshop_id = %d "
        "    AND r.start_date >= '%s' "
        "    AND r.start_date <= '%s' "
        "GROUP BY rt.id "
        "ORDER BY rt.id;",
        workshop_id, start_date, end_date);
    
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        int i = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW && i < type_count) {
            int type_id = sqlite3_column_int(stmt, 0);
            int cnt = sqlite3_column_int(stmt, 1);
            float revenue = (float)sqlite3_column_double(stmt, 2);
            
            if (type_id > 0 && type_id <= type_count) {
                type_counts[type_id - 1] = cnt;
            }
            *total_revenue += revenue;
            i++;
        }
        sqlite3_finalize(stmt);
    }
    
    return 0;
}

// Функция для получения статистики на указанную дату
int db_get_daily_stats(sqlite3* db, int workshop_id, const char* date, int* total_repairs) {
    if (!db || workshop_id <= 0 || !date || !total_repairs) return -1;
    
    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT COUNT(*) FROM repairs "
        "WHERE workshop_id = %d AND start_date = '%s';",
        workshop_id, date);
    
    sqlite3_stmt* stmt;
    *total_repairs = 0;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            *total_repairs = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    
    return 0;
}