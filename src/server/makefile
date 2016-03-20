CC=gcc
CFLAGS=-I. -g -std=c99 -lpthread

HEADERS = server_thread.h

DEPS = main.c server_thread.c conf.c

O = main.o server_thread.o conf.o
OBJ = $(patsubst %.o,$(BUILD_DIR)/server/%.o, $(O))

BUILD_DIR=../../build

.PHONY: default all clean format

default: $(BUILD_DIR)/tp2_server

all: default

format: $(DEPS)
	indent -gnu -l80 $(DEPS) $(HEADERS)
	rm -f *~

$(BUILD_DIR)/server/%.o: %.c $(DEPS)
	mkdir -p ${BUILD_DIR}/server
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILD_DIR)/tp2_server: $(OBJ)
	mkdir -p ${BUILD_DIR}
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	@echo "Cleaning build directory..."
	@rm -f $(BUILD_DIR)/server/*.o $(BUILD_DIR)/tp2_server
