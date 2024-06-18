ROOT_DIR := $(PWD)
CXX := g++
CC := gcc

CXX_FLAGS := -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Wno-unused-label -pedantic -O0 -g -std=c++23
CC_FLAGS := -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Wno-unused-label -pedantic -O0 -g -std=gnu23

SRC_DIR := $(shell realpath src)
BIN_DIR := $(shell realpath bin)
LIBS_DIR := $(BIN_DIR)/libs
OBJ_DIR := $(shell realpath obj)

APP := $(BIN_DIR)/app

SRCS := $(shell find $(SRC_DIR) -name '*.c' -type f -printf '%P\n' -o -name '*.cpp' -type f -printf '%P\n')
OBJS := $(SRCS:%=$(OBJ_DIR)/%.o)
OBJ_DIRS := $(shell dirname $(OBJS))

LDFLAGS := -L$(LIBS_DIR)
INCLUDE_FLAGS := -Iinclude/

.PHONY: all prepare run clean
all: prepare $(APP)

run: all
	$(BIN_DIR)/app -U /tmp/1234rwd.socket -T 127.0.0.1:8888

prepare:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OBJ_DIRS)
	@mkdir -p $(LIBS_DIR)

clean:
	rm -rf $(BIN_DIR)
	rm -rf $(OBJ_DIR)

$(APP): $(OBJS)
	@$(CXX) $(OBJS) -o $(APP) $(LDFLAGS) $(INCLUDE_FLAGS)
	@echo 'LD $(APP)'


$(OBJ_DIR)/%.c.o: $(SRC_DIR)/%.c
	@echo 'CC $@'
	@$(CC) -c $(CC_FLAGS) $(INCLUDE_FLAGS) $^ -o $@

$(OBJ_DIR)/%.cpp.o: $(SRC_DIR)/%.cpp
	@echo 'CXX $@'
	@$(CXX) -c $(CXX_FLAGS) $(INCLUDE_FLAGS) $^ -o $@
