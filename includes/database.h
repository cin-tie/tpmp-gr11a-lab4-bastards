#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Структуры данных
typedef struct {
    int id;
    char address[256];
    char phone[64];
    char car_brands[512];
} Workshop;

typedef struct {
    int id;
    char name[256];
    char specialization[128];
    int workshop_id;
} Mechanic;

typedef struct {
    char license_plate[16];
    char brand[64];
    char model[64];
    int year;
    char owner_name[256];
    char passport_number[64];
    char owner_address[256];
} Car;

typedef struct {
    int id;
    char name[256];
    char category[128];
    float base_price;
} RepairType;

typedef struct {
    int id;
    int workshop_id;
    int mechanic_id;
    char car_license[16];
    int repair_type_id;
    char start_date[16];
    char end_date[16];
    float cost;
    char status[32];
} Repair;

// Инициализация базы данных
sqlite3* db_init(const char* db_name);

// Закрытие базы данных
void db_close(sqlite3* db);

// Выполнение SQL запроса без возврата данных
int db_execute(sqlite3* db, const char* sql);

// Выполнение запроса с возвратом данных (callback)
int db_query(sqlite3* db, const char* sql, int (*callback)(void*,int,char**,char**), void* data);

// Получение последнего ID вставленной записи
int db_last_insert_rowid(sqlite3* db);

// Экранирование строки для SQL (защита от SQL injection)
char* db_escape_string(const char* str);

// Создание таблиц для статистики мастеров (для триггера)
int db_create_stats_tables(sqlite3* db);

// Создание триггера для обновления статистики мастеров
int db_create_triggers(sqlite3* db);

// Функция для получения статистики по мастерской за период
int db_get_workshop_stats_by_period(sqlite3* db, int workshop_id, 
                                    const char* start_date, const char* end_date,
                                    int* type_counts, int type_count, float* total_revenue);

// Функция для получения статистики на указанную дату
int db_get_daily_stats(sqlite3* db, int workshop_id, const char* date, int* total_repairs);

#endif