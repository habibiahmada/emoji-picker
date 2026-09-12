CC ?= gcc
CFLAGS ?= -O2 -pipe -Wall -Wextra
GTK := $(shell pkg-config --cflags --libs gtk+-3.0)
X11 := $(shell pkg-config --cflags --libs x11)
PREFIX ?= $(HOME)/.local
BINDIR ?= $(PREFIX)/bin

SRC := src/main.c src/popup.c src/insert.c src/history.c src/generated/emoji_data.c
OBJ := $(SRC:.c=.o)

.PHONY: all clean install uninstall data dist

all: emoji-picker

data:
	chmod +x scripts/fetch-unicode-emoji.sh scripts/generate_emoji_data.py
	./scripts/fetch-unicode-emoji.sh

src/generated/emoji_data.c src/generated/emoji_data.h:
	$(MAKE) data

emoji-picker: $(SRC) src/generated/emoji_data.h
	$(CC) $(CFLAGS) -o $@ $(SRC) $(GTK) $(X11)

# Build versioned tarball under dist/ (does not publish).
# Usage: make dist VERSION=0.1.0
dist:
	@test -n "$(VERSION)" || (echo "Usage: make dist VERSION=0.1.0" >&2; exit 2)
	chmod +x scripts/release.sh
	./scripts/release.sh $(VERSION)

install: emoji-picker
	install -d $(BINDIR)
	install -m 755 emoji-picker $(BINDIR)/emoji-picker
	install -d $(HOME)/.config/systemd/user
	install -m 644 pack/emoji-picker.service $(HOME)/.config/systemd/user/emoji-picker.service
	@echo "Run: systemctl --user daemon-reload && systemctl --user enable --now emoji-picker.service"

uninstall:
	rm -f $(BINDIR)/emoji-picker
	systemctl --user disable --now emoji-picker.service 2>/dev/null || true
	rm -f $(HOME)/.config/systemd/user/emoji-picker.service

clean:
	rm -f emoji-picker $(OBJ)
	rm -rf dist
