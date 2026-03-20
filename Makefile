# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -Iincludes -g -fprofile-arcs -ftest-coverage
LDFLAGS = -lsqlite3 -lssl -lcrypto -lgcov

# Директории
SRCDIR = src
INCDIR = includes
BINDIR = bin
BUILDDIR = build
TESTDIR = tests
COVDIR = coverage

# Исходные файлы (исключаем main.c для тестов)
SOURCES = $(filter-out $(SRCDIR)/main.c, $(wildcard $(SRCDIR)/*.c))
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

# Тестовые файлы
TEST_SOURCES = $(wildcard $(TESTDIR)/test_*.c)
TEST_TARGETS = $(TEST_SOURCES:$(TESTDIR)/%.c=$(BINDIR)/%)

# Цвета для вывода
GREEN = \033[0;32m
RED = \033[0;31m
NC = \033[0m

# Цель по умолчанию
all: $(BINDIR)/autoworkshop

# Сборка основного приложения
$(BINDIR)/autoworkshop: $(BUILDDIR)/main.o $(OBJECTS)
	@mkdir -p $(BINDIR)
	$(CC) $^ -o $@ $(LDFLAGS)
	@echo -e "$(GREEN)✓ Приложение собрано: $@$(NC)"

# Сборка объектных файлов
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Сборка тестов
$(BINDIR)/test_%: $(TESTDIR)/test_%.c $(OBJECTS)
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	@echo -e "$(GREEN)✓ Тест собран: $@$(NC)"

# Запуск всех тестов
test: $(TEST_TARGETS)
	@echo -e "\n$(GREEN)========================================$(NC)"
	@echo -e "$(GREEN)ЗАПУСК ВСЕХ ТЕСТОВ$(NC)"
	@echo -e "$(GREEN)========================================$(NC)\n"
	@total=0; passed=0; \
	for test in $(TEST_TARGETS); do \
		echo ">>> Запуск $$(basename $$test)"; \
		$$test; \
		if [ $$? -eq 0 ]; then \
			echo -e "$(GREEN)✓ $$(basename $$test) ПРОЙДЕН$(NC)\n"; \
			passed=$$((passed + 1)); \
		else \
			echo -e "$(RED)✗ $$(basename $$test) НЕ ПРОЙДЕН$(NC)\n"; \
		fi; \
		total=$$((total + 1)); \
	done; \
	echo -e "$(GREEN)========================================$(NC)"; \
	echo -e "$(GREEN)ИТОГО: $$passed из $$total тестов пройдено$(NC)"; \
	echo -e "$(GREEN)========================================$(NC)"; \
	if [ $$passed -eq $$total ]; then exit 0; else exit 1; fi

# Генерация отчета о покрытии
coverage: test
	@echo -e "\n$(GREEN)========================================$(NC)"
	@echo -e "$(GREEN)ГЕНЕРАЦИЯ ОТЧЕТА О ПОКРЫТИИ$(NC)"
	@echo -e "$(GREEN)========================================$(NC)\n"
	@mkdir -p $(COVDIR)
	@echo "Сбор данных покрытия..."
	@for src in $(SOURCES); do \
		base=$$(basename $$src .c); \
		if [ -f "$$base.gcda" ]; then \
			gcov -b -p $$src 2>/dev/null || true; \
		fi \
	done
	@echo "Создание HTML отчета..."
	@if command -v lcov >/dev/null 2>&1; then \
		lcov --capture --directory . --output-file $(COVDIR)/coverage.info --rc lcov_branch_coverage=1 2>/dev/null || true; \
		lcov --remove $(COVDIR)/coverage.info '/usr/*' '*/tests/*' '*/build/*' '*/main.c' --output-file $(COVDIR)/coverage-filtered.info 2>/dev/null || true; \
		genhtml $(COVDIR)/coverage-filtered.info --output-directory $(COVDIR)/html --branch-coverage 2>/dev/null || true; \
		echo -e "$(GREEN)✓ Отчет создан в $(COVDIR)/html/index.html$(NC)"; \
	else \
		echo -e "$(RED)✗ lcov не установлен. Установите: sudo apt install lcov$(NC)"; \
		echo "Показываем текстовый отчет:"; \
		for src in $(SOURCES); do \
			base=$$(basename $$src .c); \
			if [ -f "$$base.c.gcov" ]; then \
				echo ""; \
				echo "=== $$base.c ==="; \
				head -30 "$$base.c.gcov"; \
			fi \
		done \
	fi
	@echo ""
	@echo "Покрытие кода (простой подсчет):"
	@total_lines=0; executed_lines=0; \
	for src in $(SOURCES); do \
		base=$$(basename $$src .c); \
		if [ -f "$$base.c.gcov" ]; then \
			lines=$$(grep -c "^    [0-9]" "$$base.c.gcov" 2>/dev/null || echo 0); \
			exec=$$(grep -c "^    [1-9]" "$$base.c.gcov" 2>/dev/null || echo 0); \
			if [ $$lines -gt 0 ]; then \
				pct=$$((exec * 100 / lines)); \
				printf "  %-20s: %3d/%3d lines (%d%%)\n" "$$base.c" $$exec $$lines $$pct; \
				total_lines=$$((total_lines + lines)); \
				executed_lines=$$((executed_lines + exec)); \
			fi \
		fi \
	done; \
	if [ $$total_lines -gt 0 ]; then \
		total_pct=$$((executed_lines * 100 / total_lines)); \
		echo "----------------------------------------"; \
		printf "  %-20s: %3d/%3d lines (%d%%)\n" "ИТОГО" $$executed_lines $$total_lines $$total_pct; \
	fi

# Очистка
clean:
	rm -rf $(BUILDDIR) $(BINDIR) $(COVDIR) *.gcda *.gcno *.gcov
	@echo -e "$(GREEN)✓ Очистка выполнена$(NC)"

# Создание базы данных
init-db:
	@if [ -f garage.db ]; then rm garage.db; fi
	@sqlite3 garage.db < scripts/init_db.sql
	@echo -e "$(GREEN)✓ База данных создана: garage.db$(NC)"

# Проверка установки lcov
check-lcov:
	@if command -v lcov >/dev/null 2>&1; then \
		echo -e "$(GREEN)✓ lcov установлен$(NC)"; \
	else \
		echo -e "$(RED)✗ lcov не установлен$(NC)"; \
		echo "Установка: sudo apt install lcov (Ubuntu/Debian)"; \
		echo "или: brew install lcov (macOS)"; \
	fi

.PHONY: all test coverage clean init-db check-lcov