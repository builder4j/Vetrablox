CC       = gcc
WIN64CC  = x86_64-w64-mingw32-gcc
WIN32CC  = i686-w64-mingw32-gcc
WINDRES  = i686-w64-mingw32-windres
MAGICK   = convert
CFLAGS   = -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -O2

BACKEND  = Server/Backend
LAUNCHER = Launcher
BUILD    = build

BACKEND_SRC = $(wildcard $(BACKEND)/*.c)
BACKEND_HDR = $(wildcard $(BACKEND)/*.h)

MHD_VER  = 1.0.1
MHD_NAME = libmicrohttpd-$(MHD_VER)-w32-bin
MHD_URL  = https://ftp.gnu.org/gnu/libmicrohttpd/$(MHD_NAME).zip
MHD_SHA  = f12c375b412956bee3814aea82b169229e0e673a708cc620e374012ff2bd779d
MHD_ZIP  = $(BUILD)/deps/$(MHD_NAME).zip
MHD_SUB  = $(MHD_NAME)/x86_64/MinGW/static/mingw64
MHD_WIN  = $(BUILD)/deps/mhd-win64
MHD_LIB  = $(MHD_WIN)/lib/libmicrohttpd.a

ICON_SRC = Assets/base\ v.png
ICON_ICO = $(BUILD)/vetrablox.ico
ICON_PNG = $(BUILD)/vetrablox.png
ICON_RC  = $(BUILD)/icon.rc
ICON_OBJ = $(BUILD)/icon.o

CLIENT_SRC = Client/Unpatched/2007 (0.3.807.0)
CLIENT_OUT = Client/Patched
PATCH_OFF  = 4388632
PATCH_OLD  = roblox.com
PATCH_NEW  = localhost
BASE_URL   = http://localhost

.PHONY: all linux win backend backend-win launcher launcher-win icon client run clean distclean

all: linux win

linux: backend launcher

win: backend-win launcher-win

backend: $(BACKEND)/backend

backend-win: $(BACKEND)/backend.exe

launcher: $(LAUNCHER)/vetrablox-launcher

launcher-win: $(LAUNCHER)/launcher.exe

icon: $(ICON_ICO) $(ICON_PNG)

$(BACKEND)/backend: $(BACKEND_SRC) $(BACKEND_HDR)
	$(CC) $(CFLAGS) $(BACKEND_SRC) -o $@ $$(pkg-config --cflags --libs libmicrohttpd) -pthread

$(BACKEND)/backend.exe: $(BACKEND_SRC) $(BACKEND_HDR) $(MHD_LIB)
	$(WIN64CC) $(CFLAGS) -I$(MHD_WIN)/include $(BACKEND_SRC) -o $@ -L$(MHD_WIN)/lib -lmicrohttpd -lws2_32 -pthread -static

$(MHD_ZIP):
	mkdir -p $(dir $@)
	curl -fL -o $@.part $(MHD_URL)
	echo "$(MHD_SHA)  $@.part" | sha256sum -c -
	mv $@.part $@

$(MHD_LIB): $(MHD_ZIP)
	rm -rf $(MHD_WIN) $(BUILD)/deps/unzip
	unzip -q $< '$(MHD_SUB)/*' -d $(BUILD)/deps/unzip
	mkdir -p $(MHD_WIN)
	cp -r $(BUILD)/deps/unzip/$(MHD_SUB)/include $(BUILD)/deps/unzip/$(MHD_SUB)/lib $(MHD_WIN)/
	rm -rf $(BUILD)/deps/unzip
	touch $@

$(LAUNCHER)/vetrablox-launcher: $(LAUNCHER)/main.c
	$(CC) $(CFLAGS) $< -o $@

$(LAUNCHER)/launcher.exe: $(LAUNCHER)/main.c $(ICON_OBJ)
	$(WIN32CC) $(CFLAGS) $^ -o $@ -mwindows -static

$(ICON_ICO): $(ICON_SRC)
	mkdir -p $(BUILD)
	$(MAGICK) "$<" -background none -gravity center -extent 548x548 -define icon:auto-resize=256,64,48,32,16 $@

$(ICON_PNG): $(ICON_SRC)
	mkdir -p $(BUILD)
	$(MAGICK) "$<" -background none -gravity center -extent 548x548 -resize 256x256 $@

$(ICON_RC): $(ICON_ICO)
	printf '1 ICON "%s"\n' "$(CURDIR)/$(ICON_ICO)" > $@

$(ICON_OBJ): $(ICON_RC)
	$(WINDRES) $< -O coff -o $@

client: win launcher $(ICON_PNG)
	rm -rf "$(CLIENT_OUT)"
	cp -r "$(CLIENT_SRC)" "$(CLIENT_OUT)"
	test "$$(dd if="$(CLIENT_OUT)/Roblox.exe" bs=1 skip=$(PATCH_OFF) count=10 2>/dev/null)" = "$(PATCH_OLD)"
	printf '$(PATCH_NEW)\0' | dd of="$(CLIENT_OUT)/Roblox.exe" bs=1 seek=$(PATCH_OFF) conv=notrunc 2>/dev/null
	printf '<Settings>\n\t<ContentFolder>content</ContentFolder>\n\t<BaseUrl>$(BASE_URL)</BaseUrl>\n</Settings>' > "$(CLIENT_OUT)/AppSettings.xml"
	cp $(LAUNCHER)/launcher.exe $(LAUNCHER)/vetrablox-launcher "$(CLIENT_OUT)/"
	cp $(ICON_PNG) "$(CLIENT_OUT)/"

run: backend
	cd $(BACKEND) && ./backend

clean:
	rm -f $(BACKEND)/backend $(BACKEND)/backend.exe $(LAUNCHER)/vetrablox-launcher $(LAUNCHER)/launcher.exe
	rm -f $(ICON_ICO) $(ICON_PNG) $(ICON_RC) $(ICON_OBJ)
	rm -rf "$(CLIENT_OUT)"

distclean: clean
	rm -rf $(BUILD)
