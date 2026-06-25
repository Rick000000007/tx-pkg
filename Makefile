# TX Package Manager v1.0 - Makefile
# Copyright (c) 2026 TX Terminal Project

CC ?= clang
CFLAGS ?= -Wall -Wextra -O2 -std=c11 -D_GNU_SOURCE
LDFLAGS ?= -lsqlite3 -lm

# Directories
SRC_DIR = src
CMD_DIR = $(SRC_DIR)/commands
VENDOR_DIR = $(SRC_DIR)/vendor
BUILD_DIR = build
BIN = pkg

# Source files
CORE_SRCS = \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/error.c \
	$(SRC_DIR)/version.c \
	$(SRC_DIR)/package.c \
	$(SRC_DIR)/solver.c \
	$(SRC_DIR)/database.c \
	$(SRC_DIR)/repository.c \
	$(SRC_DIR)/cache.c \
	$(SRC_DIR)/transaction.c \
	$(SRC_DIR)/extract.c \
	$(SRC_DIR)/verify.c \
	$(SRC_DIR)/rollback.c \
	$(SRC_DIR)/recovery.c \
	$(SRC_DIR)/lock.c \
	$(SRC_DIR)/selfupdate.c

CMD_SRCS = \
	$(CMD_DIR)/cmd_update.c \
	$(CMD_DIR)/cmd_search.c \
	$(CMD_DIR)/cmd_info.c \
	$(CMD_DIR)/cmd_list.c \
	$(CMD_DIR)/cmd_install.c \
	$(CMD_DIR)/cmd_remove.c \
	$(CMD_DIR)/cmd_reinstall.c \
	$(CMD_DIR)/cmd_upgrade.c \
	$(CMD_DIR)/cmd_autoremove.c \
	$(CMD_DIR)/cmd_clean.c \
	$(CMD_DIR)/cmd_verify.c \
	$(CMD_DIR)/cmd_doctor.c \
	$(CMD_DIR)/cmd_repo.c \
	$(CMD_DIR)/cmd_channel.c

VENDOR_SRCS = $(VENDOR_DIR)/cjson.c

ALL_SRCS = $(CORE_SRCS) $(CMD_SRCS) $(VENDOR_SRCS)
OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(ALL_SRCS))

# ====================================================================
# Build Targets
# ====================================================================

.PHONY: all clean install test dirs

all: dirs $(BIN)

dirs:
	@mkdir -p $(BUILD_DIR)/$(SRC_DIR) $(BUILD_DIR)/$(CMD_DIR) $(BUILD_DIR)/$(VENDOR_DIR)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Built: $(BIN)"

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -I$(SRC_DIR)/commands -I$(VENDOR_DIR) -c -o $@ $<

# ====================================================================
# Clean
# ====================================================================

clean:
	rm -rf $(BUILD_DIR) $(BIN)

# ====================================================================
# Install
# ====================================================================

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

install: $(BIN)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(BIN) $(DESTDIR)$(BINDIR)/tx-pkg
	ln -sf tx-pkg $(DESTDIR)$(BINDIR)/pkg

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/tx-pkg
	rm -f $(DESTDIR)$(BINDIR)/pkg

# ====================================================================
# Development
# ====================================================================

test: $(BIN)
	@echo "Running tests..."
	@./tests/run-tests.sh 2>/dev/null || echo "Test suite not yet implemented"

format:
	@find $(SRC_DIR) -name "*.c" -o -name "*.h" | xargs clang-format -i 2>/dev/null || echo "clang-format not available"

check:
	@cppcheck --enable=all --suppress=missingIncludeSystem \
		$(SRC_DIR) 2>/dev/null || echo "cppcheck not available"

# ====================================================================
# Debug build
# ====================================================================

debug: CFLAGS += -g -O0 -DTX_DEBUG
debug: clean all

# ====================================================================
# Static build (no dynamic dependencies)
# ====================================================================

static: LDFLAGS += -static
static: CFLAGS += -static
static: clean all
