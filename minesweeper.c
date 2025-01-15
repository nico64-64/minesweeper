#include <stdio.h>
#include <time.h>
#include "outils_graphiques.c"


//Macros:
#define VERSION "0.1a" //version du programme


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
	non_definie = 0, //à quelque part dans la fenêtre...
	sur_grille = 1, //sur une case de la grille (la position sur la grille est donnée par pos_grille_x[] et pos_grille_y[])
	bouton_podium = 2,
	bouton_pause,
	bouton_recommencer,
	bouton_nouvelle_partie,
	bouton_reglages
};


//Variables globales liées à SDL et au graphisme:
SDL_Window* fenetre; //la fenêtre de l'application
SDL_Renderer* rend; //le renderer utilisé par l'application
TTF_Font* police; //la police utilisée par l'application
SDL_Cursor* curseur_normal; //un curseur pointeur bien ordinaire...
SDL_Cursor* curseur_txt; //un curseur en forme de "I" pour éditer du texte
int xmax = 1000; //largeur de la fenêtre
int ymax = 700; //hauteur de la fenêtre
SDL_Color fond = blanc; //couleur de l'arrière-plan de la fenêtre
SDL_Color couleur_grille = gris; //couleur des carrés la grille
SDL_Color couleur_score = noir; //couleur du texte indiquant le score du joueur
SDL_Color couleur_timer = noir; //couleur du timer
SDL_Color couleur_boutons = gris_pale; //couleur des boutons
SDL_Color couleur_txt_boutons = noir; //couleur du texte affiché sur les boutons
SDL_Color couleur_selection_clavier = bleu; //couleur des carrés de la grille ou des boutons lorsque le focus du clavier est par dessus
SDL_Color couleur_selection_curseur = bleu_efface; //couleur des carrés de la grille ou des boutons lorsque le curseur est par dessus
SDL_Color couleur_tuile[9] = {transparent, vert_pale, jaune_pale, jaune_orange, orange, orange_fonce, rouge, rouge_fonce, rouge_tres_fonce}; //couleur des tuiles révélées selon le nombre de bombes adjacentes (de 0 à 8)
char icone_podium[35] = "./source/icone_podium.png"; //nom du fichier à utiliser pour afficher l'icone du podium
char symbole_pause[35] = "./source/symbole_pause.png"; //nom du fichier contenant le symbole "pause"
char symbole_fin_de_partie[40] = "./source/symbole_fin_de_partie.png";
char image_bombe[35] = "./source/symbole_bombe.png";
char icone_drapeau[35] = "./source/icone_drapeau.png";
SDL_Texture* texture_icone_podium = NULL; //texture contenant l'icone du podium (déclarée ici car elle est créée par init() et libérée par quitter())
SDL_Texture* texture_symbole_pause = NULL; //texture contenant le symbole "pause" (déclarée ici pour les mêmes raisons que la précédente)
SDL_Texture* texture_symbole_fin_de_partie = NULL; //texture contenant le symbole de fin de partie
SDL_Texture* texture_bombe = NULL; //texture contenant l'image d'une bombe (utilisée uniquement en fin de partie)
SDL_Texture* texture_bombe_finale = NULL; //texture contenant l'image d'une bombe (utilisée uniquement en fin de partie)
SDL_Texture* texture_drapeau = NULL; //texture contenant l'image du drapeau
SDL_Texture* texture_drapeau_mal_place = NULL; //texture contenant l'image d'un drapeau mal placé
SDL_Texture* texture_nbre[9]; //array de textures contenant les chiffres/nombres de 0 à 8
SDL_Rect taille_nbre[9]; //array de rects donnant la taille (x en w et y en h) de chaque nbre, tel que définis à la ligne précédente

//Variables globales liées au jeu et aux réglages non-graphiques:
int nbre_col = 16; //nbre de colonnes dans la grille
int nbre_lignes = 16; //nbre de lignes dans la grille
int nbre_bombes = 40; //nbre de bombes cachées dans la grille
int nbre_drapeaux = 0; //nbre de drapeaux utilisés
int taille = 0; //taille des carrés dans la grille
int marge_gauche = 0; //largeur de la marge à gauche de la grille
int marge_droite = 0; //début de la marge (contenant les boutons) à droite de la grille
int pos_grille_x[2] = {0, 0}; //position {clavier, souris} du focus en x sur la grille (s'il y a lieu)
int pos_grille_y[2] = {0, 0}; //position {clavier, souris} du focus en y sur la grille (s'il y a lieu)
int tuile_finale[2] = {-1, -1}; //coordonnées {x, y} (dans la grille) de la bombe cliquée par le joueur
int nbre_tuiles_restantes = -1; //nbre de tuiles non-révélées qui ne sont pas des bombes ou des drapeaux
tuile** grille; //ptr vers la fameuse grille (les explications suivent une ligne plus bas...)
// \--> Il s'agit en fait d'un pointeur vers un pointeur vers une tuile/case de la grille.
//      Toutefois, "grille" sera plutôt utilisé comme un array de tuiles/cases, en 2D et de taille variable (grâce à calloc() (-> voir init())).
enum zone focus = 0; //zone de la fenêtre "sélectionnée" par le clavier (0 = aucun focus clavier)
_Bool cmd_line = 0; //indique au jeu si le joueur est présentement en train d'utiliser la "command line"
char cmd[100] = "cmd: "; //commande entrée par le joueur dans la "command line"
_Bool recalcul = 0; //indique à la fct reveler_tuile() si elle doit recalculer la tuile même si elle est déjà révélée
_Bool pause = 0; //indique si le jeu est en pause ou non
_Bool fin_de_partie = 0; //indique si la partie est terminée ou pas
_Bool pop_up_fin_de_partie = 0; //indique au jeu que la partie est terminée
_Bool pop_up_systeme = 1; //indique à l'application si elle doit utiliser les pop-up du système ou créer les siens (À faire!)
_Bool confirmation_quitter = 0; //indique si le jeu doit toujours demander une confirmation avant de quitter une partie en cours (À FAIRE!)


//Liste des fonctions:
int init(); //initialise l'application
void rafraichir(enum zone); //redessine la fenêtre du jeu
_Bool reveler_tuile(int, int); //révèle une tuile si elle n'est pas une bombe
void executer_cmd(); //exécute la commande entrée dans la "command line"
void quitter(int); //ferme le programme


int main (int argc, char *argv[])
{
	int erreur = 0; //code d'erreur reçu d'une fonction à renvoyer à quitter()
	char msg_erreur[200] = "Aucune erreur."; //message d'erreur correspondant à afficher
	_Bool keymod = 0; //indique si l'utilisateur est en train d'appuyer sur Shift, Ctrl ou Alt
	_Bool render_a_faire = 0; //indique au programme qu'il n'a pas encore redessiné la fenêtre (utilisé dans certains cas spécifiques seulement)
	SDL_Event ev;
	
	//Données du pop-up de confirmation pour fermer l'application:
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
	
	//Initialisation du programme:
	printf("Jeu de Minesweeper codé en C.\nVersion %s\n---\n", VERSION);
	erreur = init();
	
	//Erreur fatale lors de l'initialisation:
	if (erreur < 0 && erreur >= -5)
	{
		sprintf(msg_erreur, "Erreur fatale %d: impossible de démarrer l'application.\nOuvrez l'application depuis la console pour en savoir plus.", -erreur);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Erreur", msg_erreur, NULL); //affiche un pop-up d'erreur
		printf("Le programme s'est terminé avec le code d'erreur %d.\n", -erreur);
		return erreur;
	}
	
	//Main loop:
	while (1)
	{
		//Timer:
		//À FAIRE!!!
		
		if (nbre_tuiles_restantes <= 0)
		{fin_de_partie = 1;}
		
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
							if (keymod)
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
							else
							{reveler_tuile(pos_grille_x[0], pos_grille_y[0]);}
							rafraichir(0);
							break;
						
						case bouton_podium:
							//À venir!
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
							//À venir!
							break;
						
						case bouton_nouvelle_partie:
							//À venir!
							break;
						
						case bouton_reglages:
							//À venir!
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
					else //révélation d'une tuile
					{
						if (reveler_tuile(pos_grille_x[1], pos_grille_y[1]) && !pop_up_fin_de_partie) //fin de partie (bombe révélée)
						{/*À faire!*/}
						else
						{rafraichir(sur_grille);}
					}
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
				{/*À faire!*/}
				else if (ev.button.x >= (xmax + marge_droite - 150) / 2 && ev.button.y >= ymax / 2 + 120 && ev.button.x <= (xmax + marge_droite + 150) / 2 && ev.button.y <= ymax / 2 + 160) //bouton nouvelle partie
				{/*À faire!*/}
				else if (ev.button.x >= (xmax + marge_droite - 150) / 2 && ev.button.y >= ymax / 2 + 180 && ev.button.x <= (xmax + marge_droite + 150) / 2 && ev.button.y <= ymax / 2 + 220) //bouton réglages
				{/*À faire!*/}
				break;
			
			case SDL_TEXTINPUT:
				if (cmd_line && cmd[98] == '\000')
				{strcat(cmd, ev.text.text); rafraichir(0);}
				break;
			}
		}
	}
}


int init ()
//Initialise le programme.
//Renvoie un code d'erreur négatif ou 0 en cas de succès.
{
	SDL_Surface* icone; //surface contenant l'icone de l'application, qui sera assignée à la fenêtre de celle-ci
	SDL_Surface* surface_nbre; //surface qui contiendra un nombre (texte) à transformer en texture
	tuile* ptr_temp = NULL; //ptr temporaire servant à la création de la grille
	int nouv_bombe = -1; //variable temporaire utilisée pour indiquer le "numéro" de la case qui contiendra la prochaine bombe
	int col = -1; //variable temporaire servant à identifier la colonne de la case qui contiendra la prochaine bombe
	int erreur = 0; //erreur mineure (non-fatale) à renvoyer au main
	char nbre_a_afficher[5] = "?"; //string qui contiendra le nbre à transformer en texture
	
	//Initialisation de SDL et compagnie:
	
	if (SDL_Init(SDL_INIT_VIDEO) < 0) //initialisation de SDL
	{printf("Erreur lors de l'initialisation de SDL.\n(%s)\n", SDL_GetError()); return -1;}
	
	if (TTF_Init() < 0) //initialisation de SDL_ttf
	{printf("Erreur lors de l'initialisation de SDL_ttf.\n(%s)\n", TTF_GetError()); SDL_Quit(); return -4;}
	
	//Chargement de la police ttf:
	police = TTF_OpenFont("./source/FreeSerif.ttf", 22);
	if (police == NULL)
	{printf("Erreur lors du chargement de la police ttf:\n%s\n", TTF_GetError()); TTF_Quit(); SDL_Quit(); return -5;}
	
	//Création de la fenêtre:
	fenetre = SDL_CreateWindow("Minesweeper", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1000, 700, SDL_WINDOW_RESIZABLE);
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
	if (curseur_normal == NULL || curseur_txt == NULL) //erreur non-fatale (au pire, on restera stuck avec un curseur normal...)
	{printf("Erreur lors de la création des curseurs système texte et normal: %s.\n\n", SDL_GetError()); erreur = -10;}
	
	//Création de l'icone de la fenêtre (À FAIRE!):
	//icone = IMG_Load("./source/icone.png");
	//if (icone == NULL) //erreur non-fatale (au pire, on va s'en passer, de l'icone...)
	//{printf("Erreur lors de l'assignation de l'icone à la fenêtre du jeu (%s).\n\n", SDL_GetError()); erreur = -11;}
	//else
	//{SDL_SetWindowIcon(fenetre, icone); SDL_FreeSurface(icone);}
	
		
	//Initalisation des données du programme:
	
	srand(time(NULL)); //initialise le random number generator avec une seed aléatoire
	
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
		//printf("Bombe %d: #%d (col %d, ligne %d)\n", compteur, nouv_bombe, col, nouv_bombe - col * nbre_lignes); //pour déboggage (indique où se trouve chaque bombe)
		if (grille[col][nouv_bombe - col * nbre_lignes].etat != bombe)
		{grille[col][nouv_bombe - col * nbre_lignes].etat = bombe;} //Marque cette tuile comme étant une bombe...
		else
		{compteur--;} //...si elle n'en est pas déjà une!!!
	}
	nbre_tuiles_restantes = (nbre_col * nbre_lignes) - nbre_bombes;
	
	//Création de la texture des icones et des symboles:
	//(je fais ça tout de suite, parce que j'imagine que ça va sauver du temps vs le refaire à chaque frame...)
	texture_icone_podium = IMG_LoadTexture(rend, icone_podium);
	texture_symbole_pause = IMG_LoadTexture(rend, symbole_pause);
	texture_symbole_fin_de_partie = IMG_LoadTexture(rend, symbole_fin_de_partie);
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
	if (texture_symbole_fin_de_partie == NULL) //erreur non-fatale
	{printf("Erreur lors du chargement du symbole de fin de partie (%s).\nLe dossier \"source\" a-t-il été altéré ou déplacé?\n\n", SDL_GetError()); erreur = -12;}
	else
	{SDL_SetTextureColorMod(texture_symbole_fin_de_partie, 0, 0, 0);}
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
	for (int compteur = 0; compteur <= 8; compteur++)
	{
		sprintf(nbre_a_afficher, "%d", compteur);
		surface_nbre = TTF_RenderUTF8_Solid_Wrapped(police, nbre_a_afficher, couleur_score, taille);
		taille_nbre[compteur].w = surface_nbre->w;
		taille_nbre[compteur].h = surface_nbre->h;
		texture_nbre[compteur] = SDL_CreateTextureFromSurface(rend, surface_nbre);
		SDL_FreeSurface(surface_nbre);
	}
	
	
	//Dessin de la fenêtre:
	rafraichir(0);
	return erreur;
}


void rafraichir (enum zone curseur)
//Redessine la fenêtre de l'application.
//Reçoit en paramètre la "zone" de l'écran où se trouve le curseur.
{
	SDL_Rect rect_icone_podium = {xmax - 69, 15, 41, 41}; //41, parce que 40 détruit complètement l'icone...
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
	rect_arrondi(xmax - 80, 20, 60, 40, couleur_boutons, fond, rend); //bouton "podium"
	
	//Affichage des symbole "pause" ou "fin de partie" (si nécessaire):
	if (pause && texture_symbole_pause != NULL)
	{SDL_RenderCopy(rend, texture_symbole_pause, NULL, &rect_symbole_pause);}
	if (fin_de_partie && texture_symbole_fin_de_partie != NULL)
	{SDL_RenderCopy(rend, texture_symbole_fin_de_partie, NULL, &rect_symbole_pause);}
	
	//Modification de l'élément "sélectionné" par la souris (hovering):
	switch (curseur)
	{
	case sur_grille:
		rect_arrondi(marge_gauche + pos_grille_x[1] * (taille + 5), pos_grille_y[1] * taille + pos_grille_y[1] * 5 + (ymax - nbre_lignes * (taille + 5)) / 2, taille, taille, couleur_selection_curseur, fond, rend);
		break;
	
	case bouton_podium:
		rect_arrondi(xmax - 80, 20, 60, 40, couleur_selection_curseur, fond, rend);
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
		rect_arrondi(xmax - 80, 20, 60, 40, couleur_selection_clavier, fond, rend);
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
				if (fin_de_partie && grille[x][y].etat != bombe && texture_drapeau_mal_place != NULL)
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
			
			else if (fin_de_partie) //bombes
			{
				rect_drapeau.x = x * (taille + 5) + marge_gauche;
				rect_drapeau.y = y * (taille + 5) + (ymax - nbre_lignes * (taille + 5)) / 2;
				if (tuile_finale[0] == x && tuile_finale[1] == y && texture_bombe_finale != NULL)
				{SDL_RenderCopy(rend, texture_bombe_finale, NULL, &rect_drapeau);}
				else if (texture_bombe != NULL)
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
		fin_de_partie = 1;
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
//Pourrait aussi être utilisée par certains réglages afin d'éviter une duplication du code...
{
	for (int compteur = 5; cmd[compteur] != '\000'; compteur++)
	{cmd[compteur - 5] = cmd[compteur]; cmd[compteur - 4] = '\000';}
	
	if (!strcmp(cmd, "aide") || !strcmp(cmd, "ls") || !strcmp(cmd, "?"))
	{
		printf("\nListe des commandes:\n--------------------\n- recalculer (rc) = recalcule (et révèle) la valeur d'une tuile\n- recalculer tout (rct) = recalcule (et révèle) la valeur de chaque tuile qui n'est pas une bombe\n");
		printf("- miner (m+) = transforme une tuile en bombe\n- déminer / cacher (m-) = démine et cache une tuile\n- aide / ls (?) = affiche ce message dans le terminal\n");
		printf("- drapeau / marquer (d) = place un drapeau sur une tuile s'il n'y en avait pas ou l'enlève s'il y en avait un\n--------------------\n");
		printf("* Lorsqu'une commande fait référence à une tuile, il s'agit de la tuile précédemment sélectionnée avec le clavier.\n\n");
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
	
	cmd_line = 0;
	strcpy(cmd, "cmd: ");
	rafraichir(0);
}


void quitter (int erreur)
//Permet de quitter le programme "en toute sécurité" (en fermant bien SDL et en libérant toute mémoire assignée).
{	
	for (int compteur = 0; compteur < nbre_col; compteur++)
	{free(grille[compteur]);}
	if (erreur != 6)
	{free(grille);}
	
	if (texture_icone_podium != NULL)
	{SDL_DestroyTexture(texture_icone_podium);}
	if (texture_symbole_pause != NULL)
	{SDL_DestroyTexture(texture_symbole_pause);}
	if (texture_symbole_fin_de_partie != NULL)
	{SDL_DestroyTexture(texture_symbole_fin_de_partie);}
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
	}
	
	SDL_FreeCursor(curseur_normal);
	SDL_FreeCursor(curseur_txt);
	TTF_CloseFont(police);
	
	TTF_Quit();
	IMG_Quit();
	
	SDL_DestroyRenderer(rend);
	SDL_DestroyWindow(fenetre);
	
	SDL_Quit();
	
	printf("---\n");
	if (erreur != 0)
	{printf("Le programme s'est terminé avec le code d'erreur %d.\n", -erreur);}
	else
	{printf("À bientôt!\n");}
	exit(erreur);
}
