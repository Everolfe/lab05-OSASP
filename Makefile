CC = gcc
DEBUG_FLAGS = -g -ggdb -std=c11 -pedantic -W -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable
RELEASE_FLAGS = -std=c11 -pedantic -W -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable

# Add pthread to both compile and link flags
PTHREAD_FLAGS = -pthread

ifeq ($(MODE),debug)
    CFLAGS = $(DEBUG_FLAGS) $(PTHREAD_FLAGS)
    BUILD_DIR = build/debug
else
    CFLAGS = $(RELEASE_FLAGS) $(PTHREAD_FLAGS)
    BUILD_DIR = build/release
endif

LDFLAGS = $(PTHREAD_FLAGS)

.PHONY: all clean debug release

all: build_dir ipc

build_dir:
	mkdir -p $(BUILD_DIR)/ipc

ipc: $(BUILD_DIR)/main.o $(BUILD_DIR)/ring.o $(BUILD_DIR)/utils.o
	$(CC) $(LDFLAGS) $^ -o $(BUILD_DIR)/ipc/$@

$(BUILD_DIR)/main.o: src/main.c src/ring.h src/utils.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ring.o: src/ring.c src/ring.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/utils.o: src/utils.c src/utils.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

debug:
	$(MAKE) MODE=debug

release:
	$(MAKE) MODE=release