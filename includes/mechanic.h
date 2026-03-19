#ifndef MECHANIC_H
#define MECHANIC_H

#include "database.h"
#include <stdio.h>

// Получить всех мастеров
Mechanic* mechanic_get_all(sqlite3* db, int* count);

// Получить мастера по ID
Mechanic* mechanic_get_by_id(sqlite3* db, int id);

// Добавить нового мастера
int mechanic_insert(sqlite3* db, Mechanic* mechanic);

// Обновить информацию о мастере
int mechanic_update(sqlite3* db, Mechanic* mechanic);

// Удалить мастера
int mechanic_delete(sqlite3* db, int id);

// Получить ремонты мастера
Repair* mechanic_get_repairs(sqlite3* db, int mechanic_id, int* count);

// Получить ремонты мастера за период
Repair* mechanic_get_repairs_by_period(sqlite3* db, int mechanic_id, 
                                        const char* start_date, const char* end_date, 
                                        int* count);

// Получить ремонты мастера по видам работ
Repair* mechanic_get_repairs_by_type(sqlite3* db, int mechanic_id, int repair_type_id, int* count);

// Получить статистику по мастеру (количество ремонтов по видам)
int mechanic_get_stats_by_type(sqlite3* db, int mechanic_id, int* type_counts, int type_count);

// Получить мастерскую мастера
Workshop* mechanic_get_workshop(sqlite3* db, int mechanic_id);

#endif