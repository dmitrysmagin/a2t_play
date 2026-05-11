TARGET := a2t_play
SRC_DIR := src

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:.c=.o)

BUILD ?= debug
MSYSTEM ?= $(or $(MSYSTEM),MINGW64)

ifeq ($(BUILD),release)
  CFLAGS := -std=c99 -O2 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function
else
  CFLAGS := -std=c99 -g -Wall -Wextra -Wno-unused-parameter -Wno-unused-function
endif

SDL_CONFIG := sdl2-config
CFLAGS  += $(shell $(SDL_CONFIG) --cflags)
LDFLAGS := $(shell $(SDL_CONFIG) --libs) -lm

ifeq ($(OS),Windows_NT)
  LDFLAGS += -mconsole
endif

.PHONY: all clean install test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

-include $(OBJS:.o=.d)

clean:
	rm -f $(OBJS) $(OBJS:.o=.d) $(TARGET).exe $(TARGET)

install: $(TARGET)
	cp $(TARGET).exe /usr/local/bin/

test: $(TARGET)
	./$(TARGET).exe
