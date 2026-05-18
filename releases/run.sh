#! /bin/sh

# Script utilisé pour démarrer le jeu depuis un launcher (desktop et menu).
# Ce script s'initialise automatiquement au premier démarrage (normalement).
# On peut l'appeler avec l'argument "reset" pour le réinitialiser.

INSTALLPATH=""

if [ -n $1 ] && [ "$1" = "reset" ]
then
	sed -i "s|INSTALLPATH=\"$INSTALLPATH\"|INSTALLPATH=\"\"|g" "$0"
elif [ -z $INSTALLPATH ]
then
	if [ $(find . -name Minesweeper -type f | wc -l) -eq 1 ] && [ $(find . -name source -type d | wc -l) -eq 1 ]
	then
		sed -i "s|INSTALLPATH=\"\"|INSTALLPATH=\"$(find . -name Minesweeper | head -1 | xargs realpath)\"|g" "$0"
		"$0"
	else
		echo "Il semble que l'exécutable du projet ou le dossier source n'ont pas pu être trouvés automatiquement."
		echo "Si vous avez bel et bien installé le jeu de Minesweeper à cet endroit, éditez ce fichier ($0) pour indiquer le chemin d'accès complet au dossier où se trouvent ces éléments."
		echo "Sinon, vous pouvez aussi vous assurez que ce script peut trouver ces éléments, notamment en essayant de l'exécuter dans le répertoire où vous avez installées les sources."
	fi
else
	cd "$INSTALL_PATH"
	./Minesweeper
fi
