SDL_FLAGS = -lSDL2 -lSDL2_ttf -lSDL2_image # LDFLAGS pour SDL2 (nécessaires pour Windows et Linux)
WIN_FLAGS = -lmingw32 -lpthread -lSDL2main # LDFLAGS nécessaires pour Windows seulement
COMMON_FLAGS = -lm # LDFLAGS nécessaires pour Windows et Linux, sans compter ceux de SDL2
CFLAGS += -Wall -Wextra -Wno-switch -Wno-implicit-fallthrough # Il semblerait que mes switch-cases génère vraiment beaucoup de warnings...
MINGW = x86_64-w64-mingw32-gcc-win32 # Commande du compilateur mingw32 pour Windows


build: LDFLAGS += $(SDL_FLAGS) $(COMMON_FLAGS)
build:
	$(CC) minesweeper.c -o Minesweeper $(CFLAGS) $(LDFLAGS)

linux: build

build-windows: LDFLAGS += $(WIN_FLAGS) $(SDL_FLAGS) $(COMMON_FLAGS)
build-windows:
	$(MINGW) minesweeper.c -o Minesweeper.exe $(CFLAGS) $(LDFLAGS)

windows: build-windows

release: release-linux
release: release-windows

releases: release

release-linux: build
release-linux:
	mkdir Minesweeper_x86_64
	cp Minesweeper Minesweeper_x86_64
	cp -r releases/source Minesweeper_x86_64
	cp releases/run.sh Minesweeper_x86_64
	cp README.md Minesweeper_x86_64
	tar --create Minesweeper_x86_64 --file Minesweeper_x86_64.tar.xz
	mv Minesweeper_x86_64.tar.xz releases
	rm -r Minesweeper_x86_64

release-windows: build-windows
release-windows:
	mkdir Minesweeper_win
	mv Minesweeper.exe Minesweeper_win
	cp -r releases/source Minesweeper_win
	cp windows/*.dll Minesweeper_win
	cp README.md Minesweeper_win
	zip -r Minesweeper_win.zip Minesweeper_win
	mv Minesweeper_win.zip releases
	rm -r Minesweeper_win

clean:
	- rm Minesweeper
	- rm Minesweeper.exe
	- rm -r Minesweeper_win
	- rm -r Minesweeper_x86_64
	- rm releases/Minesweeper_x86_64.tar.xz
	- rm releases/Minesweeper_win.zip

.SILENT help:
	echo "Principales cibles pour ce projet:"
	echo "make build -> Compile le projet pour Linux"
	echo "make windows -> Compile le projet pour Windows"
	echo "make release -> Créé une release pour Linux et une autre pour Windows"
	echo "make clean -> Efface les exécutables et les releases qui ont été produites précédemment"
	echo "make help -> Affiche ce texte"
