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
TEST_REPORTS = $(TEST_SOURCES:$(TESTDIR)/%.c=$(TESTDIR)/%.report)

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
	@gcov $(SOURCES) > /dev/null 2>&1
	@lcov --capture --directory . --output-file $(COVDIR)/coverage.info --rc lcov_branch_coverage=1 > /dev/null 2>&1
	@lcov --remove $(COVDIR)/coverage.info '/usr/*' '*/tests/*' '*/build/*' --output-file $(COVDIR)/coverage-filtered.info > /dev/null 2>&1
	@genhtml $(COVDIR)/coverage-filtered.info --output-directory $(COVDIR) --branch-coverage > /dev/null 2>&1
	@echo -e "$(GREEN)✓ Отчет о покрытии создан в директории $(COVDIR)/$(NC)"
	@echo -e "$(GREEN)✓ Открой файл: $(COVDIR)/index.html в браузере$(NC)\n"
	@echo -e "Покрытие кода:"
	@lcov --summary $(COVDIR)/coverage-filtered.info 2>&1 | grep -E "lines|functions|branches" | sed 's/^/  /'

# Просмотр покрытия в браузере
view-coverage: coverage
	@if command -v xdg-open > /dev/null; then \
		xdg-open $(COVDIR)/index.html; \
	elif command -v open > /dev/null; then \
		open $(COVDIR)/index.html; \
	else \
		echo -e "Открой файл вручную: $(COVDIR)/index.html"; \
	fi

# Очистка
clean:
	rm -rf $(BUILDDIR) $(BINDIR) $(COVDIR) *.gcda *.gcno *.gcov $(TESTDIR)/*.report
	@echo -e "$(GREEN)✓ Очистка выполнена$(NC)"

# Создание базы данных
init-db:
	@if [ -f garage.db ]; then rm garage.db; fi
	@sqlite3 garage.db < scripts/init_db.sql
	@echo -e "$(GREEN)✓ База данных создана: garage.db$(NC)"

.PHONY: all test coverage view-coverage clean init-db