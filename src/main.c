#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Подключаем все необходимые заголовочные файлы
#include "database.h"
#include "auth.h"
#include "workshop.h"
#include "mechanic.h"
#include "car.h"
#include "repair.h"

// Глобальные переменные (объявлены здесь, определяются в main)
extern sqlite3* db;
extern User* current_user;

// Прототипы функций меню
void show_main_menu();
void show_admin_menu();
void show_mechanic_menu();
void handle_login();
void handle_logout();

// Функции для администратора
void admin_workshops_menu();
void admin_mechanics_menu();
void admin_reports_menu();

// Функции для мастера
void mechanic_my_repairs();
void mechanic_add_repair();
void mechanic_update_status();
void mechanic_find_car();

// Объявление глобальных переменных
sqlite3* db = NULL;
User* current_user = NULL;

int main(int argc, char* argv[]) {
    printf("====================================\n");
    printf("  АВТОМАСТЕРСКИЕ - Система управления\n");
    printf("====================================\n\n");
    
    // Инициализация БД
    db = db_init("garage.db");
    if (!db) {
        printf("Ошибка инициализации базы данных!\n");
        return 1;
    }
    
    // Инициализация системы аутентификации
    auth_init(db);
    
    // Создание таблиц для статистики и триггеров
    db_create_stats_tables(db);
    db_create_triggers(db);
    
    // Главный цикл программы
    while (1) {
        if (!current_user) {
            show_main_menu();
        } else if (current_user->role == ROLE_ADMIN) {
            show_admin_menu();
        } else if (current_user->role == ROLE_MECHANIC) {
            show_mechanic_menu();
        }
    }
    
    db_close(db);
    return 0;
}

void show_main_menu() {
    int choice;
    
    printf("\n--- ГЛАВНОЕ МЕНЮ ---\n");
    printf("1. Вход в систему\n");
    printf("2. Выход\n");
    printf("Выберите действие: ");
    
    scanf("%d", &choice);
    getchar(); // очистка буфера
    
    switch(choice) {
        case 1:
            handle_login();
            break;
        case 2:
            printf("До свидания!\n");
            exit(0);
        default:
            printf("Неверный выбор!\n");
    }
}

void handle_login() {
    char username[64];
    char password[64];
    
    printf("\n--- ВХОД В СИСТЕМУ ---\n");
    printf("Логин: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0;
    
    printf("Пароль: ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = 0;
    
    current_user = auth_login(db, username, password);
    
    if (current_user) {
        printf("Добро пожаловать, %s!\n", current_user->username);
        printf("Ваша роль: %s\n", 
               current_user->role == ROLE_ADMIN ? "Администратор" : "Мастер");
    } else {
        printf("Неверный логин или пароль!\n");
    }
}

void show_admin_menu() {
    int choice;
    
    printf("\n--- МЕНЮ АДМИНИСТРАТОРА ---\n");
    printf("1. Управление мастерскими\n");
    printf("2. Управление мастерами\n");
    printf("3. Управление автомобилями\n");
    printf("4. Отчеты\n");
    printf("5. Выход из системы\n");
    printf("Выберите действие: ");
    
    scanf("%d", &choice);
    getchar();
    
    switch(choice) {
        case 1:
            admin_workshops_menu();
            break;
        case 2:
            admin_mechanics_menu();
            break;
        case 3:
            // admin_cars_menu();
            printf("Функция в разработке\n");
            break;
        case 4:
            admin_reports_menu();
            break;
        case 5:
            handle_logout();
            break;
        default:
            printf("Неверный выбор!\n");
    }
}

void admin_workshops_menu() {
    int choice;
    
    printf("\n--- УПРАВЛЕНИЕ МАСТЕРСКИМИ ---\n");
    printf("1. Показать все мастерские\n");
    printf("2. Добавить мастерскую\n");
    printf("3. Редактировать мастерскую\n");
    printf("4. Удалить мастерскую\n");
    printf("5. Показать мастеров мастерской\n");
    printf("6. Назад\n");
    printf("Выберите действие: ");
    
    scanf("%d", &choice);
    getchar();
    
    switch(choice) {
        case 1: {
            int count;
            Workshop* workshops = workshop_get_all(db, &count);
            if (workshops) {
                printf("\nСписок мастерских:\n");
                printf("--------------------------------------------------------\n");
                for (int i = 0; i < count; i++) {
                    printf("ID: %d\n", workshops[i].id);
                    printf("Адрес: %s\n", workshops[i].address);
                    printf("Телефон: %s\n", workshops[i].phone);
                    printf("Марки: %s\n", workshops[i].car_brands);
                    printf("--------------------------------------------------------\n");
                }
                free(workshops);
            } else {
                printf("Мастерские не найдены.\n");
            }
            break;
        }
        case 2: {
            Workshop w;
            memset(&w, 0, sizeof(Workshop));
            
            printf("Введите адрес: ");
            fgets(w.address, sizeof(w.address), stdin);
            w.address[strcspn(w.address, "\n")] = 0;
            
            printf("Введите телефон: ");
            fgets(w.phone, sizeof(w.phone), stdin);
            w.phone[strcspn(w.phone, "\n")] = 0;
            
            printf("Введите марки (через запятую): ");
            fgets(w.car_brands, sizeof(w.car_brands), stdin);
            w.car_brands[strcspn(w.car_brands, "\n")] = 0;
            
            int rc = workshop_insert(db, &w);
            if (rc == SQLITE_OK) {
                printf("Мастерская добавлена с ID: %d\n", w.id);
            } else {
                printf("Ошибка при добавлении мастерской.\n");
            }
            break;
        }
        case 3: {
            int id;
            printf("Введите ID мастерской для редактирования: ");
            scanf("%d", &id);
            getchar();
            
            Workshop* w = workshop_get_by_id(db, id);
            if (w) {
                printf("Текущий адрес (%s): ", w->address);
                char new_address[256];
                fgets(new_address, sizeof(new_address), stdin);
                new_address[strcspn(new_address, "\n")] = 0;
                if (strlen(new_address) > 0) {
                    strcpy(w->address, new_address);
                }
                
                printf("Текущий телефон (%s): ", w->phone);
                char new_phone[64];
                fgets(new_phone, sizeof(new_phone), stdin);
                new_phone[strcspn(new_phone, "\n")] = 0;
                if (strlen(new_phone) > 0) {
                    strcpy(w->phone, new_phone);
                }
                
                printf("Текущие марки (%s): ", w->car_brands);
                char new_brands[512];
                fgets(new_brands, sizeof(new_brands), stdin);
                new_brands[strcspn(new_brands, "\n")] = 0;
                if (strlen(new_brands) > 0) {
                    strcpy(w->car_brands, new_brands);
                }
                
                int rc = workshop_update(db, w);
                if (rc == SQLITE_OK) {
                    printf("Мастерская обновлена.\n");
                } else {
                    printf("Ошибка при обновлении.\n");
                }
                free(w);
            } else {
                printf("Мастерская не найдена.\n");
            }
            break;
        }
        case 4: {
            int id;
            printf("Введите ID мастерской для удаления: ");
            scanf("%d", &id);
            
            int rc = workshop_delete(db, id);
            if (rc == SQLITE_OK) {
                printf("Мастерская удалена.\n");
            } else if (rc == SQLITE_CONSTRAINT) {
                printf("Невозможно удалить: в мастерской есть мастера.\n");
            } else {
                printf("Ошибка при удалении.\n");
            }
            break;
        }
        case 5: {
            int id;
            printf("Введите ID мастерской: ");
            scanf("%d", &id);
            
            int count;
            Mechanic* mechanics = workshop_get_mechanics(db, id, &count);
            if (mechanics) {
                printf("\nМастера мастерской:\n");
                for (int i = 0; i < count; i++) {
                    printf("ID: %d, Имя: %s, Специализация: %s\n", 
                            mechanics[i].id, mechanics[i].name, mechanics[i].specialization);
                }
                free(mechanics);
            } else {
                printf("Мастера не найдены.\n");
            }
            break;
        }
        case 6:
            return;
        default:
            printf("Неверный выбор!\n");
    }
}

void admin_mechanics_menu() {
    int choice;
    
    printf("\n--- УПРАВЛЕНИЕ МАСТЕРАМИ ---\n");
    printf("1. Показать всех мастеров\n");
    printf("2. Добавить мастера\n");
    printf("3. Редактировать мастера\n");
    printf("4. Удалить мастера\n");
    printf("5. Показать ремонты мастера\n");
    printf("6. Назад\n");
    printf("Выберите действие: ");
    
    scanf("%d", &choice);
    getchar();
    
    switch(choice) {
        case 1: {
            int count;
            Mechanic* mechanics = mechanic_get_all(db, &count);
            if (mechanics) {
                printf("\nСписок мастеров:\n");
                printf("--------------------------------------------------------\n");
                for (int i = 0; i < count; i++) {
                    printf("ID: %d, Имя: %s, Специализация: %s, Мастерская: %d\n",
                            mechanics[i].id, mechanics[i].name, 
                            mechanics[i].specialization, mechanics[i].workshop_id);
                }
                printf("--------------------------------------------------------\n");
                free(mechanics);
            } else {
                printf("Мастера не найдены.\n");
            }
            break;
        }
        case 2: {
            Mechanic m;
            memset(&m, 0, sizeof(Mechanic));
            
            printf("Введите имя мастера: ");
            fgets(m.name, sizeof(m.name), stdin);
            m.name[strcspn(m.name, "\n")] = 0;
            
            printf("Введите специализацию: ");
            fgets(m.specialization, sizeof(m.specialization), stdin);
            m.specialization[strcspn(m.specialization, "\n")] = 0;
            
            printf("Введите ID мастерской: ");
            scanf("%d", &m.workshop_id);
            getchar();
            
            int rc = mechanic_insert(db, &m);
            if (rc == SQLITE_OK) {
                printf("Мастер добавлен с ID: %d\n", m.id);
            } else {
                printf("Ошибка при добавлении мастера.\n");
            }
            break;
        }
        case 3: {
            int id;
            printf("Введите ID мастера для редактирования: ");
            scanf("%d", &id);
            getchar();
            
            Mechanic* m = mechanic_get_by_id(db, id);
            if (m) {
                printf("Текущее имя (%s): ", m->name);
                char new_name[256];
                fgets(new_name, sizeof(new_name), stdin);
                new_name[strcspn(new_name, "\n")] = 0;
                if (strlen(new_name) > 0) {
                    strcpy(m->name, new_name);
                }
                
                printf("Текущая специализация (%s): ", m->specialization);
                char new_spec[128];
                fgets(new_spec, sizeof(new_spec), stdin);
                new_spec[strcspn(new_spec, "\n")] = 0;
                if (strlen(new_spec) > 0) {
                    strcpy(m->specialization, new_spec);
                }
                
                printf("Текущая мастерская (%d): ", m->workshop_id);
                char new_ws[16];
                fgets(new_ws, sizeof(new_ws), stdin);
                if (strlen(new_ws) > 0) {
                    m->workshop_id = atoi(new_ws);
                }
                
                int rc = mechanic_update(db, m);
                if (rc == SQLITE_OK) {
                    printf("Мастер обновлен.\n");
                } else {
                    printf("Ошибка при обновлении.\n");
                }
                free(m);
            } else {
                printf("Мастер не найден.\n");
            }
            break;
        }
        case 4: {
            int id;
            printf("Введите ID мастера для удаления: ");
            scanf("%d", &id);
            
            int rc = mechanic_delete(db, id);
            if (rc == SQLITE_OK) {
                printf("Мастер удален.\n");
            } else if (rc == SQLITE_CONSTRAINT) {
                printf("Невозможно удалить: у мастера есть ремонты.\n");
            } else {
                printf("Ошибка при удалении.\n");
            }
            break;
        }
        case 5: {
            int id;
            printf("Введите ID мастера: ");
            scanf("%d", &id);
            
            int count;
            Repair* repairs = mechanic_get_repairs(db, id, &count);
            if (repairs) {
                printf("\nРемонты мастера:\n");
                printf("--------------------------------------------------------\n");
                for (int i = 0; i < count; i++) {
                    printf("ID: %d, Авто: %s, Дата: %s, Стоимость: %.2f, Статус: %s\n",
                            repairs[i].id, repairs[i].car_license, 
                            repairs[i].start_date, repairs[i].cost, repairs[i].status);
                }
                printf("--------------------------------------------------------\n");
                free(repairs);
            } else {
                printf("Ремонты не найдены.\n");
            }
            break;
        }
        case 6:
            return;
        default:
            printf("Неверный выбор!\n");
    }
}

void admin_reports_menu() {
    int choice;
    
    printf("\n--- ОТЧЕТЫ ---\n");
    printf("1. Отчет по мастерской за период\n");
    printf("2. Отчет по мастеру\n");
    printf("3. Статистика по маркам автомобилей\n");
    printf("4. Общая выручка по всем мастерским\n");
    printf("5. Мастерская с наибольшим количеством ремонтов\n");
    printf("6. Назад\n");
    printf("Выберите отчет: ");
    
    scanf("%d", &choice);
    getchar();
    
    switch(choice) {
        case 1: {
            int workshop_id;
            char start_date[16], end_date[16];
            
            printf("Введите номер мастерской: ");
            scanf("%d", &workshop_id);
            getchar();
            
            printf("Введите начальную дату (ГГГГ-ММ-ДД): ");
            fgets(start_date, sizeof(start_date), stdin);
            start_date[strcspn(start_date, "\n")] = 0;
            
            printf("Введите конечную дату (ГГГГ-ММ-ДД): ");
            fgets(end_date, sizeof(end_date), stdin);
            end_date[strcspn(end_date, "\n")] = 0;
            
            int count;
            Repair* repairs = workshop_get_repairs_by_period(db, workshop_id, 
                                                                start_date, end_date, 
                                                                &count);
            
            if (repairs && count > 0) {
                printf("\nРемонты за период %s - %s:\n", start_date, end_date);
                printf("--------------------------------------------------------\n");
                for (int i = 0; i < count; i++) {
                    printf("ID: %d, Авто: %s, Мастер: %d, Стоимость: %.2f, Статус: %s\n",
                            repairs[i].id, repairs[i].car_license, 
                            repairs[i].mechanic_id, repairs[i].cost, repairs[i].status);
                }
                printf("--------------------------------------------------------\n");
                free(repairs);
            } else {
                printf("Ремонтов за указанный период не найдено.\n");
            }
            break;
        }
        case 2: {
            int mechanic_id;
            printf("Введите ID мастера: ");
            scanf("%d", &mechanic_id);
            getchar();
            
            int count;
            Repair* repairs = mechanic_get_repairs(db, mechanic_id, &count);
            
            if (repairs && count > 0) {
                printf("\nРемонты мастера:\n");
                printf("--------------------------------------------------------\n");
                for (int i = 0; i < count; i++) {
                    printf("ID: %d, Авто: %s, Дата: %s, Стоимость: %.2f, Статус: %s\n",
                            repairs[i].id, repairs[i].car_license, 
                            repairs[i].start_date, repairs[i].cost, repairs[i].status);
                }
                printf("--------------------------------------------------------\n");
                free(repairs);
            } else {
                printf("Ремонтов не найдено.\n");
            }
            break;
        }
        case 3: {
            char brand[64];
            printf("Введите марку автомобиля: ");
            fgets(brand, sizeof(brand), stdin);
            brand[strcspn(brand, "\n")] = 0;
            
            int car_count;
            Car* cars = car_get_by_brand(db, brand, &car_count);
            
            if (cars && car_count > 0) {
                printf("\nАвтомобили марки %s:\n", brand);
                printf("--------------------------------------------------------\n");
                for (int i = 0; i < car_count; i++) {
                    printf("Госномер: %s, Модель: %s, Владелец: %s\n",
                            cars[i].license_plate, cars[i].model, cars[i].owner_name);
                }
                free(cars);
            } else {
                printf("Автомобили марки %s не найдены.\n", brand);
            }
            break;
        }
        case 4: {
            // Общая выручка по всем мастерским за текущий месяц
            char start_date[16] = "2026-03-01";
            char end_date[16] = "2026-03-31";
            
            int ws_count;
            Workshop* workshops = workshop_get_all(db, &ws_count);
            
            if (workshops) {
                float total_revenue = 0;
                printf("\nВыручка по мастерским за март 2026:\n");
                printf("--------------------------------------------------------\n");
                for (int i = 0; i < ws_count; i++) {
                    float revenue = workshop_get_revenue(db, workshops[i].id, start_date, end_date);
                    printf("Мастерская %d (%s): %.2f\n", 
                            workshops[i].id, workshops[i].address, revenue);
                    total_revenue += revenue;
                }
                printf("--------------------------------------------------------\n");
                printf("ИТОГО: %.2f\n", total_revenue);
                free(workshops);
            }
            break;
        }
        case 5: {
            Workshop* w = workshop_get_most_active(db);
            if (w) {
                printf("\nМастерская с наибольшим количеством ремонтов:\n");
                printf("ID: %d, Адрес: %s, Телефон: %s\n", w->id, w->address, w->phone);
                
                int count;
                Repair* repairs = workshop_get_repairs_by_period(db, w->id, 
                                                                    "1900-01-01", 
                                                                    "2100-01-01", 
                                                                    &count);
                if (repairs) {
                    printf("Всего ремонтов: %d\n", count);
                    
                    printf("\nСписок ремонтов (первые 10):\n");
                    for (int i = 0; i < count && i < 10; i++) {
                        printf("  - Ремонт #%d: %s, стоимость %.2f\n",
                                repairs[i].id, repairs[i].car_license, repairs[i].cost);
                    }
                    free(repairs);
                }
                free(w);
            } else {
                printf("Нет данных о мастерских.\n");
            }
            break;
        }
        case 6:
            return;
        default:
            printf("Неверный выбор!\n");
    }
}

void show_mechanic_menu() {
    int choice;
    
    printf("\n--- МЕНЮ МАСТЕРА ---\n");
    printf("1. Мои ремонты\n");
    printf("2. Добавить новый ремонт\n");
    printf("3. Обновить статус ремонта\n");
    printf("4. Найти автомобиль\n");
    printf("5. Выход из системы\n");
    printf("Выберите действие: ");
    
    scanf("%d", &choice);
    getchar();
    
    switch(choice) {
        case 1:
            mechanic_my_repairs();
            break;
        case 2:
            mechanic_add_repair();
            break;
        case 3:
            mechanic_update_status();
            break;
        case 4:
            mechanic_find_car();
            break;
        case 5:
            handle_logout();
            break;
        default:
            printf("Неверный выбор!\n");
    }
}

void handle_logout() {
    auth_logout(current_user);
    current_user = NULL;
    printf("Вы вышли из системы.\n");
}

void mechanic_my_repairs() {
    if (!current_user || current_user->mechanic_id <= 0) {
        printf("Ошибка: мастер не идентифицирован.\n");
        return;
    }
    
    int count;
    Repair* repairs = mechanic_get_repairs(db, current_user->mechanic_id, &count);
    
    if (repairs) {
        printf("\nМОИ РЕМОНТЫ:\n");
        printf("--------------------------------------------------------\n");
        for (int i = 0; i < count; i++) {
            printf("Ремонт #%d\n", repairs[i].id);
            printf("Авто: %s\n", repairs[i].car_license);
            printf("Дата: %s\n", repairs[i].start_date);
            printf("Стоимость: %.2f\n", repairs[i].cost);
            printf("Статус: %s\n", repairs[i].status);
            printf("--------------------------------------------------------\n");
        }
        free(repairs);
    } else {
        printf("У вас нет ремонтов.\n");
    }
}

void mechanic_add_repair() {
    if (!current_user || current_user->mechanic_id <= 0) {
        printf("Ошибка: мастер не идентифицирован.\n");
        return;
    }
    
    Repair r;
    memset(&r, 0, sizeof(Repair));
    
    r.mechanic_id = current_user->mechanic_id;
    
    // Получаем мастерскую мастера
    Workshop* w = mechanic_get_workshop(db, current_user->mechanic_id);
    if (!w) {
        printf("Ошибка: не удалось определить мастерскую.\n");
        return;
    }
    r.workshop_id = w->id;
    free(w);
    
    printf("\n--- НОВЫЙ РЕМОНТ ---\n");
    
    printf("Введите госномер автомобиля: ");
    fgets(r.car_license, sizeof(r.car_license), stdin);
    r.car_license[strcspn(r.car_license, "\n")] = 0;
    
    // Проверяем, существует ли автомобиль
    if (!car_exists(db, r.car_license)) {
        printf("Автомобиль не найден. Сначала добавьте автомобиль.\n");
        
        Car c;
        memset(&c, 0, sizeof(Car));
        strcpy(c.license_plate, r.car_license);
        
        printf("Введите марку: ");
        fgets(c.brand, sizeof(c.brand), stdin);
        c.brand[strcspn(c.brand, "\n")] = 0;
        
        printf("Введите модель: ");
        fgets(c.model, sizeof(c.model), stdin);
        c.model[strcspn(c.model, "\n")] = 0;
        
        printf("Введите год выпуска: ");
        scanf("%d", &c.year);
        getchar();
        
        printf("Введите ФИО владельца: ");
        fgets(c.owner_name, sizeof(c.owner_name), stdin);
        c.owner_name[strcspn(c.owner_name, "\n")] = 0;
        
        printf("Введите номер техпаспорта: ");
        fgets(c.passport_number, sizeof(c.passport_number), stdin);
        c.passport_number[strcspn(c.passport_number, "\n")] = 0;
        
        printf("Введите адрес владельца: ");
        fgets(c.owner_address, sizeof(c.owner_address), stdin);
        c.owner_address[strcspn(c.owner_address, "\n")] = 0;
        
        int rc = car_insert(db, &c);
        if (rc != SQLITE_OK) {
            printf("Ошибка при добавлении автомобиля.\n");
            return;
        }
        printf("Автомобиль добавлен.\n");
    }
    
    printf("Введите ID вида работ: ");
    scanf("%d", &r.repair_type_id);
    getchar();
    
    printf("Введите дату начала (ГГГГ-ММ-ДД): ");
    fgets(r.start_date, sizeof(r.start_date), stdin);
    r.start_date[strcspn(r.start_date, "\n")] = 0;
    
    printf("Введите стоимость: ");
    scanf("%f", &r.cost);
    getchar();
    
    strcpy(r.status, "in_progress");
    r.end_date[0] = '\0';
    
    int rc = repair_insert(db, &r);
    if (rc == SQLITE_OK) {
        printf("Ремонт добавлен с номером: %d\n", r.id);
    } else {
        printf("Ошибка при добавлении ремонта.\n");
    }
}

void mechanic_update_status() {
    if (!current_user || current_user->mechanic_id <= 0) {
        printf("Ошибка: мастер не идентифицирован.\n");
        return;
    }
    
    int repair_id;
    printf("Введите номер ремонта: ");
    scanf("%d", &repair_id);
    getchar();
    
    Repair* r = repair_get_by_id(db, repair_id);
    if (!r) {
        printf("Ремонт не найден.\n");
        return;
    }
    
    if (r->mechanic_id != current_user->mechanic_id) {
        printf("Этот ремонт выполняете не вы.\n");
        free(r);
        return;
    }
    
    printf("Текущий статус: %s\n", r->status);
    printf("Новый статус (completed/in_progress): ");
    char new_status[32];
    fgets(new_status, sizeof(new_status), stdin);
    new_status[strcspn(new_status, "\n")] = 0;
    
    if (strcmp(new_status, "completed") == 0 && strcmp(r->status, "completed") != 0) {
        char end_date[16];
        printf("Введите дату завершения (ГГГГ-ММ-ДД): ");
        fgets(end_date, sizeof(end_date), stdin);
        end_date[strcspn(end_date, "\n")] = 0;
        
        repair_complete(db, repair_id, end_date);
        printf("Ремонт завершен.\n");
    } else {
        strcpy(r->status, new_status);
        repair_update(db, r);
        printf("Статус обновлен.\n");
    }
    
    free(r);
}

void mechanic_find_car() {
    char plate[16];
    printf("Введите госномер: ");
    fgets(plate, sizeof(plate), stdin);
    plate[strcspn(plate, "\n")] = 0;
    
    Car* car = car_get_by_license(db, plate);
    if (car) {
        printf("\nИНФОРМАЦИЯ ОБ АВТОМОБИЛЕ:\n");
        printf("Госномер: %s\n", car->license_plate);
        printf("Марка: %s\n", car->brand);
        printf("Модель: %s\n", car->model);
        printf("Год: %d\n", car->year);
        printf("Владелец: %s\n", car->owner_name);
        printf("Техпаспорт: %s\n", car->passport_number);
        printf("Адрес: %s\n", car->owner_address);
        
        int count;
        Repair* repairs = car_get_repair_history(db, plate, &count);
        if (repairs) {
            printf("\nИстория ремонтов:\n");
            for (int i = 0; i < count; i++) {
                printf("  - %s: %s, стоимость %.2f, статус %s\n",
                        repairs[i].start_date,
                        repairs[i].status,
                        repairs[i].cost,
                        repairs[i].status);
            }
            free(repairs);
        }
        
        free(car);
    } else {
        printf("Автомобиль не найден.\n");
    }
}