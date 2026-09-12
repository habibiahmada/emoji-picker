CC ?= gcc
CFLAGS ?= -O2 -pipe -Wall -Wextra
CPPFLAGS ?= -Isrc -Isrc/generated
GTK := $(shell pkg-config --cflags --libs gtk+-3.0)
GLIB := $(shell pkg-config --cflags --libs glib-2.0)
X11 := $(shell pkg-config --cflags --libs x11)
PREFIX ?= $(HOME)/.local
BINDIR ?= $(PREFIX)/bin

APP_SRC := \
	src/main.c \
	src/ui/popup.c \
	src/ui/picker_grid.c \
	src/ui/picker_dismiss.c \
	src/model/emoji_model.c \
	src/model/search_query.c \
	src/model/history.c \
	src/insert/insert.c \
	src/platform/x11_util.c \
	src/generated/emoji_data.c

.PHONY: all clean install uninstall data dist test

all: emoji-picker

data:
	chmod +x scripts/fetch-unicode-emoji.sh scripts/generate_emoji_data.py
	./scripts/fetch-unicode-emoji.sh

src/generated/emoji_data.c src/generated/emoji_data.h:
	$(MAKE) data

emoji-picker: $(APP_SRC) src/generated/emoji_data.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $(APP_SRC) $(GTK) $(X11)

test_search_query: tests/test_search_query.c src/model/search_query.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -Itests -o $@ \
		tests/test_search_query.c src/model/search_query.c $(GLIB)

test_history: tests/test_history.c src/model/history.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -Itests -o $@ \
		tests/test_history.c src/model/history.c $(GLIB)

test_emoji_model: tests/test_emoji_model.c src/model/emoji_model.c src/model/search_query.c src/model/history.c src/generated/emoji_data.c src/generated/emoji_data.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -Itests -o $@ \
		tests/test_emoji_model.c src/model/emoji_model.c src/model/search_query.c \
		src/model/history.c src/generated/emoji_data.c $(GTK) $(X11)

test: test_search_query test_history test_emoji_model
	./test_search_query
	./test_history
	./test_emoji_model

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
	rm -f emoji-picker test_search_query test_history test_emoji_model
	rm -rf dist
