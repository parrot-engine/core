BUILD_NAME=$(shell $(CC) -dumpmachine)

SRC_DIR=src
SRC_DIR=src
ROOT_BUILD_DIR=build
BUILD_DIR=$(ROOT_BUILD_DIR)/$(BUILD_NAME)

CC=gcc
AR=ar

BEAR=bear

CFLAGS += -std=c99 -pedantic-errors
CFLAGS += -Wall -Werror
CFLAGS += -Iinclude
CFLAGS += -g
CFLAGS += $(EXTRA_CFLAGS) $(EXTRA_FLAGS)

SRCS=$(shell find $(SRC_DIR) -type f -name "*.c")
OBJS=$(patsubst %, $(BUILD_DIR)/%.o, $(SRCS))

OUTPUT=$(BUILD_DIR)/libparrotcore.a
TEST=$(BUILD_DIR)/test

.PHONY: all clean test

all: $(OUTPUT)

$(OUTPUT): $(OBJS)
	$(AR) rcs $@ $(OBJS)

test: $(TEST)
	exec $(TEST)

$(TEST): test.c $(OUTPUT)
	$(CC) $(CFLAGS) $(EXTRA_FLAGS) -o $@ test.c $(OUTPUT) -lm

$(BUILD_DIR)/%.o: %
	mkdir -p $(dir $@)
	$(BEAR) -a -- $(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -r $(BUILD_DIR)
