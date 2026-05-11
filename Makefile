TARGET     := a2t_play
TEST_TARGET := a2m_dump
SRC_DIR    := src

SRCS      := $(wildcard $(SRC_DIR)/*.c)
OBJS      := $(SRCS:.c=.o)
TEST_DEPS := $(filter-out $(SRC_DIR)/sdl.o $(SRC_DIR)/a2t.o, $(OBJS))

BUILD    ?= debug

ifeq ($(BUILD),release)
  BASE_CFLAGS := -std=c99 -O2 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function
else
  BASE_CFLAGS := -std=c99 -g -Wall -Wextra -Wno-unused-parameter -Wno-unused-function
endif

SDL_CONFIG  := sdl2-config
SDL_CFLAGS  := $(shell $(SDL_CONFIG) --cflags)
SDL_LDFLAGS := $(shell $(SDL_CONFIG) --libs)

ALL_CFLAGS  := $(BASE_CFLAGS) $(SDL_CFLAGS)
ALL_LDFLAGS := $(SDL_LDFLAGS) -lm

ifeq ($(OS),Windows_NT)
  ALL_LDFLAGS += -mconsole
endif

.PHONY: all clean install test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(ALL_CFLAGS) -o $@ $^ $(ALL_LDFLAGS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(ALL_CFLAGS) -MMD -MP -c -o $@ $<

-include $(OBJS:.o=.d)

$(TEST_TARGET): a2m_dump.c $(TEST_DEPS)
	mkdir -p test
	$(CC) $(BASE_CFLAGS) -Dclocks -I$(SRC_DIR) -o $@ $< $(TEST_DEPS) -lm

test: $(TEST_TARGET)

clean:
	rm -f $(OBJS) $(OBJS:.o=.d) $(TARGET).exe $(TARGET) \
	      $(TEST_TARGET).exe $(TEST_TARGET)

install: $(TARGET)
	cp $(TARGET).exe /usr/local/bin/
