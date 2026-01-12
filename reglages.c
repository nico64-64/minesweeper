#include "reglages_backend.c"
#include <math.h>


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
_Bool mod_couleur (SDL_Color* couleur, char titre[]); //permet de modifier la couleur d'un élément graphique du programme
//Liste des fonctions modifiant les paramètres:
void mod_couleurs(); //modifie les couleurs de la grille, des boutons, etc.
void mod_police(); //modifie la texture des nbres ds la grille
void mod_zeros(); //affiche/masque les zéros dans la grille
void mod_theme(); //switch du thème "vide" au thème "plein" et vice-versa
void mod_couleur_icones(); //change la couleur d'affichage des icones
void mod_icones_perso(); //permet de charger et utiliser ses propres icones personalisées
void mod_popup_quitter(); //affiche (ou pas) un pop-up lorsqu'on clique sur le "X"
void mod_fenetre_principale(); //modifie les dimensions de la fenêtre principale
void mod_fenetre_reglages(); //modifie les dimensions de la fenêtre des réglages
void mod_fenetre_podium(); //modifie les dimensions de la fenêtre du podium


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

//Liste des paramètres modifiables:
struct parametre param[NBRE_PARAMS] =
{
	{"apparence générale", "Vous pouvez modifier ici l'apparence de la grille (plateau de jeu) et du reste de l'application.", "Aucune information supplémentaire.", 3, \
		{{"Modifier les couleurs du jeu", "Cliquer ici pour modifier la coloration des tuiles, des boutons et bien d'autres choses.\nLa couleur des symboles ne se modifie pas ici.", 0, mod_couleurs}, \
		{"Modifier les polices du jeu", "Permet de changer les polices utilisées par le jeu ainsi que de modifier leur taille et leur couleur, incluant les nombres affichés sur les tuiles de la grille qui ne sont pas des bombes.", \
		0, mod_police}, \
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
	
	//Vérification de 2-3 petits trucs (j'haïs ça, devoir faire ça!):
	if (confirmation_quitter && param[2].option[0].non_applicable) //si on demande une confirmation, mais que le bouton pour désactiver ça est non-disponible (ce qui est un non-sens!)
	{param[2].option[0].non_applicable = 0; param[2].option[1].non_applicable = 1;}
	
	//Gestion des réglages:
	reglages_menu();
	
	//Destruction de la fenêtre et du renderer des réglages:
	SDL_DestroyRenderer(rend_r);
	SDL_DestroyWindow(fenetre_reglages);
	SDL_SetWindowResizable(fenetre, SDL_TRUE);
	SDL_GetWindowSize(fenetre, &xmax, &ymax); //xmax et ymax sont réinitialisés avec les valeurs de la fenêtre principale
	
	//Sauvegarde des nouveaux réglages:
	cree_fconfig();
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

void mod_couleurs ()
//Modifie la couleur de la grille, des boutons, etc.
{
	enum zone
	{
		//0 = ailleurs...
		//1 à 8 = nbre de bombes adjacentes
		vide = 10, //tuiles vides
		score, //nbres sur les tuiles
		boutons,
		boutons_bloques,
		boutons_txt,
		liens,
		txt_libre,
		arriere_plan,
		selection_clavier = 20,
		selection_curseur,
		plus = 50, //bouton "plus d'options"
		symboles,
		reinitialiser,
		termine
	};
	
	SDL_Color* PAS_UN_CHOIX_DE_COULEUR;
	SDL_Color* choix_couleur = PAS_UN_CHOIX_DE_COULEUR;
	SDL_Event ev;
	enum zone focus = 0;
	enum zone curseur = 0;
	char buffer[50];
	char titre[200];
	int largeur_supp = xmax - 640 - longueur_txt("Sélection curseur:", 160, police);
	_Bool plus_options;
	
	while (1)
	{
		SDL_SetColor(fond, rend_r);
		SDL_RenderClear(rend_r);
		
		TTF_SetFontStyle(police, TTF_STYLE_UNDERLINE);
		afficher_txt_centre("Modification des couleurs du jeu:", 20, xmax - 20, 10, police, couleur_timer, rend_r);
		TTF_SetFontStyle(police, TTF_STYLE_NORMAL);
		
		//Couleur des tuiles selon le nbre de bombes adjacentes:
		strcpy(buffer, "1 bombe adjacente:");
		for (int compteur = 1; compteur < 9; compteur++)
		{
			afficher_txt(buffer, 230 - longueur_txt(buffer, 210, police), compteur * 65 - 10, 210, police, couleur_timer, rend_r);
			rectangle(240, compteur * 65 - 25, 50, 50, 0, couleur_tuile[compteur], fond, rend_r);
			if (curseur == compteur)
			{rectangle(240, compteur * 65 - 25, 50, 50, 0, couleur_selection_curseur, fond, rend_r);}
			rectangle(240, compteur * 65 - 25, 50, 50, 4, couleur_timer, fond, rend_r);
			if (focus == compteur)
			{rectangle(240, compteur * 65 - 25, 50, 50, 4, couleur_selection_clavier, fond, rend_r);}
			sprintf(buffer, "%d bombes adjacentes:", compteur + 1);
		}
		
		//Couleur des autres éléments (sauf la sélection):
		afficher_txt("Sélection clavier:", xmax - 80 - longueur_txt("Sélection clavier:", 170, police), 55, 170, police, couleur_timer, rend_r);
		rectangle(xmax - 70, 40, 50, 50, 0, couleur_boutons, fond, rend_r);
		rectangle(xmax - 70, 40, 50, 50, 0, couleur_selection_clavier, fond, rend_r);
		if (curseur == selection_clavier)
		{rectangle(xmax - 70, 40, 50, 50, 0, couleur_selection_curseur, fond, rend_r);}
		rectangle(xmax - 70, 40, 50, 50, 4, couleur_timer, fond, rend_r);
		if (focus == selection_clavier)
		{rectangle(xmax - 70, 40, 50, 50, 4, couleur_selection_clavier, fond, rend_r);}
		
		afficher_txt("Sélection curseur:", xmax - 80 - longueur_txt("Sélection curseur:", 170, police), 120, 170, police, couleur_timer, rend_r);
		rectangle(xmax - 70, 105, 50, 50, 0, couleur_boutons, fond, rend_r);
		rectangle(xmax - 70, 105, 50, 50, 0, couleur_selection_curseur, fond, rend_r);
		if (curseur == selection_curseur)
		{rectangle(xmax - 70, 105, 50, 50, 0, couleur_selection_curseur, fond, rend_r);}
		rectangle(xmax - 70, 105, 50, 50, 4, couleur_timer, fond, rend_r);
		if (focus == selection_curseur)
		{rectangle(xmax - 70, 105, 50, 50, 4, couleur_selection_clavier, fond, rend_r);}
		
		
		if (xmax - 100 - longueur_txt("Sélection curseur:", 160, police) >= 540)
		{
			afficher_txt("Tuiles vides:", 480 - longueur_txt("Tuiles vides:", 160, police) + largeur_supp / 2, 55, 160, police, couleur_timer, rend_r);
			rectangle(490 + largeur_supp / 2, 40, 50, 50, 0, couleur_grille, fond, rend_r);
			
			afficher_txt("Nombres dans ", 480 - longueur_txt("Nombres dans ", 160, police) + largeur_supp / 2, 110, 160, police, couleur_timer, rend_r);
			afficher_txt("la grille:", 480 - longueur_txt("la grille:", 160, police) + largeur_supp / 2, 130, 160, police, couleur_timer, rend_r);
			rectangle(490 + largeur_supp / 2, 105, 50, 50, 0, couleur_score, fond, rend_r);
			
			afficher_txt("Boutons:", 480 - longueur_txt("Boutons:", 160, police) + largeur_supp / 2, 185, 160, police, couleur_timer, rend_r);
			rectangle(490 + largeur_supp / 2, 170, 50, 50, 0, couleur_boutons, fond, rend_r);
			
			afficher_txt("Boutons bloqués:", 480 - longueur_txt("Boutons bloqués:", 160, police) + largeur_supp / 2, 250, 160, police, couleur_timer, rend_r);
			rectangle(490 + largeur_supp / 2, 235, 50, 50, 0, couleur_boutons_bloques, fond, rend_r);
			
			afficher_txt("Texte dans ", 480 - longueur_txt("Texte dans ", 160, police) + largeur_supp / 2, 305, 160, police, couleur_timer, rend_r);
			afficher_txt("les boutons:", 480 - longueur_txt("les boutons:", 160, police) + largeur_supp / 2, 325, 160, police, couleur_timer, rend_r);
			rectangle(490 + largeur_supp / 2, 300, 50, 50, 0, couleur_txt_boutons, fond, rend_r);
			
			afficher_txt("Liens cliquables:", 480 - longueur_txt("Liens cliquables:", 160, police) + largeur_supp / 2, 380, 160, police, couleur_timer, rend_r);
			rectangle(490 + largeur_supp / 2, 365, 50, 50, 0, couleur_liens, fond, rend_r);
			
			afficher_txt("Texte libre:", 480 - longueur_txt("Texte libre:", 160, police) + largeur_supp / 2, 445, 160, police, couleur_timer, rend_r);
			rectangle(490 + largeur_supp / 2, 430, 50, 50, 0, couleur_timer, fond, rend_r);
			
			afficher_txt("Arrière-plan:", 480 - longueur_txt("Arrière-plan:", 160, police) + largeur_supp / 2, 510, 160, police, couleur_timer, rend_r);
			rectangle(490 + largeur_supp / 2, 495, 50, 50, 0, fond, fond, rend_r);
			
			for (int compteur = 0; compteur < 8; compteur++)
			{
				if (curseur == 10 + compteur)
				{rectangle(490 + largeur_supp / 2, 40 + compteur * 65, 50, 50, 0, couleur_selection_curseur, fond, rend_r);}
				rectangle(490 + largeur_supp / 2, 40 + compteur * 65, 50, 50, 4, couleur_timer, fond, rend_r);
				if (focus == 10 + compteur)
				{rectangle(490 + largeur_supp / 2, 40 + compteur * 65, 50, 50, 4, couleur_selection_clavier, fond, rend_r);}
			}
			
			plus_options = 0;
			if (focus == plus)
			{focus = 0;}
			if (curseur == plus)
			{curseur = 0;}
		}
		else
		{
			rect_arrondi(xmax - 150, ymax - 240, 130, 40, couleur_boutons, fond, rend_r);
			if (focus == plus)
			{rect_arrondi(xmax - 150, ymax - 240, 130, 40, couleur_selection_clavier, fond, rend_r);}
			if (curseur == plus)
			{rect_arrondi(xmax - 150, ymax - 240, 130, 40, couleur_selection_curseur, fond, rend_r);}
			afficher_txt_centre("Plus d'options", xmax - 150, xmax - 20, ymax - 230, police, couleur_txt_boutons, rend_r);
			
			plus_options = 1;
			if (focus >= vide && focus <= arriere_plan)
			{focus = 0;}
			if (curseur >= vide && curseur <= arriere_plan)
			{curseur = 0;}
			
		}
		
		//Boutons:
		rect_arrondi(xmax - 150, ymax - 180, 130, 40, couleur_boutons, fond, rend_r);
		if (focus == symboles)
		{rect_arrondi(xmax - 150, ymax - 180, 130, 40, couleur_selection_clavier, fond, rend_r);}
		if (curseur == symboles)
		{rect_arrondi(xmax - 150, ymax - 180, 130, 40, couleur_selection_curseur, fond, rend_r);}
		afficher_txt_centre("Modifier les", xmax - 150, xmax - 20, ymax - 175, petite_police, couleur_txt_boutons, rend_r);
		afficher_txt_centre("symboles", xmax - 150, xmax - 20, ymax - 160, petite_police, couleur_txt_boutons, rend_r);
		
		rect_arrondi(xmax - 150, ymax - 120, 130, 40, couleur_boutons, fond, rend_r);
		if (focus == reinitialiser)
		{rect_arrondi(xmax - 150, ymax - 120, 130, 40, couleur_selection_clavier, fond, rend_r);}
		if (curseur == reinitialiser)
		{rect_arrondi(xmax - 150, ymax - 120, 130, 40, couleur_selection_curseur, fond, rend_r);}
		afficher_txt_centre("Réinitialiser", xmax - 150, xmax - 20, ymax - 110, police, couleur_txt_boutons, rend_r);
		
		rect_arrondi(xmax - 150, ymax - 60, 130, 40, couleur_boutons, fond, rend_r);
		if (focus == termine)
		{rect_arrondi(xmax - 150, ymax - 60, 130, 40, couleur_selection_clavier, fond, rend_r);}
		if (curseur == termine)
		{rect_arrondi(xmax - 150, ymax - 60, 130, 40, couleur_selection_curseur, fond, rend_r);}
		afficher_txt_centre("Terminé", xmax - 150, xmax - 20, ymax - 50, police, couleur_txt_boutons, rend_r);
		
		//Modification de la couleur d'un élément:
		if (choix_couleur != PAS_UN_CHOIX_DE_COULEUR)
		{
			if (mod_couleur(choix_couleur, titre))
			{choix_couleur = PAS_UN_CHOIX_DE_COULEUR;}
			else
			{choix_couleur = NULL;}
			largeur_supp = xmax - 640 - longueur_txt("Sélection curseur:", 160, police);
		}
		
		//Gestion des events de cette page:
		else
		{
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
				{SDL_GetWindowSize(fenetre_reglages, &xmax, &ymax); largeur_supp = xmax - 640 - longueur_txt("Sélection curseur:", 160, police);}
				break;
			
			case SDL_MOUSEMOTION:
				if (ev.motion.x >= 240 && ev.motion.x <= 290)
				{
					curseur = 0;
					for (int compteur = 1; compteur < 9; compteur++)
					{
						if (ev.motion.y >= compteur * 65 - 25 && ev.motion.y <= compteur * 65 + 25)
						{curseur = compteur;}
					}
				}
				else if (!plus_options && ev.motion.x >= 490 + largeur_supp / 2 && ev.motion.x <= 540 + largeur_supp / 2)
				{
					curseur = 0;
					for (int compteur = 0; compteur < 8; compteur++)
					{
						if (ev.motion.y >= 40 + compteur * 65 && ev.motion.y <= 90 + compteur * 65)
						{curseur = vide + compteur;}
					}
				}
				else if (ev.motion.x >= xmax - 70 && ev.motion.x <= xmax - 20 && ev.motion.y >= 40 && ev.motion.y <= 90)
				{curseur = selection_clavier;}
				else if (ev.motion.x >= xmax - 70 && ev.motion.x <= xmax - 20 && ev.motion.y >= 105 && ev.motion.y <= 155)
				{curseur = selection_curseur;}
				else if (plus_options && ev.motion.x >= xmax - 150 && ev.motion.x <= xmax - 20 && ev.motion.y >= ymax - 240 && ev.motion.y <= ymax - 200)
				{curseur = plus;}
				else if (ev.motion.x >= xmax - 150 && ev.motion.x <= xmax - 20 && ev.motion.y >= ymax - 180 && ev.motion.y <= ymax - 140)
				{curseur = symboles;}
				else if (ev.motion.x >= xmax - 150 && ev.motion.x <= xmax - 20 && ev.motion.y >= ymax - 120 && ev.motion.y <= ymax - 80)
				{curseur = reinitialiser;}
				else if (ev.motion.x >= xmax - 150 && ev.motion.x <= xmax - 20 && ev.motion.y >= ymax - 60 && ev.motion.y <= ymax - 20)
				{curseur = termine;}
				else
				{curseur = 0;}
				break;
			
			case SDL_KEYDOWN:
				switch (ev.key.keysym.sym)
				{
				case SDLK_ESCAPE:
					return;
					break;
				
				case SDLK_TAB:
					if (focus >= 1 && focus <= 8)
					{
						if (!plus_options)
						{focus += 9;}
						else
						{focus = selection_clavier;}
					}
					else if (focus >= vide && focus <= arriere_plan)
					{focus = selection_clavier;}
					else if (focus == selection_clavier || focus == selection_curseur)
					{
						if (plus_options)
						{focus = plus;}
						else
						{focus = symboles;}
					}
					else
					{focus = 1;}
					break;
				
				case SDLK_RIGHT:
					if (focus >= 1 && focus <= 8)
					{
						if (!plus_options)
						{focus += 9;}
						else
						{focus = selection_clavier;}
					}
					else if (focus >= vide && focus <= arriere_plan)
					{focus = selection_clavier;}
					else if (focus < plus)
					{focus = 1;}
					break;
				
				case SDLK_LEFT:
					if (focus >= vide && focus <= arriere_plan)
					{focus -= 9;}
					else if (focus >= 1 && focus <= 8)
					{focus = selection_clavier;}
					else if (!plus_options && focus < plus)
					{focus -= 10;}
					else if (!focus)
					{focus = 1;}
					else if (focus < plus)
					{focus -= 19;}
					break;
				
				case SDLK_DOWN:
					if (focus == 8)
					{focus = 1;}
					else if (focus == arriere_plan)
					{focus = vide;}
					else if ((focus == selection_curseur || focus == termine) && plus_options)
					{focus = plus;}
					else if ((focus == selection_curseur || focus == termine) && !plus_options)
					{focus = symboles;}
					else
					{focus++;}
					break;
				
				case SDLK_UP:
					if (focus == 1)
					{focus = 8;}
					else if (focus == vide)
					{focus = arriere_plan;}
					else if (focus == selection_clavier)
					{focus = selection_curseur;}
					else if (focus == plus || (focus == symboles && !plus_options))
					{focus = selection_curseur;}
					else if (!focus)
					{focus = 1;}
					else
					{focus--;}
					break;
				
				case SDLK_RETURN:
				case SDLK_KP_ENTER:
					if (focus >= 1 && focus <= 8)
					{choix_couleur = &couleur_tuile[focus]; sprintf(titre, "Couleur des tuiles adjacentes à %d bombes:", focus);}
					else if (focus >= vide && focus <= selection_curseur)
					{
						switch (focus)
						{
						case vide:
							choix_couleur = &couleur_grille;
							strcpy(titre, "Couleur des tuiles vides:");
							break;
						
						case score:
							choix_couleur = &couleur_score;
							strcpy(titre, "Couleur des nombres dans la grille:");
							break;
						
						case boutons:
							choix_couleur = &couleur_boutons;
							strcpy(titre, "Couleur des boutons:");
							break;
						
						case boutons_bloques:
							choix_couleur = &couleur_boutons_bloques;
							strcpy(titre, "Couleur des boutons bloqués:");
							break;
						
						case boutons_txt:
							choix_couleur = &couleur_txt_boutons;
							strcpy(titre, "Couleur du texte dans les boutons:");
							break;
						
						case liens:
							choix_couleur = &couleur_liens;
							strcpy(titre, "Couleur des liens cliquables:");
							break;
						
						case txt_libre:
							choix_couleur = &couleur_timer;
							strcpy(titre, "Couleur du texte écrit sur l'arrière-plan:");
							break;
						
						case arriere_plan:
							choix_couleur = &fond;
							strcpy(titre, "Couleur de l'arrière-plan:");
							break;
						
						case selection_clavier:
							choix_couleur = &couleur_selection_clavier;
							strcpy(titre, "Couleur des objets sélectionnés avec le clavier:");
							break;
						
						case selection_curseur:
							choix_couleur = &couleur_selection_curseur;
							strcpy(titre, "Couleur des objets sélectionnés avec le curseur:");
							break;
						}
					}
					else if (focus == plus)
					{SDL_SetWindowSize(fenetre_reglages, 800, 570);} //Si l'utilisateur veut plus d'options mais a une fenêtre trop petite, on lui agrandit sa fenêtre pour qu'il les voit tous.
					else if (focus == symboles)
					{mod_couleur_icones(); return;}
					else if (focus == reinitialiser)
					{
						fond = blanc;
						couleur_grille = gris;
						couleur_score = noir;
						couleur_timer = noir;
						couleur_liens = bleu;
						couleur_boutons = gris_pale;
						couleur_boutons_bloques = gris_fonce;
						couleur_txt_boutons = noir;
						couleur_selection_clavier = bleu;
						couleur_selection_curseur = bleu_efface;
						couleur_tuile[0] = transparent;
						couleur_tuile[1] = vert_pale;
						couleur_tuile[2] = jaune_pale;
						couleur_tuile[3] = jaune_orange;
						couleur_tuile[4] = orange;
						couleur_tuile[5] = orange_fonce;
						couleur_tuile[6] = rouge;
						couleur_tuile[7] = rouge_fonce;
						couleur_tuile[8] = rouge_tres_fonce;
					}
					else if (focus == termine)
					{return;}
					break;
				}
				break;
			
			case SDL_MOUSEBUTTONDOWN:
				focus = 0;
				if (curseur >= 1 && curseur <= 8)
				{choix_couleur = &couleur_tuile[curseur]; sprintf(titre, "Couleur des tuiles adjacentes à %d bombes:", curseur);}
				else if (curseur >= vide && curseur <= selection_curseur)
				{
					switch (curseur)
					{
					case vide:
						choix_couleur = &couleur_grille;
						strcpy(titre, "Couleur des tuiles vides:");
						break;
					
					case score:
						choix_couleur = &couleur_score;
						strcpy(titre, "Couleur des nombres dans la grille:");
						break;
					
					case boutons:
						choix_couleur = &couleur_boutons;
						strcpy(titre, "Couleur des boutons:");
						break;
					
					case boutons_bloques:
						choix_couleur = &couleur_boutons_bloques;
						strcpy(titre, "Couleur des boutons bloqués:");
						break;
					
					case boutons_txt:
						choix_couleur = &couleur_txt_boutons;
						strcpy(titre, "Couleur du texte dans les boutons:");
						break;
					
					case liens:
						choix_couleur = &couleur_liens;
						strcpy(titre, "Couleur des liens cliquables:");
						break;
					
					case txt_libre:
						choix_couleur = &couleur_timer;
						strcpy(titre, "Couleur du texte écrit sur l'arrière-plan:");
						break;
					
					case arriere_plan:
						choix_couleur = &fond;
						strcpy(titre, "Couleur de l'arrière-plan:");
						break;
					
					case selection_clavier:
						choix_couleur = &couleur_selection_clavier;
						strcpy(titre, "Couleur des objets sélectionnés avec le clavier:");
						break;
					
					case selection_curseur:
						choix_couleur = &couleur_selection_curseur;
						strcpy(titre, "Couleur des objets sélectionnés avec le curseur:");
						break;
					}
				}
				else if (curseur == plus)
				{SDL_SetWindowSize(fenetre_reglages, 800, 570);} //Si l'utilisateur veut plus d'options mais a une fenêtre trop petite, on lui agrandit sa fenêtre pour qu'il les voit tous.
				else if (curseur == symboles)
				{mod_couleur_icones(); return;}
				else if (curseur == reinitialiser)
				{
					fond = blanc;
					couleur_grille = gris;
					couleur_score = noir;
					couleur_timer = noir;
					couleur_liens = bleu;
					couleur_boutons = gris_pale;
					couleur_boutons_bloques = gris_fonce;
					couleur_txt_boutons = noir;
					couleur_selection_clavier = bleu;
					couleur_selection_curseur = bleu_efface;
					couleur_tuile[0] = transparent;
					couleur_tuile[1] = vert_pale;
					couleur_tuile[2] = jaune_pale;
					couleur_tuile[3] = jaune_orange;
					couleur_tuile[4] = orange;
					couleur_tuile[5] = orange_fonce;
					couleur_tuile[6] = rouge;
					couleur_tuile[7] = rouge_fonce;
					couleur_tuile[8] = rouge_tres_fonce;
				}
				else if (curseur == termine)
				{return;}
				break;
			}
		}
	}
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
		retour = 20, //(terminé)
		appliquer,
		reinitialiser
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
		
		rect_arrondi(xmax - 550, ymax - 60, 150, 40, couleur_boutons, fond, rend_r);
		if (focus == reinitialiser)
		{rect_arrondi(xmax - 550, ymax - 60, 150, 40, couleur_selection_clavier, fond, rend_r);}
		if (curseur == reinitialiser)
		{rect_arrondi(xmax - 550, ymax - 60, 150, 40, couleur_selection_curseur, fond, rend_r);}
		afficher_txt_centre("Réinitialiser", xmax - 550, xmax - 400, ymax - 50, police, couleur_txt_boutons, rend_r);
		
		rect_arrondi(xmax - 360, ymax - 60, 150, 40, couleur_boutons, fond, rend_r);
		if (focus == appliquer)
		{rect_arrondi(xmax - 360, ymax - 60, 150, 40, couleur_selection_clavier, fond, rend_r);}
		if (curseur == appliquer)
		{rect_arrondi(xmax - 360, ymax - 60, 150, 40, couleur_selection_curseur, fond, rend_r);}
		afficher_txt_centre("Appliquer", xmax - 360, xmax - 210, ymax - 50, police, couleur_txt_boutons, rend_r);
		
		rect_arrondi(xmax - 170, ymax - 60, 150, 40, couleur_boutons, fond, rend_r);
		if (focus == retour)
		{rect_arrondi(xmax - 170, ymax - 60, 150, 40, couleur_selection_clavier, fond, rend_r);}
		if (curseur == retour)
		{rect_arrondi(xmax - 170, ymax - 60, 150, 40, couleur_selection_curseur, fond, rend_r);}
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
			if (ev.motion.y >= ymax - 60 && ev.motion.y <= ymax - 20)
			{
				if (ev.motion.x >= xmax - 550 && ev.motion.x <= xmax - 400)
				{curseur = reinitialiser;}
				else if (ev.motion.x >= xmax - 360 && ev.motion.x <= xmax - 210)
				{curseur = appliquer;}
				else if (ev.motion.x >= xmax - 170 && ev.motion.x <= xmax - 20)
				{curseur = retour;}
				else
				{curseur = 0;}
			}
			else if (ev.motion.y >= 80 && ev.motion.y <= 120 && ev.motion.x >= 30 + longueur_st_m && ev.motion.x <= xmax - 20)
			{SDL_SetCursor(curseur_txt); curseur = nom_moyen;}
			else if (ev.motion.y >= 150 && ev.motion.y <= 190 && ev.motion.x >= 30 + longueur_st_p && ev.motion.x <= xmax - 20)
			{SDL_SetCursor(curseur_txt); curseur = nom_petite;}
			else if (ev.motion.y >= ymax / 2 && ev.motion.y <= ymax / 2 + 45 && ev.motion.x >= xmax / 2 - 190 && ev.motion.x <= xmax / 2 - 110)
			{curseur = moyen_plus;}
			else if (ev.motion.y >= ymax / 2 && ev.motion.y <= ymax / 2 + 45 && ev.motion.x >= xmax / 2 + 110 && ev.motion.x <= xmax / 2 + 190)
			{curseur = petite_plus;}
			else if (ev.motion.y >= ymax / 2 + 105 && ev.motion.y <= ymax / 2 + 150 && ev.motion.x >= xmax / 2 - 190 && ev.motion.x <= xmax / 2 - 110)
			{curseur = moyen_moins;}
			else if (ev.motion.y >= ymax / 2 + 105 && ev.motion.y <= ymax / 2 + 150 && ev.motion.x >= xmax / 2 + 110 && ev.motion.x <= xmax / 2 + 190)
			{curseur = petite_moins;}
			else
			{curseur = 0; SDL_SetCursor(curseur_normal);}
			break;
		
		case SDL_MOUSEBUTTONDOWN:
			focus = 0;
			switch (curseur)
			{
			case nom_moyen:
				focus = nom_moyen;
				break;
			
			case nom_petite:
				focus = nom_petite;
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
			
			case reinitialiser:
				taille_police_normale = 22;
				taille_petite_police = 19;
				strcpy(nom_police, "./source/free_serif.ttf");
				strcpy(nom_petite_police, "./source/free_serif.ttf");
				//Pas de break! On veut que le code "coule" au case suivant!
			
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
				if (police_backup != police && police_backup != petite_police)
				{TTF_CloseFont(police_backup);}
				else
				{printf("L'ancienne police a été utilisée en guise de fallback pour la police normale.\n");}
				if (petite_police_backup != NULL && petite_police_backup != petite_police && petite_police_backup != police)
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
				if (curseur == retour)
				{return;}
				break;
			}
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
				{focus = reinitialiser;}
				break;
			
			case SDLK_RIGHT:
			case SDLK_LEFT:
				if (focus == moyen_plus || focus == moyen_moins || ((focus == retour || focus == appliquer) && ev.key.keysym.sym == SDLK_LEFT))
				{focus++;}
				else if (focus == petite_plus || focus == petite_moins || ((focus == reinitialiser || focus == appliquer) && ev.key.keysym.sym == SDLK_RIGHT))
				{focus--;}
				else if (focus == reinitialiser && ev.key.keysym.sym == SDLK_LEFT)
				{focus = retour;}
				else if (focus == retour && ev.key.keysym.sym == SDLK_RIGHT)
				{focus = reinitialiser;}
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
				
				case reinitialiser:
					taille_police_normale = 22;
					taille_petite_police = 19;
					strcpy(nom_police, "./source/free_serif.ttf");
					strcpy(nom_petite_police, "./source/free_serif.ttf");
					//Pas de break! On veut que le code "coule" au case suivant!
				
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
					if (police_backup != police && police_backup != petite_police)
					{TTF_CloseFont(police_backup);}
					else
					{printf("L'ancienne police a été utilisée en guise de fallback pour la police normale.\n");}
					if (petite_police_backup != NULL && petite_police_backup != petite_police && petite_police_backup != police)
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
//Change la couleur d'affichage des icones.
//Cette fonction ne respecte pas les standards et bonnes pratiques de la programmation.
{
	enum zone
	{
		//0 = ailleurs...
		podium = 1,
		_pause,
		victoire,
		defaite,
		drapeaux,
		drapeaux_mal_places,
		bombes,
		bombe_cliquee,
		autres_couleurs = 10,
		reinitialiser,
		termine
	};
	
	SDL_Color* PAS_UN_CHOIX_DE_COULEUR;
	SDL_Color* choix_couleur = PAS_UN_CHOIX_DE_COULEUR;
	SDL_Event ev;
	enum zone focus = 0;
	enum zone curseur = 0;
	char titre[200];
	int largeur_supp = xmax - 640;
	
	//Couleurs des symboles:
	SDL_Color couleur_podium = noir;
	SDL_Color couleur_pause = noir;
	SDL_Color couleur_victoire = noir;
	SDL_Color couleur_defaite = rouge_symboles;
	SDL_Color couleur_drapeaux = noir;
	SDL_Color couleur_drapeaux_mal_places = rouge_symboles;
	SDL_Color couleur_bombes = noir;
	SDL_Color couleur_bombe_cliquee = rouge_symboles;
	
	//Rectangles des symboles:
	SDL_Rect rect_podium = {320 + largeur_supp / 2, 40, 50, 50};
	SDL_Rect rect_pause = {320 + largeur_supp / 2, 105, 50, 50};
	SDL_Rect rect_victoire = {320 + largeur_supp / 2, 170, 50, 50};
	SDL_Rect rect_defaite = {320 + largeur_supp / 2, 235, 50, 50};
	SDL_Rect rect_drapeaux = {320 + largeur_supp / 2, 300, 50, 50};
	SDL_Rect rect_drapeaux_mal_places = {320 + largeur_supp / 2, 365, 50, 50};
	SDL_Rect rect_bombes = {320 + largeur_supp / 2, 430, 50, 50};
	SDL_Rect rect_bombe_cliquee = {320 + largeur_supp / 2, 495, 50, 50};
	
	//Symboles blancs (en buffer):
	SDL_Texture* __texture_podium = IMG_LoadTexture(rend_r, icone_podium);
	SDL_Texture* __texture_pause = IMG_LoadTexture(rend_r, symbole_pause);
	SDL_Texture* __texture_victoire = IMG_LoadTexture(rend_r, symbole_fin_de_partie);
	SDL_Texture* __texture_defaite = IMG_LoadTexture(rend_r, symbole_fin_de_partie);
	SDL_Texture* __texture_drapeaux = IMG_LoadTexture(rend_r, icone_drapeau);
	SDL_Texture* __texture_drapeaux_mal_places = IMG_LoadTexture(rend_r, icone_drapeau);
	SDL_Texture* __texture_bombes = IMG_LoadTexture(rend_r, image_bombe);
	SDL_Texture* __texture_bombe_cliquee = IMG_LoadTexture(rend_r, image_bombe);
	
	//Symboles colorés (en buffer):
	SDL_Texture* _texture_podium = __texture_podium;
	SDL_Texture* _texture_pause = __texture_pause;
	SDL_Texture* _texture_victoire = __texture_victoire;
	SDL_Texture* _texture_defaite = __texture_defaite;
	SDL_Texture* _texture_drapeaux = __texture_drapeaux;
	SDL_Texture* _texture_drapeaux_mal_places = __texture_drapeaux_mal_places;
	SDL_Texture* _texture_bombes = __texture_bombes;
	SDL_Texture* _texture_bombe_cliquee = __texture_bombe_cliquee;
	
	
	//Coloration des textures (et vérification de leur existence...):
	//(Les images qu'on a loadées sont dessinées en blanc, ce qui nous permet de changer très facilement leur couleur en la multipliant par la couleur désirée.)
	if (_texture_podium == NULL)
	{printf("Erreur lors du chargement de l'icone podium (%s).\nLe dossier \"source\" a-t-il été altéré ou déplacé?\n\n", SDL_GetError()); erreur = -12;}
	else
	{SDL_SetTextureColorMod(_texture_podium, 0, 0, 0);}
	
	if (_texture_pause == NULL)
	{printf("Erreur lors du chargement du symbole \"pause\" (%s).\nLe dossier \"source\" a-t-il été altéré ou déplacé?\n\n", SDL_GetError()); erreur = -12;}
	else
	{SDL_SetTextureColorMod(_texture_pause, 0, 0, 0);}
	
	if (_texture_victoire == NULL)
	{printf("Erreur lors du chargement du symbole de fin de partie (%s).\nLe dossier \"source\" a-t-il été altéré ou déplacé?\n\n", SDL_GetError()); erreur = -12;}
	else
	{SDL_SetTextureColorMod(_texture_victoire, 0, 0, 0);}
	
	if (_texture_defaite == NULL)
	{printf("Erreur lors du chargement du symbole de fin de partie (%s).\nLe dossier \"source\" a-t-il été altéré ou déplacé?\n\n", SDL_GetError()); erreur = -12;}
	else
	{SDL_SetTextureColorMod(_texture_defaite, 143, 23, 23);}
	
	if (_texture_drapeaux == NULL)
	{printf("Erreur lors du chargement de l'icone drapeau (%s).\nLe dossier \"source\" a-t-il été altéré ou déplacé?\n\n", SDL_GetError()); erreur = -13;}
	else
	{SDL_SetTextureColorMod(_texture_drapeaux, 0, 0, 0);}
	
	if (_texture_drapeaux_mal_places == NULL)
	{printf("Erreur lors du chargement de l'icone drapeau (%s).\nLe dossier \"source\" a-t-il été altéré ou déplacé?\n\n", SDL_GetError()); erreur = -13;}
	else
	{SDL_SetTextureColorMod(_texture_drapeaux_mal_places, 143, 23, 23);}
	
	if (_texture_bombes == NULL)
	{printf("Erreur lors du chargement de l'image d'une mine (%s).\nLe dossier \"source\" a-t-il été altéré ou déplacé?\n\n", SDL_GetError()); erreur = -12;}
	else
	{SDL_SetTextureColorMod(_texture_bombes, 0, 0, 0);}
	
	if (_texture_bombe_cliquee == NULL || texture_bombe_finale == NULL)
	{printf("Erreur lors du chargement de l'image d'une mine (%s).\nLe dossier \"source\" a-t-il été altéré ou déplacé?\n\n", SDL_GetError()); erreur = -12;}
	else
	{SDL_SetTextureColorMod(_texture_bombe_cliquee, 143, 23, 23);}
	
	
	while (1)
	{
		SDL_SetColor(fond, rend_r);
		SDL_RenderClear(rend_r);
		
		TTF_SetFontStyle(police, TTF_STYLE_UNDERLINE);
		afficher_txt_centre("Modification de la couleur des symboles:", 20, xmax - 20, 10, police, couleur_timer, rend_r);
		TTF_SetFontStyle(police, TTF_STYLE_NORMAL);
		
		//Couleurs modifiables:
		afficher_txt("Symbole du podium:", 310 - longueur_txt("Symbole du podium:", 200, police) + largeur_supp / 2, 55, 200, police, couleur_timer, rend_r);
		SDL_RenderCopy(rend_r, _texture_podium, NULL, &rect_podium);
		
		afficher_txt("Symbole de pause:", 310 - longueur_txt("Symbole de pause:", 200, police) + largeur_supp / 2, 120, 200, police, couleur_timer, rend_r);
		SDL_RenderCopy(rend_r, _texture_pause, NULL, &rect_pause);
		
		afficher_txt("Symbole de victoire:", 310 - longueur_txt("Symbole de victoire:", 200, police) + largeur_supp / 2, 185, 200, police, couleur_timer, rend_r);
		SDL_RenderCopy(rend_r, _texture_victoire, NULL, &rect_victoire);
		
		afficher_txt("Symbole de défaite:", 310 - longueur_txt("Symbole de défaite:", 200, police) + largeur_supp / 2, 250, 200, police, couleur_timer, rend_r);
		SDL_RenderCopy(rend_r, _texture_defaite, NULL, &rect_defaite);
		
		afficher_txt("Drapeaux:", 310 - longueur_txt("Drapeaux:", 200, police) + largeur_supp / 2, 315, 200, police, couleur_timer, rend_r);
		SDL_RenderCopy(rend_r, _texture_drapeaux, NULL, &rect_drapeaux);
		
		afficher_txt("Drapeaux mal placés:", 310 - longueur_txt("Drapeaux mal placés:", 200, police) + largeur_supp / 2, 380, 200, police, couleur_timer, rend_r);
		SDL_RenderCopy(rend_r, _texture_drapeaux_mal_places, NULL, &rect_drapeaux_mal_places);
		
		afficher_txt("Bombes:", 310 - longueur_txt("Bombes:", 200, police) + largeur_supp / 2, 445, 200, police, couleur_timer, rend_r);
		SDL_RenderCopy(rend_r, _texture_bombes, NULL, &rect_bombes);
		
		afficher_txt("Bombe cliquée:", 310 - longueur_txt("Bombe cliquée:", 200, police) + largeur_supp / 2, 510, 200, police, couleur_timer, rend_r);
		SDL_RenderCopy(rend_r, _texture_bombe_cliquee, NULL, &rect_bombe_cliquee);
		
		for (int compteur = 1; compteur <= 8; compteur++)
		{
			if (curseur == compteur)
			{rect_arrondi(320 + largeur_supp / 2, compteur * 65 - 25, 50, 50, couleur_selection_curseur, fond, rend_r);}
			if (focus == compteur)
			{rectangle(320 + largeur_supp / 2, compteur * 65 - 25, 50, 50, 4, couleur_selection_clavier, fond, rend_r);}
		}
		
		//Boutons:
		rect_arrondi(xmax - 150, ymax - 180, 130, 40, couleur_boutons, fond, rend_r);
		if (focus == autres_couleurs)
		{rect_arrondi(xmax - 150, ymax - 180, 130, 40, couleur_selection_clavier, fond, rend_r);}
		if (curseur == autres_couleurs)
		{rect_arrondi(xmax - 150, ymax - 180, 130, 40, couleur_selection_curseur, fond, rend_r);}
		afficher_txt_centre("Modifier les", xmax - 150, xmax - 20, ymax - 175, petite_police, couleur_txt_boutons, rend_r);
		afficher_txt_centre("autres couleurs", xmax - 150, xmax - 20, ymax - 160, petite_police, couleur_txt_boutons, rend_r);
		
		rect_arrondi(xmax - 150, ymax - 120, 130, 40, couleur_boutons, fond, rend_r);
		if (focus == reinitialiser)
		{rect_arrondi(xmax - 150, ymax - 120, 130, 40, couleur_selection_clavier, fond, rend_r);}
		if (curseur == reinitialiser)
		{rect_arrondi(xmax - 150, ymax - 120, 130, 40, couleur_selection_curseur, fond, rend_r);}
		afficher_txt_centre("Réinitialiser", xmax - 150, xmax - 20, ymax - 110, police, couleur_txt_boutons, rend_r);
		
		rect_arrondi(xmax - 150, ymax - 60, 130, 40, couleur_boutons, fond, rend_r);
		if (focus == termine)
		{rect_arrondi(xmax - 150, ymax - 60, 130, 40, couleur_selection_clavier, fond, rend_r);}
		if (curseur == termine)
		{rect_arrondi(xmax - 150, ymax - 60, 130, 40, couleur_selection_curseur, fond, rend_r);}
		afficher_txt_centre("Terminé", xmax - 150, xmax - 20, ymax - 50, police, couleur_txt_boutons, rend_r);
		
		//Modification de la couleur d'un élément:
		if (choix_couleur != PAS_UN_CHOIX_DE_COULEUR)
		{
			if (mod_couleur(choix_couleur, titre))
			{
				choix_couleur = PAS_UN_CHOIX_DE_COULEUR;
				
				//Mise à jour des textures:
				_texture_podium = __texture_podium;
				_texture_pause = __texture_pause;
				_texture_victoire = __texture_victoire;
				_texture_defaite = __texture_defaite;
				_texture_drapeaux = __texture_drapeaux;
				_texture_drapeaux_mal_places = __texture_drapeaux_mal_places;
				_texture_bombes = __texture_bombes;
				_texture_bombe_cliquee = __texture_bombe_cliquee;
				SDL_SetTextureColorMod(_texture_podium, couleur_podium.r, couleur_podium.g, couleur_podium.b);
				SDL_SetTextureColorMod(_texture_pause, couleur_pause.r, couleur_pause.g, couleur_pause.b);
				SDL_SetTextureColorMod(_texture_victoire, couleur_victoire.r, couleur_victoire.g, couleur_victoire.b);
				SDL_SetTextureColorMod(_texture_defaite, couleur_defaite.r, couleur_defaite.g, couleur_defaite.b);
				SDL_SetTextureColorMod(_texture_drapeaux, couleur_drapeaux.r, couleur_drapeaux.g, couleur_drapeaux.b);
				SDL_SetTextureColorMod(_texture_drapeaux_mal_places, couleur_drapeaux_mal_places.r, couleur_drapeaux_mal_places.g, couleur_drapeaux_mal_places.b);
				SDL_SetTextureColorMod(_texture_bombes, couleur_bombes.r, couleur_bombes.g, couleur_bombes.b);
				SDL_SetTextureColorMod(_texture_bombe_cliquee, couleur_bombe_cliquee.r, couleur_bombe_cliquee.g, couleur_bombe_cliquee.b);
				
			}
			else
			{choix_couleur = NULL;}
			largeur_supp = xmax - 640;
		}
		
		//Gestion des events de cette page:
		else
		{
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
				{
					SDL_GetWindowSize(fenetre_reglages, &xmax, &ymax);
					largeur_supp = xmax - 640;
					
					rect_podium.x = 320 + largeur_supp / 2;
					rect_pause.x = 320 + largeur_supp / 2;
					rect_victoire.x = 320 + largeur_supp / 2;
					rect_defaite.x = 320 + largeur_supp / 2;
					rect_drapeaux.x = 320 + largeur_supp / 2;
					rect_drapeaux_mal_places.x = 320 + largeur_supp / 2;
					rect_bombes.x = 320 + largeur_supp / 2;
					rect_bombe_cliquee.x = 320 + largeur_supp / 2;
				}
				break;
			
			case SDL_MOUSEMOTION:
				if (ev.motion.x >= 320 + largeur_supp / 2 && ev.motion.x <= 370 + largeur_supp / 2)
				{
					curseur = 0;
					for (int compteur = 0; compteur < 8; compteur++)
					{
						if (ev.motion.y >= 40 + compteur * 65 && ev.motion.y <= 90 + compteur * 65)
						{curseur = podium + compteur;}
					}
				}
				else if (ev.motion.x >= xmax - 150 && ev.motion.x <= xmax - 20 && ev.motion.y >= ymax - 180 && ev.motion.y <= ymax - 140)
				{curseur = autres_couleurs;}
				else if (ev.motion.x >= xmax - 150 && ev.motion.x <= xmax - 20 && ev.motion.y >= ymax - 120 && ev.motion.y <= ymax - 80)
				{curseur = reinitialiser;}
				else if (ev.motion.x >= xmax - 150 && ev.motion.x <= xmax - 20 && ev.motion.y >= ymax - 60 && ev.motion.y <= ymax - 20)
				{curseur = termine;}
				else
				{curseur = 0;}
				break;
			
			case SDL_KEYDOWN:
				switch (ev.key.keysym.sym)
				{
				case SDLK_ESCAPE:
					return;
					break;
				
				case SDLK_TAB:
					if (focus >= 1 && focus <= 8)
					{focus = autres_couleurs;}
					else
					{focus = 1;}
					break;
				
				case SDLK_RIGHT:
				case SDLK_LEFT:
					if (!focus)
					{focus = 1;}
					break;
				
				case SDLK_DOWN:
					if (focus == 8)
					{focus = 1;}
					else if (focus == termine)
					{focus = autres_couleurs;}
					else
					{focus++;}
					break;
				
				case SDLK_UP:
					if (focus == 1)
					{focus = 8;}
					else if (focus == autres_couleurs)
					{focus = termine;}
					else if (!focus)
					{focus = 1;}
					else
					{focus--;}
					break;
				
				case SDLK_RETURN:
				case SDLK_KP_ENTER:
					switch (focus)
					{
					case podium:
						choix_couleur = &couleur_podium;
						strcpy(titre, "Couleur du symbole du podium:");
						break;
					
					case _pause:
						choix_couleur = &couleur_pause;
						strcpy(titre, "Couleur du symbole de pause:");
						break;
					
					case victoire:
						choix_couleur = &couleur_victoire;
						strcpy(titre, "Couleur du symbole de victoire:");
						break;
					
					case defaite:
						choix_couleur = &couleur_defaite;
						strcpy(titre, "Couleur du symbole de défaite:");
						break;
					
					case drapeaux:
						choix_couleur = &couleur_drapeaux;
						strcpy(titre, "Couleur des drapeaux:");
						break;
					
					case drapeaux_mal_places:
						choix_couleur = &couleur_drapeaux_mal_places;
						strcpy(titre, "Couleur des drapeaux mal placés:");
						break;
					
					case bombes:
						choix_couleur = &couleur_bombes;
						strcpy(titre, "Couleur des bombes:");
						break;
					
					case bombe_cliquee:
						choix_couleur = &couleur_bombe_cliquee;
						strcpy(titre, "Couleur de la bombe cliquée:");
						break;
					
					case autres_couleurs:
						mod_couleurs();
						return;
					
					case reinitialiser:
						couleur_podium = noir;
						couleur_pause = noir;
						couleur_victoire = noir;
						couleur_defaite = rouge_symboles;
						couleur_drapeaux = noir;
						couleur_drapeaux_mal_places = rouge_symboles;
						couleur_bombes = noir;
						couleur_bombe_cliquee = rouge_symboles;
						
						_texture_podium = __texture_podium;
						_texture_pause = __texture_pause;
						_texture_victoire = __texture_victoire;
						_texture_defaite = __texture_defaite;
						_texture_drapeaux = __texture_drapeaux;
						_texture_drapeaux_mal_places = __texture_drapeaux_mal_places;
						_texture_bombes = __texture_bombes;
						_texture_bombe_cliquee = __texture_bombe_cliquee;
						
						SDL_SetTextureColorMod(_texture_podium, couleur_podium.r, couleur_podium.g, couleur_podium.b);
						SDL_SetTextureColorMod(_texture_pause, couleur_pause.r, couleur_pause.g, couleur_pause.b);
						SDL_SetTextureColorMod(_texture_victoire, couleur_victoire.r, couleur_victoire.g, couleur_victoire.b);
						SDL_SetTextureColorMod(_texture_defaite, couleur_defaite.r, couleur_defaite.g, couleur_defaite.b);
						SDL_SetTextureColorMod(_texture_drapeaux, couleur_drapeaux.r, couleur_drapeaux.g, couleur_drapeaux.b);
						SDL_SetTextureColorMod(_texture_drapeaux_mal_places, couleur_drapeaux_mal_places.r, couleur_drapeaux_mal_places.g, couleur_drapeaux_mal_places.b);
						SDL_SetTextureColorMod(_texture_bombes, couleur_bombes.r, couleur_bombes.g, couleur_bombes.b);
						SDL_SetTextureColorMod(_texture_bombe_cliquee, couleur_bombe_cliquee.r, couleur_bombe_cliquee.g, couleur_bombe_cliquee.b);
						break;
					
					case termine:
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
						
						SDL_DestroyTexture(_texture_podium);
						SDL_DestroyTexture(__texture_podium);
						SDL_DestroyTexture(_texture_pause);
						SDL_DestroyTexture(__texture_pause);
						SDL_DestroyTexture(_texture_victoire);
						SDL_DestroyTexture(__texture_victoire);
						SDL_DestroyTexture(_texture_defaite);
						SDL_DestroyTexture(__texture_defaite);
						SDL_DestroyTexture(_texture_drapeaux);
						SDL_DestroyTexture(__texture_drapeaux);
						SDL_DestroyTexture(_texture_drapeaux_mal_places);
						SDL_DestroyTexture(__texture_drapeaux_mal_places);
						SDL_DestroyTexture(_texture_bombes);
						SDL_DestroyTexture(__texture_bombes);
						SDL_DestroyTexture(_texture_bombe_cliquee);
						SDL_DestroyTexture(__texture_bombe_cliquee);
						
						texture_icone_podium = IMG_LoadTexture(rend, icone_podium);
						texture_symbole_pause = IMG_LoadTexture(rend, symbole_pause);
						texture_symbole_fin_de_partie = IMG_LoadTexture(rend, symbole_fin_de_partie);
						texture_symbole_fin_de_partie_defaite = IMG_LoadTexture(rend, symbole_fin_de_partie);
						texture_bombe = IMG_LoadTexture(rend, image_bombe);
						texture_bombe_finale = IMG_LoadTexture(rend, image_bombe);
						texture_drapeau = IMG_LoadTexture(rend, icone_drapeau);
						texture_drapeau_mal_place = IMG_LoadTexture(rend, icone_drapeau);
						
						SDL_SetTextureColorMod(texture_icone_podium, couleur_podium.r, couleur_podium.g, couleur_podium.b);
						SDL_SetTextureColorMod(texture_symbole_pause, couleur_pause.r, couleur_pause.g, couleur_pause.b);
						SDL_SetTextureColorMod(texture_symbole_fin_de_partie, couleur_victoire.r, couleur_victoire.g, couleur_victoire.b);
						SDL_SetTextureColorMod(texture_symbole_fin_de_partie_defaite, couleur_defaite.r, couleur_defaite.g, couleur_defaite.b);
						SDL_SetTextureColorMod(texture_drapeau, couleur_drapeaux.r, couleur_drapeaux.g, couleur_drapeaux.b);
						SDL_SetTextureColorMod(texture_drapeau_mal_place, couleur_drapeaux_mal_places.r, couleur_drapeaux_mal_places.g, couleur_drapeaux_mal_places.b);
						SDL_SetTextureColorMod(texture_bombe, couleur_bombes.r, couleur_bombes.g, couleur_bombes.b);
						SDL_SetTextureColorMod(texture_bombe_finale, couleur_bombe_cliquee.r, couleur_bombe_cliquee.g, couleur_bombe_cliquee.b);
						return;
					}
					break;
				}
				break;
			
			case SDL_MOUSEBUTTONDOWN:
				focus = 0;
				switch (curseur)
				{
				case podium:
					choix_couleur = &couleur_podium;
					strcpy(titre, "Couleur du symbole du podium:");
					break;
				
				case _pause:
					choix_couleur = &couleur_pause;
					strcpy(titre, "Couleur du symbole de pause:");
					break;
				
				case victoire:
					choix_couleur = &couleur_victoire;
					strcpy(titre, "Couleur du symbole de victoire:");
					break;
				
				case defaite:
					choix_couleur = &couleur_defaite;
					strcpy(titre, "Couleur du symbole de défaite:");
					break;
				
				case drapeaux:
					choix_couleur = &couleur_drapeaux;
					strcpy(titre, "Couleur des drapeaux:");
					break;
				
				case drapeaux_mal_places:
					choix_couleur = &couleur_drapeaux_mal_places;
					strcpy(titre, "Couleur des drapeaux mal placés:");
					break;
				
				case bombes:
					choix_couleur = &couleur_bombes;
					strcpy(titre, "Couleur des bombes:");
					break;
				
				case bombe_cliquee:
					choix_couleur = &couleur_bombe_cliquee;
					strcpy(titre, "Couleur de la bombe cliquée:");
					break;
				
				case autres_couleurs:
					mod_couleurs();
					return;
				
				case reinitialiser:
					couleur_podium = noir;
					couleur_pause = noir;
					couleur_victoire = noir;
					couleur_defaite = rouge_symboles;
					couleur_drapeaux = noir;
					couleur_drapeaux_mal_places = rouge_symboles;
					couleur_bombes = noir;
					couleur_bombe_cliquee = rouge_symboles;
					
					_texture_podium = __texture_podium;
					_texture_pause = __texture_pause;
					_texture_victoire = __texture_victoire;
					_texture_defaite = __texture_defaite;
					_texture_drapeaux = __texture_drapeaux;
					_texture_drapeaux_mal_places = __texture_drapeaux_mal_places;
					_texture_bombes = __texture_bombes;
					_texture_bombe_cliquee = __texture_bombe_cliquee;
					
					SDL_SetTextureColorMod(_texture_podium, couleur_podium.r, couleur_podium.g, couleur_podium.b);
					SDL_SetTextureColorMod(_texture_pause, couleur_pause.r, couleur_pause.g, couleur_pause.b);
					SDL_SetTextureColorMod(_texture_victoire, couleur_victoire.r, couleur_victoire.g, couleur_victoire.b);
					SDL_SetTextureColorMod(_texture_defaite, couleur_defaite.r, couleur_defaite.g, couleur_defaite.b);
					SDL_SetTextureColorMod(_texture_drapeaux, couleur_drapeaux.r, couleur_drapeaux.g, couleur_drapeaux.b);
					SDL_SetTextureColorMod(_texture_drapeaux_mal_places, couleur_drapeaux_mal_places.r, couleur_drapeaux_mal_places.g, couleur_drapeaux_mal_places.b);
					SDL_SetTextureColorMod(_texture_bombes, couleur_bombes.r, couleur_bombes.g, couleur_bombes.b);
					SDL_SetTextureColorMod(_texture_bombe_cliquee, couleur_bombe_cliquee.r, couleur_bombe_cliquee.g, couleur_bombe_cliquee.b);
					break;
				
				case termine:
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
					
					SDL_DestroyTexture(_texture_podium);
					SDL_DestroyTexture(__texture_podium);
					SDL_DestroyTexture(_texture_pause);
					SDL_DestroyTexture(__texture_pause);
					SDL_DestroyTexture(_texture_victoire);
					SDL_DestroyTexture(__texture_victoire);
					SDL_DestroyTexture(_texture_defaite);
					SDL_DestroyTexture(__texture_defaite);
					SDL_DestroyTexture(_texture_drapeaux);
					SDL_DestroyTexture(__texture_drapeaux);
					SDL_DestroyTexture(_texture_drapeaux_mal_places);
					SDL_DestroyTexture(__texture_drapeaux_mal_places);
					SDL_DestroyTexture(_texture_bombes);
					SDL_DestroyTexture(__texture_bombes);
					SDL_DestroyTexture(_texture_bombe_cliquee);
					SDL_DestroyTexture(__texture_bombe_cliquee);
					
					texture_icone_podium = IMG_LoadTexture(rend, icone_podium);
					texture_symbole_pause = IMG_LoadTexture(rend, symbole_pause);
					texture_symbole_fin_de_partie = IMG_LoadTexture(rend, symbole_fin_de_partie);
					texture_symbole_fin_de_partie_defaite = IMG_LoadTexture(rend, symbole_fin_de_partie);
					texture_bombe = IMG_LoadTexture(rend, image_bombe);
					texture_bombe_finale = IMG_LoadTexture(rend, image_bombe);
					texture_drapeau = IMG_LoadTexture(rend, icone_drapeau);
					texture_drapeau_mal_place = IMG_LoadTexture(rend, icone_drapeau);
					
					SDL_SetTextureColorMod(texture_icone_podium, couleur_podium.r, couleur_podium.g, couleur_podium.b);
					SDL_SetTextureColorMod(texture_symbole_pause, couleur_pause.r, couleur_pause.g, couleur_pause.b);
					SDL_SetTextureColorMod(texture_symbole_fin_de_partie, couleur_victoire.r, couleur_victoire.g, couleur_victoire.b);
					SDL_SetTextureColorMod(texture_symbole_fin_de_partie_defaite, couleur_defaite.r, couleur_defaite.g, couleur_defaite.b);
					SDL_SetTextureColorMod(texture_drapeau, couleur_drapeaux.r, couleur_drapeaux.g, couleur_drapeaux.b);
					SDL_SetTextureColorMod(texture_drapeau_mal_place, couleur_drapeaux_mal_places.r, couleur_drapeaux_mal_places.g, couleur_drapeaux_mal_places.b);
					SDL_SetTextureColorMod(texture_bombe, couleur_bombes.r, couleur_bombes.g, couleur_bombes.b);
					SDL_SetTextureColorMod(texture_bombe_finale, couleur_bombe_cliquee.r, couleur_bombe_cliquee.g, couleur_bombe_cliquee.b);
					return;
				}
				break;
			}
		}
	}
}

void mod_icones_perso ()
//Permet de charger et utiliser ses propres icones personalisées
{
	enum zone
	{
		//ailleurs = 0,
		podium = 1,
		_pause,
		victoire,
		defaite,
		drapeau,
		drapeau_mal_place,
		bombe,
		bombe_cliquee,
		reinitialiser = 10,
		termine
	};
	
	SDL_Event ev;
	enum zone focus = 0;
	enum zone curseur = 0;
	char symbole_fin_de_partie_defaite[sizeof(symbole_fin_de_partie)];
	char icone_drapeau_mal_place[sizeof(icone_drapeau)];
	char image_bombe_finale[sizeof(image_bombe)];
	
	//Rectanles d'affichage symboles:
	SDL_Rect rect_podium = {20 + (xmax - 580) / 2, 40, 50, 50};
	SDL_Rect rect_pause = {20 + (xmax - 580) / 2, 105, 50, 50};
	SDL_Rect rect_victoire = {20 + (xmax - 580) / 2, 170, 50, 50};
	SDL_Rect rect_defaite = {20 + (xmax - 580) / 2, 235, 50, 50};
	SDL_Rect rect_drapeaux = {20 + (xmax - 580) / 2, 300, 50, 50};
	SDL_Rect rect_drapeaux_mal_places = {20 + (xmax - 580) / 2, 365, 50, 50};
	SDL_Rect rect_bombes = {20 + (xmax - 580) / 2, 430, 50, 50};
	SDL_Rect rect_bombe_cliquee = {20 + (xmax - 580) / 2, 495, 50, 50};
	
	//Textures des symboles (elles doivent être refaites, vu que ce n'est pas le même renderer...):
	SDL_Texture* texture_podium = IMG_LoadTexture(rend_r, icone_podium);
	SDL_Texture* texture_pause = IMG_LoadTexture(rend_r, symbole_pause);
	SDL_Texture* texture_victoire = IMG_LoadTexture(rend_r, symbole_fin_de_partie);
	SDL_Texture* texture_defaite = IMG_LoadTexture(rend_r, symbole_fin_de_partie);
	SDL_Texture* texture_drapeaux = IMG_LoadTexture(rend_r, icone_drapeau);
	SDL_Texture* texture_drapeaux_mal_places = IMG_LoadTexture(rend_r, icone_drapeau);
	SDL_Texture* texture_bombes = IMG_LoadTexture(rend_r, image_bombe);
	SDL_Texture* texture_bombe_cliquee = IMG_LoadTexture(rend_r, image_bombe);
	
	//Retranscription de la source de certaines icones:
	strcpy(symbole_fin_de_partie_defaite, symbole_fin_de_partie);
	strcpy(icone_drapeau_mal_place, icone_drapeau);
	strcpy(image_bombe_finale, image_bombe);
	
	
	while (1)
	{
		//Arrière-plan:
		SDL_SetRenderDrawColor(rend_r, fond.r, fond.g, fond.b, fond.a);
		SDL_RenderClear(rend_r);
		
		//Titre:
		TTF_SetFontStyle(police, TTF_STYLE_UNDERLINE);
		afficher_txt_centre("Charger de nouveaux symboles:", 0, xmax, 10, police, couleur_timer, rend_r);
		TTF_SetFontStyle(police, TTF_STYLE_NORMAL);
		
		//Boutons:
		for (int compteur = 1; compteur <= 8; compteur++)
		{
			rect_arrondi(20 + (xmax - 580) / 2, compteur * 65 - 25, 50, 50, couleur_boutons, fond, rend_r);
			if (curseur == compteur)
			{rect_arrondi(20 + (xmax - 580) / 2, compteur * 65 - 25, 50, 50, couleur_selection_curseur, fond, rend_r);}
			if (focus == compteur)
			{rectangle(20 + (xmax - 580) / 2, compteur * 65 - 25, 50, 50, 4, couleur_selection_clavier, fond, rend_r);}
		}
		
		//Texte et icones:
		afficher_txt("Symbole du podium", 80 + (xmax - 580) / 2, 55, 200, police, couleur_timer, rend_r);
		SDL_RenderCopy(rend_r, texture_podium, NULL, &rect_podium);
		
		afficher_txt("Symbole de pause", 80 + (xmax - 580) / 2, 120, 200, police, couleur_timer, rend_r);
		SDL_RenderCopy(rend_r, texture_pause, NULL, &rect_pause);
		
		afficher_txt("Symbole de victoire", 80 + (xmax - 580) / 2, 185, 200, police, couleur_timer, rend_r);
		SDL_RenderCopy(rend_r, texture_victoire, NULL, &rect_victoire);
		
		afficher_txt("Symbole de défaite", 80 + (xmax - 580) / 2, 250, 200, police, couleur_timer, rend_r);
		SDL_RenderCopy(rend_r, texture_defaite, NULL, &rect_defaite);
		
		afficher_txt("Drapeaux", 80 + (xmax - 580) / 2, 315, 200, police, couleur_timer, rend_r);
		SDL_RenderCopy(rend_r, texture_drapeaux, NULL, &rect_drapeaux);
		
		afficher_txt("Drapeaux mal placés", 80 + (xmax - 580) / 2, 380, 200, police, couleur_timer, rend_r);
		SDL_RenderCopy(rend_r, texture_drapeaux_mal_places, NULL, &rect_drapeaux_mal_places);
		
		afficher_txt("Bombes", 80 + (xmax - 580) / 2, 445, 200, police, couleur_timer, rend_r);
		SDL_RenderCopy(rend_r, texture_bombes, NULL, &rect_bombes);
		
		afficher_txt("Bombe cliquée", 80 + (xmax - 580) / 2, 510, 200, police, couleur_timer, rend_r);
		SDL_RenderCopy(rend_r, texture_bombe_cliquee, NULL, &rect_bombe_cliquee);
		
		//Explications:
		afficher_txt("Cliquer sur un symbole pour en charger un nouveau.\nToutes les images doivent avoir un fond transparent!\nSi l'image choisie n'a pas les bonnes dimensions, elle sera étirée ou compressée en conséquence.\nLes \
images devraient déjà être de la bonne couleur, car le programme permet seulement de colorier les symboles par défaut.\nSi vous remplacez les images par défaut par les vôtres, ces dernières devront être blanches et avoir le même \
nom que les anciennes. Vous devrez aussi redémarrer le programme.", 310 + (xmax - 580) / 2, 45, 240 + (xmax - 580) / 2, petite_police, couleur_txt_boutons, rend_r);
		afficher_txt("Seul le drapeau a un fallback sans image.", 250 + (xmax - 580), ymax - 60, 200, petite_police, couleur_timer, rend_r);
		
		//Boutons d'action:
		rect_arrondi(xmax - 150, ymax - 120, 130, 40, couleur_boutons, fond, rend_r);
		if (focus == reinitialiser)
		{rect_arrondi(xmax - 150, ymax - 120, 130, 40, couleur_selection_clavier, fond, rend_r);}
		if (curseur == reinitialiser)
		{rect_arrondi(xmax - 150, ymax - 120, 130, 40, couleur_selection_curseur, fond, rend_r);}
		afficher_txt_centre("Réinitialiser", xmax - 150, xmax - 20, ymax - 110, police, couleur_txt_boutons, rend_r);
		
		rect_arrondi(xmax - 150, ymax - 60, 130, 40, couleur_boutons, fond, rend_r);
		if (focus == termine)
		{rect_arrondi(xmax - 150, ymax - 60, 130, 40, couleur_selection_clavier, fond, rend_r);}
		if (curseur == termine)
		{rect_arrondi(xmax - 150, ymax - 60, 130, 40, couleur_selection_curseur, fond, rend_r);}
		afficher_txt_centre("Terminé", xmax - 150, xmax - 20, ymax - 50, police, couleur_txt_boutons, rend_r);
		
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
			{
				SDL_GetWindowSize(fenetre_reglages, &xmax, &ymax);
				
				rect_podium.x = 20 + (xmax - 580) / 2;
				rect_pause.x = 20 + (xmax - 580) / 2;
				rect_victoire.x = 20 + (xmax - 580) / 2;
				rect_defaite.x = 20 + (xmax - 580) / 2;
				rect_drapeaux.x = 20 + (xmax - 580) / 2;
				rect_drapeaux_mal_places.x = 20 + (xmax - 580) / 2;
				rect_bombes.x = 20 + (xmax - 580) / 2;
				rect_bombe_cliquee.x = 20 + (xmax - 580) / 2;
			}
			break;
		
		case SDL_MOUSEMOTION:
			if (ev.motion.x >= 20 + (xmax - 580) / 2 && ev.motion.x <= 70 + (xmax - 580) / 2)
			{
				curseur = 0;
				for (int compteur = 0; compteur < 8; compteur++)
				{
					if (ev.motion.y >= 40 + compteur * 65 && ev.motion.y <= 90 + compteur * 65)
					{curseur = podium + compteur;}
				}
			}
			else if (ev.motion.x >= xmax - 150 && ev.motion.x <= xmax - 20 && ev.motion.y >= ymax - 120 && ev.motion.y <= ymax - 80)
			{curseur = reinitialiser;}
			else if (ev.motion.x >= xmax - 150 && ev.motion.x <= xmax - 20 && ev.motion.y >= ymax - 60 && ev.motion.y <= ymax - 20)
			{curseur = termine;}
			else
			{curseur = 0;}
			break;
		
		case SDL_KEYDOWN:
			switch (ev.key.keysym.sym)
			{
			case SDLK_ESCAPE:
				return;
			
			case SDLK_TAB:
				if (focus >= podium && focus <= bombe_cliquee)
				{focus = reinitialiser;}
				else
				{focus = podium;}
				break;
			
			case SDLK_UP:
				if (focus > podium && focus <= bombe_cliquee)
				{focus--;}
				else if (focus == podium)
				{focus = bombe_cliquee;}
				else if (focus == termine)
				{focus = reinitialiser;}
				else if (focus == reinitialiser)
				{focus = termine;}
				else if (!focus)
				{focus = podium;}
				break;
			
			case SDLK_DOWN:
				if (focus >= podium && focus < bombe_cliquee)
				{focus++;}
				else if (focus == bombe_cliquee || !focus)
				{focus = podium;}
				else if (focus == termine)
				{focus = reinitialiser;}
				else if (focus == reinitialiser)
				{focus = termine;}
				break;
			
			case SDLK_RIGHT:
			case SDLK_LEFT:
				if (!focus)
				{focus++;}
				break;
			
			case SDLK_RETURN:
			case SDLK_KP_ENTER:
				switch (focus)
				{
				case podium:
					demander_txt("Charger un nouvel icone", "Pour charger un nouveau symbole de podium, suivez les étapes suivantes:\n1. Placer le fichier contenant le nouveau symbole dans le dossier nommé \"source\".\n2. Dans \
cette fenêtre, effacer le nom de l'ancien fichier (en gardant le \"./source/\").\n3. Écrire le nom du nouveau fichier, sans oublier l'extension (.png, .jpeg, etc.).\n4. Cliquer \"Terminé\"!\n\nL'arrière-plan doit être \
transparent. Les dimensions idéales sont 80x80 pixels.", icone_podium, sizeof(icone_podium), fenetre_reglages);
					if (texture_icone_podium != NULL)
					{SDL_DestroyTexture(texture_icone_podium);}
					texture_icone_podium = IMG_LoadTexture(rend, icone_podium);
					break;
				
				case _pause:
					demander_txt("Charger un nouvel icone", "Pour charger un nouveau symbole de pause, suivez les étapes suivantes:\n1. Placer le fichier contenant le nouveau symbole dans le dossier nommé \"source\".\n2. Dans \
cette fenêtre, effacer le nom de l'ancien fichier (en gardant le \"./source/\").\n3. Écrire le nom du nouveau fichier, sans oublier l'extension (.png, .jpeg, etc.).\n4. Cliquer \"Terminé\"!\n\nL'arrière-plan doit être \
transparent. Les dimensions idéales sont 80x80 pixels.", symbole_pause, sizeof(symbole_pause), fenetre_reglages);
					if (texture_symbole_pause != NULL)
					{SDL_DestroyTexture(texture_symbole_pause);}
					texture_symbole_pause = IMG_LoadTexture(rend, symbole_pause);
					break;
				
				case victoire:
					demander_txt("Charger un nouvel icone", "Pour charger un nouveau symbole de victoire, suivez les étapes suivantes:\n1. Placer le fichier contenant le nouveau symbole dans le dossier nommé \"source\".\n2. Dans \
cette fenêtre, effacer le nom de l'ancien fichier (en gardant le \"./source/\").\n3. Écrire le nom du nouveau fichier, sans oublier l'extension (.png, .jpeg, etc.).\n4. Cliquer \"Terminé\"!\n\nL'arrière-plan doit être \
transparent. Les dimensions idéales sont 80x80 pixels.", symbole_fin_de_partie, sizeof(symbole_fin_de_partie), fenetre_reglages);
					if (texture_symbole_fin_de_partie != NULL)
					{SDL_DestroyTexture(texture_symbole_fin_de_partie);}
					texture_symbole_fin_de_partie = IMG_LoadTexture(rend, symbole_fin_de_partie);
					break;
				
				case defaite:
					demander_txt("Charger un nouvel icone", "Pour charger un nouveau symbole de défaite, suivez les étapes suivantes:\n1. Placer le fichier contenant le nouveau symbole dans le dossier nommé \"source\".\n2. Dans \
cette fenêtre, effacer le nom de l'ancien fichier (en gardant le \"./source/\").\n3. Écrire le nom du nouveau fichier, sans oublier l'extension (.png, .jpeg, etc.).\n4. Cliquer \"Terminé\"!\n\nL'arrière-plan doit être \
transparent. Les dimensions idéales sont 80x80 pixels.", symbole_fin_de_partie_defaite, sizeof(symbole_fin_de_partie_defaite), fenetre_reglages);
					if (texture_symbole_fin_de_partie_defaite != NULL)
					{SDL_DestroyTexture(texture_symbole_fin_de_partie_defaite);}
					texture_symbole_fin_de_partie_defaite = IMG_LoadTexture(rend, symbole_fin_de_partie_defaite);
					break;
				
				case drapeau:
					demander_txt("Charger un nouvel icone", "Pour charger une nouvelle image de drapeau, suivez les étapes suivantes:\n1. Placer le fichier contenant le nouveau symbole dans le dossier nommé \"source\".\n2. Dans \
cette fenêtre, effacer le nom de l'ancien fichier (en gardant le \"./source/\").\n3. Écrire le nom du nouveau fichier, sans oublier l'extension (.png, .jpeg, etc.).\n4. Cliquer \"Terminé\"!\n\nL'arrière-plan doit être \
transparent. Les dimensions idéales sont 80x80 pixels.", icone_drapeau, sizeof(icone_drapeau), fenetre_reglages);
					if (texture_drapeau != NULL)
					{SDL_DestroyTexture(texture_drapeau);}
					texture_drapeau = IMG_LoadTexture(rend, icone_drapeau);
					break;
				
				case drapeau_mal_place:
					demander_txt("Charger un nouvel icone", "Pour charger une nouvelle image de drapeau mal placé, suivez les étapes suivantes:\n1. Placer le fichier contenant le nouveau symbole dans le dossier nommé \
\"source\".\n2. Dans cette fenêtre, effacer le nom de l'ancien fichier (en gardant le \"./source/\").\n3. Écrire le nom du nouveau fichier, sans oublier l'extension (.png, .jpeg, etc.).\n4. Cliquer \"Terminé\"!\n\nL'arrière-plan \
doit être transparent. Les dimensions idéales sont 80x80 pixels.", icone_drapeau_mal_place, sizeof(icone_drapeau_mal_place), fenetre_reglages);
					if (texture_drapeau_mal_place != NULL)
					{SDL_DestroyTexture(texture_drapeau_mal_place);}
					texture_drapeau_mal_place = IMG_LoadTexture(rend, icone_drapeau_mal_place);
					break;
				
				case bombe:
					demander_txt("Charger un nouvel icone", "Pour charger une nouvelle image de bombe, suivez les étapes suivantes:\n1. Placer le fichier contenant le nouveau symbole dans le dossier nommé \"source\".\n2. Dans \
cette fenêtre, effacer le nom de l'ancien fichier (en gardant le \"./source/\").\n3. Écrire le nom du nouveau fichier, sans oublier l'extension (.png, .jpeg, etc.).\n4. Cliquer \"Terminé\"!\n\nL'arrière-plan doit être \
transparent. Les dimensions idéales sont 80x80 pixels.", image_bombe, sizeof(image_bombe), fenetre_reglages);
					if (texture_bombe != NULL)
					{SDL_DestroyTexture(texture_bombe);}
					texture_bombe = IMG_LoadTexture(rend, image_bombe);
					break;
					
				case bombe_cliquee:
					demander_txt("Charger un nouvel icone", "Pour charger une nouvelle image de bombe cliquée, suivez les étapes suivantes:\n1. Placer le fichier contenant le nouveau symbole dans le dossier nommé \"source\".\n2. \
Dans cette fenêtre, effacer le nom de l'ancien fichier (en gardant le \"./source/\").\n3. Écrire le nom du nouveau fichier, sans oublier l'extension (.png, .jpeg, etc.).\n4. Cliquer \"Terminé\"!\n\nL'arrière-plan doit être \
transparent. Les dimensions idéales sont 80x80 pixels.", image_bombe_finale, sizeof(image_bombe_finale), fenetre_reglages);
					if (texture_bombe_finale != NULL)
					{SDL_DestroyTexture(texture_bombe_finale);}
					texture_bombe_finale = IMG_LoadTexture(rend, image_bombe_finale);
					break;
				
				case reinitialiser:
					strcpy(param[1].option[0].nom, "Utiliser les symboles pleins");
					mod_theme();
					break;
				
				case termine:
					return;
				}
				
				//À faire: Détruire les anciennes textures, faire la même chose pour le clavier et tester (ça bug, je le sais).
				if (texture_podium != NULL)
				{SDL_DestroyTexture(texture_podium);}
				if (texture_pause != NULL)
				{SDL_DestroyTexture(texture_pause);}
				if (texture_victoire != NULL)
				{SDL_DestroyTexture(texture_victoire);}
				if (texture_defaite != NULL)
				{SDL_DestroyTexture(texture_defaite);}
				if (texture_drapeaux != NULL)
				{SDL_DestroyTexture(texture_drapeaux);}
				if (texture_drapeaux_mal_places != NULL)
				{SDL_DestroyTexture(texture_drapeaux_mal_places);}
				if (texture_bombes != NULL)
				{SDL_DestroyTexture(texture_bombes);}
				if (texture_bombe_cliquee != NULL)
				{SDL_DestroyTexture(texture_bombe_cliquee);}
				
				texture_podium = IMG_LoadTexture(rend_r, icone_podium);
				texture_pause = IMG_LoadTexture(rend_r, symbole_pause);
				texture_victoire = IMG_LoadTexture(rend_r, symbole_fin_de_partie);
				texture_defaite = IMG_LoadTexture(rend_r, symbole_fin_de_partie_defaite);
				texture_drapeaux = IMG_LoadTexture(rend_r, icone_drapeau);
				texture_drapeaux_mal_places = IMG_LoadTexture(rend_r, icone_drapeau_mal_place);
				texture_bombes = IMG_LoadTexture(rend_r, image_bombe);
				texture_bombe_cliquee = IMG_LoadTexture(rend_r, image_bombe_finale);
				break;
			}
			break;
		
		case SDL_MOUSEBUTTONDOWN:
			switch (curseur)
			{
			case podium:
				demander_txt("Charger un nouvel icone", "Pour charger un nouveau symbole de podium, suivez les étapes suivantes:\n1. Placer le fichier contenant le nouveau symbole dans le dossier nommé \"source\".\n2. Dans \
cette fenêtre, effacer le nom de l'ancien fichier (en gardant le \"./source/\").\n3. Écrire le nom du nouveau fichier, sans oublier l'extension (.png, .jpeg, etc.).\n4. Cliquer \"Terminé\"!\n\nL'arrière-plan doit être \
transparent. Les dimensions idéales sont 80x80 pixels.", icone_podium, sizeof(icone_podium), fenetre_reglages);
				if (texture_icone_podium != NULL)
				{SDL_DestroyTexture(texture_icone_podium);}
				texture_icone_podium = IMG_LoadTexture(rend, icone_podium);
				break;
			
			case _pause:
				demander_txt("Charger un nouvel icone", "Pour charger un nouveau symbole de pause, suivez les étapes suivantes:\n1. Placer le fichier contenant le nouveau symbole dans le dossier nommé \"source\".\n2. Dans \
cette fenêtre, effacer le nom de l'ancien fichier (en gardant le \"./source/\").\n3. Écrire le nom du nouveau fichier, sans oublier l'extension (.png, .jpeg, etc.).\n4. Cliquer \"Terminé\"!\n\nL'arrière-plan doit être \
transparent. Les dimensions idéales sont 80x80 pixels.", symbole_pause, sizeof(symbole_pause), fenetre_reglages);
				if (texture_symbole_pause != NULL)
				{SDL_DestroyTexture(texture_symbole_pause);}
				texture_symbole_pause = IMG_LoadTexture(rend, symbole_pause);
				break;
			
			case victoire:
				demander_txt("Charger un nouvel icone", "Pour charger un nouveau symbole de victoire, suivez les étapes suivantes:\n1. Placer le fichier contenant le nouveau symbole dans le dossier nommé \"source\".\n2. Dans \
cette fenêtre, effacer le nom de l'ancien fichier (en gardant le \"./source/\").\n3. Écrire le nom du nouveau fichier, sans oublier l'extension (.png, .jpeg, etc.).\n4. Cliquer \"Terminé\"!\n\nL'arrière-plan doit être \
transparent. Les dimensions idéales sont 80x80 pixels.", symbole_fin_de_partie, sizeof(symbole_fin_de_partie), fenetre_reglages);
				if (texture_symbole_fin_de_partie != NULL)
				{SDL_DestroyTexture(texture_symbole_fin_de_partie);}
				texture_symbole_fin_de_partie = IMG_LoadTexture(rend, symbole_fin_de_partie);
				break;
			
			case defaite:
				demander_txt("Charger un nouvel icone", "Pour charger un nouveau symbole de défaite, suivez les étapes suivantes:\n1. Placer le fichier contenant le nouveau symbole dans le dossier nommé \"source\".\n2. Dans \
cette fenêtre, effacer le nom de l'ancien fichier (en gardant le \"./source/\").\n3. Écrire le nom du nouveau fichier, sans oublier l'extension (.png, .jpeg, etc.).\n4. Cliquer \"Terminé\"!\n\nL'arrière-plan doit être \
transparent. Les dimensions idéales sont 80x80 pixels.", symbole_fin_de_partie_defaite, sizeof(symbole_fin_de_partie_defaite), fenetre_reglages);
				if (texture_symbole_fin_de_partie_defaite != NULL)
				{SDL_DestroyTexture(texture_symbole_fin_de_partie_defaite);}
				texture_symbole_fin_de_partie_defaite = IMG_LoadTexture(rend, symbole_fin_de_partie_defaite);
				break;
			
			case drapeau:
				demander_txt("Charger un nouvel icone", "Pour charger une nouvelle image de drapeau, suivez les étapes suivantes:\n1. Placer le fichier contenant le nouveau symbole dans le dossier nommé \"source\".\n2. Dans \
cette fenêtre, effacer le nom de l'ancien fichier (en gardant le \"./source/\").\n3. Écrire le nom du nouveau fichier, sans oublier l'extension (.png, .jpeg, etc.).\n4. Cliquer \"Terminé\"!\n\nL'arrière-plan doit être \
transparent. Les dimensions idéales sont 80x80 pixels.", icone_drapeau, sizeof(icone_drapeau), fenetre_reglages);
				if (texture_drapeau != NULL)
				{SDL_DestroyTexture(texture_drapeau);}
				texture_drapeau = IMG_LoadTexture(rend, icone_drapeau);
				break;
			
			case drapeau_mal_place:
				demander_txt("Charger un nouvel icone", "Pour charger une nouvelle image de drapeau mal placé, suivez les étapes suivantes:\n1. Placer le fichier contenant le nouveau symbole dans le dossier nommé \
\"source\".\n2. Dans cette fenêtre, effacer le nom de l'ancien fichier (en gardant le \"./source/\").\n3. Écrire le nom du nouveau fichier, sans oublier l'extension (.png, .jpeg, etc.).\n4. Cliquer \"Terminé\"!\n\nL'arrière-plan \
doit être transparent. Les dimensions idéales sont 80x80 pixels.", icone_drapeau_mal_place, sizeof(icone_drapeau_mal_place), fenetre_reglages);
				if (texture_drapeau_mal_place != NULL)
				{SDL_DestroyTexture(texture_drapeau_mal_place);}
				texture_drapeau_mal_place = IMG_LoadTexture(rend, icone_drapeau_mal_place);
				break;
			
			case bombe:
				demander_txt("Charger un nouvel icone", "Pour charger une nouvelle image de bombe, suivez les étapes suivantes:\n1. Placer le fichier contenant le nouveau symbole dans le dossier nommé \"source\".\n2. Dans \
cette fenêtre, effacer le nom de l'ancien fichier (en gardant le \"./source/\").\n3. Écrire le nom du nouveau fichier, sans oublier l'extension (.png, .jpeg, etc.).\n4. Cliquer \"Terminé\"!\n\nL'arrière-plan doit être \
transparent. Les dimensions idéales sont 80x80 pixels.", image_bombe, sizeof(image_bombe), fenetre_reglages);
				if (texture_bombe != NULL)
				{SDL_DestroyTexture(texture_bombe);}
				texture_bombe = IMG_LoadTexture(rend, image_bombe);
				break;
				
			case bombe_cliquee:
				demander_txt("Charger un nouvel icone", "Pour charger une nouvelle image de bombe cliquée, suivez les étapes suivantes:\n1. Placer le fichier contenant le nouveau symbole dans le dossier nommé \"source\".\n2. \
Dans cette fenêtre, effacer le nom de l'ancien fichier (en gardant le \"./source/\").\n3. Écrire le nom du nouveau fichier, sans oublier l'extension (.png, .jpeg, etc.).\n4. Cliquer \"Terminé\"!\n\nL'arrière-plan doit être \
transparent. Les dimensions idéales sont 80x80 pixels.", image_bombe_finale, sizeof(image_bombe_finale), fenetre_reglages);
				if (texture_bombe_finale != NULL)
				{SDL_DestroyTexture(texture_bombe_finale);}
				texture_bombe_finale = IMG_LoadTexture(rend, image_bombe_finale);
				break;
			
			case reinitialiser:
				strcpy(param[1].option[0].nom, "Utiliser les symboles pleins");
				mod_theme();
				break;
			
			case termine:
				return;
			}
			
			//À faire: Détruire les anciennes textures, faire la même chose pour le clavier et tester (ça bug, je le sais).
			if (texture_podium != NULL)
			{SDL_DestroyTexture(texture_podium);}
			if (texture_pause != NULL)
			{SDL_DestroyTexture(texture_pause);}
			if (texture_victoire != NULL)
			{SDL_DestroyTexture(texture_victoire);}
			if (texture_defaite != NULL)
			{SDL_DestroyTexture(texture_defaite);}
			if (texture_drapeaux != NULL)
			{SDL_DestroyTexture(texture_drapeaux);}
			if (texture_drapeaux_mal_places != NULL)
			{SDL_DestroyTexture(texture_drapeaux_mal_places);}
			if (texture_bombes != NULL)
			{SDL_DestroyTexture(texture_bombes);}
			if (texture_bombe_cliquee != NULL)
			{SDL_DestroyTexture(texture_bombe_cliquee);}
			
			texture_podium = IMG_LoadTexture(rend_r, icone_podium);
			texture_pause = IMG_LoadTexture(rend_r, symbole_pause);
			texture_victoire = IMG_LoadTexture(rend_r, symbole_fin_de_partie);
			texture_defaite = IMG_LoadTexture(rend_r, symbole_fin_de_partie_defaite);
			texture_drapeaux = IMG_LoadTexture(rend_r, icone_drapeau);
			texture_drapeaux_mal_places = IMG_LoadTexture(rend_r, icone_drapeau_mal_place);
			texture_bombes = IMG_LoadTexture(rend_r, image_bombe);
			texture_bombe_cliquee = IMG_LoadTexture(rend_r, image_bombe_finale);
			break;
		}
	}
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
		if (*y < 500)
		{*y = 500;}
	}
	else //fenêtre des réglages ou du podium
	{
		if (*x < 580)
		{*x = 580;}
		if (*y < 570)
		{*y = 570;}
	}
}

_Bool mod_couleur (SDL_Color* ptr_couleur, char titre[])
//Permet à l'utilisateur de choisir une nouvelle couleur pour un élément graphique du programme.
//Doit recevoir en paramètre un pointeur vers la structure SDL_Color associé à cet élément lors du premier appel à la fonction.
//Ensuite, ptr_couleur doit être NULL pour continuer à travailler sur le même élément.
//Renvoie 0 si ce n'est pas terminé et 1 lorsque tout est terminé.
{
	enum zone
	{
		//0 = ailleurs...
		rouge = 1,
		vert,
		bleu,
		alpha,
		bouton_color_picker = 9,
		annuler = 10,
		appliquer,
		termine
	};
	
	SDL_Event ev;
	SDL_Surface* surface_nbre;
	static SDL_Color* couleur;
	SDL_Color couleur_modifiee;
	FILE* cp_return;
	static enum zone focus = 0;
	static enum zone curseur = 0;
	int largeur_supp = xmax - 580;
	static _Bool keymod = 0;
	static char r[4], g[4], b[4], a[4];
	char cp_value[50] = "";
	char nbre_a_afficher[5] = "?"; //string qui contiendra le nbre à transformer en texture
	
	if (ptr_couleur != NULL)
	{
		couleur = ptr_couleur;
		couleur_modifiee = *ptr_couleur;
		
		//Conversion des valeurs numériques rgba en strings de 1 à 3 caractères:
		sprintf(r, "%d", couleur->r);
		sprintf(g, "%d", couleur->g);
		sprintf(b, "%d", couleur->b);
		sprintf(a, "%d", couleur->a);
	}
	else
	{
		sscanf(r, "%hhd", &couleur_modifiee.r);
		sscanf(g, "%hhd", &couleur_modifiee.g);
		sscanf(b, "%hhd", &couleur_modifiee.b);
		sscanf(a, "%hhd", &couleur_modifiee.a);
	}
	
	//Pop-up:
	rectangle(60, 100, xmax - 120, ymax - 200, 0, couleur_boutons, fond, rend_r);
	rectangle(60, 100, xmax - 120, ymax - 200, 5, couleur_timer, fond, rend_r);
	
	//Titre:
	TTF_SetFontStyle(police, TTF_STYLE_UNDERLINE);
	afficher_txt_centre(titre, 60, xmax - 60, 115, police, couleur_txt_boutons, rend_r);
	TTF_SetFontStyle(police, TTF_STYLE_NORMAL);
	
	//Boîtes de texte RGBA:
	for (int compteur = 0; compteur < 4; compteur++)
	{
		rectangle(170 + largeur_supp / 2, 160 + 60 * compteur, 130, 40, 0, fond, couleur_boutons, rend_r);
		if (focus == compteur + 1)
		{rectangle(170 + largeur_supp / 2, 160 + 60 * compteur, 130, 40, 0, couleur_selection_curseur, couleur_boutons, rend_r);}
		rectangle(170 + largeur_supp / 2, 160 + 60 * compteur, 130, 40, 4, couleur_txt_boutons, couleur_boutons, rend_r);
	}
	
	afficher_txt("Rouge:", 96 + largeur_supp / 2, 170, 100, police, couleur_txt_boutons, rend_r);
	afficher_txt(r, 180 + largeur_supp / 2, 170, 110, police, couleur_timer, rend_r);
	
	afficher_txt("Vert:", 114 + largeur_supp / 2, 230, 100, police, couleur_txt_boutons, rend_r);
	afficher_txt(g, 180 + largeur_supp / 2, 230, 110, police, couleur_timer, rend_r);
	
	afficher_txt("Bleu:", 112 + largeur_supp / 2, 290, 100, police, couleur_txt_boutons, rend_r);
	afficher_txt(b, 180 + largeur_supp / 2, 290, 110, police, couleur_timer, rend_r);
	
	afficher_txt("Alpha:", 100 + largeur_supp / 2, 350, 100, police, couleur_txt_boutons, rend_r);
	afficher_txt(a, 180 + largeur_supp / 2, 350, 110, police, couleur_timer, rend_r);
	
	//Échantillon:
	rectangle(360 + largeur_supp / 2, 160, 100, 100, 0, couleur_modifiee, couleur_boutons, rend_r);
	rectangle(360 + largeur_supp / 2, 160, 100, 100, 4, couleur_txt_boutons, couleur_boutons, rend_r);
	rectangle(430 + largeur_supp / 2, 150, 40, 40, 0, *couleur, couleur_boutons, rend_r);
	rectangle(430 + largeur_supp / 2, 150, 40, 40, 3, couleur_txt_boutons, couleur_boutons, rend_r);
	SDL_SetColor(couleur_modifiee, rend_r);
	SDL_RenderDrawPoint(rend_r, 429 + largeur_supp / 2, 190);
	
	//Bouton et explication du color picker:
	rect_arrondi(340 + largeur_supp / 2, 270, 140, 40, fond, couleur_boutons, rend_r);
	if (focus == bouton_color_picker)
	{rect_arrondi(340 + largeur_supp / 2, 270, 140, 40, couleur_selection_clavier, couleur_boutons, rend_r);}
	if (curseur == bouton_color_picker)
	{rect_arrondi(340 + largeur_supp / 2, 270, 140, 40, couleur_selection_curseur, couleur_boutons, rend_r);}
	afficher_txt_centre("Color Picker", 340 + largeur_supp / 2, 480 + largeur_supp / 2, 280, police, couleur_timer, rend_r);
	afficher_txt("Pour en savoir plus sur le color picker, faites un clic droit sur le bouton.", 330 + largeur_supp / 2, 320, 160 + largeur_supp / 2, petite_police, couleur_txt_boutons, rend_r);
	
	//Boutons du bas:
	rect_arrondi(xmax - 480, ymax - 160, 120, 40, fond, couleur_boutons, rend_r);
	if (focus == annuler)
	{rect_arrondi(xmax - 480, ymax - 160, 120, 40, couleur_selection_clavier, couleur_boutons, rend_r);}
	if (curseur == annuler)
	{rect_arrondi(xmax - 480, ymax - 160, 120, 40, couleur_selection_curseur, couleur_boutons, rend_r);}
	afficher_txt_centre("Annuler", xmax - 480, xmax - 360, ymax - 150, police, couleur_timer, rend_r);
	
	rect_arrondi(xmax - 340, ymax - 160, 120, 40, fond, couleur_boutons, rend_r);
	if (focus == appliquer)
	{rect_arrondi(xmax - 340, ymax - 160, 120, 40, couleur_selection_clavier, couleur_boutons, rend_r);}
	if (curseur == appliquer)
	{rect_arrondi(xmax - 340, ymax - 160, 120, 40, couleur_selection_curseur, couleur_boutons, rend_r);}
	afficher_txt_centre("Appliquer", xmax - 340, xmax - 220, ymax - 150, police, couleur_timer, rend_r);
	
	rect_arrondi(xmax - 200, ymax - 160, 120, 40, fond, couleur_boutons, rend_r);
	if (focus == termine)
	{rect_arrondi(xmax - 200, ymax - 160, 120, 40, couleur_selection_clavier, couleur_boutons, rend_r);}
	if (curseur == termine)
	{rect_arrondi(xmax - 200, ymax - 160, 120, 40, couleur_selection_curseur, couleur_boutons, rend_r);}
	afficher_txt_centre("Terminé", xmax - 200, xmax - 80, ymax - 150, police, couleur_timer, rend_r);
	
	//Affichage et gestion des events:
	SDL_RenderPresent(rend_r);
	SDL_WaitEvent(&ev);
	
	switch (ev.type)
	{
	case SDL_WINDOWEVENT:
		if (ev.window.windowID != ID_fenetre_reglages) //si l'utilisateur joue avec l'autre fenêtre (lui donnant ainsi le focus, déclenchant cet event), on veut le ramener dans les réglages
		{SDL_RaiseWindow(fenetre_reglages); SDL_FlashWindow(fenetre_reglages, SDL_FLASH_UNTIL_FOCUSED);}
		else if (ev.window.event == SDL_WINDOWEVENT_CLOSE) //SDL_QUIT ne fonctionne pas avec plusieurs fenêtres ouvertes...
		{focus = 0; curseur = 0; keymod = 0; return 1;}
		else
		{SDL_GetWindowSize(fenetre_reglages, &xmax, &ymax);}
		break;
	
	case SDL_MOUSEMOTION:
		if (ev.motion.y >= ymax - 160 && ev.motion.y <= ymax - 120)
		{
			SDL_SetCursor(curseur_normal);
			curseur = 0;
			for (int compteur = 0; compteur < 3; compteur++)
			{
				if (ev.motion.x >= xmax - 480 + 140 * compteur && ev.motion.x <= xmax - 360 + 140 * compteur)
				{curseur = annuler + compteur;}
			}
		}
		else if (ev.motion.x >= 170 + largeur_supp / 2 && ev.motion.x <= 300 + largeur_supp / 2)
		{
			curseur = 0;
			for (int compteur = 0; compteur < 4; compteur++)
			{
				if (ev.motion.y >= 160 + 60 * compteur && ev.motion.y <= 200 + 60 * compteur)
				{curseur = rouge + compteur; SDL_SetCursor(curseur_txt);}
			}
			if (!curseur)
			{SDL_SetCursor(curseur_normal);}
		}
		else if (ev.motion.x >= 340 + largeur_supp / 2 && ev.motion.y >= 270 && ev.motion.x <= 480 + largeur_supp / 2 && ev.motion.y <= 310)
		{curseur = bouton_color_picker;}
		else
		{curseur = 0; SDL_SetCursor(curseur_normal);}
		break;
	
	case SDL_KEYDOWN:
		switch (ev.key.keysym.sym)
		{
		case SDLK_ESCAPE:
			focus = 0;
			curseur = 0;
			keymod = 0;
			return 1;
			break;
		
		case SDLK_TAB:
			if (focus >= rouge && focus <= alpha)
			{focus = bouton_color_picker;}
			else if (focus == bouton_color_picker)
			{focus = annuler;}
			else
			{focus = rouge;}
			break;
		
		case SDLK_UP:
			if (focus >= vert && focus <= alpha)
			{focus--;}
			else if (focus >= annuler && focus <= termine)
			{focus = bouton_color_picker;}
			else if (focus == rouge)
			{focus = alpha;}
			else if (!focus)
			{focus++;}
			break;
		
		case SDLK_DOWN:
			if (focus >= rouge && focus < alpha)
			{focus++;}
			else if (focus == alpha)
			{focus = rouge;}
			else if (focus == bouton_color_picker)
			{focus = appliquer;}
			else if (!focus)
			{focus++;}
			break;
		
		case SDLK_LEFT:
			if (focus == bouton_color_picker)
			{focus = rouge;}
			else if (focus == annuler)
			{focus = termine;}
			else if (!focus)
			{focus++;}
			else if (focus > annuler)
			{focus--;}
			break;
		
		case SDLK_RIGHT:
			if (focus >= rouge && focus <= alpha)
			{focus = bouton_color_picker;}
			else if (!focus)
			{focus = rouge;}
			else if (focus == termine)
			{focus = annuler;}
			else if (focus != bouton_color_picker)
			{focus++;}
			break;
		
		case SDLK_LSHIFT:
		case SDLK_RSHIFT:
			keymod = 1;
			break;
		
		case SDLK_RETURN:
		case SDLK_KP_ENTER:
			switch (focus)
			{
			case rouge:
			case vert:
			case bleu:
			case alpha:
				focus = 0;
				break;
			
			case bouton_color_picker:
				if (keymod)
				{
					demander_txt("Réglages du color picker", "Entrer ici la commande à exécuter pour ouvrir le color picker de votre choix.\n\nLe color picker est un programme externe et la commande préinscrite \
(\"zenity --color-selection\") pourrait ne pas fonctionner, voire faire crasher le programme.\nZenity est un ensemble d'outils disponible sur les distributions Linux de type Debian/Ubuntu. Vous pouvez l'installer en entrant \
la commande \"sudo apt install zenity\" dans votre terminal*.\n", color_picker, sizeof(color_picker), fenetre_reglages);
					keymod = 0;
				}
				else
				{
					cp_return = popen(color_picker, "r"); //popen agit comme system, mais il nous permet d'intercepter ce que la commande imprime dans le terminal
					fgets(cp_value, 49, cp_return); //copie de cette string ("rgb(R,G,B)") normalement affichée dans le terminal
					pclose(cp_return); //fermeture du "pipe" créé par popen
					if (extraction_rgb)
					{extraire_rgba(cp_value, r, g, b, a);}
				}
				break;
			
			case annuler:
				sprintf(r, "%d", couleur->r);
				sprintf(g, "%d", couleur->g);
				sprintf(b, "%d", couleur->b);
				sprintf(a, "%d", couleur->a);
				break;
			
			case appliquer:
			case termine:
				sscanf(r, "%hhd", &couleur->r);
				sscanf(g, "%hhd", &couleur->g);
				sscanf(b, "%hhd", &couleur->b);
				sscanf(a, "%hhd", &couleur->a);
				
				if (couleur == &couleur_score) //Les textures des nombres affichées dans la grille étant loadées à l'avance, il faut faire ça ici aussi...
				{
					for (int compteur = 0; compteur < 8; compteur++)
					{
						if (texture_nbre[compteur] != NULL)
						{SDL_DestroyTexture(texture_nbre[compteur]);}
						else if (!erreur && (compteur > 0 || afficher_zeros))
						{erreur = -14; printf("Erreur 14: La texture du chiffre %d n'a pas pu être chargée (au cas où vous ne l'auriez pas remarqué...).\n", compteur);}
					}
					for (int compteur = 1 - afficher_zeros; compteur <= 8; compteur++)
					{
						sprintf(nbre_a_afficher, "%d", compteur);
						surface_nbre = TTF_RenderUTF8_Solid_Wrapped(police, nbre_a_afficher, couleur_score, taille);
						taille_nbre[compteur].w = surface_nbre->w;
						taille_nbre[compteur].h = surface_nbre->h;
						texture_nbre[compteur] = SDL_CreateTextureFromSurface(rend, surface_nbre);
						SDL_FreeSurface(surface_nbre);
					}
				}
				
				if (focus == termine)
				{focus = 0; curseur = 0; keymod = 0; return 1;}
				break;
			}
			break;
		
		case SDLK_BACKSPACE:
			switch (focus)
			{
			case rouge:
				if (r[1] == '\000')
				{r[0] = '0';}
				else
				{tronquer(r);}
				break;
			
			case vert:
				if (g[1] == '\000')
				{g[0] = '0';}
				else
				{tronquer(g);}
				break;
			
			case bleu:
				if (b[1] == '\000')
				{b[0] = '0';}
				else
				{tronquer(b);}
				break;
			
			case alpha:
				if (a[1] == '\000')
				{a[0] = '0';}
				else
				{tronquer(a);}
				break;
			}
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
			switch (focus)
			{
			case rouge:
				if (!strcmp(r, "0")) //enlève le zéro unique si nécessaire
				{r[0] = '\000';}
				if (r[0] != '\000' && r[0] > '2' && r[1] != '\000') {} //bloque les nombres de 300 et plus
				else if (r[1] != '\000' && r[0] == '2' && (r[1] > '5' || ((ev.key.keysym.sym > SDLK_5 && ev.key.keysym.sym <= SDLK_9) || (ev.key.keysym.sym > SDLK_KP_5 && ev.key.keysym.sym <= SDLK_KP_9)))) {}
				else if (strlen(r) < 3) //ajoute le chiffre à la valeur                                        \-> bloque les nombres de 256 à 259 et de 260 à 299
				{
					if (ev.key.keysym.sym < SDLK_KP_1 || ev.key.keysym.sym > SDLK_KP_0)
					{r[strlen(r)] = ev.key.keysym.sym - SDLK_0 + '0';}
					else if (ev.key.keysym.sym == SDLK_KP_0) //keypad
					{r[strlen(r)] = '0';}
					else //keypad
					{r[strlen(r)] = ev.key.keysym.sym - SDLK_KP_1 + 1 + '0';}
				}
				break;
			
			case vert:
				if (!strcmp(g, "0")) //enlève le zéro unique si nécessaire
				{g[0] = '\000';}
				if (g[0] != '\000' && g[0] > '2' && g[1] != '\000') {} //bloque les nombres de 300 et plus
				else if (g[1] != '\000' && g[0] == '2' && (g[1] > '5' || ((ev.key.keysym.sym > SDLK_5 && ev.key.keysym.sym <= SDLK_9) || (ev.key.keysym.sym > SDLK_KP_5 && ev.key.keysym.sym <= SDLK_KP_9)))) {}
				else if (strlen(g) < 3) //ajoute le chiffre à la valeur                                        \-> bloque les nombres de 256 à 259 et de 260 à 299
				{
					if (ev.key.keysym.sym < SDLK_KP_1 || ev.key.keysym.sym > SDLK_KP_0)
					{g[strlen(g)] = ev.key.keysym.sym - SDLK_0 + '0';}
					else if (ev.key.keysym.sym == SDLK_KP_0) //keypad
					{g[strlen(g)] = '0';}
					else //keypad
					{g[strlen(g)] = ev.key.keysym.sym - SDLK_KP_1 + 1 + '0';}
				}
				break;
			
			case bleu:
				if (!strcmp(b, "0")) //enlève le zéro unique si nécessaire
				{b[0] = '\000';}
				if (b[0] != '\000' && b[0] > '2' && b[1] != '\000') {} //bloque les nombres de 300 et plus
				else if (b[1] != '\000' && b[0] == '2' && (b[1] > '5' || ((ev.key.keysym.sym > SDLK_5 && ev.key.keysym.sym <= SDLK_9) || (ev.key.keysym.sym > SDLK_KP_5 && ev.key.keysym.sym <= SDLK_KP_9)))) {}
				else if (strlen(b) < 3) //ajoute le chiffre à la valeur                                        \-> bloque les nombres de 256 à 259 et de 260 à 299
				{
					if (ev.key.keysym.sym < SDLK_KP_1 || ev.key.keysym.sym > SDLK_KP_0)
					{b[strlen(b)] = ev.key.keysym.sym - SDLK_0 + '0';}
					else if (ev.key.keysym.sym == SDLK_KP_0) //keypad
					{b[strlen(b)] = '0';}
					else //keypad
					{b[strlen(b)] = ev.key.keysym.sym - SDLK_KP_1 + 1 + '0';}
				}
				break;
			
			case alpha:
				if (!strcmp(a, "0")) //enlève le zéro unique si nécessaire
				{a[0] = '\000';}
				if (a[0] != '\000' && a[0] > '2' && a[1] != '\000') {} //bloque les nombres de 300 et plus
				else if (a[1] != '\000' && a[0] == '2' && (a[1] > '5' || ((ev.key.keysym.sym > SDLK_5 && ev.key.keysym.sym <= SDLK_9) || (ev.key.keysym.sym > SDLK_KP_5 && ev.key.keysym.sym <= SDLK_KP_9)))) {}
				else if (strlen(a) < 3) //ajoute le chiffre à la valeur                                        \-> bloque les nombres de 256 à 259 et de 260 à 299
				{
					if (ev.key.keysym.sym < SDLK_KP_1 || ev.key.keysym.sym > SDLK_KP_0)
					{a[strlen(a)] = ev.key.keysym.sym - SDLK_0 + '0';}
					else if (ev.key.keysym.sym == SDLK_KP_0) //keypad
					{a[strlen(a)] = '0';}
					else //keypad
					{a[strlen(a)] = ev.key.keysym.sym - SDLK_KP_1 + 1 + '0';}
				}
				break;
			}
			break;
		}
		break;
	
	case SDL_KEYUP:
		if (ev.key.keysym.sym == SDLK_LSHIFT || ev.key.keysym.sym == SDLK_RSHIFT)
		{keymod = 0;}
		break;
	
	case SDL_MOUSEBUTTONDOWN:
		if (curseur >= rouge && curseur <= alpha)
		{focus = curseur;}
		else if (curseur == bouton_color_picker)
		{
			if (ev.button.button == SDL_BUTTON_RIGHT)
			{
				demander_txt("Réglages du color picker", "Entrer ici la commande à exécuter pour ouvrir le color picker de votre choix.\n\nLe color picker est un programme externe et la commande préinscrite \
(\"zenity --color-selection\") pourrait ne pas fonctionner, voire faire crasher le programme.\nZenity est un ensemble d'outils disponible sur les distributions Linux de type Debian/Ubuntu. Vous pouvez l'installer en entrant \
la commande \"sudo apt install zenity\" dans votre terminal.", color_picker, sizeof(color_picker), fenetre_reglages);
			}
			else
			{
				SDL_WaitEvent(&ev); //prend l'event "MOUSEBUTTONUP", car sinon, le color picker ne reçoit rien de la souris, vu qu'il pense que le bouton n'a jamais été lâché...
				cp_return = popen(color_picker, "r"); //popen agit comme system, mais il nous permet d'intercepter ce que la commande imprime dans le terminal
				fgets(cp_value, 49, cp_return); //copie de cette string ("rgb(R,G,B)") normalement affichée dans le terminal
				pclose(cp_return); //fermeture du "pipe" créé par popen
				if (extraction_rgb)
				{extraire_rgba(cp_value, r, g, b, a);}
			}
		}
		else if (curseur == annuler)
		{
			sprintf(r, "%d", couleur->r);
			sprintf(g, "%d", couleur->g);
			sprintf(b, "%d", couleur->b);
			sprintf(a, "%d", couleur->a);
		}
		else if (curseur == appliquer || curseur == termine)
		{
			sscanf(r, "%hhd", &couleur->r);
			sscanf(g, "%hhd", &couleur->g);
			sscanf(b, "%hhd", &couleur->b);
			sscanf(a, "%hhd", &couleur->a);
			
			if (couleur == &couleur_score) //Les textures des nombres affichées dans la grille étant loadées à l'avance, il faut faire ça ici aussi...
			{
				for (int compteur = 0; compteur < 8; compteur++)
				{
					if (texture_nbre[compteur] != NULL)
					{SDL_DestroyTexture(texture_nbre[compteur]);}
					else if (!erreur && (compteur > 0 || afficher_zeros))
					{erreur = -14; printf("Erreur 14: La texture du chiffre %d n'a pas pu être chargée (au cas où vous ne l'auriez pas remarqué...).\n", compteur);}
				}
				for (int compteur = 1 - afficher_zeros; compteur <= 8; compteur++)
				{
					sprintf(nbre_a_afficher, "%d", compteur);
					surface_nbre = TTF_RenderUTF8_Solid_Wrapped(police, nbre_a_afficher, couleur_score, taille);
					taille_nbre[compteur].w = surface_nbre->w;
					taille_nbre[compteur].h = surface_nbre->h;
					texture_nbre[compteur] = SDL_CreateTextureFromSurface(rend, surface_nbre);
					SDL_FreeSurface(surface_nbre);
				}
			}
			
			if (curseur == termine)
			{focus = 0; curseur = 0; keymod = 0; return 1;}
		}
		break;
	}
	
	return 0;
}