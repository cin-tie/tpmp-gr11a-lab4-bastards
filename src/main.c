#include "mechanic.h"
#include "car.h"
#include "repair.h"

// Реализация admin_workshops_menu
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
                fgets(new_address, sizeof(new_address),