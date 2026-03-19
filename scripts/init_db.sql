-- scripts/init_db.sql
-- Скрипт для полной инициализации базы данных автомастерских

-- Отключаем внешние ключи временно
PRAGMA foreign_keys = OFF;

-- Удаляем существующие таблицы (если есть)
DROP TABLE IF EXISTS users;
DROP TABLE IF EXISTS repairs;
DROP TABLE IF EXISTS repair_types;
DROP TABLE IF EXISTS cars;
DROP TABLE IF EXISTS mechanics;
DROP TABLE IF EXISTS workshops;
DROP TABLE IF EXISTS mechanic_stats;
DROP TABLE IF EXISTS workshop_stats;

-- Включаем внешние ключи
PRAGMA foreign_keys = ON;

-- =====================================================
-- 1. СОЗДАНИЕ ТАБЛИЦ
-- =====================================================

-- Таблица мастерских
CREATE TABLE workshops (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    address TEXT NOT NULL,
    phone TEXT,
    car_brands TEXT
);

-- Таблица мастеров
CREATE TABLE mechanics (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    specialization TEXT,
    workshop_id INTEGER NOT NULL,
    FOREIGN KEY (workshop_id) REFERENCES workshops(id)
);

-- Таблица автомобилей
CREATE TABLE cars (
    license_plate TEXT PRIMARY KEY,
    brand TEXT NOT NULL,
    model TEXT,
    year INTEGER,
    owner_name TEXT NOT NULL,
    passport_number TEXT UNIQUE NOT NULL,
    owner_address TEXT
);

-- Таблица видов работ
CREATE TABLE repair_types (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    category TEXT,
    base_price REAL
);

-- Таблица ремонтов
CREATE TABLE repairs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    workshop_id INTEGER NOT NULL,
    mechanic_id INTEGER NOT NULL,
    car_license TEXT NOT NULL,
    repair_type_id INTEGER NOT NULL,
    start_date TEXT NOT NULL,
    end_date TEXT,
    cost REAL NOT NULL,
    status TEXT NOT NULL DEFAULT 'in_progress',
    FOREIGN KEY (workshop_id) REFERENCES workshops(id),
    FOREIGN KEY (mechanic_id) REFERENCES mechanics(id),
    FOREIGN KEY (car_license) REFERENCES cars(license_plate),
    FOREIGN KEY (repair_type_id) REFERENCES repair_types(id)
);

-- Таблица пользователей системы
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT UNIQUE NOT NULL,
    password_hash TEXT NOT NULL,
    role TEXT NOT NULL,
    mechanic_id INTEGER,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (mechanic_id) REFERENCES mechanics(id)
);

-- Таблицы для статистики (для триггеров)
CREATE TABLE mechanic_stats (
    mechanic_id INTEGER PRIMARY KEY,
    total_repairs INTEGER DEFAULT 0,
    total_revenue REAL DEFAULT 0.0,
    last_repair_date TEXT,
    FOREIGN KEY (mechanic_id) REFERENCES mechanics(id)
);

CREATE TABLE workshop_stats (
    workshop_id INTEGER PRIMARY KEY,
    total_repairs INTEGER DEFAULT 0,
    total_revenue REAL DEFAULT 0.0,
    last_repair_date TEXT,
    FOREIGN KEY (workshop_id) REFERENCES workshops(id)
);

-- =====================================================
-- 2. СОЗДАНИЕ ИНДЕКСОВ
-- =====================================================

CREATE INDEX idx_cars_brand ON cars(brand);
CREATE INDEX idx_mechanics_workshop ON mechanics(workshop_id);
CREATE INDEX idx_repairs_workshop ON repairs(workshop_id);
CREATE INDEX idx_repairs_mechanic ON repairs(mechanic_id);
CREATE INDEX idx_repairs_car ON repairs(car_license);
CREATE INDEX idx_repairs_start_date ON repairs(start_date);
CREATE INDEX idx_repairs_status ON repairs(status);

-- =====================================================
-- 3. СОЗДАНИЕ ТРИГГЕРОВ
-- =====================================================

-- Триггер для обновления статистики мастеров при добавлении ремонта
CREATE TRIGGER update_mechanic_stats_insert
AFTER INSERT ON repairs
BEGIN
    INSERT OR REPLACE INTO mechanic_stats (mechanic_id, total_repairs, total_revenue, last_repair_date)
    SELECT NEW.mechanic_id,
           COALESCE((SELECT total_repairs FROM mechanic_stats WHERE mechanic_id = NEW.mechanic_id), 0) + 1,
           COALESCE((SELECT total_revenue FROM mechanic_stats WHERE mechanic_id = NEW.mechanic_id), 0) + NEW.cost,
           NEW.start_date
    WHERE NEW.mechanic_id IS NOT NULL;
END;

-- Триггер для обновления статистики мастерских при добавлении ремонта
CREATE TRIGGER update_workshop_stats_insert
AFTER INSERT ON repairs
BEGIN
    INSERT OR REPLACE INTO workshop_stats (workshop_id, total_repairs, total_revenue, last_repair_date)
    SELECT NEW.workshop_id,
           COALESCE((SELECT total_repairs FROM workshop_stats WHERE workshop_id = NEW.workshop_id), 0) + 1,
           COALESCE((SELECT total_revenue FROM workshop_stats WHERE workshop_id = NEW.workshop_id), 0) + NEW.cost,
           NEW.start_date
    WHERE NEW.workshop_id IS NOT NULL;
END;

-- =====================================================
-- 4. ЗАПОЛНЕНИЕ ТЕСТОВЫМИ ДАННЫМИ
-- =====================================================

-- Мастерские
INSERT INTO workshops (address, phone, car_brands) VALUES
('ул. Ленина, 15, Минск', '+375 17 123-45-67', 'Toyota, Honda, Mazda'),
('пр. Независимости, 78, Минск', '+375 29 234-56-78', 'BMW, Audi, Mercedes'),
('ул. Московская, 23, Гомель', '+375 232 45-67-89', 'Renault, Peugeot, Citroen'),
('ул. Советская, 5, Брест', '+375 162 34-56-78', 'Volkswagen, Skoda, Seat'),
('пр. Пушкина, 42, Витебск', '+375 212 67-89-01', 'Ford, Opel, Chevrolet');

-- Мастера
INSERT INTO mechanics (name, specialization, workshop_id) VALUES
('Иванов Иван Иванович', 'Кузовные работы', 1),
('Петров Петр Петрович', 'Двигатель', 1),
('Сидоров Сидор Сидорович', 'Ходовая часть', 2),
('Козлов Николай Алексеевич', 'Электрика', 2),
('Новиков Андрей Сергеевич', 'ТО', 3),
('Морозов Дмитрий Владимирович', 'Кузовные работы', 3),
('Волков Сергей Павлович', 'Двигатель', 4),
('Зайцев Алексей Игоревич', 'Ходовая часть', 4),
('Павлов Артем Викторович', 'ТО', 5),
('Соколов Михаил Олегович', 'Электрика', 5);

-- Автомобили
INSERT INTO cars (license_plate, brand, model, year, owner_name, passport_number, owner_address) VALUES
('1234 AB-7', 'Toyota', 'Corolla', 2019, 'Алексеев Дмитрий Николаевич', 'AB1234567', 'ул. Кирова, 10, Минск'),
('5678 CD-7', 'BMW', 'X5', 2020, 'Смирнова Елена Петровна', 'BC2345678', 'пр. Гагарина, 25, Минск'),
('9012 EF-7', 'Renault', 'Logan', 2018, 'Кузнецов Андрей Викторович', 'CD3456789', 'ул. Лермонтова, 3, Гомель'),
('3456 GH-7', 'Volkswagen', 'Passat', 2021, 'Васильева Татьяна Игоревна', 'DE4567890', 'ул. Победы, 15, Брест'),
('7890 IJ-7', 'Ford', 'Focus', 2017, 'Тимошенко Игорь Петрович', 'EF5678901', 'пр. Строителей, 8, Витебск'),
('1235 KL-7', 'Honda', 'CR-V', 2022, 'Мельник Ольга Сергеевна', 'FG6789012', 'ул. Лесная, 42, Минск'),
('1236 MN-7', 'Audi', 'Q7', 2019, 'Гончаров Максим Алексеевич', 'GH7890123', 'ул. Солнечная, 7, Минск'),
('1237 OP-7', 'Peugeot', '308', 2020, 'Шевченко Анна Владимировна', 'HI8901234', 'ул. Новая, 12, Гомель'),
('1238 QR-7', 'Skoda', 'Octavia', 2021, 'Коваленко Ирина Петровна', 'IJ9012345', 'пр. Мира, 5, Брест'),
('1239 ST-7', 'Opel', 'Astra', 2018, 'Ткаченко Виктор Николаевич', 'JK0123456', 'ул. Зеленая, 33, Витебск');

-- Виды работ
INSERT INTO repair_types (name, category, base_price) VALUES
('Замена масла', 'ТО', 50.00),
('Замена масляного фильтра', 'ТО', 15.00),
('Замена воздушного фильтра', 'ТО', 20.00),
('Замена топливного фильтра', 'ТО', 25.00),
('Замена ремня ГРМ', 'Двигатель', 150.00),
('Регулировка клапанов', 'Двигатель', 120.00),
('Замена маслосъёмных колпачков', 'Двигатель', 200.00),
('Замена тормозных колодок', 'Ходовая часть', 80.00),
('Замена амортизаторов', 'Ходовая часть', 180.00),
('Замена сайлентблоков', 'Ходовая часть', 150.00),
('Покраска крыла', 'Кузовные работы', 300.00),
('Рихтовка двери', 'Кузовные работы', 250.00),
('Замена фары', 'Электрика', 100.00),
('Диагностика электроники', 'Электрика', 60.00),
('Замена аккумулятора', 'Электрика', 30.00);

-- Ремонты
INSERT INTO repairs (workshop_id, mechanic_id, car_license, repair_type_id, start_date, end_date, cost, status) VALUES
(1, 1, '1234 AB-7', 11, '2026-03-01', '2026-03-03', 320.00, 'completed'),
(1, 2, '1234 AB-7', 5, '2026-03-01', '2026-03-03', 150.00, 'completed'),
(2, 3, '5678 CD-7', 8, '2026-03-02', '2026-03-02', 80.00, 'completed'),
(2, 4, '5678 CD-7', 14, '2026-03-02', '2026-03-02', 60.00, 'completed'),
(3, 5, '9012 EF-7', 1, '2026-03-03', NULL, 50.00, 'in_progress'),
(3, 6, '3456 GH-7', 12, '2026-03-03', '2026-03-05', 270.00, 'completed'),
(4, 7, '7890 IJ-7', 2, '2026-03-04', '2026-03-04', 15.00, 'completed'),
(4, 8, '1235 KL-7', 9, '2026-03-04', '2026-03-06', 190.00, 'completed'),
(5, 9, '1236 MN-7', 3, '2026-03-05', NULL, 20.00, 'in_progress'),
(5, 10, '1237 OP-7', 10, '2026-03-05', '2026-03-07', 160.00, 'completed');

-- =====================================================
-- 5. СОЗДАНИЕ ПОЛЬЗОВАТЕЛЕЙ (ПАРОЛЬ: password)
-- =====================================================

-- Хеш SHA256 для 'password'
-- 5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8

INSERT INTO users (username, password_hash, role, mechanic_id) VALUES
('admin', '5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8', 'admin', NULL),
('ivanov', '5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8', 'mechanic', 1),
('petrov', '5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8', 'mechanic', 2),
('sidorov', '5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8', 'mechanic', 3),
('kozlov', '5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8', 'mechanic', 4),
('novikov', '5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8', 'mechanic', 5),
('morozov', '5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8', 'mechanic', 6),
('volkov', '5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8', 'mechanic', 7),
('zaycev', '5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8', 'mechanic', 8),
('pavlov', '5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8', 'mechanic', 9),
('sokolov', '5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8', 'mechanic', 10);

-- =====================================================
-- 6. ПРОВЕРКА ДАННЫХ
-- =====================================================

-- Вывод статистики
SELECT 'Количество мастерских: ' || COUNT(*) FROM workshops;
SELECT 'Количество мастеров: ' || COUNT(*) FROM mechanics;
SELECT 'Количество автомобилей: ' || COUNT(*) FROM cars;
SELECT 'Количество ремонтов: ' || COUNT(*) FROM repairs;
SELECT 'Количество пользователей: ' || COUNT(*) FROM users;

-- Успешное завершение
SELECT 'База данных успешно инициализирована!' as message;