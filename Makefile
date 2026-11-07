VERSION ?= 0.4.0
CC ?= gcc
CFLAGS ?= -O2 -pipe -Wall -Wextra
CPPFLAGS ?= -Isrc -Isrc/generated -DAPP_VERSION=\"$(VERSION)\"
GTK := $(shell pkg-config --cflags --libs gtk+-3.0)
GLIB := $(shell pkg-config --cflags --libs glib-2.0)
X11 := $(shell pkg-config --cflags --libs x11)
FONTCONFIG := $(shell pkg-config --cflags --libs fontconfig)
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
	src/model/skin_tone.c \
	src/insert/insert.c \
	src/platform/font_support.c \
	src/platform/x11_util.c \
	src/ui/picker_tone.c \
	src/ui/grid_layout.c \
	src/generated/emoji_data.c

.PHONY: all clean install uninstall data dist test update

all: emoji-picker

data:
	bash scripts/fetch-unicode-emoji.sh

src/generated/emoji_data.c src/generated/emoji_data.h:
	$(MAKE) data

emoji-picker: $(APP_SRC) src/generated/emoji_data.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $(APP_SRC) $(GTK) $(X11) $(FONTCONFIG)

test_search_query: tests/test_search_query.c src/model/search_query.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -Itests -o $@ \
		tests/test_search_query.c src/model/search_query.c $(GLIB)

test_history: tests/test_history.c src/model/history.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -Itests -o $@ \
		tests/test_history.c src/model/history.c $(GLIB)

test_emoji_model: tests/test_emoji_model.c src/model/emoji_model.c src/model/search_query.c src/model/history.c src/model/skin_tone.c src/generated/emoji_data.c src/generated/emoji_data.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -Itests -o $@ \
		tests/test_emoji_model.c src/model/emoji_model.c src/model/search_query.c \
		src/model/history.c src/model/skin_tone.c src/generated/emoji_data.c $(GTK) $(X11)

test_skin_tone: tests/test_skin_tone.c src/model/skin_tone.c src/generated/emoji_data.c src/generated/emoji_data.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -Itests -o $@ \
		tests/test_skin_tone.c src/model/skin_tone.c src/generated/emoji_data.c $(GLIB)

test_grid_layout: tests/test_grid_layout.c src/ui/grid_layout.c src/ui/grid_layout.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -Itests -o $@ \
		tests/test_grid_layout.c src/ui/grid_layout.c

test: test_search_query test_history test_emoji_model test_skin_tone test_grid_layout
	./test_search_query
	./test_history
	./test_emoji_model
	./test_skin_tone
	./test_grid_layout
	chmod +x tests/test_install.sh && ./tests/test_install.sh

dist:
	@test -n "$(VERSION)" || (echo "Usage: make dist VERSION=0.1.0" >&2; exit 2)
	bash scripts/release.sh $(VERSION)

install: emoji-picker
	chmod +x install.sh
	./install.sh --prefix=$(PREFIX)

uninstall:
	chmod +x install.sh
	./install.sh --uninstall --prefix=$(PREFIX)

update:
	chmod +x install.sh
	./install.sh --update --prefix=$(PREFIX)

clean:
	rm -f emoji-picker test_search_query test_history test_emoji_model test_skin_tone test_grid_layout
	rm -rf dist
