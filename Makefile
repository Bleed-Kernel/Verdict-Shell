ELF_TARGET_64 = verdict

CC 	:= x86_64-elf-gcc

ELF_BIN_DIR_64 = bin
ELF_OBJ_DIR_64 = $(ELF_BIN_DIR_64)/obj

LIB_DIR = sysroot/lib
LIBC = $(LIB_DIR)/blibc.a
CRT0 = $(LIB_DIR)/start.o

BLIBC_REPO = https://github.com/Bleed-Kernel/blibc.git
BLIBC_DIR  ?= ../blibc

COMMON_CFLAGS = \
	-ffreestanding \
	-fno-stack-protector \
	-fno-stack-check \
	-Wall -Wextra \
	-nostdlib \
	-no-pie \
	-m64 \
	-Iinclude \
	-Isysroot/include \
	-Isysroot/include/bleed \
	-nostdinc

LDFLAGS = \
	-static \
	-nostdlib \
	-no-pie \
	-m64 \
	-Iinclude \
	-L$(LIB_DIR) \
	-Isysroot/include \
	-Isysroot/lib

SRCS = $(shell find src -name '*.c')
ELF_OBJS_64 = $(patsubst src/%.c,$(ELF_OBJ_DIR_64)/%.o,$(SRCS))

all: $(ELF_TARGET_64)

OBJCOPY := $(shell command -v x86_64-elf-objcopy >/dev/null 2>&1 && echo x86_64-elf-objcopy || echo objcopy)
ICONS   := $(wildcard res/icon-16.png res/icon-32.png res/icon-48.png)

define embed_icons
	@for px in 16 32 48; do \
		if [ -f res/icon-$$px.png ]; then \
			$(OBJCOPY) --add-section .bleed.icon$$px=res/icon-$$px.png \
				--set-section-flags .bleed.icon$$px=noload,readonly $(1) || exit 1; \
		fi; \
	done
	@if [ -f res/app.txt ]; then \
		$(OBJCOPY) --add-section .bleed.app=res/app.txt \
			--set-section-flags .bleed.app=noload,readonly $(1) || exit 1; \
	fi
endef

$(ELF_TARGET_64): blibc $(ELF_OBJS_64) $(ICONS)
	@mkdir -p $(ELF_BIN_DIR_64)
	$(CC) $(LDFLAGS) \
		-o $(ELF_BIN_DIR_64)/$(ELF_TARGET_64) \
		$(CRT0) \
		$(ELF_OBJS_64) \
		-l:blibc.a
	$(call embed_icons,$(ELF_BIN_DIR_64)/$(ELF_TARGET_64))

$(ELF_OBJS_64): blibc

$(ELF_OBJ_DIR_64)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(COMMON_CFLAGS) -c $< -o $@

blibc:
	@echo "[BLIBC] Preparing blibc"
	@if [ ! -d "$(BLIBC_DIR)" ]; then \
		git clone $(BLIBC_REPO) $(BLIBC_DIR); \
	fi
	$(MAKE) -C $(BLIBC_DIR)

	@echo "[BLIBC] Syncing sysroot"
	@mkdir -p sysroot
	@cp -r $(BLIBC_DIR)/sysroot/* sysroot/

build: $(ELF_TARGET_64)

clean:
	rm -rf bin

distclean:
	rm -rf bin sysroot external

.PHONY: all clean distclean blibc
