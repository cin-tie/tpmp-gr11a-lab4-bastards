#ifndef CAR_H
#define CAR_H

#include "database.h"
#include <stdio.h>

// Получить все автомобили
Car* car_get_all(sqlite3* db, int* count);

// Получить автомобиль по госномеру
Car* car_get_by_license(sqlite3* db, const char* license_plate);

// Получить автомобили по марке
Car* car_get_by_brand(sqlite3* db, const char* brand, int* count);

// Получить автомобили по владельцу
Car* car_get_by_owner(sqlite3* db, const char* owner_name, int* count);

// Добавить новый автомобиль
int car_insert(sqlite3* db, Car* car);

// Обновить информацию об автомобиле
int car_update(sqlite3* db, Car* car);

// Удалить автомобиль
int car_delete(sqlite3* db, const char* license_plate);

// Получить историю ремонтов автомобиля
Repair* car_get_repair_history(sqlite3* db, const char* license_plate, int* count);

// Проверить существование автомобиля
int car_exists(sqlite3* db, const char* license_plate);

#endif