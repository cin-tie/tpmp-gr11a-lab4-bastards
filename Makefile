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

# Исходные файлы
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

# Тесты
TEST_SOURCES = $(wildcard $(TESTDIR)/*.c)
TEST_TARGETS = $(TEST_SOURCES:$(TESTDIR)/%.c=$(BINDIR)/%)

# Цель по умолчанию
all: $(BINDIR)/autoworkshop

# Сборка основного приложения
$(BINDIR)/autoworkshop: $(OBJECTS)
	@mkdir -p $(BINDIR)
	$(CC) $^ -o $@ $(LDFLAGS)

# Сборка объектных файлов
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Сборка тестов
$(BINDIR)/test_%: $(TESTDIR)/test_%.c $(filter-out $(BUILDDIR)/main.o, $(OBJECTS))
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test: $(TEST_TARGETS)
	@echo "========================================"
	@echo "ЗАПУСК ВСЕХ ТЕСТОВ"
	@echo "========================================"
	@for test in $(TEST_TARGETS); do \
		echo ">>> Запуск $$(basename $$test)"; \
		$$test; \
		if [ $$? -eq 0 ]; then \
			echo "✓ $$(basename $$test) пройден\n"; \
		else \
			echo "✗ $$(basename $$test) не пройден\n"; \
			exit 1; \
		fi \
	done
	@echo "========================================"
	@echo "ВСЕ ТЕСТЫ ПРОЙДЕНЫ"
	@echo "========================================"

# Покрытие кода
coverage:
	@echo "========================================"
	@echo "ГЕНЕРАЦИЯ ОТЧЕТА О ПОКРЫТИИ КОДА"
	@echo "========================================"
	@gcov $(SOURCES)
	@mkdir -p coverage
	@lcov --capture --directory . --output-file coverage/coverage.info
	@genhtml coverage/coverage.info --output-directory coverage
	@echo "Отчет о покрытии создан в директории coverage/"
	@echo "Открой coverage/index.html в браузере"

# Очистка
clean:
	rm -rf $(BUILDDIR) $(BINDIR) coverage *.gcda *.gcno *.gcov


.PHONY: all test coverage clean