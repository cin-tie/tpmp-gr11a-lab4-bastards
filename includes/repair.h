#ifndef REPAIR_H
#define REPAIR_H

#include "database.h"
#include <stdio.h>

// Получить все ремонты
Repair* repair_get_all(sqlite3* db, int* count);

// Получить ремонт по ID
Repair* repair_get_by_id(sqlite3* db, int id);

// Добавить новый ремонт
int repair_insert(sqlite3* db, Repair* repair);

// Обновить информацию о ремонте
int repair_update(sqlite3* db, Repair* repair);

// Удалить ремонт
int repair_delete(sqlite3* db, int id);

// Завершить ремонт (установить end_date и status = 'completed')
int repair_complete(sqlite3* db, int id, const char* end_date);

// Получить ремонты по статусу
Repair* repair_get_by_status(sqlite3* db, const char* status, int* count);

// Получить ремонты по виду работ
Repair* repair_get_by_type(sqlite3* db, int repair_type_id, int* count);

// Получить статистику по ремонтам за период
int repair_get_stats_by_period(sqlite3* db, int workshop_id, 
                                const char* start_date, const char* end_date,
                                int* type_counts, int type_count, float* total_revenue);

// Получить информацию о мастере, выполнившем ремонт
Mechanic* repair_get_mechanic(sqlite3* db, int repair_id);

// Получить информацию об автомобиле
Car* repair_get_car(sqlite3* db, int repair_id);

// Получить информацию о мастерской
Workshop* repair_get_workshop(sqlite3* db, int repair_id);

// Получить информацию о виде работ
RepairType* repair_get_type(sqlite3* db, int repair_id);

#endif