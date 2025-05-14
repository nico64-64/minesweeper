#include <stdio.h>
#include <time.h>
#include "reglages.c"


//Structs & enums:
enum etat_case
//Liste des états que peut prendre une case (tuile) de la grille:
{
	//0 à 8 = 0 à 8 bombes adjacentes,
	bombe = 9,
	inconnu = 10 //case non-révélée qui n'est pas une bombe
};

typedef struct tuile //J'aurais préféré "case", mais c'est un mot réservé...
//Structure d'un carré dans la grille du jeu:
{
	enum etat_case etat; //état de la case (bombe? Sinon, combien de bombes adjacentes?) (le nombre de bombes adjacentes est inutilisé si la case n'est pas encore révélée)
	_Bool drapeau; //le joueur a-t-il marqué la case comme étant une bombe?
	_Bool revelee; //indique si le joueur a déjà révélé cette case
} tuile;

enum zone
//Liste des zones de la fenêtre où peut se trouver le focus:
{
	non_defini = 0, //à quelque part dans la fenêtre...
	sur_grille = 1, //sur une case de la grille (la position sur la grille est donnée par pos_grille_x[] et pos_grille_y[])
	bouton_podium = 2, //dans le menu ou au cours d'une partie
	bouton_pause,
	bouton_recommencer,
	bouton_nouvelle_partie,
	bouton_reglages, //dans le menu ou au cours d'une partie
	bouton_facile = 21,
	bouton_moyen,
	bouton_difficile,
	bouton_grille_perso,
	gp = 31, //gp = grille personalisée (création de celle-ci)
	gp_plus_col,
	gp_moins_col,
	gp_plus_lignes,
	gp_moins_lignes,
	gp_plus_bombes,
	gp_moins_bombes,
	gp_retour,
	gp_lancer
};


//Variables globales liées au graphisme du jeu:
SDL_Rect taille_nbre[9]; //array de rects donnant la taille (x en w et y en h) de chaque nbre, tel que définis à la ligne précédente
int taille = 0; //taille des carrés dans la grille
int marge_gauche = 0; //largeur de la marge à gauche de la grille
int marge_droite = 0; //début de la marge (contenant les boutons) à droite de la grille

//Variables globales liées à la grille:
int nbre_col = 16; //nbre de colonnes dans la grille
int nbre_lignes = 16; //nbre de lignes dans la grille
tuile** grille; //ptr vers la fameuse grille (les explications suivent une ligne plus bas...)
// \--> Il s'agit en fait d'un pointeur vers un pointeur vers une tuile/case de la grille.
//      Toutefois, "grille" sera plutôt utilisé comme un array de tuiles/cases, en 2D et de taille variable (grâce à calloc() (-> voir nouvelle_partie())).
int nbre_bombes = 40; //nbre de bombes cachées dans la grille
int nbre_drapeaux = 0; //nbre de drapeaux utilisés

//Variables globales liées à la sélection de zones par le joueur:
enum zone focus = 0; //zone de la fenêtre "sélectionnée" par le clavier (0 = aucun focus clavier)
int pos_grille_x[2] = {0, 0}; //position {clavier, souris} du focus en x sur la grille (s'il y a lieu)
int pos_grille_y[2] = {0, 0}; //position {clavier, souris} du focus en y sur la grille (s'il y a lieu)
int tuile_finale[2] = {-1, -1}; //coordonnées {x, y} (dans la grille) de la bombe cliquée par le joueur

//Variables globales (surtout des "flags") indiquant "l'état" de la partie / du jeu:
int nbre_tuiles_restantes = -1; //nbre de tuiles non-révélées qui ne sont pas des bombes ou des drapeaux
int pts = 0; //nbre de drapeaux placés au bon endroit
_Bool pause = 0; //indique si le jeu est en pause ou non
int fin_de_partie = 0; //indique si la partie est terminée ou pas (0 = en cours, 1 = partie terminée (victoire ou défaite pas encore déterminé), 2 = défaite, 3 = victoire)

//Variables globales liées à la "command line"
_Bool cmd_line = 0; //indique au jeu si le joueur est présentement en train d'utiliser la "command line"
char cmd[100] = "cmd: "; //commande entrée par le joueur dans la "command line"
_Bool recalcul = 0; //indique à la fct reveler_tuile() si elle doit recalculer la tuile même si elle est déjà révélée
_Bool debogage = 0; //permet d'afficher les coordonnées de chaque bombe dans la console au début de chaque partie

//Autres variables globales:
int ancien_nbre_col = 0; //ancien nbre de colonnes (pour la libération de la mémoire utilisée par la grille entre 2 parties sur des grilles de différentes tailles)
int erreur = 0; //code d'erreur


//Liste des fonctions:
void gestion_param(char arg[]); //gère les paramètres reçus par l'application à son ouverture
int init(); //initialise et démarre l'application
void menu(); //gère le menu principal de l'application
_Bool grille_perso(); //permet la création d'une grille de taille variable
void rafraichir_menu(enum zone); //redessine le menu principal
void nouvelle_partie(); //démarre une nouvelle partie
void partie(); //loop gérant les events pendant une partie
void rafraichir(enum zone); //redessine la fenêtre du jeu
_Bool reveler_tuile(int, int); //révèle une tuile si elle n'est pas une bombe
void executer_cmd(); //exécute la commande entrée dans la "command line"
void quitter(); //ferme le programme


int main (int argc, char *argv[])
{
	char msg_erreur[200] = "Aucune erreur."; //message d'erreur correspondant à afficher
	SDL_Event ev;
	
	//Données du pop-up de confirmation pour fermer l'application (déclaré ici (et ailleurs) afin d'éviter une erreur comme quoi "fenetre" n'est pas cst...):
	int choix_popup_quitter = 0;
	SDL_MessageBoxButtonData boutons_popup_quitter[2] =
	{
		{SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Annuler"},
		{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Quitter"}
	};
	SDL_MessageBoxData popup_quitter =
	{
		SDL_MESSAGEBOX_INFORMATION,
		fenetre,
		"Voulez-vous vraiment quitter?",
		"La partie en cours sera perdue.",
		2,
		boutons_popup_quitter,
		NULL
	};
	
	
	//Gestion des arguments reçus par le programme:
	for (int num_arg = 1; num_arg < argc; num_arg++)
	{gestion_param(argv[num_arg]);}
	
	
	//Démarrage et initialisation du programme et de SDL:
	printf("Jeu de Minesweeper codé en C.\nVersion %s\n---\n", VERSION);
	erreur = init();
	
	//Erreur fatale lors de l'ouverture/initialisation du programme:
	if (erreur < 0 && erreur >= -5)
	{
		sprintf(msg_erreur, "Erreur fatale %d: impossible de démarrer l'application.\nOuvrez l'application depuis la console pour en savoir plus.", -erreur);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Erreur", msg_erreur, NULL); //affiche un pop-up d'erreur
		printf("Le programme s'est terminé avec le code d'erreur %d.\n", -erreur);
		return erreur;
	}
	
	//Affichage du menu principal:
	menu();
	
	//Ne devrait jamais rouler:
	quitter(-100);
}


void gestion_param(char arg[])
//Gère les paramètres reçus par l'application à son ouverture.
{
	if (!strcmp(arg, "-?") || !strcmp(arg, "-a") || !strcmp(arg, "--aide"))
	{
		printf("Jeu de Minesweeper codé en C.\n\nVoici la liste des options que peut recevoir le programme à son démarrage:\n");
		printf("--aide (-a ou -?)  affiche ce texte, puis quitte\n");
		printf("--version (-v)     affiche la version du programme, puis quitte\n");
		exit(0);
	}
	
	else if (!strcmp(arg, "-v") || !strcmp(arg, "--version"))
	{printf("Jeu de Minesweeper codé en C.\nVersion %s\n", VERSION); exit(0);}
}


int init()
//Initialise le programme (appelé une seule fois, au début).
//Renvoie un code d'erreur négatif ou 0 en cas de succès.
{
    SDL_Surface* icone; //surface contenant l'icone de l'application, qui sera assignée à la fenêtre de celle-ci
	SDL_Surface* surface_nbre; //surface qui contiendra un nombre (texte) à transformer en texture
	char nbre_a_afficher[5] = "?"; //string qui contiendra le nbre à transformer en texture
	
	
	//Initialisation et démarrage de SDL et compagnie:
	
	if (SDL_Init(SDL_INIT_VIDEO) < 0) //initialisation de SDL
	{printf("Erreur lors de l'initialisation de SDL.\n(%s)\n", SDL_GetError()); return -1;}
	
	if (TTF_Init() < 0) //initialisation de SDL_ttf
	{printf("Erreur lors de l'initialisation de SDL_ttf.\n(%s)\n", TTF_GetError()); SDL_Quit(); return -4;}
	
	//Chargement des polices ttf:
	police = TTF_OpenFont(nom_police, taille_police_normale);
	if (police == NULL)
	{printf("Erreur lors du chargement de la police ttf:\n%s\n", TTF_GetError()); TTF_Quit(); SDL_Quit(); return -5;}
	petite_police = TTF_OpenFont(nom_petite_police, taille_petite_police);
	if (petite_police == NULL) //erreur non-fatale (cette police n'est quand même pas très utilisée...)
	{printf("Erreur 15: Impossible de créer la petite police (%s).\n", TTF_GetError());}
	
	//Création de la fenêtre:
	fenetre = SDL_CreateWindow("Minesweeper", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, largeur_fenetre[0], hauteur_fenetre[0], SDL_WINDOW_RESIZABLE);
	if (fenetre == NULL)
	{printf("Erreur lors de la création de la fenêtre SDL:\n%s\n", SDL_GetError()); TTF_CloseFont(police); TTF_Quit(); SDL_Quit(); return -2;}
	
	//Création du renderer:
	rend = SDL_CreateRenderer(fenetre, -1, 0);
	if (rend == NULL)
	{printf("Erreur lors de la création du renderer via SDL:\n%s\n", SDL_GetError()); TTF_CloseFont(police); TTF_Quit(); SDL_DestroyWindow(fenetre); SDL_Quit(); return -3;}
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND); //permet l'utilisation de couleurs semi-transparentes (et transparentes)
	
	//Taille minimale de la fenêtre:
	SDL_SetWindowMinimumSize(fenetre, 650, 500); //doit être placé après la création du renderer pour que ça marche (bug)
	
	//Création des différents curseurs:
	curseur_normal = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	curseur_txt = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
	curseur_clic = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
	if (curseur_normal == NULL || curseur_txt == NULL || curseur_clic == NULL) //erreur non-fatale (au pire, on restera stuck avec un curseur normal...)
	{printf("Erreur lors de la création des curseurs système texte et normal: %s.\n\n", SDL_GetError()); erreur = -10;}
	
	//Création de l'icone de la fenêtre (À FAIRE!):
	//icone = IMG_Load("./source/icone.png");
	//if (icone == NULL) //erreur non-fatale (au pire, on va s'en passer, de l'icone...)
	//{printf("Erreur lors de l'assignation de l'icone à la fenêtre du jeu (%s).\n\n", SDL_GetError()); erreur = -11;}
	//else
	//{SDL_SetWindowIcon(fenetre, icone); SDL_FreeSurface(icone);}
	
	
	//Création et initialisation des textures du programme:
	
	//Création de la texture des icones et des symboles:
	//(je fais ça tout de suite, parce que j'imagine que ça va sauver du temps vs le refaire à chaque frame...)
	texture_icone_podium = IMG_LoadTexture(rend, icone_podium);
	texture_symbole_pause = IMG_LoadTexture(rend, symbole_pause);
	texture_symbole_fin_de_partie = IMG_LoadTexture(rend, symbole_fin_de_partie);
	texture_symbole_fin_de_partie_defaite = IMG_LoadTexture(rend, symbole_fin_de_partie);
	texture_bombe = IMG_LoadTexture(rend, image_bombe);
	texture_bombe_finale = IMG_LoadTexture(rend, image_bombe);
	texture_drapeau = IMG_LoadTexture(rend, icone_drapeau);
	texture_drapeau_mal_place = IMG_LoadTexture(rend, icone_drapeau);
	
	//Coloration des textures (et vérification de leur existence...):
	//(Les images qu'on a loadées sont dessinées en blanc, ce qui nous permet de changer très facilement leur couleur en la multipliant par la couleur désirée.)
	if (texture_icone_podium == NULL) //erreur non-fatale (au pire, le bouton sera vide...)
	{printf("Erreur lors du chargement de l'icone podium (%s).\nLe dossier \"source\" a-t-il été altéré ou déplacé?\n\n", SDL_GetError()); erreur = -12;}
	else
	{SDL_SetTextureColorMod(texture_icone_podium, 0, 0, 0);}
	if (texture_symbole_pause == NULL) //erreur non-fatale (au pire, on ne saura jamais si le jeu est en pause (sauf via le timer...))
	{printf("Erreur lors du chargement du symbole \"pause\" (%s).\nLe dossier \"source\" a-t-il été altéré ou déplacé?\n\n", SDL_GetError()); erreur = -12;}
	else
	{SDL_SetTextureColorMod(texture_symbole_pause, 0, 0, 0);}
	if (texture_symbole_fin_de_partie == NULL || texture_symbole_fin_de_partie_defaite == NULL) //erreur non-fatale
	{printf("Erreur lors du chargement du symbole de fin de partie (%s).\nLe dossier \"source\" a-t-il été altéré ou déplacé?\n\n", SDL_GetError()); erreur = -12;}
	else
	{SDL_SetTextureColorMod(texture_symbole_fin_de_partie, 0, 0, 0); SDL_SetTextureColorMod(texture_symbole_fin_de_partie_defaite, 143, 23, 23);}
	if (texture_bombe == NULL || texture_bombe_finale == NULL) //erreur non-fatale (au pire, on ne verra jamais les bombes (cases vides)...)
	{printf("Erreur lors du chargement de l'image d'une mine (%s).\nLe dossier \"source\" a-t-il été altéré ou déplacé?\n\n", SDL_GetError()); erreur = -12;}
	else
	{SDL_SetTextureColorMod(texture_bombe, 0, 0, 0); SDL_SetTextureColorMod(texture_bombe_finale, 143, 23, 23);}
	if (texture_drapeau == NULL || texture_drapeau_mal_place == NULL) //erreur presque fatale (c'est quand même grave, si on voit pas les drapeaux...)
	{
		printf("Erreur lors du chargement de l'icone drapeau (%s).\nLe dossier \"source\" a-t-il été altéré ou déplacé?\n\n", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Erreur", "L'icone drapeau n'a pas pu être chargée.\nUn \"X\" remplacera donc les drapeaux.\nConsultez la console pour plus de détails.", NULL);
		erreur = -13;
	}
	else
	{SDL_SetTextureColorMod(texture_drapeau, 0, 0, 0); SDL_SetTextureColorMod(texture_drapeau_mal_place, 143, 23, 23);}
		
	//Création des texture des nbres qui indiqueront combien de bombes sont adjacentes à une tuile:
	//(faire ça une seule fois et à l'avance me sauve BCP de lag plus tard...)
	for (int compteur = 1 - afficher_zeros; compteur <= 8; compteur++)
	{
		sprintf(nbre_a_afficher, "%d", compteur);
		surface_nbre = TTF_RenderUTF8_Solid_Wrapped(police, nbre_a_afficher, couleur_score, taille);
		taille_nbre[compteur].w = surface_nbre->w;
		taille_nbre[compteur].h = surface_nbre->h;
		texture_nbre[compteur] = SDL_CreateTextureFromSurface(rend, surface_nbre);
		SDL_FreeSurface(surface_nbre);
	}
	
	return 0;
}


void menu ()
//Gère le menu principal:
{
	SDL_Event ev;
	
	int choix_popup_quitter = 0;
	SDL_MessageBoxButtonData boutons_popup_quitter[2] =
	{
		{SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Non"},
		{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Oui"}
	};
	SDL_MessageBoxData popup_quitter =
	{
		SDL_MESSAGEBOX_INFORMATION,
		fenetre,
		"Voulez-vous vraiment quitter?",
		"Voulez-vous vraiment fermer l'application?",
		2,
		boutons_popup_quitter,
		NULL
	};
	
	
	while (1)
	{
		SDL_WaitEvent(&ev);
		
		switch (ev.type)
		{
		case SDL_QUIT:
			quitter(0);
			break;
		
		case SDL_WINDOWEVENT:
			//Pour gérer plus en détail les windowevents, on peut switch(ev.window.event) et ainsi trouver exactement ce qui vient de se passer.
			//(wiki.libsdl.org/SDL2/SDL_WindowEventID = valeur que peut prendre ev.window.event)
			//Pour le moment, je vais seulement trouver la nouvelle taille de la fenêtre et la redessiner.
			SDL_GetWindowSize(fenetre, &xmax, &ymax);
			rafraichir_menu(0);
			break;
		
		case SDL_KEYDOWN:
			switch (ev.key.keysym.sym)
			{
			case SDLK_ESCAPE:
				SDL_ShowMessageBox(&popup_quitter, &choix_popup_quitter);
				if (choix_popup_quitter)
				{quitter(0);}
				break;
			
			case SDLK_RETURN:
			case SDLK_KP_ENTER:
			case SDLK_SPACE:
				switch (focus)
				{
				case bouton_facile:
					nbre_col = 8;
					nbre_lignes = 8;
					nbre_bombes = 10;
					partie();
					break;
				
				case bouton_moyen:
					nbre_col = 16;
					nbre_lignes = 16;
					nbre_bombes = 16;
					partie();
					break;
				
				case bouton_difficile:
					nbre_col = 30;
					nbre_lignes = 20;
					nbre_bombes = 100;
					partie();
					break;
				
				case bouton_grille_perso:
					if (grille_perso())
					{partie();}
					else
					{focus = bouton_grille_perso; rafraichir_menu(0);}
					break;
				
				case bouton_podium:
					//À faire!
					break;
				
				case bouton_reglages:
					reglages();
					rafraichir_menu(0);
					break;
				}
				break;
			
			case SDLK_UP:
				if (focus > bouton_facile && focus <= bouton_grille_perso)
				{focus--;}
				else if (focus == bouton_facile)
				{focus = bouton_grille_perso;}
				else if (focus == bouton_podium)
				{focus = bouton_reglages;}
				else if (focus == bouton_reglages)
				{focus = bouton_podium;}
				else if (focus == non_defini)
				{focus = bouton_facile;}
				rafraichir_menu(0);
				break;
			
			case SDLK_DOWN:
				if (focus >= bouton_facile && focus < bouton_grille_perso)
				{focus++;}
				else if (focus == bouton_grille_perso || focus == non_defini)
				{focus = bouton_facile;}
				else if (focus == bouton_podium)
				{focus = bouton_reglages;}
				else if (focus == bouton_reglages)
				{focus = bouton_podium;}
				rafraichir_menu(0);
				break;
			
			case SDLK_RIGHT:
			case SDLK_LEFT:
				if (focus >= bouton_facile && focus <= bouton_grille_perso)
				{focus = bouton_podium;}
				else if (focus == bouton_podium || focus == bouton_reglages || focus == non_defini)
				{focus = bouton_facile;}
				rafraichir_menu(0);
				break;
			
			case SDLK_TAB:
				if (focus >= bouton_facile && focus <= bouton_grille_perso)
				{focus = bouton_podium;}
				else if (focus == bouton_podium)
				{focus = bouton_reglages;}
				else //bouton réglages ou focus non-defini
				{focus = bouton_facile;}
				rafraichir_menu(0);
				break;
			}
			break;
		
		case SDL_MOUSEMOTION:
			if (ev.motion.x >= xmax / 3 && ev.motion.x <= 2 * xmax / 3)
			{
				if (ev.motion.y >= ymax / 2 - 230 && ev.motion.y <= ymax / 2 - 130) //bouton "facile"
				{rafraichir_menu(bouton_facile);}
				else if (ev.motion.y >= ymax / 2 - 100 && ev.motion.y <= ymax / 2) //bouton "moyen"
				{rafraichir_menu(bouton_moyen);}
				else if (ev.motion.y >= ymax / 2 + 30 && ev.motion.y <= ymax / 2 + 130) //bouton "difficile"
				{rafraichir_menu(bouton_difficile);}
				else if (ev.motion.y >= ymax / 2 + 160 && ev.motion.y <= ymax / 2 + 230) //bouton "grille personalisée"
				{rafraichir_menu(bouton_grille_perso);}
				else
				{rafraichir_menu(0);}
			}
			else if (ev.motion.x >= xmax - 85 && ev.motion.x <= xmax - 23 && ev.motion.y >= 20 && ev.motion.y <= 62) //bouton podium
			{rafraichir_menu(bouton_podium);}
			else if (ev.motion.x >= xmax - 170 && ev.motion.x <= xmax - 20 && ev.motion.y >= ymax - 60 && ev.motion.y <= ymax - 20) //bouton réglages
			{rafraichir_menu(bouton_reglages);}
			else
			{rafraichir_menu(0);}
			break;
		
		case SDL_MOUSEBUTTONDOWN:
			if (ev.button.x >= xmax / 3 && ev.button.x <= 2 * xmax / 3)
			{
				if (ev.button.y >= ymax / 2 - 230 && ev.button.y <= ymax / 2 - 130) //bouton "facile"
				{nbre_col = 8; nbre_lignes = 8; nbre_bombes = 10; partie();}
				else if (ev.button.y >= ymax / 2 - 100 && ev.button.y <= ymax / 2) //bouton "moyen"
				{nbre_col = 16; nbre_lignes = 16; nbre_bombes = 40; partie();}
				else if (ev.button.y >= ymax / 2 + 30 && ev.button.y <= ymax / 2 + 130) //bouton "difficile"
				{nbre_col = 30; nbre_lignes = 20; nbre_bombes = 100; partie();}
				else if (ev.button.y >= ymax / 2 + 160 && ev.button.y <= ymax / 2 + 230) //bouton "grille personalisée"
				{
					if (grille_perso())
					{partie();}
					else
					{rafraichir_menu(0); focus = non_defini;}
				}
			}
			else if (ev.button.x >= xmax - 85 && ev.button.x <= xmax - 23 && ev.button.y >= 20 && ev.button.y <= 62) //bouton podium
			{/*À faire!*/}
			else if (ev.button.x >= xmax - 170 && ev.button.x <= xmax - 20 && ev.button.y >= ymax - 60 && ev.button.y <= ymax - 20) //bouton réglages
			{reglages(); rafraichir_menu(0);}
			break;
		}
	}
}


_Bool grille_perso ()
//Permet au joueur de se créer une grille de taille variable et de choisir le nombre de bombes qui s'y trouveront.
//Renvoie 1 pour lancer une nouvelle partie ou 0 si l'utilisateur a changé d'avis.
{
	SDL_Event ev;
	
	rafraichir_menu(gp); //dessine d'abord le "pop-up"...
	
	while (1)
	{
		SDL_WaitEvent(&ev);
		
		switch (ev.type)
		{
		case SDL_QUIT:
			quitter(0);
			break;
		
		case SDL_WINDOWEVENT:
			SDL_GetWindowSize(fenetre, &xmax, &ymax);
			rafraichir_menu(gp);
			break;
		
		case SDL_KEYDOWN:
			switch (ev.key.keysym.sym)
			{
			case SDLK_ESCAPE:
				return 0;
				break;
			
			case SDLK_RETURN:
			case SDLK_KP_ENTER:
			case SDLK_SPACE:
				switch (focus)
				{
				case gp_plus_col:
					if (nbre_col < 50)
					{nbre_col++;}
					break;
				
				case gp_moins_col:
					if (nbre_col > 2)
					{nbre_col--;}
					break;
				
				case gp_plus_lignes:
					if (nbre_lignes < 50)
					{nbre_lignes++;}
					break;
				
				case gp_moins_lignes:
					if (nbre_lignes > 2)
					{nbre_lignes--;}
					break;
				
				case gp_plus_bombes:
					nbre_bombes++;
					break;
				
				case gp_moins_bombes:
					if (nbre_bombes > 1)
					{nbre_bombes--;}
					break;
				
				case gp_retour:
					return 0;
					break;
				
				case gp_lancer:
					if (nbre_bombes <= nbre_col * nbre_lignes)
					{return 1;}
					break;
				}
				break;
			
			case SDLK_TAB:
			case SDLK_RIGHT:
				if (focus >= gp_plus_col && focus < gp_plus_bombes)
				{focus += 2;}
				else if (focus == gp_plus_bombes || focus == gp_moins_bombes)
				{
					if (ev.key.keysym.sym == SDLK_RIGHT)
					{focus = gp_plus_col + focus - gp_plus_bombes;}
					else
					{focus = gp_retour;}
				}
				else if (focus == gp_retour)
				{focus = gp_lancer;}
				else if (ev.key.keysym.sym == SDLK_RIGHT && focus == gp_lancer)
				{focus = gp_retour;}
				else //donc focus = gp_lancer ou non-défini
				{focus = gp_plus_col;}
				break;
			
			case SDLK_LEFT:
				if (focus > gp_moins_col && focus <= gp_moins_bombes)
				{focus -= 2;}
				else if (focus == gp_plus_col || focus == gp_moins_col)
				{focus = gp_plus_bombes + focus - gp_plus_col;}
				else if (focus == gp_retour)
				{focus = gp_lancer;}
				else if (focus == gp_lancer)
				{focus = gp_retour;}
				else //donc focus = non-defini:
				{focus = gp_plus_col;}
				break;
			
			case SDLK_UP:
			case SDLK_DOWN:
				if (focus == gp_moins_col || focus == gp_moins_lignes || focus == gp_moins_bombes)
				{focus--;}
				else if (focus == gp_plus_col || focus == gp_plus_lignes || focus == gp_plus_bombes)
				{focus++;}
				else //donc focus = bouton retour ou lancer OU non-défini
				{focus = gp_plus_col;}
				break;
			}
			rafraichir_menu(gp);
			break;
		
		case SDL_MOUSEMOTION:
			if (ev.motion.x >= xmax / 2 - 190 && ev.motion.y >= ymax / 2 - 90 && ev.motion.x <= xmax / 2 - 110 && ev.motion.y <= ymax / 2 - 45)
			{rafraichir_menu(gp_plus_col);}
			else if (ev.motion.x >= xmax / 2 - 40 && ev.motion.y >= ymax / 2 - 90 && ev.motion.x <= xmax / 2 + 40 && ev.motion.y <= ymax / 2 - 45)
			{rafraichir_menu(gp_plus_lignes);}
			else if (ev.motion.x >= xmax / 2 + 110 && ev.motion.y >= ymax / 2 - 90 && ev.motion.x <= xmax / 2 + 190 && ev.motion.y <= ymax / 2 - 45)
			{rafraichir_menu(gp_plus_bombes);}
			else if (ev.motion.x >= xmax / 2 - 190 && ev.motion.y >= ymax / 2 + 15 && ev.motion.x <= xmax / 2 - 110 && ev.motion.y <= ymax / 2 + 60)
			{rafraichir_menu(gp_moins_col);}
			else if (ev.motion.x >= xmax / 2 - 40 && ev.motion.y >= ymax / 2 + 15 && ev.motion.x <= xmax / 2 + 40 && ev.motion.y <= ymax / 2 + 60)
			{rafraichir_menu(gp_moins_lignes);}
			else if (ev.motion.x >= xmax / 2 + 110 && ev.motion.y >= ymax / 2 + 15 && ev.motion.x <= xmax / 2 + 190 && ev.motion.y <= ymax / 2 + 60)
			{rafraichir_menu(gp_moins_bombes);}
			else if (ev.motion.x >= xmax / 6 + 30 && ev.motion.y >= 4 * ymax / 5 - 60 && ev.motion.x <= xmax / 6 + 180 && ev.motion.y <= 4 * ymax / 5 - 20)
			{rafraichir_menu(gp_retour);}
			else if (ev.motion.x >= 5 * xmax / 6 - 180 && ev.motion.y >= 4 * ymax / 5 - 60 && ev.motion.x <= 5 * xmax / 6 - 30 && ev.motion.y <= 4 * ymax / 5 - 20)
			{rafraichir_menu(gp_lancer);}
			else
			{rafraichir_menu(gp);}
			break;
		
		case SDL_MOUSEBUTTONDOWN:
			focus = gp; //perte du focus clavier
			if (ev.motion.x >= xmax / 2 - 190 && ev.motion.y >= ymax / 2 - 90 && ev.motion.x <= xmax / 2 - 110 && ev.motion.y <= ymax / 2 - 45)
			{nbre_col++; rafraichir_menu(gp_plus_col);}
			else if (ev.motion.x >= xmax / 2 - 40 && ev.motion.y >= ymax / 2 - 90 && ev.motion.x <= xmax / 2 + 40 && ev.motion.y <= ymax / 2 - 45)
			{nbre_lignes++; rafraichir_menu(gp_plus_lignes);}
			else if (ev.motion.x >= xmax / 2 + 110 && ev.motion.y >= ymax / 2 - 90 && ev.motion.x <= xmax / 2 + 190 && ev.motion.y <= ymax / 2 - 45)
			{nbre_bombes++; rafraichir_menu(gp_plus_bombes);}
			else if (ev.motion.x >= xmax / 2 - 190 && ev.motion.y >= ymax / 2 + 15 && ev.motion.x <= xmax / 2 - 110 && ev.motion.y <= ymax / 2 + 60 && nbre_col > 2)
			{nbre_col--; rafraichir_menu(gp_moins_col);}
			else if (ev.motion.x >= xmax / 2 - 40 && ev.motion.y >= ymax / 2 + 15 && ev.motion.x <= xmax / 2 + 40 && ev.motion.y <= ymax / 2 + 60 && nbre_lignes > 2)
			{nbre_lignes--; rafraichir_menu(gp_moins_lignes);}
			else if (ev.motion.x >= xmax / 2 + 110 && ev.motion.y >= ymax / 2 + 15 && ev.motion.x <= xmax / 2 + 190 && ev.motion.y <= ymax / 2 + 60 && nbre_bombes > 1)
			{nbre_bombes--; rafraichir_menu(gp_moins_bombes);}
			else if (ev.motion.x >= xmax / 6 + 30 && ev.motion.y >= 4 * ymax / 5 - 60 && ev.motion.x <= xmax / 6 + 180 && ev.motion.y <= 4 * ymax / 5 - 20)
			{return 0;}
			else if (ev.motion.x >= 5 * xmax / 6 - 180 && ev.motion.y >= 4 * ymax / 5 - 60 && ev.motion.x <= 5 * xmax / 6 - 30 && ev.motion.y <= 4 * ymax / 5 - 20 && nbre_bombes <= nbre_col * nbre_lignes)
			{return 1;}
			break;
		}
	}
}


void rafraichir_menu (enum zone curseur)
//Redessine le menu principal.
//Reçoit en paramètre la "zone" de l'écran où se trouve le curseur.
{
	SDL_Rect rect_icone_podium = {xmax - 74, 15, 41, 41}; //41, parce que 40 détruit complètement l'icone...
	char buffer[5] = "???";
	
	//Arrière-plan de la fenêtre:
	SDL_SetColor(fond, rend);
	SDL_RenderClear(rend);
	
	//Dessin des boutons:
	rect_arrondi(xmax / 3, ymax / 2 - 230, xmax / 3, 100, couleur_boutons, fond, rend); //facile
	rect_arrondi(xmax / 3, ymax / 2 - 100, xmax / 3, 100, couleur_boutons, fond, rend); //moyen
	rect_arrondi(xmax / 3, ymax / 2 + 30, xmax / 3, 100, couleur_boutons, fond, rend); //difficile
	rect_arrondi(xmax / 3, ymax / 2 + 160, xmax / 3, 70, couleur_boutons, fond, rend); //personnalisé
	
	rect_arrondi(xmax - 85, 20, 60, 40, couleur_boutons, fond, rend); //bouton "podium"
	rect_arrondi(xmax - 170, ymax - 60, 150, 40, couleur_boutons, fond, rend); //bouton réglages
	
	if (curseur < gp) //permet de diminuer le lag lors de la création d'une grille personalisée (je crois...)
	{
		//Affiche la sélection clavier:
		switch (focus)
		{
		case bouton_podium:
			rect_arrondi(xmax - 85, 20, 60, 40, couleur_selection_clavier, fond, rend);
			break;
		
		case bouton_facile:
		case bouton_moyen:
		case bouton_difficile:
			rect_arrondi(xmax / 3, ymax / 2 - 230 + 130 * (focus - bouton_facile), xmax / 3, 100, couleur_selection_clavier, fond, rend);
			break;
		
		case bouton_grille_perso:
			rect_arrondi(xmax / 3, ymax / 2 + 160, xmax / 3, 70, couleur_selection_clavier, fond, rend);
			break;
		
		case bouton_reglages:
			rect_arrondi(xmax - 170, ymax - 60, 150, 40, couleur_selection_clavier, fond, rend);
			break;
		}
		
		//Affiche la sélection curseur:
		switch (curseur)
		{
		case bouton_podium:
			rect_arrondi(xmax - 85, 20, 60, 40, couleur_selection_curseur, fond, rend);
			break;
		
		case bouton_facile:
		case bouton_moyen:
		case bouton_difficile:
			rect_arrondi(xmax / 3, ymax / 2 - 230 + 130 * (curseur - bouton_facile), xmax / 3, 100, couleur_selection_curseur, fond, rend);
			break;
		
		case bouton_grille_perso:
			rect_arrondi(xmax / 3, ymax / 2 + 160, xmax / 3, 70, couleur_selection_curseur, fond, rend);
			break;
		
		case bouton_reglages:
			rect_arrondi(xmax - 170, ymax - 60, 150, 40, couleur_selection_curseur, fond, rend);
			break;
		}
	}
	
	//Affichage du symbole "podium" sur son bouton:
	if (texture_icone_podium != NULL)
	{SDL_RenderCopy(rend, texture_icone_podium, NULL, &rect_icone_podium);}
	
	//Affichage du texte sur les boutons:
	afficher_txt_centre("Difficulté facile:", xmax / 3, 2* xmax / 3, ymax / 2 - 220, police, couleur_txt_boutons, rend);
	afficher_txt_centre("8 x 8", xmax / 3, 2* xmax / 3, ymax / 2 - 190, police, couleur_txt_boutons, rend);
	afficher_txt_centre("10 mines", xmax / 3, 2* xmax / 3, ymax / 2 - 160, police, couleur_txt_boutons, rend);
	afficher_txt_centre("Difficulté moyenne:", xmax / 3, 2* xmax / 3, ymax / 2 - 90, police, couleur_txt_boutons, rend);
	afficher_txt_centre("16 x 16", xmax / 3, 2* xmax / 3, ymax / 2 - 60, police, couleur_txt_boutons, rend);
	afficher_txt_centre("40 mines", xmax / 3, 2* xmax / 3, ymax / 2 - 30, police, couleur_txt_boutons, rend);
	afficher_txt_centre("Difficulté élevée:", xmax / 3, 2* xmax / 3, ymax / 2 + 40, police, couleur_txt_boutons, rend);
	afficher_txt_centre("30 x 20", xmax / 3, 2* xmax / 3, ymax / 2 + 70, police, couleur_txt_boutons, rend);
	afficher_txt_centre("100 mines", xmax / 3, 2* xmax / 3, ymax / 2 + 100, police, couleur_txt_boutons, rend);
	afficher_txt_centre("Grille Personnalisée", xmax / 3, 2* xmax / 3, ymax / 2 + 185, police, couleur_txt_boutons, rend);
	afficher_txt_centre("réglages", xmax - 170, xmax - 20, ymax - 52, police, couleur_txt_boutons, rend);
	
	//Dessin de l'interface de création d'une grille personnalisée
	if (curseur >= gp)
	{
		//Dessin du "pop-up":
		rectangle(xmax / 6, ymax / 5, 2 * xmax / 3, 3 * ymax / 5, 0, couleur_boutons, fond, rend);
		rectangle(xmax / 6, ymax / 5, 2 * xmax / 3, 3 * ymax / 5, 5, couleur_timer, fond, rend);
		
		//Titres des 3 modules:
		afficher_txt_centre("Nombre de", xmax / 2 - 200, xmax / 2 - 100, ymax / 2 - 140, petite_police, couleur_txt_boutons, rend);
		afficher_txt_centre("colonnes:", xmax / 2 - 200, xmax / 2 - 100, ymax / 2 - 120, petite_police, couleur_txt_boutons, rend);
		afficher_txt_centre("Nombre de", xmax / 2 - 100, xmax / 2 + 100, ymax / 2 - 140, petite_police, couleur_txt_boutons, rend);
		afficher_txt_centre("lignes:", xmax / 2 - 100, xmax / 2 + 100, ymax / 2 - 120, petite_police, couleur_txt_boutons, rend);
		afficher_txt_centre("Nombre de", xmax / 2 + 100, xmax / 2 + 200, ymax / 2 - 140, petite_police, couleur_txt_boutons, rend);
		afficher_txt_centre("mines:", xmax / 2 + 100, xmax / 2 + 200, ymax / 2 - 120, petite_police, couleur_txt_boutons, rend);
		
		//Dessin des 3 modules:
		for (int compteur = 0; compteur < 3; compteur++)
		{
			rectangle(xmax / 2 - 190 + 150 * compteur, ymax / 2 - 90, 80, 45, 0, fond, couleur_boutons, rend); //haut
			if (curseur == gp_plus_col + compteur * 2)
			{rectangle(xmax / 2 - 190 + 150 * compteur, ymax / 2 - 90, 80, 45, 0, couleur_selection_curseur, couleur_boutons, rend);}
			else if (focus == gp_plus_col + compteur * 2)
			{rectangle(xmax / 2 - 190 + 150 * compteur, ymax / 2 - 90, 80, 45, 0, couleur_selection_clavier, couleur_boutons, rend);}
			rectangle(xmax / 2 - 190 + 150 * compteur, ymax / 2 + 15, 80, 45, 0, fond, couleur_boutons, rend); //bas
			if (curseur == gp_moins_col + compteur * 2)
			{rectangle(xmax / 2 - 190 + 150 * compteur, ymax / 2 + 15, 80, 45, 0, couleur_selection_curseur, couleur_boutons, rend);}
			else if (focus == gp_moins_col + compteur * 2)
			{rectangle(xmax / 2 - 190 + 150 * compteur, ymax / 2 + 15, 80, 45, 0, couleur_selection_clavier, couleur_boutons, rend);}
			rectangle(xmax / 2 - 190 + 150 * compteur, ymax / 2 - 90, 80, 150, 3, couleur_txt_boutons, couleur_boutons, rend); //boîte
			SDL_RenderDrawLine(rend, xmax / 2 - 190 + 150 * compteur, ymax / 2 - 46, xmax / 2 - 110 + 150 * compteur, ymax / 2 - 46); //haut
			SDL_RenderDrawLine(rend, xmax / 2 - 190 + 150 * compteur, ymax / 2 - 45, xmax / 2 - 110 + 150 * compteur, ymax / 2 - 45);
			SDL_RenderDrawLine(rend, xmax / 2 - 190 + 150 * compteur, ymax / 2 - 44, xmax / 2 - 110 + 150 * compteur, ymax / 2 - 44);
			SDL_RenderDrawLine(rend, xmax / 2 - 190 + 150 * compteur, ymax / 2 + 13, xmax / 2 - 110 + 150 * compteur, ymax / 2 + 13); //bas
			SDL_RenderDrawLine(rend, xmax / 2 - 190 + 150 * compteur, ymax / 2 + 14, xmax / 2 - 110 + 150 * compteur, ymax / 2 + 14);
			SDL_RenderDrawLine(rend, xmax / 2 - 190 + 150 * compteur, ymax / 2 + 15, xmax / 2 - 110 + 150 * compteur, ymax / 2 + 15);
			if (compteur == 0)
			{sprintf(buffer, "%d", nbre_col);}
			else if (compteur == 1)
			{sprintf(buffer, "%d", nbre_lignes);}
			else
			{sprintf(buffer, "%d", nbre_bombes);}
			afficher_txt_centre(buffer, xmax / 2 - 190 + 150 * compteur, xmax / 2 - 110 + 150 * compteur, ymax / 2 - 25, police, couleur_txt_boutons, rend);
			SDL_RenderDrawLine(rend, xmax / 2 - 165 + 150 * compteur, ymax / 2 - 60, xmax / 2 - 150 + 150 * compteur, ymax / 2 - 75); // ^
			SDL_RenderDrawLine(rend, xmax / 2 - 150 + 150 * compteur, ymax / 2 - 75, xmax / 2 - 135 + 150 * compteur, ymax / 2 - 60);
			SDL_RenderDrawLine(rend, xmax / 2 - 165 + 150 * compteur, ymax / 2 - 61, xmax / 2 - 150 + 150 * compteur, ymax / 2 - 76);
			SDL_RenderDrawLine(rend, xmax / 2 - 150 + 150 * compteur, ymax / 2 - 76, xmax / 2 - 135 + 150 * compteur, ymax / 2 - 61);
			SDL_RenderDrawLine(rend, xmax / 2 - 165 + 150 * compteur, ymax / 2 - 59, xmax / 2 - 150 + 150 * compteur, ymax / 2 - 74);
			SDL_RenderDrawLine(rend, xmax / 2 - 150 + 150 * compteur, ymax / 2 - 74, xmax / 2 - 135 + 150 * compteur, ymax / 2 - 59);
			SDL_RenderDrawLine(rend, xmax / 2 - 165 + 150 * compteur, ymax / 2 + 30, xmax / 2 - 150 + 150 * compteur, ymax / 2 + 45); // \/
			SDL_RenderDrawLine(rend, xmax / 2 - 150 + 150 * compteur, ymax / 2 + 45, xmax / 2 - 135 + 150 * compteur, ymax / 2 + 30);
			SDL_RenderDrawLine(rend, xmax / 2 - 165 + 150 * compteur, ymax / 2 + 29, xmax / 2 - 150 + 150 * compteur, ymax / 2 + 44);
			SDL_RenderDrawLine(rend, xmax / 2 - 150 + 150 * compteur, ymax / 2 + 44, xmax / 2 - 135 + 150 * compteur, ymax / 2 + 29);
			SDL_RenderDrawLine(rend, xmax / 2 - 165 + 150 * compteur, ymax / 2 + 31, xmax / 2 - 150 + 150 * compteur, ymax / 2 + 46);
			SDL_RenderDrawLine(rend, xmax / 2 - 150 + 150 * compteur, ymax / 2 + 46, xmax / 2 - 135 + 150 * compteur, ymax / 2 + 31);
		}
		
		//Boutons retour et lancer:
		rect_arrondi(xmax / 6 + 30, 4 * ymax / 5 - 60, 150, 40, fond, couleur_boutons, rend); //retour
		if (curseur == gp_retour)
		{rect_arrondi(xmax / 6 + 30, 4 * ymax / 5 - 60, 150, 40, couleur_selection_curseur, couleur_boutons, rend);}
		else if (focus == gp_retour)
		{rect_arrondi(xmax / 6 + 30, 4 * ymax / 5 - 60, 150, 40, couleur_selection_clavier, couleur_boutons, rend);}
		afficher_txt_centre("annuler", xmax / 6 + 30, xmax / 6 + 180, 4 * ymax / 5 - 50, police, couleur_timer, rend);
		rect_arrondi(5 * xmax / 6 - 180, 4 * ymax / 5 - 60, 150, 40, fond, couleur_boutons, rend); //lancer
		if (curseur == gp_lancer)
		{rect_arrondi(5 * xmax / 6 - 180, 4 * ymax / 5 - 60, 150, 40, couleur_selection_curseur, couleur_boutons, rend);}
		else if (focus == gp_lancer)
		{rect_arrondi(5 * xmax / 6 - 180, 4 * ymax / 5 - 60, 150, 40, couleur_selection_clavier, couleur_boutons, rend);}
		afficher_txt_centre("démarrer", 5 * xmax / 6 - 180, 5 * xmax / 6 - 30, 4 * ymax / 5 - 50, police, couleur_timer, rend);
	}
	
	//Affichage de tout ça:
	SDL_RenderPresent(rend);
}


void nouvelle_partie ()
//Initialise une partie (appelé au début de chaque partie).
{
	tuile* ptr_temp = NULL; //ptr temporaire servant à la création de la grille
	int nouv_bombe = -1; //variable temporaire utilisée pour indiquer le "numéro" de la case qui contiendra la prochaine bombe
	int col = -1; //variable temporaire servant à identifier la colonne de la case qui contiendra la prochaine bombe
	SDL_Event calcul_taille_fenetre; //faux windowevent SDL qui sera envoyé artificiellement afin de s'assurer que les calculs de la taille des différents éléments du jeu soient faits
	
	
	//Libération de la mémoire utilisée par la grille précédente:
	if (grille != NULL)
	{
	    for (int compteur = 0; compteur < ancien_nbre_col; compteur++)
	    {free(grille[compteur]);}
	    if (erreur != 6)
	    {free(grille);}
	}
	ancien_nbre_col = nbre_col;
	
	//Initialisation le random number generator avec une seed aléatoire:
	srand(time(NULL));
	
	//Création de la grille:
	grille = calloc(nbre_col, sizeof(tuile*)); //syntaxe trouvée ici: stackoverflow.com/questions/57614091/allocating-a-2d-array-using-calloc (1ère réponse, 2e exemple)
	if (grille == NULL)
	{
		printf("Erreur lors de l'allocation de la mémoire au moment de créer la grille.\n");
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Erreur", "Erreur fatale 6: impossible de démarrer l'application.\nOuvrez l'application depuis la console pour en savoir plus.", NULL); //affiche un pop-up d'erreur
		quitter(-6);
	}
	else
	{
		for (int compteur = 0; compteur < nbre_col; compteur++)
		{
			grille[compteur] = calloc(nbre_lignes, sizeof(tuile));
			if (grille[compteur] == NULL)
			{
				printf("Erreur lors de l'allocation de la mémoire au moment de créer la %de colonne de la grille.\n", compteur);
				SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Erreur", "Erreur fatale 7: impossible de démarrer l'application.\nOuvrez l'application depuis la console pour en savoir plus.", NULL); //affiche un pop-up d'erreur
				quitter(-7);
			}
			
			//Initialise les 3 variables de chaque tuile/case:
			for (int c2 = 0; c2 < nbre_lignes; c2++)
			{
				grille[compteur][c2].drapeau = 0;
				grille[compteur][c2].revelee = 0;
				grille[compteur][c2].etat = inconnu;
			}
		}
	}
	
	//Placement des bombes:
	for (int compteur = 0; compteur < nbre_bombes; compteur++)
	{
		nouv_bombe = rand() % (nbre_col * nbre_lignes); //trouve un nbre aléatoire entre 0 et le nbre de tuiles/cases de la grille (il s'agit donc du "numéro" d'une case de la grille)
		for (col = 0; nouv_bombe - col * nbre_lignes >= nbre_lignes; col++) {} //trouve dans quelle colonne cette case se situe
		if (debogage)
		{printf("Bombe %d: #%d (col %d, ligne %d)\n", compteur, nouv_bombe, col, nouv_bombe - col * nbre_lignes);} //pour débogage (indique où se trouve chaque bombe)
		if (grille[col][nouv_bombe - col * nbre_lignes].etat != bombe)
		{grille[col][nouv_bombe - col * nbre_lignes].etat = bombe;} //Marque cette tuile comme étant une bombe...
		else
		{compteur--;} //...si elle n'en est pas déjà une!!!
	}
	if (debogage)
	{printf("\n");}
	
	//Réinitialisation de différentes variables:
	fin_de_partie = 0;
	pause = 0;
	recalcul = 0;
	nbre_drapeaux = 0;
	nbre_tuiles_restantes = (nbre_col * nbre_lignes) - nbre_bombes;
	strcpy(cmd, "cmd: ");
	focus = non_defini;
	
	//S'assure que les calculs de la taille des différents éléments sera faits:
	calcul_taille_fenetre.type = SDL_WINDOWEVENT;
	SDL_PushEvent(&calcul_taille_fenetre);
}


void partie ()
//Gère les events d'une partie:
{
	_Bool keymod = 0; //indique si l'utilisateur est en train d'appuyer sur Shift, Ctrl ou Alt
	_Bool render_a_faire = 0; //indique au programme qu'il n'a pas encore redessiné la fenêtre (utilisé dans certains cas spécifiques seulement)
	SDL_Event ev;
	
	//Données du pop-up de confirmation pour fermer l'application (déclaré ici (et ailleurs) afin d'éviter une erreur comme quoi "fenetre" n'est pas cst...):
	int choix_popup_quitter = 0;
	SDL_MessageBoxButtonData boutons_popup_quitter[2] =
	{
		{SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Annuler"},
		{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Quitter"}
	};
	SDL_MessageBoxData popup_quitter =
	{
		SDL_MESSAGEBOX_INFORMATION,
		fenetre,
		"Voulez-vous vraiment quitter?",
		"La partie en cours sera perdue.",
		2,
		boutons_popup_quitter,
		NULL
	};
	
	//Données du pop-up de confirmation pour recommencer:
	SDL_MessageBoxButtonData boutons_popup_recommencer[2] =
	{
		{SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Annuler"},
		{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Recommencer"}
	};
	SDL_MessageBoxData popup_recommencer =
	{
		SDL_MESSAGEBOX_INFORMATION,
		fenetre,
		"Voulez-vous vraiment recommencer?",
		"La partie en cours sera perdue.",
		2,
		boutons_popup_recommencer,
		NULL
	};
	
	
	nouvelle_partie(); //préparation de la grille, etc.
	
    while (1)
    {
        //Timer:
		//À FAIRE!!!
		
		//Vérifie si tous les drapeaux on été placés ou si toutes les tuiles non-bombes ont été dévoilées:
		if (!fin_de_partie && (nbre_tuiles_restantes <= 0 || nbre_drapeaux == nbre_bombes))
		{
			strcpy(cmd, "cmd: rct");
			executer_cmd();
			if (nbre_drapeaux == nbre_bombes)
			{strcpy(cmd, "cmd: vv -i"); executer_cmd();}
			else
			{fin_de_partie = 3;}
		}
		
		//Gestion de l'input:
		if (SDL_PollEvent(&ev)) //SDL_PollEvent renvoie 1 s'il trouve un event et 0 s'il n'en trouve pas
		{				// \--> Si on veut avoir un timer, on ne peut pas juste attendre les events!
			switch (ev.type)
			{
			case SDL_QUIT:
				if (confirmation_quitter)
				{SDL_ShowMessageBox(&popup_quitter, &choix_popup_quitter);}
				if (choix_popup_quitter || !confirmation_quitter)
				{quitter(erreur);}
				break;
			
			case SDL_WINDOWEVENT:
				//Pour gérer plus en détail les windowevents, on peut switch(ev.window.event) et ainsi trouver exactement ce qui vient de se passer.
				//(wiki.libsdl.org/SDL2/SDL_WindowEventID = valeur que peut prendre ev.window.event)
				//Pour le moment, je vais seulement trouver la nouvelle taille de la fenêtre et la redessiner.
				SDL_GetWindowSize(fenetre, &xmax, &ymax);
				
				//Trouve la taille des carrés dans la grille (ainsi que d'autres infos liées à la grille):
				taille = (xmax - 220) / nbre_col; //selon x...
				if (taille > ymax * 13/14 / nbre_lignes) //...ou y
				{taille = ymax * 13/14 / nbre_lignes;}
				taille -= 5; //ajustement pour laisser de l'espace entre les carrés...
				marge_gauche = (xmax - (taille + 5) * nbre_col - 190) * 1 / 3; //taille de la marge à gauche de la grille
				marge_droite = marge_gauche + (taille + 5) * nbre_col;
				
				rafraichir(0);
				break;
			
			case SDL_KEYDOWN:
				switch (ev.key.keysym.sym)
				{
				case SDLK_ESCAPE:
					if (cmd_line)
					{cmd_line = 0; rafraichir(0);}
					else
					{
						SDL_ShowMessageBox(&popup_quitter, &choix_popup_quitter);
						if (choix_popup_quitter)
						{quitter(erreur);}
					}
					break;
				
				case SDLK_PAUSE:
				case SDLK_p:
					if (!fin_de_partie && !cmd_line)
					{
						if (!pause)
						{pause = 1;}
						else
						{pause = 0;}
						rafraichir(0);
					}
					break;
				
				case SDLK_RETURN:
				case SDLK_KP_ENTER:
				case SDLK_SPACE:
					if (cmd_line)
					{
						if (ev.key.keysym.sym != SDLK_SPACE)
						{executer_cmd();}
					}
					else
					{
						switch (focus)
						{
						case sur_grille:
							if (keymod && !pause && !fin_de_partie)
							{
								if (!grille[pos_grille_x[0]][pos_grille_y[0]].revelee)
								{
									if (grille[pos_grille_x[0]][pos_grille_y[0]].drapeau)
									{
										grille[pos_grille_x[0]][pos_grille_y[0]].drapeau = 0;
										nbre_drapeaux--;
										if (grille[pos_grille_x[0]][pos_grille_y[0]].etat != bombe)
										{nbre_tuiles_restantes++;}
									}
									else if (nbre_drapeaux < nbre_bombes)
									{
										grille[pos_grille_x[0]][pos_grille_y[0]].drapeau = 1;
										nbre_drapeaux++;
										if (grille[pos_grille_x[0]][pos_grille_y[0]].etat != bombe)
										{nbre_tuiles_restantes--;}
									}
								}
							}
							else if (!pause && !fin_de_partie)
							{reveler_tuile(pos_grille_x[0], pos_grille_y[0]);}
							rafraichir(0);
							break;
						
						case bouton_podium:
							/*if (!fin_de_partie)
							{pause = 1; rafraichir(0);}
							//À venir! (dans une fenêtre séparée par défaut?)
							rafraichir(0);*/
							break;
						
						case bouton_pause:
							if (!fin_de_partie)
							{
								if (!pause)
								{pause = 1;}
								else
								{pause = 0;}
								rafraichir(0);
							}
							break;
						
						case bouton_recommencer:
							if (fin_de_partie > 0)
							{nouvelle_partie();}
							else
							{
								SDL_ShowMessageBox(&popup_recommencer, &choix_popup_quitter);
								if (choix_popup_quitter)
								{nouvelle_partie();}
							}
							break;
						
						case bouton_nouvelle_partie:
							if (fin_de_partie > 0)
							{rafraichir_menu(0); focus = 0; return;}
							else
							{
								SDL_ShowMessageBox(&popup_quitter, &choix_popup_quitter);
								if (choix_popup_quitter)
								{rafraichir_menu(0); focus = 0; return;}
							}
							break;
						
						case bouton_reglages:
							if (!fin_de_partie)
							{pause = 1; rafraichir(0);}
							reglages();
							rafraichir(0);
							break;
						}
					}
					break;
				
				case SDLK_TAB:
					if (!keymod && !cmd_line)
					{
						if (!focus || (focus >= bouton_pause && focus <= bouton_reglages))
						{focus = sur_grille; pos_grille_x[0] = 0; pos_grille_y[0] = 0;}
						else if (focus == bouton_podium)
						{focus = bouton_pause;}
						else //focus == sur_grille
						{focus = bouton_podium;}
					}
					else if (!cmd_line)
					{
						if (!focus || focus == bouton_podium)
						{focus = sur_grille; pos_grille_x[0] = 0; pos_grille_y[0] = 0;}
						else if (focus == sur_grille)
						{focus = bouton_pause;}
						else //focus >= bouton_pause && focus <= bouton_reglages
						{focus = bouton_podium;}
					}
					rafraichir(0);
					break;
				
				case SDLK_UP:
					if (focus == sur_grille && pos_grille_y[0] > 0 && !cmd_line)
					{pos_grille_y[0]--;}
					else if (focus > bouton_pause && focus <= bouton_reglages && !cmd_line)
					{focus--;}
					else if (focus == bouton_pause && !cmd_line)
					{focus = bouton_reglages;}
					else if (!focus && !cmd_line)
					{focus = sur_grille; pos_grille_x[0] = 0; pos_grille_y[0] = 0;}
					rafraichir(0);
					break;
				
				case SDLK_DOWN:
					if (focus == sur_grille && pos_grille_y[0] < nbre_lignes - 1 && !cmd_line)
					{pos_grille_y[0]++;}
					else if (focus >= bouton_podium /*non, ce n'est pas un typo!*/ && focus < bouton_reglages && !cmd_line)
					{focus++;}
					else if (focus == bouton_reglages && !cmd_line)
					{focus = bouton_pause;}
					else if (!focus && !cmd_line)
					{focus = sur_grille; pos_grille_x[0] = 0; pos_grille_y[0] = 0;}
					rafraichir(0);
					break;
				
				case SDLK_RIGHT:
					if (focus == sur_grille && pos_grille_x[0] < nbre_col - 1 && !cmd_line)
					{pos_grille_x[0]++;}
					else if (!focus && !cmd_line)
					{focus = sur_grille; pos_grille_x[0] = 0; pos_grille_y[0] = 0;}
					rafraichir(0);
					break;
				
				case SDLK_LEFT:
					if (focus == sur_grille && pos_grille_x[0] > 0 && !cmd_line)
					{pos_grille_x[0]--;}
					else if (!focus && !cmd_line)
					{focus = sur_grille; pos_grille_x[0] = 0; pos_grille_y[0] = 0;}
					rafraichir(0);
					break;
				
				case SDLK_m:
				case SDLK_d:
				case SDLK_f:
				case SDLK_INSERT:
					if (focus == sur_grille && !grille[pos_grille_x[0]][pos_grille_y[0]].revelee && !cmd_line)
					{
						if (grille[pos_grille_x[0]][pos_grille_y[0]].drapeau)
						{
							grille[pos_grille_x[0]][pos_grille_y[0]].drapeau = 0;
							nbre_drapeaux--;
							if (grille[pos_grille_x[0]][pos_grille_y[0]].etat != bombe)
							{nbre_tuiles_restantes++;}
						}
						else if (nbre_drapeaux < nbre_bombes)
						{
							grille[pos_grille_x[0]][pos_grille_y[0]].drapeau = 1;
							nbre_drapeaux++;
							if (grille[pos_grille_x[0]][pos_grille_y[0]].etat != bombe)
							{nbre_tuiles_restantes--;}
						}
					}
					rafraichir(0);
					break;
				
				case SDLK_SLASH:
					if (!cmd_line)
					{cmd_line = 1; SDL_PollEvent(&ev); /*enlève l'inévitable text input "/"...*/ rafraichir(0);}
					break;
				
				case SDLK_BACKSPACE:
					if (cmd_line && cmd[5] != '\000')
					{tronquer(cmd); rafraichir(0);}
					break;
				
				case SDLK_LCTRL:
				case SDLK_RCTRL:
				case SDLK_LSHIFT:
				case SDLK_RSHIFT:
				case SDLK_LALT:
				case SDLK_RALT:
					keymod = 1;
					break;
				}
				break;
			
			case SDL_KEYUP:
				if (ev.key.keysym.sym == SDLK_LCTRL || ev.key.keysym.sym == SDLK_RCTRL || ev.key.keysym.sym == SDLK_LALT || ev.key.keysym.sym == SDLK_RALT || ev.key.keysym.sym == SDLK_LSHIFT || ev.key.keysym.sym == SDLK_RSHIFT)
				{keymod = 0;}
				break;
			
			case SDL_MOUSEMOTION:
				if (ev.motion.x >= xmax - 80 && ev.motion.y >= 20 && ev.motion.x <= xmax - 20 && ev.motion.y <= 60)
				{rafraichir(bouton_podium); render_a_faire = 0;}
				else if (ev.motion.x >= marge_gauche && ev.motion.y >= (ymax - nbre_lignes * (taille + 5)) / 2 \
					&& ev.motion.x <= nbre_col * (taille + 5) + marge_gauche - 5 && ev.motion.y <= nbre_lignes * (taille + 5) + (ymax - nbre_lignes * (taille + 5)) / 2 - 5)
				{
					pos_grille_x[1] = (ev.motion.x - marge_gauche) / (taille + 5);
					pos_grille_y[1] = (ev.motion.y - (ymax - nbre_lignes * (taille + 5)) / 2) / (taille + 5);
					rafraichir(sur_grille);
				}
				else
				{
					render_a_faire = 1;
					for (int compteur = 0; compteur < 4; compteur++)
					{
						if (ev.motion.x >= marge_droite + (xmax - marge_droite - 150) / 2 && ev.motion.y >= ymax / 2 + compteur * 60 \
							&& ev.motion.x <= marge_droite + (xmax - marge_droite + 150) / 2 && ev.motion.y <= ymax / 2 + 40 + compteur * 60)
						{rafraichir(bouton_pause + compteur); render_a_faire = 0;}
					}
					if (render_a_faire)
					{rafraichir(0); render_a_faire = 0;}
				}
				break;
			
			case SDL_MOUSEBUTTONDOWN:
				focus = 0; //cliquer fait automatiquement perdre le focus clavier (peu importe où on clique)
				if (cmd_line)
				{cmd_line = 0; rafraichir(0);}
				else if (ev.button.x >= xmax - 80 && ev.button.y >= 20 && ev.button.x <= xmax - 20 && ev.button.y <= 60) //bouton podium
				{/*À faire!*/}
				else if (!fin_de_partie && ev.button.x >= marge_gauche && ev.button.y >= (ymax - nbre_lignes * (taille + 5)) / 2 \
					&& ev.button.x <= nbre_col * (taille + 5) + marge_gauche - 5 && ev.button.y <= nbre_lignes * (taille + 5) + (ymax - nbre_lignes * (taille + 5)) / 2 - 5) //grille
				{
					if ((keymod || ev.button.button == SDL_BUTTON_RIGHT) && !grille[pos_grille_x[1]][pos_grille_y[1]].revelee) //placement d'un drapeau
					{
						if (grille[pos_grille_x[1]][pos_grille_y[1]].drapeau) //je présume que je peux utiliser pos_grille[1] ici, vu qu'on ne peut pas vraiment cliquer nulle part sans y avoir d'abord amené la souris...
						{
							grille[pos_grille_x[1]][pos_grille_y[1]].drapeau = 0;
							nbre_drapeaux--;
							if (grille[pos_grille_x[1]][pos_grille_y[1]].etat != bombe)
							{nbre_tuiles_restantes++;}
						}
						else if (nbre_drapeaux < nbre_bombes)
						{
							grille[pos_grille_x[1]][pos_grille_y[1]].drapeau = 1;
							nbre_drapeaux++;
							if (grille[pos_grille_x[1]][pos_grille_y[1]].etat != bombe)
							{nbre_tuiles_restantes--;}
						}
					}
					else if (!fin_de_partie) //révélation d'une tuile
					{reveler_tuile(pos_grille_x[1], pos_grille_y[1]);}
					rafraichir(sur_grille);
				}
				else if (ev.button.x >= (xmax + marge_droite - 150) / 2 && ev.button.y >= ymax / 2 && ev.button.x <= (xmax + marge_droite + 150) / 2 && ev.button.y <= ymax / 2 + 40 && !fin_de_partie) //bouton pause
				{
					if (!pause)
					{pause = 1;}
					else
					{pause = 0;}
					rafraichir(bouton_pause);
				}
				else if (ev.button.x >= (xmax + marge_droite - 150) / 2 && ev.button.y >= ymax / 2 + 60 && ev.button.x <= (xmax + marge_droite + 150) / 2 && ev.button.y <= ymax / 2 + 100) //bouton recommencer
				{
				    if (fin_de_partie > 0)
				    {nouvelle_partie();}
				    else
				    {
						SDL_ShowMessageBox(&popup_recommencer, &choix_popup_quitter);
						if (choix_popup_quitter)
						{nouvelle_partie();}
					}
				}
				else if (ev.button.x >= (xmax + marge_droite - 150) / 2 && ev.button.y >= ymax / 2 + 120 && ev.button.x <= (xmax + marge_droite + 150) / 2 && ev.button.y <= ymax / 2 + 160) //bouton nouvelle partie
				{
					if (fin_de_partie > 0)
					{rafraichir_menu(0); focus = 0; return;}
					else
					{
						SDL_ShowMessageBox(&popup_quitter, &choix_popup_quitter);
						if (choix_popup_quitter)
						{rafraichir_menu(0); focus = 0; return;}
					}
				}
				else if (ev.button.x >= (xmax + marge_droite - 150) / 2 && ev.button.y >= ymax / 2 + 180 && ev.button.x <= (xmax + marge_droite + 150) / 2 && ev.button.y <= ymax / 2 + 220) //bouton réglages
				{
					if (!fin_de_partie)
					{pause = 1; rafraichir(0);}
					reglages();
					rafraichir(0);
				}
				break;
			
			case SDL_TEXTINPUT:
				if (cmd_line && cmd[98] == '\000')
				{strcat(cmd, ev.text.text); rafraichir(0);}
				break;
			}
		}
    }
}


void rafraichir (enum zone curseur)
//Redessine la fenêtre de l'application.
//Reçoit en paramètre la "zone" de l'écran où se trouve le curseur.
{
	SDL_Rect rect_icone_podium = {xmax - 74, 15, 41, 41}; //41, parce que 40 détruit complètement l'icone...
	SDL_Rect rect_symbole_pause = {marge_droite + 40, 20, 61, 61};
	SDL_Rect rect_drapeau = {0, 0, taille, taille};
	char score[10] = "ERREUR"; //"score" affiché à droite de la grille (nbre de drapeaux utilisé / nbre de bombes dans la grille)
	char nbre_bombes_adjacentes[5] = "?"; //nbre de bombes adjacentes à une tuile (string contenant ce nbre qui sera affiché sur chaque tuile révélée)
	
	//Arrière-plan de la fenêtre:
	SDL_SetColor(fond, rend);
	SDL_RenderClear(rend);
	
	//Dessin de la grille:
	for (int y = 0; y < nbre_lignes; y++)
	{
		for (int x = 0; x < nbre_col; x++)
		{
			if (!grille[x][y].revelee)
			{rect_arrondi(x * (taille + 5) + marge_gauche, y * (taille + 5) + (ymax - nbre_lignes * (taille + 5)) / 2, taille, taille, couleur_grille, fond, rend);}
			else if (grille[x][y].etat > 0)
			{rect_arrondi(x * (taille + 5) + marge_gauche, y * (taille + 5) + (ymax - nbre_lignes * (taille + 5)) / 2, taille, taille, couleur_tuile[grille[x][y].etat], fond, rend);}
			//On ne dessine pas les cases qui valent zéro (un oubli à la base, mais finalement, c'est pas mal beau de même...)
		}
	}
	
	//Écriture du score et affichage du timer:
	rectangle(marge_droite + 40, ymax / 4 - 40, xmax - marge_droite - 80, 120, 3, noir, fond, rend);
	sprintf(score, "%d / %d", nbre_drapeaux, nbre_bombes);
	rect_arrondi(marge_droite + (xmax - marge_droite - afficher_txt_centre(score, marge_droite, xmax, ymax / 4 - 10, police, couleur_txt_boutons, rend)) / 2 - 10, ymax / 4 - 20, \
		longueur_txt_centre(score, marge_droite, xmax, police) + 20, 40, couleur_grille, fond, rend);
	afficher_txt_centre("00:00", marge_droite, xmax, ymax / 4 + 40, police, couleur_timer, rend);
	
	//Dessin des boutons:
	rect_arrondi(marge_droite + (xmax - marge_droite - 150) / 2, ymax / 2, 150, 40, couleur_boutons, fond, rend); //pause
	rect_arrondi(marge_droite + (xmax - marge_droite - 150) / 2, ymax / 2 + 60, 150, 40, couleur_boutons, fond, rend); //recommencer
	rect_arrondi(marge_droite + (xmax - marge_droite - 150) / 2, ymax / 2 + 120, 150, 40, couleur_boutons, fond, rend); //nouvelle partie
	rect_arrondi(marge_droite + (xmax - marge_droite - 150) / 2, ymax / 2 + 180, 150, 40, couleur_boutons, fond, rend); //réglages
	rect_arrondi(xmax - 85, 20, 60, 40, couleur_boutons, fond, rend); //bouton "podium"
	
	//Affichage des symbole "pause" ou "fin de partie" (si nécessaire):
	if (fin_de_partie == 2 && texture_symbole_fin_de_partie_defaite != NULL)
	{SDL_RenderCopy(rend, texture_symbole_fin_de_partie_defaite, NULL, &rect_symbole_pause);}
	else if (fin_de_partie > 0 && texture_symbole_fin_de_partie != NULL)
	{SDL_RenderCopy(rend, texture_symbole_fin_de_partie, NULL, &rect_symbole_pause);}
	else if (pause && texture_symbole_pause != NULL)
	{SDL_RenderCopy(rend, texture_symbole_pause, NULL, &rect_symbole_pause);}
	
	//Modification de l'élément "sélectionné" par la souris (hovering):
	switch (curseur)
	{
	case sur_grille:
		rect_arrondi(marge_gauche + pos_grille_x[1] * (taille + 5), pos_grille_y[1] * taille + pos_grille_y[1] * 5 + (ymax - nbre_lignes * (taille + 5)) / 2, taille, taille, couleur_selection_curseur, fond, rend);
		break;
	
	case bouton_podium:
		rect_arrondi(xmax - 85, 20, 60, 40, couleur_selection_curseur, fond, rend);
		break;
	
	case bouton_pause:
	case bouton_recommencer:
	case bouton_nouvelle_partie:
	case bouton_reglages:
		rect_arrondi(marge_droite + (xmax - marge_droite - 150) / 2, ymax / 2 + 60 * (curseur - bouton_pause), 150, 40, couleur_selection_curseur, fond, rend);
		break;
	}
	
	//Modification de l'élément "sélectionné" par le clavier:
	switch (focus)
	{
	case sur_grille:
		rect_arrondi(marge_gauche + pos_grille_x[0] * (taille + 5), pos_grille_y[0] * taille + pos_grille_y[0] * 5 + (ymax - nbre_lignes * (taille + 5)) / 2, taille, taille, couleur_selection_clavier, fond, rend);
		break;
	
	case bouton_podium:
		rect_arrondi(xmax - 85, 20, 60, 40, couleur_selection_clavier, fond, rend);
		break;
	
	case bouton_pause:
	case bouton_recommencer:
	case bouton_nouvelle_partie:
	case bouton_reglages:
		rect_arrondi(marge_droite + (xmax - marge_droite - 150) / 2, ymax / 2 + 60 * (focus - bouton_pause), 150, 40, couleur_selection_clavier, fond, rend);
		break;
	}
	
	//Affichage du symbole "podium" sur son bouton:
	if (texture_icone_podium != NULL)
	{SDL_RenderCopy(rend, texture_icone_podium, NULL, &rect_icone_podium);}
	
	//Écriture du texte dans les boutons:
	afficher_txt("pause", marge_droite + (xmax - marge_droite - 50) / 2, ymax / 2 + 8, 150, police, couleur_txt_boutons, rend);
	afficher_txt("recommencer", marge_droite + (xmax - marge_droite - 122) / 2, ymax / 2 + 68, 150, police, couleur_txt_boutons, rend);
	afficher_txt("nouvelle partie", marge_droite + (xmax - marge_droite - 132) / 2, ymax / 2 + 128, 150, police, couleur_txt_boutons, rend);
	afficher_txt("réglages", marge_droite + (xmax - marge_droite - 70) / 2, ymax / 2 + 188, 150, police, couleur_txt_boutons, rend);
	
	//Affichage des drapeaux et des chiffres dans la grille:
	for (int y = 0; y < nbre_lignes; y++)
	{
		for (int x = 0; x < nbre_col; x++)
		{
			if (grille[x][y].drapeau) //drapeaux
			{
				rect_drapeau.x = x * (taille + 5) + marge_gauche;
				rect_drapeau.y = y * (taille + 5) + (ymax - nbre_lignes * (taille + 5)) / 2;
				if (fin_de_partie > 0 && grille[x][y].etat != bombe && texture_drapeau_mal_place != NULL)
				{SDL_RenderCopy(rend, texture_drapeau_mal_place, NULL, &rect_drapeau);}
				else if (texture_drapeau != NULL)
				{SDL_RenderCopy(rend, texture_drapeau, NULL, &rect_drapeau);}
				else //Trace des "X" au lieu d'une image de drapeau (en guise de fallback...)
				{
					SDL_SetColor(couleur_score, rend);
					SDL_RenderDrawLine(rend, rect_drapeau.x, rect_drapeau.y, rect_drapeau.x + taille, rect_drapeau.y + taille);
					SDL_RenderDrawLine(rend, rect_drapeau.x, rect_drapeau.y + 1, rect_drapeau.x + taille - 1, rect_drapeau.y + taille);
					SDL_RenderDrawLine(rend, rect_drapeau.x + 1, rect_drapeau.y, rect_drapeau.x + taille, rect_drapeau.y + taille - 1);
					SDL_RenderDrawLine(rend, rect_drapeau.x + taille, rect_drapeau.y, rect_drapeau.x, rect_drapeau.y + taille);
					SDL_RenderDrawLine(rend, rect_drapeau.x + taille - 1, rect_drapeau.y, rect_drapeau.x, rect_drapeau.y + taille - 1);
					SDL_RenderDrawLine(rend, rect_drapeau.x + taille, rect_drapeau.y + 1, rect_drapeau.x + 1, rect_drapeau.y + taille);
				}
			}
			
			else if (grille[x][y].revelee) //nbre de bombes adjacentes
			{				
				taille_nbre[grille[x][y].etat].x = marge_gauche + x * (taille + 5) + (taille - taille_nbre[grille[x][y].etat].w) / 2;
				taille_nbre[grille[x][y].etat].y = (ymax - nbre_lignes * (taille + 5)) / 2 + y * (taille + 5) + taille / 2 - 10;
				SDL_RenderCopy(rend, texture_nbre[grille[x][y].etat], NULL, &taille_nbre[grille[x][y].etat]);
			}
			
			else if (fin_de_partie > 0) //bombes
			{
				rect_drapeau.x = x * (taille + 5) + marge_gauche;
				rect_drapeau.y = y * (taille + 5) + (ymax - nbre_lignes * (taille + 5)) / 2;
				if (tuile_finale[0] == x && tuile_finale[1] == y && texture_bombe_finale != NULL)
				{SDL_RenderCopy(rend, texture_bombe_finale, NULL, &rect_drapeau);}
				else if (texture_bombe != NULL && grille[x][y].etat == bombe)
				{SDL_RenderCopy(rend, texture_bombe, NULL, &rect_drapeau);}
			}
		}
	}
	
	//Affichage de la "command line" (si nécessaire):
	if (cmd_line)
	{
		rect_arrondi(40, ymax - 70, xmax - 80, 30, blanc, fond, rend);
		rectangle(40, ymax - 70, xmax - 80, 30, 3, noir, fond, rend);
		afficher_txt(cmd, 50, ymax - 65, xmax - 100, police, noir, rend);
	}
	
	//Affichage à l'écran:
	SDL_RenderPresent(rend);
}


_Bool reveler_tuile(int x, int y)
//Vérifie si la tuile située aux coordonnées (dans la grille) reçues en paramètres est une bombe et la révèle (ainsi que les tuiles adjacentes) si elle n'en est pas une.
//Renvoie 1 si la tuile est une bombe ou 0 si elle n'en est pas une.
{
	int contour_x[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
	int contour_y[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
	
	if ((grille[x][y].revelee && !recalcul) || grille[x][y].drapeau) //si le joueur a cliqué sur une tuile/case déjà révélée ou marquée par un drapeau, il ne se passe rien
	{return 0;}
	
	if (grille[x][y].etat == bombe) //si le joueur tente de révéler une bombe, la partie se termine drette là!
	{
		for (int xf = 0; xf < nbre_col; xf++)
		{
			for (int yf = 0; yf < nbre_lignes; yf++)
			{
				if (!grille[xf][yf].revelee && !grille[xf][yf].drapeau && grille[xf][yf].etat != bombe)
				{reveler_tuile(xf, yf);}
			}
		}
		fin_de_partie = 2;
		tuile_finale[0] = x;
		tuile_finale[1] = y;
		return 1;
	}
	
	//À partir d'ici, on sait que la tuile reçue doit bel et bien être révélée:
	grille[x][y].revelee = 1;
	nbre_tuiles_restantes--;
	
	//Trouve le nombre de bombes adjacentes:
	grille[x][y].etat = 0;
	if (y > 0)
	{
		if (x > 0 && grille[x-1][y-1].etat == bombe)
		{grille[x][y].etat += 1;}
		if (grille[x][y-1].etat == bombe)
		grille[x][y].etat += 1;
		if (x < nbre_col - 1 && grille[x+1][y-1].etat == bombe)
		{grille[x][y].etat += 1;}
	}
	
	if (x > 0 && grille[x-1][y].etat == bombe)
	{grille[x][y].etat += 1;}
	if (x < nbre_col - 1 && grille[x+1][y].etat == bombe)
	{grille[x][y].etat += 1;}
	
	if (y < nbre_lignes - 1)
	{
		if (x > 0 && grille[x-1][y+1].etat == bombe)
		{grille[x][y].etat += 1;}
		if (grille[x][y+1].etat == bombe)
		{grille[x][y].etat += 1;}
		if (x < nbre_col - 1 && grille[x+1][y+1].etat == bombe)
		{grille[x][y].etat += 1;}
	}
	
	//Révèle toutes les tuiles adjacentes si la tuile révélée n'est pas adjacente à aucune bombe:
	if (grille[x][y].etat == 0)
	{
		for (int compteur = 0; compteur < 8; compteur++)
		{
			if (x + contour_x[compteur] >= 0 && y + contour_y[compteur] >= 0 && x + contour_x[compteur] < nbre_col && y + contour_y[compteur] < nbre_lignes \
				&& !grille[x + contour_x[compteur]][y + contour_y[compteur]].revelee && !grille[x + contour_x[compteur]][y + contour_y[compteur]].drapeau)
			{reveler_tuile(x + contour_x[compteur], y + contour_y[compteur]);}
		}
	}
	
	return 0;
}


void executer_cmd ()
//Exécute la commande entrée par l'utilisateur.
//Également utilisé par certains réglages afin d'éviter une duplication du code...
{
	for (int compteur = 5; cmd[compteur] != '\000'; compteur++)
	{cmd[compteur - 5] = cmd[compteur]; cmd[compteur - 4] = '\000';}
	
	if (!strcmp(cmd, "/"))
	{/*On ne fait rien. Au fond, l'utilisateur voulait probablement juste fermer la barre de la même manière qu'il l'a ouverte...*/}
	else if (!strcmp(cmd, "aide") || !strcmp(cmd, "ls") || !strcmp(cmd, "?"))
	{
		printf("\nListe des commandes:\n--------------------\n- aide / ls (?) = affiche ce message dans le terminal\n- recalculer (rc) = recalcule (et révèle) la valeur d'une tuile* (parfois nécessaire après une autre commande)\n");
		printf("- recalculer tout (rct) = révèle et recalcule la valeur de chaque tuile qui n'est pas une bombe\n- vérifier victoire (vv) = vérifie si vous avez gagné ou perdu et affiche quelques infos à ce sujet dans le terminal\n");
		printf("  > vérifier victoire --interne (vv -i) = effectue la commande sans rien afficher dans le terminal\n- miner (m+) = transforme une tuile* en bombe\n- déminer / cacher (m-) = démine et cache une tuile*\n");
		printf("- drapeau / marquer (d) = place un drapeau sur une tuile* s'il n'y en avait pas ou l'enlève s'il y en avait un\n- focus (xy) = affiche les coordonnées** d'une tuile* dans le terminal\n");
		printf("- déboguer (db) = active ou désactive le mode débogage, qui affiche les coordonnées** de chaque bombe dans le terminal au début de chaque partie\n--------------------\n");
		printf("* Lorsqu'une commande fait référence à une tuile, il s'agit de la tuile précédemment sélectionnée avec le clavier.\n");
		printf("** Lorsqu'une commande affiche des coordonnées dans le terminal, celles-ci sont affichées sous le format \"(x, y)\", où x et y débutent à 0 (et non 1).\n\n");
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Aide des commandes", \
		"La liste des commandes acceptées a été envoyée au terminal.\nAssurez-vous d'avoir d'abord démarré l'application depuis un terminal pour pouvoir le consulter.", fenetre);
	}
	else if (!strcmp(cmd, "recalculer") || !strcmp(cmd, "rc"))
	{
		recalcul = 1;
		reveler_tuile(pos_grille_x[0], pos_grille_y[0]);
		recalcul = 0;
		printf("Tuile (%d, %d) recalculée: %d\n", pos_grille_x[0], pos_grille_y[0], grille[pos_grille_x[0]][pos_grille_y[0]].etat);
	}
	else if (!strcmp(cmd, "recalculer tout") || !strcmp(cmd, "rct"))
	{
		recalcul = 1;
		for (int x = 0; x < nbre_col; x++)
		{
			for (int y = 0; y < nbre_lignes; y++)
			{
				if (grille[x][y].etat != bombe)
				{reveler_tuile(x, y);}
			}
		}
		recalcul = 0;
	}
	else if (!strcmp(cmd, "vérifier victoire") || !strcmp(cmd, "vv") || !strcmp(cmd, "vérifier victoire --interne") || !strcmp(cmd, "vv -i"))
	{
		pts = 0;
		
		for (int x = 0; x < nbre_col; x++)
		{
			for (int y = 0; y < nbre_lignes; y++)
			{
				if (grille[x][y].etat == bombe && grille[x][y].drapeau)
				{pts++;}
			}
		}
		if (pts == nbre_bombes)
		{
			fin_de_partie = 3;
			if (!strcmp(cmd, "vérifier victoire") || !strcmp(cmd, "vv"))
			{printf("Victoire! (%d drapeaux bien placés / %d mines)\n", pts, nbre_bombes);}
		}
		else
		{
			fin_de_partie = 2;
			if (!strcmp(cmd, "vérifier victoire") || !strcmp(cmd, "vv"))
			{printf("Défaite... (%d drapeaux bien placés / %d mines + %d mines non-révélées + %d drapeaux mal placés)\n", pts, nbre_bombes, nbre_bombes - pts, nbre_drapeaux - pts);}
		}
	}
	else if (!strcmp(cmd, "miner") || !strcmp(cmd, "m+"))
	{
		if (grille[pos_grille_x[0]][pos_grille_y[0]].etat != bombe)
		{nbre_bombes++; nbre_tuiles_restantes--; printf("La tuile (%d, %d) est déjà minée.\n", pos_grille_x[0], pos_grille_y[0]);}
		else
		{printf("Tuile (%d, %d) minée manuellement.\n", pos_grille_x[0], pos_grille_y[0]);}
		grille[pos_grille_x[0]][pos_grille_y[0]].etat = bombe;
		grille[pos_grille_x[0]][pos_grille_y[0]].revelee = 0;
	}
	else if (!strcmp(cmd, "déminer") || !strcmp(cmd, "m-") || !strcmp(cmd, "cacher"))
	{
		if (grille[pos_grille_x[0]][pos_grille_y[0]].etat == bombe)
		{nbre_bombes--; nbre_tuiles_restantes++; printf("Tuile (%d, %d) déminée manuellement.\n", pos_grille_x[0], pos_grille_y[0]);}
		else
		{printf("Tuile (%d, %d) cachée manuellement.\n", pos_grille_x[0], pos_grille_y[0]);}
		grille[pos_grille_x[0]][pos_grille_y[0]].etat = inconnu;
		grille[pos_grille_x[0]][pos_grille_y[0]].revelee = 0;
	}
	else if (!strcmp(cmd, "drapeau") || !strcmp(cmd, "marquer") || !strcmp(cmd, "d"))
	{
		if (grille[pos_grille_x[0]][pos_grille_y[0]].etat != bombe && grille[pos_grille_x[0]][pos_grille_y[0]].drapeau)
		{nbre_tuiles_restantes++;}
		else if (grille[pos_grille_x[0]][pos_grille_y[0]].etat != bombe && !grille[pos_grille_x[0]][pos_grille_y[0]].revelee)
		{nbre_tuiles_restantes--;}
		if (grille[pos_grille_x[0]][pos_grille_y[0]].drapeau)
		{grille[pos_grille_x[0]][pos_grille_y[0]].drapeau = 0; nbre_drapeaux--; printf("drapeau enlevé en (%d, %d).\n", pos_grille_x[0], pos_grille_y[0]);}
		else
		{grille[pos_grille_x[0]][pos_grille_y[0]].drapeau = 1; grille[pos_grille_x[0]][pos_grille_y[0]].revelee = 0; nbre_drapeaux++; printf("drapeau placé en (%d, %d).\n", pos_grille_x[0], pos_grille_y[0]);}
	}
	else if (!strcmp(cmd, "déboguer") || !strcmp(cmd, "db"))
	{
		if (!debogage)
		{
			printf("Mode débogage activé. L'emplacement de chaque bombe sera affiché dans le terminal dès la prochaine partie.\n");
			debogage = 1;
		}
		else
		{printf("Mode débogage désactivé.\n"); debogage = 0;}
	}
	else if (!strcmp(cmd, "focus") || !strcmp(cmd, "xy"))
	{printf("Tuile sélectionnée: (%d, %d)\n", pos_grille_x[0], pos_grille_y[0]);}
	else
	{
		printf("\"%s\" n'est pas une commande reconnue.\n", cmd);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Commande non-reconnue", "Entrez \"aide\" pour consulter la liste des commandes acceptées.", fenetre);
	}
	
	cmd_line = 0;
	strcpy(cmd, "cmd: ");
	rafraichir(0);
}


void quitter ()
//Permet de quitter le programme "en toute sécurité" (en fermant bien SDL et en libérant toute mémoire assignée).
{	
	char buffer[175] = "Le programme s'est terminé avec le code d'erreur ";
	
	//Libération de la mémoire utilisée par la grille:
	if (grille != NULL)
	{
	    for (int compteur = 0; compteur < ancien_nbre_col; compteur++)
	    {free(grille[compteur]);}
	    if (erreur != 6)
	    {free(grille);}
	}
	
	//Destruction des textures du programme:
	if (texture_icone_podium != NULL)
	{SDL_DestroyTexture(texture_icone_podium);}
	if (texture_symbole_pause != NULL)
	{SDL_DestroyTexture(texture_symbole_pause);}
	if (texture_symbole_fin_de_partie != NULL)
	{SDL_DestroyTexture(texture_symbole_fin_de_partie);}
	if (texture_symbole_fin_de_partie_defaite != NULL)
	{SDL_DestroyTexture(texture_symbole_fin_de_partie_defaite);}
	if (texture_bombe != NULL)
	{SDL_DestroyTexture(texture_bombe);}
	if (texture_bombe_finale != NULL)
	{SDL_DestroyTexture(texture_bombe_finale);}
	if (texture_drapeau != NULL)
	{SDL_DestroyTexture(texture_drapeau);}
	if (texture_drapeau_mal_place != NULL)
	{SDL_DestroyTexture(texture_drapeau_mal_place);}
	for (int compteur = 0; compteur < 8; compteur++)
	{
		if (texture_nbre[compteur] != NULL)
		{SDL_DestroyTexture(texture_nbre[compteur]);}
		else if (!erreur && (compteur > 0 || afficher_zeros))
		{erreur = -14; printf("Erreur 14: La texture du chiffre %d n'a pas pu être chargée (au cas où vous ne l'auriez pas remarqué...).\n", compteur);}
	}
	
	//Destruction des polices et curseurs:
	SDL_FreeCursor(curseur_normal);
	SDL_FreeCursor(curseur_txt);
	SDL_FreeCursor (curseur_clic);
	TTF_CloseFont(police);
	if (petite_police != NULL)
	{TTF_CloseFont(petite_police);}
	
	//Fermeture de SDL_ttf et SDL_image:
	TTF_Quit();
	IMG_Quit();
	
	//Destruction de la fenêtre de l'application et fermeture de SDL:
	SDL_DestroyRenderer(rend);
	SDL_DestroyWindow(fenetre);
	SDL_Quit();
	
	//Affichage (dans la console) d'un message de fermeture:
	printf("---\n");
	if (erreur != 0)
	{
		sprintf(buffer, "Le programme s'est terminé avec le code d'erreur %d.\nOuvrez le programme depuis un terminal pour en savoir plus.", -erreur);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Erreur", buffer, NULL);
		printf("Le programme s'est terminé avec le code d'erreur %d.\n", -erreur);
	}
	else
	{printf("À bientôt!\n");}
	exit(erreur);
}