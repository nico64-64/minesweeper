#include "outils_graphiques.c"


#define VERSION "0.2" //version du programme
#define OS "Linux" //OS pour lequel le programme est compilé

#define NBRE_PARAMS 4 //nbre de paramètres modifiables (jusqu'à 9)
#define OPTION_VIDE {"aucun", "aucun", 0, NULL} //raccourcis d'écriture pour une option vide/inutilisée


//Structure d'une option pour un paramètre:
struct option
{
	char nom[60]; //nom de l'option
	char descr[200]; //courte description de l'option (ne sera pas tjrs affichée)
	_Bool non_applicable; //indique si l'option peut être cliquée ou pas
	void (*fct)(); //fonction qui gère l'application des modifications
	// \-> Ça aurait aussi pu être un int, char, _Bool... (!!!)
};

//Structure d'un paramètre modifiable:
struct parametre
{
	char nom[50]; //nom du paramètre
	char descr[200]; //courte description toujours affichée
	char details[300]; //détails supplémentaires qui ne seront pas toujours affichés
	int nbre_options; //nombre d'options disponibles avec ce paramètre (jusqu'à 4)
	struct option option[4]; //options disponibles
	int defaut; //indique quelle option est l'option par défaut (0 à 3) ou -1 s'il n'y en a pas / ne s'applique pas
};

//Liste des zones sélectionnables des réglages:
enum zone_reglages
{
	accueil = 0, //non-défini
	//1 à 9 = boutons des paramètres #1 à #9 (écran d'accueil seulement)
	bouton_retour = 10,
	option1 = 11, //lors de la modification d'un paramètre seulement (incluant les 3 zones suivantes)
	option2,
	option3,
	option4,
	lien_details = 16,
	bouton_retour_details
};


//Liste des fonctions de ce fichier:
void reglages(); //Initialise et désinitialise les réglages. À appeler pour y accéder.
void reglages_menu(); //Gère l'accueil des réglages du jeu.
void modifier_param(int num /*numéro du paramètre à modifier*/); //permet au joueur de consulter/modifier un paramètre
void mod_dimensions_fenetre (int* x, int* y, char nom_fenetre[]); //permet de modifier la taille par défaut d'une fenêtre
//Liste des fonctions modifiant les paramètres:
void mod_couleurs_grille(); //modifie les couleurs de la grille
void mod_police(); //modifie la texture des nbres ds la grille
void mod_zeros(); //affiche/masque les zéros dans la grille
void mod_theme(); //switch du thème "vide" au thème "plein" et vice-versa
void mod_couleur_icones(); //change la couleur d'affichage des icones
void mod_icones_perso(); //permet de charger et utiliser ses propres icones personalisées
void mod_popup_quitter(); //affiche (ou pas) un pop-up lorsqu'on clique sur le "X"
void mod_fenetre_principale(); //modifie les dimensions de la fenêtre principale
void mod_fenetre_reglages(); //modifie les dimensions de la fenêtre des réglages
void mod_fenetre_podium(); //modifie les dimensions de la fenêtre du podium


//Couleurs des objets:
SDL_Color fond = blanc; //couleur de l'arrière-plan de la fenêtre
SDL_Color couleur_grille = gris; //couleur des carrés la grille
SDL_Color couleur_score = noir; //couleur des nbres dans la grille, du texte indiquant le "score" du joueur et de bien d'autres choses...
SDL_Color couleur_timer = noir; //couleur du timer
SDL_Color couleur_liens = bleu; //couleur des liens cliquables
SDL_Color couleur_boutons = gris_pale; //couleur des boutons
SDL_Color couleur_boutons_bloques = gris_fonce; //couleur des boutons que l'utilisateur ne peut pas cliquer (non-disponibles)
SDL_Color couleur_txt_boutons = noir; //couleur du texte affiché sur les boutons
SDL_Color couleur_selection_clavier = bleu; //couleur des carrés de la grille ou des boutons lorsque le focus du clavier est par dessus
SDL_Color couleur_selection_curseur = bleu_efface; //couleur des carrés de la grille ou des boutons lorsque le curseur est par dessus
SDL_Color couleur_tuile[9] = {transparent, vert_pale, jaune_pale, jaune_orange, orange, orange_fonce, rouge, rouge_fonce, rouge_tres_fonce}; //couleur des tuiles révélées selon le nombre de bombes adjacentes (de 0 à 8)

//Fichiers contenant les images et les polices utilisées par le jeu:
char icone_podium[35] = "./source/icone_podium.png";
char symbole_pause[35] = "./source/symbole_pause.png";
char symbole_fin_de_partie[40] = "./source/symbole_fin_de_partie.png";
char image_bombe[35] = "./source/symbole_bombe.png";
char icone_drapeau[35] = "./source/icone_drapeau.png";
char nom_police[45] = "./source/FreeSerif.ttf";
char nom_petite_police[45] = "./source/FreeSerif.ttf";

//Textures des objets:
SDL_Texture* texture_icone_podium = NULL; //texture contenant l'icone du podium (déclarée ici car elle est créée par init() et libérée par quitter())
SDL_Texture* texture_symbole_pause = NULL; //texture contenant le symbole "pause" (déclarée ici pour les mêmes raisons que la précédente)
SDL_Texture* texture_symbole_fin_de_partie = NULL; //texture contenant le symbole de fin de partie (victoire)
SDL_Texture* texture_symbole_fin_de_partie_defaite = NULL; //texture contenant le symbole de fin de partie (défaite)
SDL_Texture* texture_bombe = NULL; //texture contenant l'image d'une bombe (utilisée uniquement en fin de partie)
SDL_Texture* texture_bombe_finale = NULL; //texture contenant l'image d'une bombe (utilisée uniquement en fin de partie)
SDL_Texture* texture_drapeau = NULL; //texture contenant l'image du drapeau
SDL_Texture* texture_drapeau_mal_place = NULL; //texture contenant l'image d'un drapeau mal placé
SDL_Texture* texture_nbre[9]; //array de textures contenant les chiffres/nombres de 0 à 8

//Variables globales liées au graphisme des réglages:
SDL_Window* fenetre_reglages;
Uint32 ID_fenetre_reglages;
SDL_Renderer* rend_r;

//Variables globales liées aux réglages:
const _Bool pop_up_systeme = 1; //indique à l'application si elle doit utiliser les pop-up du système ou créer les siens (Abandonné...)
_Bool confirmation_quitter = 0; //indique si le jeu doit toujours demander une confirmation avant de quitter une partie en cours
_Bool afficher_zeros = 0; //indique au programme qu'il doit afficher les zéros sur la grille
int largeur_fenetre[3] = {1000, 800, 800}; //largeur (en pixels) des 3 fenêtres du programme (dans l'ordre: principale, réglages, podium)
int hauteur_fenetre[3] = {700, 700, 700}; //longueur (en pixels) des 3 fenêtres du programme (dans l'ordre: principale, réglages, podium)
int taille_police_normale = 22;
int taille_petite_police = 19;

//Liste des paramètres modifiables:
struct parametre param[NBRE_PARAMS] =
{
	{"apparence générale", "Vous pouvez modifier ici l'apparence de la grille (plateau de jeu) et du reste de l'application.", "Aucune information supplémentaire.", 3, \
		{{"Modifier les couleurs de la grille", "Cliquer ici pour modifier la coloration des tuiles qui ne sont pas des bombes.", 0, mod_couleurs_grille}, \
		{"Modifier les polices du jeu", \
		"Permet de changer les polices utilisées par le jeu ainsi que de modifier leur taille et leur couleur, incluant les nombres affichés sur les tuiles de la grille qui ne sont pas des bombes.", 0, mod_police}, \
		{"Afficher les zéros dans la grille", "Cliquer ici pour afficher ou masquer le chiffre \"0\" sur les tuiles qui ne sont pas adjacentes à aucune bombe.", 0, mod_zeros}, OPTION_VIDE}, -1},
	{"choix des symboles", "Vous pouvez choisir ici quels symboles vous souhaitez utiliser.", "Il est conseillé d'utiliser un des 2 thèmes (symboles pleins ou vides) plutôt que des icones tierces.", 3, \
		{{"Utiliser les symboles vides", "Utiliser des symboles pleins ou vides pour les icones (podium, drapeaux, bombes, etc.).\nThèmes par défaut.", 0, mod_theme}, \
		{"Utiliser des symboles personnalisés", "Importer et utiliser des symboles autres que ceux fournis avec le programme.\nNon-recommandé.", 0, mod_icones_perso}, \
		{"Changer les couleurs des symboles", "Modifier les couleurs des symboles utilisés pour les icones (podium, drapeaux, bombes, etc.).\nFonctionne seulement avec les icones des 2 thèmes du programme.", 0, mod_couleur_icones}}, \
		0},
	{"confirmation avant de quitter", "L'application doit-elle toujours demander une confirmation avant de se fermer?", \
		"Un pop-up de confirmation s'affichera toujours lorsque vous appuyez sur \"escape\".\nCeci contrôle l'apparition du pop-up lorsque vous cliquez sur le \"x\" en haut de la fenêtre.", 2, \
		{{"Seulement si \"Escape\" est appuyé", \
		"Cliquez ici pour que le programme ne demande une confirmation avant de quitter que si \"Escape\" est appuyé.\n(I.e. cliquer sur le \"X\" de la fenêtre fermera immédiatement le programme.)", 1, mod_popup_quitter}, \
		{"Toujours demander une confirmation", "Choisissez cette option si vous voulez que le programme demande toujours une confirmation avant de quitter.", 0, mod_popup_quitter}, OPTION_VIDE, OPTION_VIDE}, 0},
	{"taille des fenêtres à l'ouverture", "Vous pouvez décider ici de la taille que doivent avoir les fenêtres de l'application à leur apparition.", "La taille minimale des fenêtres ne sera pas modifiée.", 3, \
		{{"Taille de la fenêtre principale", "Cliquez ici pour modifier les dimensions de la fenêtre principale (celle qui affiche le menu principal et le jeu).", 0, mod_fenetre_principale}, \
		{"Taille de la fenêtre des réglages", "Cliquez ici pour modifier les dimensions de la fenêtre des réglages (celle-ci).", 0, mod_fenetre_reglages}, \
		{"Taille de la fenêtre du podium", "Cliquez ici pour modifier les dimensions de la fenêtre du podium (celle qui affiche le palmarès des meilleurs temps).", 0, mod_fenetre_podium}, OPTION_VIDE}, -1}
};


//Variables globales externes utilisées par les fonctions de ce fichier:
extern int erreur;
extern int taille;
extern SDL_Rect taille_nbre[9];

//Fonctions externes utilisées par les fonctions de ce fichier:
extern void quitter();


void reglages ()
//Fonction à appeler pour accèder aux réglages.
//Crée une nouvelle fenêtre (et son renderer) pour les réglages, puis la détruit lorsque terminé.
//Appelle "reglages_menu()" pour faire la "vraie job".
{
	//Création de la fenêtre:
	fenetre_reglages = SDL_CreateWindow("Minesweeper - Réglages", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, largeur_fenetre[1], hauteur_fenetre[1], SDL_WINDOW_RESIZABLE);
	if (fenetre_reglages == NULL)
	{printf("Erreur lors de la création de la fenêtre SDL des réglages:\n%s\n", SDL_GetError()); erreur = -52; return;}
	
	//Création du renderer:
	rend_r = SDL_CreateRenderer(fenetre_reglages, -1, 0);
	if (rend_r == NULL)
	{printf("Erreur lors de la création du renderer SDL des réglages:\n%s\n", SDL_GetError()); SDL_DestroyWindow(fenetre_reglages); erreur = -53; return;}
	SDL_SetRenderDrawBlendMode(rend_r, SDL_BLENDMODE_BLEND); //permet l'utilisation de couleurs semi-transparentes (et transparentes)
	
	//Taille minimale et ID de la fenêtre:
	SDL_SetWindowMinimumSize(fenetre_reglages, 580, 570); //doit être placé après la création du renderer pour que ça marche (bug)
	ID_fenetre_reglages = SDL_GetWindowID(fenetre_reglages);
	SDL_SetWindowResizable(fenetre, SDL_FALSE); //la fenêtre principale n'a plus d'affaire à se faire resizer...
	SDL_GetWindowSize(fenetre_reglages, &xmax, &ymax); //xmax et ymax se rapportent maintenant aux dimensions de la fenêtre des réglages...
	SDL_RaiseWindow(fenetre_reglages);
	
	//Gestion des réglages:
	reglages_menu();
	
	//Destruction de la fenêtre et du renderer des réglages:
	SDL_DestroyRenderer(rend_r);
	SDL_DestroyWindow(fenetre_reglages);
	SDL_SetWindowResizable(fenetre, SDL_TRUE);
	SDL_GetWindowSize(fenetre, &xmax, &ymax); //xmax et ymax sont réinitialisés avec les valeurs de la fenêtre principale
}

void reglages_menu ()
//Gère le menu d'accueil des réglages.
{
	char buffer[150];
	SDL_Event ev; //ce qui vient de se passer...
	enum zone_reglages focus = 0; //sélection clavier
	enum zone_reglages curseur = 0; //sélection curseur
	
	while (1)
	{
		//Affichage:
		SDL_SetColor(fond, rend_r);
		SDL_RenderClear(rend_r);
		TTF_SetFontStyle(police, TTF_STYLE_UNDERLINE);
		afficher_txt_centre("Réglages:", 0, xmax, 10, police, couleur_timer, rend_r);
		TTF_SetFontStyle(police, TTF_STYLE_NORMAL);
		
		if (focus == 10)
		{rect_arrondi(xmax - 170, ymax - 60, 150, 40, couleur_selection_clavier, fond, rend_r);}
		else
		{rect_arrondi(xmax - 170, ymax - 60, 150, 40, couleur_boutons, fond, rend_r);}
		if (curseur == 10)
		{rect_arrondi(xmax - 170, ymax - 60, 150, 40, couleur_selection_curseur, fond, rend_r);}
		afficher_txt_centre("retour", xmax - 170, xmax - 20, ymax - 50, police, couleur_txt_boutons, rend_r);
		
		sprintf(buffer, "Minesweeper en C, version %s sur %s.", VERSION, OS);
		afficher_txt(buffer, 20, ymax - 40, xmax - 210, petite_police, couleur_timer, rend_r);
		
		for (int compteur = 0; compteur < NBRE_PARAMS; compteur++)
		{
			if (focus - 1 == compteur)
			{rect_arrondi(xmax / 4, 50 + compteur * (50 + (ymax - 500) / 10), xmax / 2, (35 + (ymax - 500) / 20), couleur_selection_clavier, fond, rend_r);}
			else
			{rect_arrondi(xmax / 4, 50 + compteur * (50 + (ymax - 500) / 10), xmax / 2, (35 + (ymax - 500) / 20), couleur_boutons, fond, rend_r);}
			if (curseur - 1 == compteur)
			{rect_arrondi(xmax / 4, 50 + compteur * (50 + (ymax - 500) / 10), xmax / 2, (35 + (ymax - 500) / 20), couleur_selection_curseur, fond, rend_r);}
			
			afficher_txt_centre(param[compteur].nom, xmax / 4, 3 * xmax / 4, 60 + compteur * (50 + (ymax - 500) / 10), police, couleur_txt_boutons, rend_r);
		}		
		
		SDL_RenderPresent(rend_r);
		SDL_WaitEvent(&ev);
		
		switch (ev.type)
		{
		case SDL_WINDOWEVENT:
			if (ev.window.windowID != ID_fenetre_reglages) //si l'utilisateur joue avec l'autre fenêtre (lui donnant ainsi le focus, déclenchant cet event), on veut le ramener dans les réglages
			{SDL_RaiseWindow(fenetre_reglages); SDL_FlashWindow(fenetre_reglages, SDL_FLASH_UNTIL_FOCUSED);}
			else if (ev.window.event == SDL_WINDOWEVENT_CLOSE) //SDL_QUIT ne fonctionne pas avec plusieurs fenêtres ouvertes...
			{return;}
			else
			{SDL_GetWindowSize(fenetre_reglages, &xmax, &ymax);}
			break;
		
		case SDL_KEYDOWN:
			switch (ev.key.keysym.sym)
			{
			case SDLK_ESCAPE:
				return;
				break;
			
			case SDLK_TAB:
				if (!focus || focus == 10)
				{focus = 1;}
				else /*if (focus < 10)*/
				{focus = 10;}
				break;
			
			case SDLK_RIGHT:
			case SDLK_LEFT:
				if (!focus)
				{focus = 1;}
				break;
			
			case SDLK_UP:
				if (!focus)
				{focus = 1;}
				else if (focus == 1)
				{focus = NBRE_PARAMS;}
				else if (focus > 1 && focus <= NBRE_PARAMS)
				{focus--;}
				break;
			
			case SDLK_DOWN:
				if (focus >= 0 && focus < NBRE_PARAMS)
				{focus++;}
				else if (focus == NBRE_PARAMS)
				{focus = 1;}
				break;
			
			case SDLK_RETURN:
			case SDLK_KP_ENTER:
			case SDLK_SPACE:
				if (focus == 10)
				{return;}
				else if (focus > 0 && focus <= NBRE_PARAMS)
				{modifier_param(focus - 1);}
				break;
			}
			break;
		
		case SDL_MOUSEMOTION:
			if (ev.motion.x >= xmax - 170 && ev.motion.y >= ymax - 60 && ev.motion.x <= xmax - 20 && ev.motion.y <= ymax - 20)
			{curseur = 10;}
			else if (ev.motion.x >= xmax / 4 && ev.motion.x <= 3 * xmax / 4)
			{
				curseur = 0;
				for (int compteur = 0; compteur < NBRE_PARAMS; compteur++)
				{
					if (ev.motion.y >= 50 + compteur * (50 + (ymax - 500) / 10) && ev.motion.y <= 90 + compteur * (50 + (ymax - 500) / 10))
					{curseur = compteur + 1;}
				}
			}
			else
			{curseur = 0;}
			break;
		
		case SDL_MOUSEBUTTONDOWN:
			focus = 0;
			if (curseur == 10)
			{return;}
			else if (curseur > 0 && curseur <= NBRE_PARAMS)
			{modifier_param(curseur - 1); curseur = 0;}
			break;
		}
	}
}

void modifier_param (int num)
//Affiche l'écran permettant au joueur de consultez et/ou modifier un paramètre.
//Doit recevoir le numéro de ce paramètre (sa position dans l'array "param") en paramètre.
{
	SDL_Event ev;
	enum zone_reglages focus = 0;
	enum zone_reglages curseur = 0;
	int afficher_details = 0; //0 = normal, -1 = détails généraux, 1-4 = détails du paramètre
	
	while (1)
	{
		//Redessinage:
		SDL_SetColor(fond, rend_r);
		SDL_RenderClear(rend_r);
		
		afficher_txt(param[num].descr, xmax / 6, 80, 2 * xmax / 3, police, couleur_timer, rend_r); //description
		TTF_SetFontStyle(police, TTF_STYLE_UNDERLINE);
		afficher_txt_centre(param[num].nom, 0, xmax, 20, police, couleur_timer, rend_r); //titre
		
		if (curseur == lien_details || focus == lien_details)
		{TTF_SetFontStyle(police, TTF_STYLE_BOLD | TTF_STYLE_UNDERLINE);}
		afficher_txt("en savoir plus", xmax / 6, 260, 200, police, couleur_liens, rend_r); //lien "en savoir plus"
		TTF_SetFontStyle(police, TTF_STYLE_NORMAL);
		
		if (param[num].option[0].non_applicable)
		{rect_arrondi(xmax / 16, 300, 3 * xmax / 8, ymax / 8, couleur_boutons_bloques, fond, rend_r);}
		else if (focus == option1)
		{rect_arrondi(xmax / 16, 300, 3 * xmax / 8, ymax / 8, couleur_selection_clavier, fond, rend_r);}
		else
		{rect_arrondi(xmax / 16, 300, 3 * xmax / 8, ymax / 8, couleur_boutons, fond, rend_r);}
		if (curseur == option1)
		{rect_arrondi(xmax / 16, 300, 3 * xmax / 8, ymax / 8, couleur_selection_curseur, fond, rend_r);}
		afficher_txt(param[num].option[0].nom, xmax / 8, 265 + ymax / 16, xmax / 4, police, couleur_txt_boutons, rend_r); //1ère option (haut gauche)
		
		if (param[num].nbre_options >= 2)
		{
			if (param[num].option[1].non_applicable)
			{rect_arrondi(9 * xmax / 16, 300, 3 * xmax / 8, ymax / 8, couleur_boutons_bloques, fond, rend_r);}
			else if (focus == option2)
			{rect_arrondi(9 * xmax / 16, 300, 3 * xmax / 8, ymax / 8, couleur_selection_clavier, fond, rend_r);}
			else
			{rect_arrondi(9 * xmax / 16, 300, 3 * xmax / 8, ymax / 8, couleur_boutons, fond, rend_r);}
			if (curseur == option2)
			{rect_arrondi(9 * xmax / 16, 300, 3 * xmax / 8, ymax / 8, couleur_selection_curseur, fond, rend_r);}
			afficher_txt(param[num].option[1].nom, 5 * xmax / 8, 265 + ymax / 16, xmax / 4, police, couleur_txt_boutons, rend_r); //2e option (haut droite)
		}
		
		if (param[num].nbre_options >= 3)
		{
			if (param[num].option[2].non_applicable)
			{rect_arrondi(xmax / 16, 350 + ymax / 8, 3 * xmax / 8, ymax / 8, couleur_boutons_bloques, fond, rend_r);}
			else if (focus == option3)
			{rect_arrondi(xmax / 16, 350 + ymax / 8, 3 * xmax / 8, ymax / 8, couleur_selection_clavier, fond, rend_r);}
			else
			{rect_arrondi(xmax / 16, 350 + ymax / 8, 3 * xmax / 8, ymax / 8, couleur_boutons, fond, rend_r);}
			if (curseur == option3)
			{rect_arrondi(xmax / 16, 350 + ymax / 8, 3 * xmax / 8, ymax / 8, couleur_selection_curseur, fond, rend_r);}
			afficher_txt(param[num].option[2].nom, xmax / 8, 315 + 3 * ymax / 16, xmax / 4, police, couleur_txt_boutons, rend_r); //3e option (bas gauche)
		}
		
		if (param[num].nbre_options >= 4)
		{
			if (param[num].option[3].non_applicable)
			{rect_arrondi(9 * xmax / 16, 350 + ymax / 8, 3 * xmax / 8, ymax / 8, couleur_boutons_bloques, fond, rend_r);}
			else if (focus == option4)
			{rect_arrondi(9 * xmax / 16, 350 + ymax / 8, 3 * xmax / 8, ymax / 8, couleur_selection_clavier, fond, rend_r);}
			else
			{rect_arrondi(9 * xmax / 16, 350 + ymax / 8, 3 * xmax / 8, ymax / 8, couleur_boutons, fond, rend_r);}
			if (curseur == option4)
			{rect_arrondi(9 * xmax / 16, 350 + ymax / 8, 3 * xmax / 8, ymax / 8, couleur_selection_curseur, fond, rend_r);}
			afficher_txt(param[num].option[3].nom, 5 * xmax / 8, 315 + 3 * ymax / 16, xmax / 4, police, couleur_txt_boutons, rend_r); //4e option (bas droite)
		}
		
		afficher_txt("Faites un clic droit sur une option pour en apprendre davantage à son sujet.", xmax / 16, ymax - 50, 15 * xmax / 16 - 190, petite_police, couleur_timer, rend_r); //petite info pratique...
		if (focus == bouton_retour)
		{rect_arrondi(xmax - 170, ymax - 60, 150, 40, couleur_selection_clavier, fond, rend_r);}
		else
		{rect_arrondi(xmax - 170, ymax - 60, 150, 40, couleur_boutons, fond, rend_r);}
		if (curseur == bouton_retour)
		{rect_arrondi(xmax - 170, ymax - 60, 150, 40, couleur_selection_curseur, fond, rend_r);}
		afficher_txt_centre("retour", xmax - 170, xmax - 20, ymax - 50, police, couleur_txt_boutons, rend_r); //bouton retour
		
		if (afficher_details != 0)
		{
			//"Pop-up":
			rectangle(xmax / 6, ymax / 5, 2 * xmax / 3, 3 * ymax / 5, 0, couleur_boutons, fond, rend_r);
			rectangle(xmax / 6, ymax / 5, 2 * xmax / 3, 3 * ymax / 5, 5, couleur_timer, fond, rend_r);
			
			//Titre et détails:
			TTF_SetFontStyle(police, TTF_STYLE_UNDERLINE);
			afficher_txt_centre("Détails:", xmax / 6, 5 * xmax / 6, ymax / 5 + 20, police, couleur_txt_boutons, rend_r);
			TTF_SetFontStyle(police, TTF_STYLE_NORMAL);
			if (afficher_details == -1)
			{afficher_txt(param[num].details, xmax / 6 + 20, ymax / 5 + 70, 2 * xmax / 3 - 40, police, couleur_txt_boutons, rend_r);}
			else
			{afficher_txt(param[num].option[afficher_details - 1].descr, xmax / 6 + 20, ymax / 5 + 70, 2 * xmax / 3 - 40, police, couleur_txt_boutons, rend_r);}
			
			//Bouton retour:
			rect_arrondi(5 * xmax / 6 - 180, 4 * ymax / 5 - 60, 150, 40, fond, couleur_boutons, rend_r);
			if (curseur == bouton_retour_details)
			{rect_arrondi(5 * xmax / 6 - 180, 4 * ymax / 5 - 60, 150, 40, couleur_selection_curseur, couleur_boutons, rend_r);}
			else if (focus == bouton_retour_details)
			{rect_arrondi(5 * xmax / 6 - 180, 4 * ymax / 5 - 60, 150, 40, couleur_selection_clavier, couleur_boutons, rend_r);}
			afficher_txt_centre("retour", 5 * xmax / 6 - 180, 5 * xmax / 6 - 30, 4 * ymax / 5 - 50, police, couleur_timer, rend_r);
		}
		
		SDL_RenderPresent(rend_r);
		
		//Gestion de l'input:
		SDL_WaitEvent(&ev);
		
		switch (ev.type)
		{
		case SDL_WINDOWEVENT:
			if (ev.window.windowID != ID_fenetre_reglages) //si l'utilisateur joue avec l'autre fenêtre (lui donnant ainsi le focus, déclenchant cet event), on veut le ramener dans les réglages
			{SDL_RaiseWindow(fenetre_reglages); SDL_FlashWindow(fenetre_reglages, SDL_FLASH_UNTIL_FOCUSED);}
			else if (ev.window.event == SDL_WINDOWEVENT_CLOSE) //SDL_QUIT ne fonctionne pas avec plusieurs fenêtres ouvertes...
			{return;}
			else
			{SDL_GetWindowSize(fenetre_reglages, &xmax, &ymax);}
			break;
		
		case SDL_KEYDOWN:
			switch (ev.key.keysym.sym)
			{
			case SDLK_ESCAPE:
				if (afficher_details != 0)
				{afficher_details = 0; focus = lien_details;}
				else
				{return;}
				break;
			
			case SDLK_TAB:
				if (afficher_details != 0)
				{
					if (focus != bouton_retour_details)
					{focus = bouton_retour_details;}
				}
				else if (!focus || focus == lien_details)
				{focus = option1;}
				else if (focus == bouton_retour)
				{focus = lien_details;}
				else //options 1 à 4
				{focus = bouton_retour;}
				break;
			
			case SDLK_UP:
			case SDLK_DOWN:
				if (afficher_details != 0)
				{
					if (focus != bouton_retour_details)
					{focus = bouton_retour_details;}
				}
				else if (focus == option3 || focus == option4)
				{focus -= 2;}
				else if (focus == option1 && param[num].nbre_options > 2)
				{focus = option3;}
				else if (focus == option2 && param[num].nbre_options > 3)
				{focus = option4;}
				else if (!focus)
				{focus = option1;}
				break;
			
			case SDLK_RIGHT:
			case SDLK_LEFT:
				if (afficher_details != 0)
				{
					if (focus != bouton_retour_details)
					{focus = bouton_retour_details;}
				}
				else if (focus == option1 && param[num].nbre_options > 1)
				{focus = option2;}
				else if (focus == option3 && param[num].nbre_options > 3)
				{focus = option4;}
				else if (focus == option2 || focus == option4)
				{focus--;}
				else if (!focus)
				{focus = option1;}
				break;
			
			case SDLK_RETURN:
			case SDLK_KP_ENTER:
			case SDLK_SPACE:
				if (focus == bouton_retour_details)
				{afficher_details = 0; focus = lien_details;}
				else if (focus == bouton_retour)
				{return;}
				else if (focus == lien_details)
				{afficher_details = -1;}
				else if (focus >= option1 && focus <= option4)
				{param[num].option[focus - option1].fct(); /*focus = 0;*/ return;}
				break;
			}
			//S'assure que le focus ne se ramasse pas sur un bouton bloqué/"non-applicable":
			if (focus >= option1 && focus <= option4)
			{
				while (focus > option1 && param[num].option[focus - option1].non_applicable)
				{focus--;}
				while (focus - option1 < param[num].nbre_options && param[num].option[focus - option1].non_applicable)
				{focus++;}
				if (focus == option4 && param[num].option[focus - option1].non_applicable)
				{focus = lien_details;}
			}
			break;
		
		case SDL_MOUSEMOTION:
			if (afficher_details != 0) //un pop-up affichant des détails est présentement affiché
			{
				if (ev.motion.x >= 5 * xmax / 6 - 180 && ev.motion.y >= 4 * ymax / 5 - 60 && ev.motion.x <= 5 * xmax / 6 - 30 && ev.motion.y <= 4 * ymax / 5 - 20)
				{curseur = bouton_retour_details;}
				else
				{curseur = 0;}
			}
			else if (ev.motion.x >= xmax / 6 && ev.motion.y >= 260 && ev.motion.x <= xmax / 6 + longueur_txt("en savoir plus", 200, police) && ev.motion.y <= 280)
			{SDL_SetCursor(curseur_clic); curseur = lien_details;} //souris sur le lien "plus de détails"
			else //souris sur le bouton retour, une des 4 options ou ailleurs
			{
				SDL_SetCursor(curseur_normal);
				if (ev.motion.x >= xmax - 170 && ev.motion.y >= ymax - 60 && ev.motion.x <= xmax - 20 && ev.motion.y <= ymax - 20)
				{curseur = bouton_retour;}
				else if (!param[num].option[0].non_applicable && ev.motion.x >= xmax / 16 && ev.motion.y >= 300 && ev.motion.x <= 7 * xmax / 16 && ev.motion.y <= ymax / 8 + 300)
				{curseur = option1;}
				else if (param[num].nbre_options >= 2 && !param[num].option[1].non_applicable && ev.motion.x >= 9 * xmax / 16 && ev.motion.y >= 300 && ev.motion.x <= 15 * xmax / 16 && ev.motion.y <= ymax / 8 + 300)
				{curseur = option2;}
				else if (param[num].nbre_options >= 3 && !param[num].option[2].non_applicable && ev.motion.x >= xmax / 16 && ev.motion.y >= ymax / 8 + 350 && ev.motion.x <= 7 * xmax / 16 && ev.motion.y <= ymax / 4 + 350)
				{curseur = option3;}
				else if (param[num].nbre_options >= 4 && !param[num].option[3].non_applicable && ev.motion.x >= 9 * xmax / 16 && ev.motion.y >= ymax / 8 + 350 && ev.motion.x <= 15 * xmax / 16 && ev.motion.y <= ymax / 4 + 350)
				{curseur = option4;}
				else
				{curseur = 0;}
			}
			break;
		
		case SDL_MOUSEBUTTONDOWN:
			focus = 0;
			if (ev.button.button == SDL_BUTTON_RIGHT)
			{
				if (curseur >= option1 && curseur <= option4)
				{afficher_details = curseur - option1 + 1;}
			}
			else
			{
				switch (curseur)
				{
				case bouton_retour:
					return;
					break;
				
				case lien_details:
					afficher_details = -1;
					SDL_SetCursor(curseur_normal);
					break;
				
				case bouton_retour_details:
					afficher_details = 0;
					break;
				
				case option1:
				case option2:
				case option3:
				case option4:
					param[num].option[curseur - option1].fct();
					//curseur = 0;
					return;
					break;
				}
			}
			break;
		}
	}
}


//Fonctions de modification des paramètres (1 fct / paramètre):

void mod_couleurs_grille ()
//Modifie la couleur des tuiles révélées dans la grille
{
	//...
}

void mod_police ()
//Modifie la texture des nbres ds la grille
{
	enum zone
	{
		//0 = indéfini
		nom_moyen = 1,
		nom_petite,
		moyen_plus = 10,
		petite_plus,
		moyen_moins = 15,
		petite_moins,
		retour = 20,
		appliquer
	};
	
	SDL_Event ev;
	TTF_Font* police_backup = NULL;
	TTF_Font* petite_police_backup = NULL;
	char taille_normale[5]; //string contenant la taille de la police normale
	char taille_petite[5]; //string contenant la taille de la petite police
	int longueur_st_m; //longueur du "sous-titre" demandant quelle police moyenne utiliser
	int longueur_st_p; //longueur du "sous-titre" demandant quelle petite police utiliser
	enum zone curseur = 0;
	enum zone focus = 0;
	char buffer[5];
	SDL_Surface* surface_nbre; //surface qui contiendra un nombre (texte) à transformer en texture
	int compteur = 0;
	
	while (1)
	{
		sprintf(taille_normale, "%d", taille_police_normale);
		sprintf(taille_petite, "%d", taille_petite_police);
		longueur_st_m = longueur_txt("Police moyenne à utiliser:", 1000, police);
		longueur_st_p = longueur_txt("Petite police à utiliser:", 1000, police);
		
		SDL_SetColor(fond, rend_r);
		SDL_RenderClear(rend_r);
		TTF_SetFontStyle(police, TTF_STYLE_UNDERLINE);
		afficher_txt_centre("Modifier les polices du jeu:", 0, xmax, 30, police, couleur_timer, rend_r);
		TTF_SetFontStyle(police, TTF_STYLE_NORMAL);
		
		afficher_txt("Police moyenne à utiliser:", 20, 90, longueur_st_m, police, couleur_timer, rend_r);
		rectangle(30 + longueur_st_m, 80, xmax - 50 - longueur_st_m, 40, 0, couleur_boutons, fond, rend_r);
		if (focus == nom_moyen)
		{rectangle(30 + longueur_st_m, 80, xmax - 50 - longueur_st_m, 40, 0, couleur_selection_curseur, fond, rend_r);}
		rectangle(30 + longueur_st_m, 80, xmax - 50 - longueur_st_m, 40, 4, couleur_txt_boutons, fond, rend_r);
		afficher_txt(nom_police, 40 + longueur_st_m, 90, xmax - 70 - longueur_st_m, police, noir, rend_r);
		
		afficher_txt("Petite police à utiliser:", 20, 160, longueur_st_p, police, couleur_timer, rend_r);
		rectangle(30 + longueur_st_p, 150, xmax - 50 - longueur_st_p, 40, 0, couleur_boutons, fond, rend_r);
		if (focus == nom_petite)
		{rectangle(30 + longueur_st_p, 150, xmax - 50 - longueur_st_p, 40, 0, couleur_selection_curseur, fond, rend_r);}
		rectangle(30 + longueur_st_p, 150, xmax - 50 - longueur_st_p, 40, 4, couleur_txt_boutons, fond, rend_r);
		afficher_txt(nom_petite_police, 40 + longueur_st_p, 160, xmax - 70 - longueur_st_p, police, noir, rend_r);
		
		//Taille des polices:
		afficher_txt_centre("Taille de la", xmax / 2 - 250, xmax / 2 - 50, ymax / 2 - 60, police, couleur_timer, rend_r);
		afficher_txt_centre("police normale:", xmax / 2 - 250, xmax / 2 - 50, ymax / 2 - 35, police, couleur_timer, rend_r);
		afficher_txt_centre("Taille de la", xmax / 2, xmax / 2 + 300, ymax / 2 - 60, police, couleur_timer, rend_r);
		afficher_txt_centre("petite police:", xmax / 2, xmax / 2 + 300, ymax / 2 - 35, police, couleur_timer, rend_r);
		for (int compteur = 0; compteur < 2; compteur++)
		{
			rectangle(xmax / 2 - 190 + 300 * compteur, ymax / 2, 80, 45, 0, couleur_boutons, fond, rend_r); //haut
			if (curseur == moyen_plus + compteur)
			{rectangle(xmax / 2 - 190 + 300 * compteur, ymax / 2, 80, 45, 0, couleur_selection_curseur, couleur_boutons, rend_r);}
			else if (focus == moyen_plus + compteur)
			{rectangle(xmax / 2 - 190 + 300 * compteur, ymax / 2, 80, 45, 0, couleur_selection_clavier, couleur_boutons, rend_r);}
			rectangle(xmax / 2 - 190 + 300 * compteur, ymax / 2 + 105, 80, 45, 0, couleur_boutons, fond, rend_r); //bas
			if (curseur == moyen_moins + compteur)
			{rectangle(xmax / 2 - 190 + 300 * compteur, ymax / 2 + 105, 80, 45, 0, couleur_selection_curseur, couleur_boutons, rend_r);}
			else if (focus == moyen_moins + compteur)
			{rectangle(xmax / 2 - 190 + 300 * compteur, ymax / 2 + 105, 80, 45, 0, couleur_selection_clavier, couleur_boutons, rend_r);}
			rectangle(xmax / 2 - 190 + 300 * compteur, ymax / 2, 80, 150, 3, couleur_txt_boutons, couleur_boutons, rend_r); //boîte
			SDL_RenderDrawLine(rend_r, xmax / 2 - 190 + 300 * compteur, ymax / 2 + 44, xmax / 2 - 110 + 300 * compteur, ymax / 2 + 44); //haut
			SDL_RenderDrawLine(rend_r, xmax / 2 - 190 + 300 * compteur, ymax / 2 + 45, xmax / 2 - 110 + 300 * compteur, ymax / 2 + 45);
			SDL_RenderDrawLine(rend_r, xmax / 2 - 190 + 300 * compteur, ymax / 2 + 46, xmax / 2 - 110 + 300 * compteur, ymax / 2 + 46);
			SDL_RenderDrawLine(rend_r, xmax / 2 - 190 + 300 * compteur, ymax / 2 + 103, xmax / 2 - 110 + 300 * compteur, ymax / 2 + 103); //bas
			SDL_RenderDrawLine(rend_r, xmax / 2 - 190 + 300 * compteur, ymax / 2 + 104, xmax / 2 - 110 + 300 * compteur, ymax / 2 + 104);
			SDL_RenderDrawLine(rend_r, xmax / 2 - 190 + 300 * compteur, ymax / 2 + 105, xmax / 2 - 110 + 300 * compteur, ymax / 2 + 105);
			if (!compteur)
			{sprintf(buffer, "%d", taille_police_normale);}
			else
			{sprintf(buffer, "%d", taille_petite_police);}
			afficher_txt_centre(buffer, xmax / 2 - 190 + 300 * compteur, xmax / 2 - 110 + 300 * compteur, ymax / 2 + 65, police, couleur_txt_boutons, rend_r);
			SDL_RenderDrawLine(rend_r, xmax / 2 - 165 + 300 * compteur, ymax / 2 + 30, xmax / 2 - 150 + 300 * compteur, ymax / 2 + 15); // ^
			SDL_RenderDrawLine(rend_r, xmax / 2 - 150 + 300 * compteur, ymax / 2 + 15, xmax / 2 - 135 + 300 * compteur, ymax / 2 + 30);
			SDL_RenderDrawLine(rend_r, xmax / 2 - 165 + 300 * compteur, ymax / 2 + 29, xmax / 2 - 150 + 300 * compteur, ymax / 2 + 14);
			SDL_RenderDrawLine(rend_r, xmax / 2 - 150 + 300 * compteur, ymax / 2 + 14, xmax / 2 - 135 + 300 * compteur, ymax / 2 + 29);
			SDL_RenderDrawLine(rend_r, xmax / 2 - 165 + 300 * compteur, ymax / 2 + 31, xmax / 2 - 150 + 300 * compteur, ymax / 2 + 16);
			SDL_RenderDrawLine(rend_r, xmax / 2 - 150 + 300 * compteur, ymax / 2 + 16, xmax / 2 - 135 + 300 * compteur, ymax / 2 + 31);
			SDL_RenderDrawLine(rend_r, xmax / 2 - 165 + 300 * compteur, ymax / 2 + 120, xmax / 2 - 150 + 300 * compteur, ymax / 2 + 135); // \/
			SDL_RenderDrawLine(rend_r, xmax / 2 - 150 + 300 * compteur, ymax / 2 + 135, xmax / 2 - 135 + 300 * compteur, ymax / 2 + 120);
			SDL_RenderDrawLine(rend_r, xmax / 2 - 165 + 300 * compteur, ymax / 2 + 119, xmax / 2 - 150 + 300 * compteur, ymax / 2 + 134);
			SDL_RenderDrawLine(rend_r, xmax / 2 - 150 + 300 * compteur, ymax / 2 + 134, xmax / 2 - 135 + 300 * compteur, ymax / 2 + 119);
			SDL_RenderDrawLine(rend_r, xmax / 2 - 165 + 300 * compteur, ymax / 2 + 121, xmax / 2 - 150 + 300 * compteur, ymax / 2 + 136);
			SDL_RenderDrawLine(rend_r, xmax / 2 - 150 + 300 * compteur, ymax / 2 + 136, xmax / 2 - 135 + 300 * compteur, ymax / 2 + 121);
		}
		
		rect_arrondi(xmax - 360, ymax - 60, 150, 40, couleur_boutons, fond, rend_r);
		if (curseur == appliquer)
		{rect_arrondi(xmax - 360, ymax - 60, 150, 40, couleur_selection_curseur, fond, rend_r);}
		else if (focus == appliquer)
		{rect_arrondi(xmax - 360, ymax - 60, 150, 40, couleur_selection_clavier, fond, rend_r);}
		afficher_txt_centre("Appliquer", xmax - 360, xmax - 210, ymax - 50, police, couleur_txt_boutons, rend_r);
		
		rect_arrondi(xmax - 170, ymax - 60, 150, 40, couleur_boutons, fond, rend_r);
		if (curseur == retour)
		{rect_arrondi(xmax - 170, ymax - 60, 150, 40, couleur_selection_curseur, fond, rend_r);}
		else if (focus == retour)
		{rect_arrondi(xmax - 170, ymax - 60, 150, 40, couleur_selection_clavier, fond, rend_r);}
		afficher_txt_centre("Terminé", xmax - 170, xmax - 20, ymax - 50, police, couleur_txt_boutons, rend_r);
		
		afficher_txt("Ce texte utilise la petite police.\nTous les autres textes de cette fenêtre utilisent la police normale.", 20, ymax - 110, xmax - 40, petite_police, couleur_timer, rend_r);
		
		SDL_RenderPresent(rend_r);
		SDL_WaitEvent(&ev);
		
		switch (ev.type)
		{
		case SDL_WINDOWEVENT:
			if (ev.window.windowID != ID_fenetre_reglages) //si l'utilisateur joue avec l'autre fenêtre (lui donnant ainsi le focus, déclenchant cet event), on veut le ramener dans les réglages
			{SDL_RaiseWindow(fenetre_reglages); SDL_FlashWindow(fenetre_reglages, SDL_FLASH_UNTIL_FOCUSED);}
			else if (ev.window.event == SDL_WINDOWEVENT_CLOSE) //SDL_QUIT ne fonctionne pas avec plusieurs fenêtres ouvertes...
			{return;}
			else
			{SDL_GetWindowSize(fenetre_reglages, &xmax, &ymax);}
			break;

		case SDL_MOUSEMOTION:
			//if (ev.motion.x < )
			//{
				//...
			//}
			break;
		
		case SDL_KEYDOWN:
			switch (ev.key.keysym.sym)
			{
			case SDLK_ESCAPE:
				return;
				break;
			
			case SDLK_TAB:
				if (focus == 0 || focus >= retour)
				{focus = nom_moyen;}
				else if (focus == nom_moyen || focus == nom_petite)
				{focus = moyen_plus;}
				else
				{focus = appliquer;}
				break;
			
			case SDLK_RIGHT:
			case SDLK_LEFT:
				if (focus == moyen_plus || focus == moyen_moins || focus == retour)
				{focus++;}
				else if (focus == petite_plus || focus == petite_moins || focus == appliquer)
				{focus--;}
				else if (!focus)
				{focus = moyen_plus;}
				break;
			
			case SDLK_UP:
			case SDLK_DOWN:
				if (focus == nom_moyen)
				{focus = nom_petite;}
				else if (focus == nom_petite)
				{focus = nom_moyen;}
				else if (focus == moyen_plus || focus == petite_plus)
				{focus += 5;}
				else if (focus == moyen_moins || focus == petite_moins)
				{focus -= 5;}
				else if (!focus)
				{focus = moyen_plus;}
				break;
			
			case SDLK_RETURN:
			case SDLK_KP_ENTER:
				switch (focus)
				{
				case nom_moyen:
				case nom_petite:
					focus = 0;
					break;
				
				case moyen_plus:
					if (taille_police_normale < 30)
					{taille_police_normale++;}
					break;
				
				case petite_plus:
					if (taille_petite_police < 25)
					{taille_petite_police++;}
					break;
				
				case moyen_moins:
					if (taille_police_normale > 0)
					{taille_police_normale--;}
					break;
				
				case petite_moins:
					if (taille_petite_police > 0)
					{taille_petite_police--;}
					break;
				
				case retour:
				case appliquer:
					police_backup = police;
					petite_police_backup = petite_police;
					//Création des nouvelles polices:
					police = TTF_OpenFont(nom_police, taille_police_normale);
					if (police == NULL)
					{
						printf("Erreur 16: Impossible de charger la nouvelle police normale (%s).\n", TTF_GetError());
						erreur = -16;
						police = police_backup;
					}
					petite_police = TTF_OpenFont(nom_petite_police, taille_petite_police);
					if (petite_police == NULL)
					{printf("Erreur 16: Impossible de charger la nouvelle petite police (%s).\n", TTF_GetError()); erreur = -16; petite_police = petite_police_backup;}
					//Libération des anciennes polices:
					if (police_backup != police)
					{TTF_CloseFont(police_backup);}
					else
					{printf("L'ancienne police a été utilisée en guise de fallback pour la police normale.\n");}
					if (petite_police_backup != NULL && petite_police_backup != petite_police)
					{TTF_CloseFont(petite_police_backup);}
					else if (petite_police == NULL)
					{printf("Aucun fallback trouvé pour la petite police (erreur 17). Certains textes ne seront pas affichés.\n"); erreur = -17;}
					else
					{printf("L'ancienne police a été utilisée en guise de fallback pour la petite police.\n");}
					//Destruction des anciennes textures des nombres dans la grille:
					for (int compteur = 0; compteur < 8; compteur++)
					{
						if (texture_nbre[compteur] != NULL)
						{SDL_DestroyTexture(texture_nbre[compteur]);}
						else if (!erreur && (compteur > 0 || afficher_zeros))
						{erreur = -14; printf("Erreur 14: La texture du chiffre %d n'a pas pu être chargée (au cas où vous ne l'auriez pas remarqué...).\n", compteur);}
					}
					//Création des texture des nbres qui indiqueront combien de bombes sont adjacentes à une tuile:
					for (int compteur = 1 - afficher_zeros; compteur <= 8; compteur++)
					{
						sprintf(buffer, "%d", compteur);
						surface_nbre = TTF_RenderUTF8_Solid_Wrapped(police, buffer, couleur_score, taille);
						taille_nbre[compteur].w = surface_nbre->w;
						taille_nbre[compteur].h = surface_nbre->h;
						texture_nbre[compteur] = SDL_CreateTextureFromSurface(rend, surface_nbre);
						SDL_FreeSurface(surface_nbre);
					}
					//Revient au menu des réglages si on a cliqué sur "terminé":
					if (focus == retour)
					{return;}
					break;
				}
				break;
			
			case SDLK_BACKSPACE:
				if (focus == nom_moyen && nom_police[9] != '\000')
				{
					for (compteur = 0; nom_police[compteur] != '\000'; compteur++) {}
					nom_police[compteur - 1] = '\000';
				}
				if (focus == nom_petite && nom_petite_police[9] != '\000')
				{
					for (compteur = 0; nom_petite_police[compteur] != '\000'; compteur++) {}
					nom_petite_police[compteur - 1] = '\000';
				}
				break;
			}
			break;
		
		case SDL_TEXTINPUT:
			if (focus == nom_moyen)
			{
				for (compteur = 0; nom_police[compteur] != '\000'; compteur++) {}
				if (compteur < 44)
				{strcat(nom_police, ev.text.text);}
			}
			else if (focus == nom_petite)
			{
				for (compteur = 0; nom_petite_police[compteur] != '\000'; compteur++) {}
				if (compteur < 44)
				{strcat(nom_petite_police, ev.text.text);}
			}
			break;
		}
	}
}

void mod_zeros ()
//Affiche/masque les zéros dans la grille
{
	char nbre_a_afficher[5] = "0";
	SDL_Surface* surface_nbre = NULL;
	
	if (texture_nbre[0] == NULL)
	{
		surface_nbre = TTF_RenderUTF8_Solid_Wrapped(police, nbre_a_afficher, couleur_score, taille);
		taille_nbre[0].w = surface_nbre->w;
		taille_nbre[0].h = surface_nbre->h;
		texture_nbre[0] = SDL_CreateTextureFromSurface(rend, surface_nbre);
		SDL_FreeSurface(surface_nbre);
		afficher_zeros = 1;
		strcpy(param[0].option[2].nom, "Masquer les zéros");
	}
	else if (afficher_zeros)
	{afficher_zeros = 0; strcpy(param[0].option[2].nom, "Afficher les zéros");}
	else //if (!afficher_zeros)
	{afficher_zeros = 1; strcpy(param[0].option[2].nom, "Masquer les zéros");}
}

void mod_theme ()
//Switch du thème "vide" au thème "plein" et vice-versa
{
	if (!strcmp(param[1].option[0].nom, "Utiliser les symboles pleins"))
	{
		strcpy(icone_podium, "./source/icone_podium.png");
		strcpy(symbole_pause, "./source/symbole_pause.png");
		strcpy(symbole_fin_de_partie, "./source/symbole_fin_de_partie.png");
		strcpy(image_bombe, "./source/symbole_bombe.png");
		strcpy(icone_drapeau, "./source/icone_drapeau.png");
			
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
		if (texture_icone_podium == NULL)
		{printf("Erreur lors du chargement de l'icone podium (%s).\nLe dossier \"source\" a-t-il été altéré ou déplacé?\n\n", SDL_GetError()); erreur = -12;}
		else
		{SDL_SetTextureColorMod(texture_icone_podium, 0, 0, 0);}
		if (texture_symbole_pause == NULL)
		{printf("Erreur lors du chargement du symbole \"pause\" (%s).\nLe dossier \"source\" a-t-il été altéré ou déplacé?\n\n", SDL_GetError()); erreur = -12;}
		else
		{SDL_SetTextureColorMod(texture_symbole_pause, 0, 0, 0);}
		if (texture_symbole_fin_de_partie == NULL || texture_symbole_fin_de_partie_defaite == NULL)
		{printf("Erreur lors du chargement du symbole de fin de partie (%s).\nLe dossier \"source\" a-t-il été altéré ou déplacé?\n\n", SDL_GetError()); erreur = -12;}
		else
		{SDL_SetTextureColorMod(texture_symbole_fin_de_partie, 0, 0, 0); SDL_SetTextureColorMod(texture_symbole_fin_de_partie_defaite, 143, 23, 23);}
		if (texture_bombe == NULL || texture_bombe_finale == NULL)
		{printf("Erreur lors du chargement de l'image d'une mine (%s).\nLe dossier \"source\" a-t-il été altéré ou déplacé?\n\n", SDL_GetError()); erreur = -12;}
		else
		{SDL_SetTextureColorMod(texture_bombe, 0, 0, 0); SDL_SetTextureColorMod(texture_bombe_finale, 143, 23, 23);}
		if (texture_drapeau == NULL || texture_drapeau_mal_place == NULL)
		{
			printf("Erreur lors du chargement de l'icone drapeau (%s).\nLe dossier \"source\" a-t-il été altéré ou déplacé?\n\n", SDL_GetError());
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Erreur", "L'icone drapeau n'a pas pu être chargée.\nUn \"X\" remplacera donc les drapeaux.\nConsultez la console pour plus de détails.", NULL);
			erreur = -13;
		}
		else
		{SDL_SetTextureColorMod(texture_drapeau, 0, 0, 0); SDL_SetTextureColorMod(texture_drapeau_mal_place, 143, 23, 23);}
		
		strcpy(param[1].option[0].nom, "Utiliser les symboles vides");
	}
	else
	{
		strcpy(icone_podium, "./source/icone_podium_vide.png");
		strcpy(symbole_pause, "./source/symbole_pause_vide.png");
		strcpy(symbole_fin_de_partie, "./source/symbole_fin_de_partie_vide.png");
		strcpy(image_bombe, "./source/symbole_bombe_vide.png");
		strcpy(icone_drapeau, "./source/icone_drapeau_vide.png");
		
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
		if (texture_icone_podium == NULL)
		{printf("Erreur lors du chargement de l'icone podium (%s).\nLe dossier \"source\" a-t-il été altéré ou déplacé?\n\n", SDL_GetError()); erreur = -12;}
		else
		{SDL_SetTextureColorMod(texture_icone_podium, 0, 0, 0);}
		if (texture_symbole_pause == NULL)
		{printf("Erreur lors du chargement du symbole \"pause\" (%s).\nLe dossier \"source\" a-t-il été altéré ou déplacé?\n\n", SDL_GetError()); erreur = -12;}
		else
		{SDL_SetTextureColorMod(texture_symbole_pause, 0, 0, 0);}
		if (texture_symbole_fin_de_partie == NULL || texture_symbole_fin_de_partie_defaite == NULL)
		{printf("Erreur lors du chargement du symbole de fin de partie (%s).\nLe dossier \"source\" a-t-il été altéré ou déplacé?\n\n", SDL_GetError()); erreur = -12;}
		else
		{SDL_SetTextureColorMod(texture_symbole_fin_de_partie, 0, 0, 0); SDL_SetTextureColorMod(texture_symbole_fin_de_partie_defaite, 143, 23, 23);}
		if (texture_bombe == NULL || texture_bombe_finale == NULL)
		{printf("Erreur lors du chargement de l'image d'une mine (%s).\nLe dossier \"source\" a-t-il été altéré ou déplacé?\n\n", SDL_GetError()); erreur = -12;}
		else
		{SDL_SetTextureColorMod(texture_bombe, 0, 0, 0); SDL_SetTextureColorMod(texture_bombe_finale, 143, 23, 23);}
		if (texture_drapeau == NULL || texture_drapeau_mal_place == NULL)
		{
			printf("Erreur lors du chargement de l'icone drapeau (%s).\nLe dossier \"source\" a-t-il été altéré ou déplacé?\n\n", SDL_GetError());
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Erreur", "L'icone drapeau n'a pas pu être chargée.\nUn \"X\" remplacera donc les drapeaux.\nConsultez la console pour plus de détails.", NULL);
			erreur = -13;
		}
		else
		{SDL_SetTextureColorMod(texture_drapeau, 0, 0, 0); SDL_SetTextureColorMod(texture_drapeau_mal_place, 143, 23, 23);}
		
		strcpy(param[1].option[0].nom, "Utiliser les symboles pleins");
	}
}

void mod_couleur_icones ()
//Change la couleur d'affichage des icones
{
	//...
}

void mod_icones_perso ()
//Permet de charger et utiliser ses propres icones personalisées
{
	//...
}

void mod_popup_quitter ()
//Affiche (ou pas) un pop-up lorsqu'on clique sur le "X"
{
	if (confirmation_quitter)
	{confirmation_quitter = 0; param[2].option[0].non_applicable = 1; param[2].option[1].non_applicable = 0;}
	else
	{confirmation_quitter = 1; param[2].option[0].non_applicable = 0; param[2].option[1].non_applicable = 1;}
}

void mod_fenetre_principale () //modifie les dimensions de la fenêtre principale
{mod_dimensions_fenetre(&largeur_fenetre[0], &hauteur_fenetre[0], "principale");} //J'aurais bien fait un macro, mais les stupidités d'un "void" pas "void" ont fini par m'avoir...

void mod_fenetre_reglages () //modifie les dimensions de la fenêtre des réglages
{mod_dimensions_fenetre(&largeur_fenetre[1], &hauteur_fenetre[1], "des réglages");}

void mod_fenetre_podium () //modifie les dimensions de la fenêtre du podium
{mod_dimensions_fenetre(&largeur_fenetre[2], &hauteur_fenetre[2], "du podium");}

void mod_dimensions_fenetre (int* x, int* y, char nom_fenetre[])
//Modifie les dimensions par défaut d'une fenêtre.
//Appelée par les 3 fcts particulières.
//Reçoit en paramètres 2 ptrs vers les variables contenant la largeur et la hauteur de la fenêtre, qui seront remplies par la fct.
//Le paramètre nom_fenetre servira à identifier la fenêtre dont on modifie les dimensions.
{
	SDL_Event ev;
	int taille_txt_largeur = longueur_txt("Largeur de la fenêtre:", 1000, police);
	int taille_txt_hauteur = longueur_txt("Hauteur de la fenêtre:", 1000, police);
	int selection = 0; //1 = largeur, 2 = hauteur
	int sel_termine = 0; //Est-ce que le bouton "terminé" est sélectionné? 0 = non, 1 = souris, 2 = clavier
	char largeur[10];
	char hauteur[10];
	char titre[60] = "Dimensions par défaut de la fenêtre ";
	_Bool termine = 0;
	
	strcat(titre, nom_fenetre);
	strcat(titre, ":");
	while (!termine)
	{
		sprintf(largeur, "%d", *x);
		sprintf(hauteur, "%d", *y);
		
		SDL_SetColor(fond, rend_r);
		SDL_RenderClear(rend_r);
		TTF_SetFontStyle(police, TTF_STYLE_UNDERLINE);
		afficher_txt_centre(titre, 0, xmax, 30, police, couleur_timer, rend_r);
		TTF_SetFontStyle(police, TTF_STYLE_NORMAL);
		
		afficher_txt("Largeur de la fenêtre:", 20, 90, taille_txt_largeur, police, couleur_timer, rend_r);
		rectangle(30 + taille_txt_largeur, 80, xmax - 50 - taille_txt_largeur, 40, 0, couleur_boutons, fond, rend_r);
		rectangle(30 + taille_txt_largeur, 80, xmax - 50 - taille_txt_largeur, 40, 4, couleur_txt_boutons, fond, rend_r);
		afficher_txt("Hauteur de la fenêtre:", 20, 160, taille_txt_hauteur, police, couleur_timer, rend_r);
		rectangle(30 + taille_txt_hauteur, 150, xmax - 50 - taille_txt_hauteur, 40, 0, couleur_boutons, fond, rend_r);
		rectangle(30 + taille_txt_hauteur, 150, xmax - 50 - taille_txt_hauteur, 40, 4, couleur_txt_boutons, fond, rend_r);
		
		if (sel_termine == 2)
		{rect_arrondi(460 + (xmax - 580) / 2, 210, 100, 40, couleur_selection_clavier, fond, rend_r);}
		else
		{
			rect_arrondi(460 + (xmax - 580) / 2, 210, 100, 40, couleur_boutons, fond, rend_r);
			if (sel_termine == 1)
			{rect_arrondi(460 + (xmax - 580) / 2, 210, 100, 40, couleur_selection_curseur, fond, rend_r);}
		}
		afficher_txt_centre("Terminé", 460 + (xmax - 580) / 2, 560 + (xmax - 580) / 2, 220, police, couleur_txt_boutons, rend_r);
		
		afficher_txt("Toutes les dimensions sont en pixels.", 20, 270, xmax - 40, police, couleur_timer, rend_r);
		afficher_txt("Si votre résolution système n'est pas de 100%, tout pourait paraître tros grand ou trop petit, surtout si vous avez activé le \"fractionnal scaling\".", 20, 300, xmax - 40, police, couleur_timer, rend_r);
		if (!strcmp(nom_fenetre, "principale"))
		{afficher_txt("Les dimensions minimales de la fenêtre principale sont de 650 par 500.", 20, 390, xmax - 40, police, couleur_timer, rend_r);}
		else
		{afficher_txt("Les dimensions minimales des fenêtres des réglages et du podium sont de 580 par 570.", 20, 390, xmax - 40, police, couleur_timer, rend_r);}
		afficher_txt("Si vous entrez une valeur plus petite que la valeur minimale appropriée, la valeur minimale sera utilisée.", 20, 450, xmax - 40, police, couleur_timer, rend_r);
		
		if (selection == 1)
		{rectangle(30 + taille_txt_largeur, 80, xmax - 50 - taille_txt_largeur, 40, 0, couleur_selection_curseur, fond, rend_r);}
		else if (selection == 2)
		{rectangle(30 + taille_txt_hauteur, 150, xmax - 50 - taille_txt_hauteur, 40, 0, couleur_selection_curseur, fond, rend_r);}
		
		afficher_txt(largeur, 40 + taille_txt_largeur, 90, 120, police, couleur_txt_boutons, rend_r);
		afficher_txt(hauteur, 40 + taille_txt_hauteur, 160, 120, police, couleur_txt_boutons, rend_r);
		
		SDL_WaitEvent(&ev);
		switch (ev.type)
		{
		case SDL_WINDOWEVENT:
			if (ev.window.windowID != ID_fenetre_reglages) //si l'utilisateur joue avec l'autre fenêtre (lui donnant ainsi le focus, déclenchant cet event), on veut le ramener dans les réglages
			{SDL_RaiseWindow(fenetre_reglages); SDL_FlashWindow(fenetre_reglages, SDL_FLASH_UNTIL_FOCUSED);}
			else if (ev.window.event == SDL_WINDOWEVENT_CLOSE) //SDL_QUIT ne fonctionne pas avec plusieurs fenêtres ouvertes...
			{termine = 1;}
			else
			{SDL_GetWindowSize(fenetre_reglages, &xmax, &ymax);}
			break;
		
		case SDL_MOUSEMOTION:
			if ((ev.motion.x >= 30 + taille_txt_largeur && ev.motion.x <= xmax - 20 && ev.motion.y >= 80 && ev.motion.y <= 120) \
				|| (ev.motion.x >= 30 + taille_txt_hauteur && ev.motion.x <= xmax - 20 && ev.motion.y >= 150 && ev.motion.y <= 190))
			{SDL_SetCursor(curseur_txt); sel_termine = 0;}
			else if (ev.motion.x >= 460 + (xmax - 580) / 2 && ev.motion.x <= 560 + (xmax - 580) / 2 && ev.motion.y >= 220 && ev.motion.y <= 260)
			{sel_termine = 1;}
			else
			{SDL_SetCursor(curseur_normal); sel_termine = 0;}
			break;
		
		case SDL_MOUSEBUTTONDOWN:
			if (ev.button.x >= 30 + taille_txt_largeur && ev.button.x <= xmax - 20 && ev.button.y >= 80 && ev.button.y <= 120)
			{selection = 1;}
			else if (ev.button.x >= 30 + taille_txt_hauteur && ev.button.x <= xmax - 20 && ev.button.y >= 150 && ev.button.y <= 190)
			{selection = 2;}
			else if (ev.button.x >= 460 + (xmax - 580) / 2 && ev.button.x <= 560 + (xmax - 580) / 2 && ev.button.y >= 220 && ev.button.y <= 260)
			{termine = 1;}
			else
			{selection = 0;}
			break;
		
		case SDL_KEYDOWN:
			switch (ev.key.keysym.sym)
			{
			case SDLK_ESCAPE:
				if (selection != 0 || sel_termine == 2)
				{selection = 0; sel_termine = 0;}
				else
				{termine = 1;}
				break;
			
			case SDLK_RIGHT:
			case SDLK_LEFT:
				if (!selection)
				{selection = 1; sel_termine = 0;}
				break;
			
			case SDLK_TAB:
				if (!selection)
				{selection = 1; sel_termine = 0;}
				else
				{sel_termine = 2; selection = 0;}
				break;
			
			case SDLK_UP:
			case SDLK_DOWN:
				if (selection == 2)
				{selection = 1; sel_termine = 0;}
				else
				{selection++; sel_termine = 0;}
				break;
			
			case SDLK_RETURN:
			case SDLK_KP_ENTER:
				if (sel_termine == 2)
				{termine = 1;}
				else
				{selection = 0; sel_termine = 0;}
				break;
			
			case SDLK_BACKSPACE:
				if (selection == 1)
				{*x = *x / 10;}
				else if (selection == 2)
				{*y = *y / 10;}
				break;
			
			case SDLK_0: case SDLK_KP_0:
			case SDLK_1: case SDLK_KP_1:
			case SDLK_2: case SDLK_KP_2:
			case SDLK_3: case SDLK_KP_3:
			case SDLK_4: case SDLK_KP_4:
			case SDLK_5: case SDLK_KP_5:
			case SDLK_6: case SDLK_KP_6:
			case SDLK_7: case SDLK_KP_7:
			case SDLK_8: case SDLK_KP_8:
			case SDLK_9: case SDLK_KP_9:
				if (selection == 1 && *x / 1000 < 1)
				{
					if (ev.key.keysym.sym < SDLK_KP_1 || ev.key.keysym.sym > SDLK_KP_0)
					{*x = *x * 10 + ev.key.keysym.sym - SDLK_0;}
					else if (ev.key.keysym.sym == SDLK_KP_0)
					{*x = *x * 10;} //Pourquoi, mais POURQUOI ont-ils commencé les chiffres du keypad avec 1 au lieu de 0?!!
					else
					{*x = *x * 10 + ev.key.keysym.sym - SDLK_KP_1 + 1;}
				}
				else if (selection == 2 && *y / 1000 < 1)
				{
					if (ev.key.keysym.sym < SDLK_KP_1 || ev.key.keysym.sym > SDLK_KP_0)
					{*y = *y * 10 + ev.key.keysym.sym - SDLK_0;}
					else if (ev.key.keysym.sym == SDLK_KP_0)
					{*y = *y * 10;}
					else
					{*y = *y * 10 + ev.key.keysym.sym - SDLK_KP_1 + 1;}
				}
				break;
			}
		}
		SDL_RenderPresent(rend_r);
	}
	
	if (!strcmp(nom_fenetre, "principale"))
	{
		if (*x < 650)
		{*x = 650;}
		if (*y < 600)
		{*y = 600;}
	}
	else //fenêtre des réglages ou du podium
	{
		if (*x < 580)
		{*x = 580;}
		if (*y < 570)
		{*y = 570;}
	}
}