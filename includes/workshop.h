#ifndef WORKSHOP_H
#define WORKSHOP_H

#include "database.h"
#include <stdio.h>

// Получить все мастерские
Workshop* workshop_get_all(sqlite3* db, int* count);

// Получить мастерскую по ID
Workshop* workshop_get_by_id(sqlite3* db, int id);

// Добавить новую мастерскую
int workshop_insert(sqlite3* db, Workshop* workshop);

// Обновить информацию о мастерской
int workshop_update(sqlite3* db, Workshop* workshop);

// Удалить мастерскую
int workshop_delete(sqlite3* db, int id);

// Получить список мастеров мастерской
Mechanic* workshop_get_mechanics(sqlite3* db, int workshop_id, int* count);

// Получить ремонты мастерской за период
Repair* workshop_get_repairs_by_period(sqlite3* db, int workshop_id, 
                                        const char* start_date, const char* end_date, 
                                        int* count);

// Получить общую выручку мастерской за период
float workshop_get_revenue(sqlite3* db, int workshop_id, 
                            const char* start_date, const char* end_date);

// Получить мастерскую с наибольшим количеством ремонтов
Workshop* workshop_get_most_active(sqlite3* db);

#endif