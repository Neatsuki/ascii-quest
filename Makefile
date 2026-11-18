# ASCII QUEST - 32-bit x86 NASM + C dungeon crawler for Linux
#
# Build requirements (Debian/Ubuntu/WSL):
#   sudo apt install nasm gcc-multilib libc6-dev-i386
#
# The result is a genuine 32-bit ELF executable: check with `file rpg`.
# The C side is split by responsibility; see src/game.h for the module map.

NASM      := nasm
CC        := gcc
NASMFLAGS := -f elf32 -g -F dwarf
CFLAGS    := -m32 -march=i386 -std=gnu11 -O2 -Wall -Wextra \
             -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables
LDFLAGS   := -m32 -no-pie -Wl,-z,noexecstack

BUILD  := build
TARGET := rpg

MODULES := data ui render world dungeon combat inventory npc save main
OBJS    := $(BUILD)/asmfuncs.o $(addprefix $(BUILD)/,$(addsuffix .o,$(MODULES)))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILD)/asmfuncs.o: src/asmfuncs.asm | $(BUILD)
	$(NASM) $(NASMFLAGS) -o $@ $<

$(BUILD)/%.o: src/%.c src/game.h | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD):
	mkdir -p $(BUILD)

run: $(TARGET)
	./$(TARGET)

clean:
	$(RM) $(TARGET) $(OBJS)

test: $(TARGET)
	python3 tests/smoke_test.py

.PHONY: all run clean test
