# Jeu Graphique de Minesweeper

Développé en C avec SDL2

par Nicolas Audette

## Installation
- Sur Windows, suivez les instructions de la release v1.0.
- Sur un système Linux x86_64, vous pouvez suivre les instructions de la release v1.0.
- Sur un autre système, construisez manuellement le programme.

## Construction manuelle
Vous devriez pouvoir compiler le programme avec la commande `gcc minesweeper.c -o Minesweeper -lSDL2 -lSDL_image -lSDL_ttf -lm` sur à peu près n'importe quel système Linux.

Assurez-vous d'avoir d'abord installé les dépendances (`libsdl2`, `libsdl2_image`, `libsdl2_ttf` et `libsdl2-dev`, `libsdl2_image-dev`, `libsdl2_ttf-dev` pour la construction).

## Avancement
Le module du podium n'a pas encore été fait. Tout le reste du programme est terminé, mais quelques fonctionnalités pourraient être ajoutées à l'avenir.

## Notes

### Color picker

Les réglages de l'application font appel à un programme externe comme color picker. Si celui-ci ne fonctionne pas ou n'est pas installé, vous pouvez quand même modifier les couleurs directement via leurs valeurs rgba.

#### Sur Linux
Le color picker par défaut est celui de `zenity`, disponible sur la plupart des systèmes Linux.

Vous pouvez toutefois le changer pour un autre, mais celui-ci devra fournir le résultat sous format "rgb(R,G,B)" ou "rgba(R,G,B,A)". Il est aussi possible de désactiver le parsing de cette string, mais dans ce cas, l'output du color picker ne sera jamais transféré dans les variables rgba correspondantes. Il serait toutefois facile de remédier à cela. Désactiver cela permet aussi à l'application de ne pas crasher si zenity n'est pas installer. Vous devrez modifier manuellement le fichier `./source/reglages.txt` généré par le programme.

#### Sur Windows
Cette fonctionnalité est non-fonctionnelle sur Windows, mais il est inutile de désactiver le parsing de l'output de la commande.

### Dossier `source`

Ce dossier contient les images et les polices utilisées par le programme.

Il doit être placé au même endroit que l'exécutable, sans quoi le programme sera incapable d'afficher du texte autrement que par des pop-ups.

Puisque les chemins d'accès par défaut sont relatifs, veuillez utiliser le script `run.sh` pour démarrer l'application (surtout si vous vous créez un launcher ou un fichier `.desktop`).

### Utilisation du terminal

#### Sur Linux
Ce programme peut être exécuté via le terminal ou simplement en double-cliquant l'exécutable.

Lorsque démarré depuis le terminal, quelques paramètres peuvent lui être passés. Pour en connaître la liste, entrez `./Minesweeper -?`.

Les options de débogage (dont la ligne de commande interne) nécessitent que le programme ait été démarré depuis le terminal.

#### Sur Windows
L'exécutable de la release v1.0 a été construit de manière à ce qu'une fenêtre de terminal s'ouvre même si le programme est juste double-cliqué. Cependant, si l'exécutable est double-cliqué et que la codepage par défaut n'est pas UTF-8, l'affichage des caractères accentués sera brisé. On peut corriger cela en entrant `chcp 65001` dans le terminal juste avant de démarrer le programme (depuis ce même terminal).

Lorsque démarré depuis le terminal, quelques paramètres peuvent lui être passés. Pour en connaître la liste, entrez `Minesweeper.exe -?`. Les options suivent les standards d'options Unix, même sur Windows.

Les options de débogage (dont la ligne de commande interne) nécessitent que le programme ait été démarré depuis le terminal.

### Chronomètre

Le chronomètre roule sur un thread séparé, alors que tout le reste du programme tient en un seul thread (sauf le backend SDL). Si vous trouvez que le programme demande trop de puissance, essayez de désactiver le chronomètre. Il y a ici une assez grande place à amélioration...

### Icônes et symboles alternatifs

Vous pouvez utiliser les symboles par défaut pleins ou vides sans problèmes, mais utiliser vos propres icônes est beaucoup plus complexe...