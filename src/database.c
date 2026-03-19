#include "../includes/database.h"

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