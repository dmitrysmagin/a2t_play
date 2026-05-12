TARGET     := a2t_play
TEST_TARGET := a2m_dump
SRC_DIR    := src

SRCS      := src/sdl.c src/a2t.c src/depack.c src/sixpack.c src/unlzh.c src/unlzw.c src/unlzss.c src/opl3.c src/debug.c
OBJS      := $(SRCS:.c=.o)
TEST_DEPS := $(filter-out $(SRC_DIR)/sdl.o $(SRC_DIR)/a2t.o, $(OBJS))

BUILD    ?= debug

# First explicit rule is $(BUILD_TMP); without this, plain `make` would only
# create .build_tmp and not build a2t_play.
.DEFAULT_GOAL := all

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

BUILD_TMP := $(CURDIR)/.build_tmp
TMP_ENV   := TMP=$(BUILD_TMP) TEMP=$(BUILD_TMP) TMPDIR=$(BUILD_TMP)

$(BUILD_TMP):
	mkdir -p $(BUILD_TMP)

.PHONY: all clean install test

all: $(TARGET)

$(TARGET): $(OBJS) | $(BUILD_TMP)
	$(TMP_ENV) $(CC) $(ALL_CFLAGS) -o $@ $^ $(ALL_LDFLAGS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_TMP)
	$(TMP_ENV) $(CC) $(ALL_CFLAGS) -MMD -MP -c -o $@ $<

-include $(OBJS:.o=.d)

$(TEST_TARGET): src/a2m_dump.c src/a2t.c src/a2t.h $(TEST_DEPS) | $(BUILD_TMP)
	$(TMP_ENV) $(CC) $(BASE_CFLAGS) -Dclocks -I$(SRC_DIR) -o $@ $< $(TEST_DEPS) -lm

test: $(TEST_TARGET)

clean:
	rm -f $(OBJS) $(OBJS:.o=.d) $(TARGET).exe $(TARGET) \
	      $(TEST_TARGET).exe $(TEST_TARGET)
	rm -rf $(BUILD_TMP)

install: $(TARGET)
	cp $(TARGET).exe /usr/local/bin/
