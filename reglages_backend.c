#include "outils_graphiques.c"


#define VERSION "0.3" //version du programme
#define OS "Linux" //OS pour lequel le programme est compilé


//Liste des fonctions de ce fichier:
void cree_fconfig(); //enregistre les réglages dans le fichier de sauvegarde de ceux-ci
_Bool demander_txt(char titre[], char explications[], char input[], int max, SDL_Window* fenetre_source); //permet de demander du texte à l'utilisateur (pas vraiment un réglage, mais bon...)
_Bool lire_fconfig(); //lit et applique les réglages enregistrés dans le fichier de sauvegarde des réglages


//Variables Globales:

//Fichier de sauvegarde des réglages:
char fconfig_nom[50] = "./source/reglages.txt";

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
char nom_police[45] = "./source/free_serif.ttf";
char nom_petite_police[45] = "./source/free_serif.ttf";

//Autres variables globales liées aux réglages:
const _Bool pop_up_systeme = 1; //indique à l'application si elle doit utiliser les pop-up du système ou créer les siens (Abandonné...)
_Bool extraction_rgb = 1; //indique si les valeurs rgb reçues du color picker doivent être extraits comme avec zenity (ne sert à rien pour l'instant)
_Bool confirmation_quitter = 0; //indique si le jeu doit toujours demander une confirmation avant de quitter une partie en cours
_Bool afficher_zeros = 0; //indique au programme qu'il doit afficher les zéros sur la grille
int largeur_fenetre[3] = {1000, 800, 800}; //largeur (en pixels) des 3 fenêtres du programme (dans l'ordre: principale, réglages, podium)
int hauteur_fenetre[3] = {700, 700, 700}; //longueur (en pixels) des 3 fenêtres du programme (dans l'ordre: principale, réglages, podium)
int taille_police_normale = 22;
int taille_petite_police = 19;
char color_picker[50] = "zenity --color-selection"; //commande à utiliser pour ouvrir le color picker

//Variables externes:
extern int erreur;
extern _Bool debogage;


//Fonctions:

_Bool lire_fconfig ()
//Lit et applique les réglages depuis le fichier où ils sont enregistrés.
//Renvoie 1 en cas de réussite et 0 en cas d'erreur.
{
	enum _type
	{
	 	//0 = ignoré / rien
	 	boolean = 1, //_Bool (0 ou 1)
	 	nbre, //int (nbre entier)
	 	virgule, //double (nbre à virgule)
	 	car, //char (caractère) (inutilisé présentement)
	 	txt, //char[] (texte sans formatage particulier)
	 	fichier, //char[] (nom et chemin d'accès d'un fichier)
	 	couleur, //SDL_Color (couleur sous forme d'une string de format "rgba(R, G, B, A)")
	 	dimensions //int + int (largeur et hauteur de quelque chose sous forme de string de format "L x H")
	};
	
	enum _etat
	{
		commentaire = 0, //texte ignoré
		nom = 1, //nom du réglage
		valeur = 2, //valeur du réglage
		autre = 3, // "[" ou "]"
		termine = 4, //le dernier paramètre a été lu (on a frappé la "[ FIN ]")
		erreur = 5
	};
	
	enum _type type = 0; //type du paramètre (indique quel ptr sera utilisé)
	enum _etat etat = 0; //"valeur" de ce qu'on va lire (est-ce que c'est le nom d'un paramètre, sa veleur, un commentaire, etc.)
	
	_Bool* ptr_bool = NULL; //ptr vers le _Bool du paramètre modifié
	int* ptr_int = NULL; //ptr vers l'int du paramètre modifié
	int* ptr_int2 = NULL; //ptr vers le 2e int modifié (pour les dimensions)
	char* ptr_str = NULL; //ptr vers la str du paramètre modifié
	SDL_Color* ptr_couleur = NULL; //ptr vers la struct SDL_Color modifiée
	
	int buffint = 0; //buffer pour la conversion du texte en _Bool
	int compteur = 0; //compteur utilisé pour la conversion du texte en SDL_Color
	char buffer[400] = "1"; //buffer contenant la ligne lue dans le fichier
	char* mot; //mot en cours d'analyse (délimité par strtok)
	char errstr[200] = "Erreur inconnue!"; //message d'erreur affiché
	FILE* fconfig = fopen(fconfig_nom, "r+"); //fichier de sauvegarde des réglages
	fpos_t pos;
	
	
	//Impossible d'ouvrir le fichier par défaut:
	if (fconfig == NULL)
	{
		if (police == NULL || petite_police == NULL)
		{SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Erreur", "Les polices par défaut n'ont pas pu être chargées par le programme et aucun fichier de sauvegarde des paramètres n'a été trouvé.\n\
La fenêtre suivante vous offrira la possibilité d'entrer (à l'aveuglette) le nom d'un fichier de sauvegarde des paramètres s'il y en a un.\nSinon, assurez-vous de placer un fichier de police TTF nommé \"free_serif.ttf\" \
dans le dossier \"source\" pour rendre ce programme utilisable.", NULL);
		strcpy(fconfig_nom, "");}
		
		if (!demander_txt("Fichier de sauvegarde des réglages", "Aucun fichier de sauvegarde des réglages n'a été trouvé par le programme.\n\nSi c'est la première fois que vous ouvrez ce programme, \
vous pouvez ignorer ce message en cliquant sur \"Annuler\".\n\nSinon, vous pouvez entrer ici le nom (et le chemin d'accès) du bon fichier de sauvegarde. Si aucun fichier valide n'est fourni, un fichier par défaut sera créé.", \
fconfig_nom, sizeof(fconfig_nom), fenetre))
		{cree_fconfig(); return 1;} //l'utilisateur a cliqué "Annuler"
		
		fconfig = fopen(fconfig_nom, "r+");
		if (fconfig == NULL)
		{cree_fconfig(); return 0;} //fichier invalide
	}
	
	//Parsing et enregistrement des réglages:
	while (etat != termine && etat != erreur)
	{
		fgetpos(fconfig, &pos);
		fgets(buffer, sizeof(buffer), fconfig); //prend une ligne complète (normalement)
		enleve_majuscule(buffer);
		mot = strtok(buffer, " ()|;:,\n"); //sépare le 1er "mot" de la ligne
		while (mot != NULL && etat != erreur && etat != termine) //analyse de la ligne mot à mot jusqu'à ce qu'on arrive à la fin de la ligne, qu'on ait terminé ou qu'il y ait une erreur
		{
			//Début d'un paramètre ("["):
			if (!strcmp(mot, "[") && etat == commentaire)
			{etat = nom;}
			
			//Fin d'un paramètre ("]"):
			else if (!strcmp(mot, "]") && etat != commentaire)
			{etat = commentaire;}
			
			//Nom du paramètre (1er mot entre crochet):
			else if (etat == nom)
			{
				compteur = 0;
				if (!strcmp(mot, "fin")) //indicateur de fin du document ("[ FIN ]")
				{etat = termine;}
				
				else if (!strcmp(mot, "version")) //indicateur de version
				{etat = autre; type = txt; /* À faire! */}
				
				else if (!strcmp(mot, "confirmation_quitter"))
				{etat = valeur; type = boolean; ptr_bool = &confirmation_quitter;}
				
				else if (!strcmp(mot, "couleur_fond"))
				{etat = valeur; type = couleur; ptr_couleur = &fond;}
				else if (!strcmp(mot, "couleur_texte_libre"))
				{etat = valeur; type = couleur; ptr_couleur = &couleur_timer;}
				else if (!strcmp(mot, "couleur_boutons"))
				{etat = valeur; type = couleur; ptr_couleur = &couleur_boutons;}
				else if (!strcmp(mot, "couleur_boutons_bloques"))
				{etat = valeur; type = couleur; ptr_couleur = &couleur_boutons_bloques;}
				else if (!strcmp(mot, "couleur_texte_boutons"))
				{etat = valeur; type = couleur; ptr_couleur = &couleur_txt_boutons;}
				else if (!strcmp(mot, "couleur_liens"))
				{etat = valeur; type = couleur; ptr_couleur = &couleur_liens;}
				else if (!strcmp(mot, "couleur_tuiles_vides"))
				{etat = valeur; type = couleur; ptr_couleur = &couleur_grille;}
				
				else if (!strcmp(mot, "couleur_tuiles_1"))
				{etat = valeur; type = couleur; ptr_couleur = &couleur_tuile[1];}
				else if (!strcmp(mot, "couleur_tuiles_2"))
				{etat = valeur; type = couleur; ptr_couleur = &couleur_tuile[2];}
				else if (!strcmp(mot, "couleur_tuiles_3"))
				{etat = valeur; type = couleur; ptr_couleur = &couleur_tuile[3];}
				else if (!strcmp(mot, "couleur_tuiles_4"))
				{etat = valeur; type = couleur; ptr_couleur = &couleur_tuile[4];}
				else if (!strcmp(mot, "couleur_tuiles_5"))
				{etat = valeur; type = couleur; ptr_couleur = &couleur_tuile[5];}
				else if (!strcmp(mot, "couleur_tuiles_6"))
				{etat = valeur; type = couleur; ptr_couleur = &couleur_tuile[6];}
				else if (!strcmp(mot, "couleur_tuiles_7"))
				{etat = valeur; type = couleur; ptr_couleur = &couleur_tuile[7];}
				else if (!strcmp(mot, "couleur_tuiles_8"))
				{etat = valeur; type = couleur; ptr_couleur = &couleur_tuile[8];}
				
				else if (!strcmp(mot, "couleur_selection_clavier"))
				{etat = valeur; type = couleur; ptr_couleur = &couleur_selection_clavier;}
				else if (!strcmp(mot, "couleur_tuiles_curseur"))
				{etat = valeur; type = couleur; ptr_couleur = &couleur_selection_curseur;}
				
				else if (!strcmp(mot, "symbole_podium"))
				{etat = valeur; type = fichier; strcpy(icone_podium, ""); ptr_str = &icone_podium[0]; buffint = sizeof(icone_podium); compteur = 0;}
				else if (!strcmp(mot, "symbole_pause"))
				{etat = valeur; type = fichier; strcpy(symbole_pause, ""); ptr_str = &symbole_pause[0]; buffint = sizeof(symbole_pause); compteur = 0;}
				else if (!strcmp(mot, "symbole_victoire"))
				{etat = valeur; type = fichier; strcpy(symbole_fin_de_partie, ""); ptr_str = &symbole_fin_de_partie[0]; buffint = sizeof(symbole_fin_de_partie); compteur = 0;}
				else if (!strcmp(mot, "symbole_drapeau"))
				{etat = valeur; type = fichier; strcpy(icone_drapeau, ""); ptr_str = &icone_drapeau[0]; buffint = sizeof(icone_drapeau); compteur = 0;}
				else if (!strcmp(mot, "symbole_bombe"))
				{etat = valeur; type = fichier; strcpy(image_bombe, ""); ptr_str = &image_bombe[0]; buffint = sizeof(image_bombe); compteur = 0;}
				
				else if (!strcmp(mot, "police") || !strcmp(mot, "police_normale"))
				{etat = valeur; type = fichier; strcpy(nom_police, ""); ptr_str = &nom_police[0]; buffint = sizeof(nom_police); compteur = 0;}
				else if (!strcmp(mot, "petite_police"))
				{etat = valeur; type = fichier; strcpy(nom_petite_police, ""); ptr_str = &nom_petite_police[0]; buffint = sizeof(nom_petite_police); compteur = 0;}
				else if (!strcmp(mot, "taille_police"))
				{etat = valeur; type = nbre; ptr_int = &taille_police_normale;}
				else if (!strcmp(mot, "taille_petite_police"))
				{etat = valeur; type = nbre; ptr_int = &taille_petite_police;}
				
				else if (!strcmp(mot, "color_picker"))
				{etat = valeur; type = txt; strcpy(color_picker, ""); ptr_str = &color_picker[0]; buffint = sizeof(color_picker); compteur = 0;}
				else if (!strcmp(mot, "color_picker_extraction_rgb") || !strcmp(mot, "color_picker_extraction_rgba"))
				{etat = valeur; type = boolean; ptr_bool = &extraction_rgb;}
				
				else if (!strcmp(mot, "afficher_zeros"))
				{etat = valeur; type = boolean; ptr_bool = &afficher_zeros;}
				else if (!strcmp(mot, "debogage"))
				{etat = valeur; type = boolean; ptr_bool = &debogage;}
				
				else if (!strcmp(mot, "taille_fenetre_principale"))
				{etat = valeur; type = dimensions; ptr_int = &largeur_fenetre[0]; ptr_int2 = &hauteur_fenetre[0]; compteur = 0;}
				else if (!strcmp(mot, "taille_fenetre_reglages"))
				{etat = valeur; type = dimensions; ptr_int = &largeur_fenetre[1]; ptr_int2 = &hauteur_fenetre[1]; compteur = 0;}
				else if (!strcmp(mot, "taille_fenetre_podium"))
				{etat = valeur; type = dimensions; ptr_int = &largeur_fenetre[2]; ptr_int2 = &hauteur_fenetre[2]; compteur = 0;}
			}
			
			//Valeur du paramètre:
			else if (etat == valeur)
			{
				switch (type)
				{
				case nbre:
					if (est_un_nbre(mot))
					{sscanf(mot, "%d", ptr_int); etat = autre;}
					else
					{etat = erreur; strcpy(errstr, "Ce paramètre est supposé être un nombre.");}
					break;
				
				case boolean:
					if (est_un_bool(mot))
					{
						sscanf(mot, "%d", &buffint);
						*ptr_bool = buffint;
						etat = autre;
					}
					else
					{etat = erreur; strcpy(errstr, "Ce paramètre est supposé être un boolean.");}
					break;
				
				case txt:
				case fichier:
					if (strlen(ptr_str) + strlen(mot) < buffint) //buffint == sizeof(ptr_str)
					{
						if (compteur != 0)
						{strcat(ptr_str, " ");}
						strcat(ptr_str, mot);
						compteur++;
					}
					else
					{etat = erreur; strcpy(errstr, "Le texte entré dépasse le nombre maximal de caractères pour ce paramètre.");}
					break;
				
				case couleur:
					switch (compteur)
					{
					case 0:
						if (strcmp(mot, "rgb") != 0 && strcmp(mot, "rgba") != 0)
						{etat = erreur; strcpy(errstr, "Ce paramètre est une couleur qui doit être fournie en format RGBA. (\"rgb\" ou \"rgba\" manquant)");}
						else
						{compteur++;}
						break;
					
					case 1:
						if (est_un_nbre(mot))
						{sscanf(mot, "%hhd", &ptr_couleur->r); compteur++;}
						else
						{etat = erreur; strcpy(errstr, "Ce paramètre est une couleur qui doit être fournie en format RGBA. (R doit être un nombre)");}
						break;
					
					case 2:
						if (est_un_nbre(mot))
						{sscanf(mot, "%hhd", &ptr_couleur->g); compteur++;}
						else
						{printf("\n\n\nERR: %s\n\n\n", mot); etat = erreur; strcpy(errstr, "Ce paramètre est une couleur qui doit être fournie en format RGBA. (G doit être un nombre)");}
						break;
					
					case 3:
						if (est_un_nbre(mot))
						{sscanf(mot, "%hhd", &ptr_couleur->b); compteur++;}
						else
						{etat = erreur; strcpy(errstr, "Ce paramètre est une couleur qui doit être fournie en format RGBA. (B doit être un nombre)");}
						break;
					
					case 4:
						if (est_un_nbre(mot))
						{sscanf(mot, "%hhd", &ptr_couleur->a); etat = autre; compteur = 0;}
						else
						{etat = erreur; strcpy(errstr, "Ce paramètre est une couleur qui doit être fournie en format RGBA. (A doit être un nombre)");}
						break;
					}
					break;
				
				case dimensions:
					switch (compteur)
					{
					case 0:
						if (est_un_nbre(mot))
						{sscanf(mot, "%d", ptr_int); compteur++;}
						else
						{etat = erreur; strcpy(errstr, "La première valeur de ce paramètre doit être un nombre.");}
						break;
					
					case 1:
						if (!strcmp(mot, "x"))
						{compteur++;}
						else
						{etat = erreur; strcpy(errstr, "Les 2 valeurs de ce paramètre doivent être séparées par un \"x\".");}
						break;
					
					case 2:
						if (est_un_nbre(mot))
						{sscanf(mot, "%d", ptr_int2); etat = autre;}
						else
						{etat = erreur; strcpy(errstr, "La deuxième valeur de ce paramètre doit être un nombre.");}
						break;
					}
					break;
				}
			}
			
			//Identification et séparation du prochain mot de la ligne
			mot = strtok(NULL, " ()|;:,\n"); //prend un nouveau "mot"
		}			//On ne PEUT PAS délimiter les mots avec les caractères suivants: "./\\\"[]"
	}
	
	if (etat == erreur)
	{
		fsetpos(fconfig, &pos);
		fgets(buffer, sizeof(buffer), fconfig);
		printf("Erreur: %s\nErreur à la ligne suivante: %sPlus précisément au mot suivant: \"%s\".\n", errstr, buffer, mot);
		fclose(fconfig);
		return 0;
	}
	
	fclose(fconfig);
	return 1;
}


void cree_fconfig ()
//Enregistre les réglages actuels en les écrivant dans un fichier txt.
{
	_Bool mod = 1; //indique si on modifie un fichier ou si on en cree un nouveau
	char mot1[200] = "";
	char mot2[200] = "";
	char mot3[200] = "";
	fpos_t pos1;
	fpos_t pos2;
	fpos_t pos3;
	FILE* fconfig = fopen(fconfig_nom, "r+");
	time_t date_heure = time(NULL);
	SDL_Color couleur_tuile_defaut[9] = {transparent, vert_pale, jaune_pale, jaune_orange, orange, orange_fonce, rouge, rouge_fonce, rouge_tres_fonce};
	
	//Vérifie si le fichier de config existe:
	if (fconfig == NULL)
	{mod = 0;}
	
	//Si ce fichier existe, on se rend à la fin de celui-ci
	if (mod)
	{		
		do
		{
			pos3 = pos2;
			pos2 = pos1;
			strcpy(mot3, mot2);
			strcpy(mot2, mot1);
			fgetpos(fconfig, &pos1);
			fscanf(fconfig, "%s", mot1);
			enleve_majuscule(mot1);
		} while (!(!strcmp(mot3, "[") && !strcmp(mot2, "fin") && !strcmp(mot1, "]")));
		fsetpos(fconfig, &pos3);
		fprintf(fconfig, "\n\nModifications du %s", ctime(&date_heure));
	}
	
	//Si le fichier de config n'existe pas, on en créé un:
	else //!mod
	{
		fconfig = fopen(fconfig_nom, "w+");
		if (fconfig == NULL)
		{SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Erreur", "Impossible de créer un fichier pour sauvegarder les réglages du jeu.\nCette application a-t-elle les privilèges nécessaires?", NULL); return;}
		
		fprintf(fconfig, "REGLAGES.TXT\nFichier de sauvegarde des réglages du jeu de Minesweeper.\n\n");
		fprintf(fconfig, "Seul le texte placé entre braquettes (exemple: [ Texte lu ] VS Texte ignoré) sera lu par le programme. Tout autre texte sera ignoré.\n");
		fprintf(fconfig, "Les braquettes doivent être séparées du texte par un espace et le texte à l'intérieur des braquettes doit être le nom d'un paramètre suivi de la valeur de celui-ci.\n");
		fprintf(fconfig, "Le marqueur \"[ FIN ]\" doit obligatoirement être placé après le dernier réglage.\nLa syntaxe du document doit être respectée pour que ce fichier puisse être utilisé par le programme.\n");
		fprintf(fconfig, "Ce fichier est généré automatiquement.\n\n");
		
		fprintf(fconfig, "Version du programme (Ne pas modifier!):\n[ Version %s ]\n\n\n", VERSION);
		fprintf(fconfig, "Réglages Généraux:\n");
	}
	
	if (!mod || confirmation_quitter != 0)
	{fprintf(fconfig, "[ Confirmation_Quitter %d ]\n\n", confirmation_quitter);}
	
	if (!mod)
	{fprintf(fconfig, "Réglages Graphiques (couleurs générales):\n");}
	if (!mod || cmp_couleurs(fond, blanc) != 0)
	{fprintf(fconfig, "[ Couleur_Fond rgba(%d, %d, %d, %d) ]\n", fond.r, fond.g, fond.b, fond.a);}
	if (!mod || cmp_couleurs(couleur_timer, noir) != 0)
	{fprintf(fconfig, "[ Couleur_Texte_Libre rgba(%d, %d, %d, %d) ]\n", couleur_timer.r, couleur_timer.g, couleur_timer.b, couleur_timer.a);}
	if (!mod || cmp_couleurs(couleur_boutons, gris_pale) != 0)
	{fprintf(fconfig, "[ Couleur_Boutons rgba(%d, %d, %d, %d) ]\n", couleur_boutons.r, couleur_boutons.g, couleur_boutons.b, couleur_boutons.a);}
	if (!mod || cmp_couleurs(couleur_boutons_bloques, gris_fonce) != 0)
	{fprintf(fconfig, "[ Couleur_Boutons_Bloques rgba(%d, %d, %d, %d) ]\n", couleur_boutons_bloques.r, couleur_boutons_bloques.g, couleur_boutons_bloques.b, couleur_boutons_bloques.a);}
	if (!mod || cmp_couleurs(couleur_txt_boutons, noir) != 0)
	{fprintf(fconfig, "[ Couleur_Texte_Boutons rgba(%d, %d, %d, %d) ]\n", couleur_txt_boutons.r, couleur_txt_boutons.g, couleur_txt_boutons.b, couleur_txt_boutons.a);}
	if (!mod || cmp_couleurs(couleur_liens, bleu) != 0)
	{fprintf(fconfig, "[ Couleur_Liens rgba(%d, %d, %d, %d) ]\n\n", couleur_liens.r, couleur_liens.g, couleur_liens.b, couleur_liens.a);}
	
	if (!mod)
	{fprintf(fconfig, "Réglages Graphiques (couleurs de la grille):\n");}
	if (!mod || cmp_couleurs(couleur_grille, gris) != 0)
	{fprintf(fconfig, "[ Couleur_Tuiles_Vides rgba(%d, %d, %d, %d) ]\n", couleur_grille.r, couleur_grille.g, couleur_grille.b, couleur_grille.a);}
	if (!mod)
	{fprintf(fconfig, "Couleurs des tuiles selon le nombre de bombes adjacentes:\n");}
	for (int compteur = 1; compteur < 9; compteur++)
	{
		if (!mod || cmp_couleurs(couleur_tuile[compteur], couleur_tuile_defaut[compteur]) != 0)
		{fprintf(fconfig, "[ Couleur_Tuiles_%d rgba(%d, %d, %d, %d) ]\n", compteur, couleur_tuile[compteur].r, couleur_tuile[compteur].g, couleur_tuile[compteur].b, couleur_tuile[compteur].a);}
	}
	
	if (!mod)
	{fprintf(fconfig, "\nRéglages Graphiques (couleur de la sélection):\n");}
	if (!mod || cmp_couleurs(couleur_selection_clavier, bleu) != 0)
	{fprintf(fconfig, "[ Couleur_Selection_Clavier rgba(%d, %d, %d, %d) ]\n", couleur_selection_clavier.r, couleur_selection_clavier.g, couleur_selection_clavier.b, couleur_selection_clavier.a);}
	if (!mod || cmp_couleurs(couleur_selection_curseur, bleu_efface) != 0)
	{fprintf(fconfig, "[ Couleur_Selection_Curseur rgba(%d, %d, %d, %d) ]\n\n", couleur_selection_curseur.r, couleur_selection_curseur.g, couleur_selection_curseur.b, couleur_selection_curseur.a);}
	
	if (!mod)
	{fprintf(fconfig, "Réglages Graphiques (icones et symboles):\n");}
	if (!mod || strcmp(icone_podium, "./source/icone_podium.png") != 0)
	{fprintf(fconfig, "[ Symbole_Podium %s ]\n", icone_podium);}
	if (!mod || strcmp(symbole_pause, "./source/symbole_pause.png") != 0)
	{fprintf(fconfig, "[ Symbole_Pause %s ]\n", symbole_pause);}
	if (!mod || strcmp(symbole_fin_de_partie, "./source/symbole_fin_de_partie.png") != 0)
	{fprintf(fconfig, "[ Symbole_Victoire %s ]\n", symbole_fin_de_partie);}
	if (!mod || strcmp(image_bombe, "./source/symbole_bombe.png") != 0)
	{fprintf(fconfig, "[ Symbole_Bombe %s ]\n", image_bombe);}
	if (!mod || strcmp(icone_drapeau, "./source/icone_drapeau.png") != 0)
	{fprintf(fconfig, "[ Symbole_Drapeau %s ]\n\n", icone_drapeau);}
	
	if (!mod)
	{fprintf(fconfig, "Réglages Graphiques (polices):\n");}
	if (!mod || strcmp(nom_police, "./source/free_serif.ttf") != 0)
	{fprintf(fconfig, "[ Police %s ]\n", nom_police);}
	if (!mod || strcmp(nom_petite_police, "./source/free_serif.ttf") != 0)
	{fprintf(fconfig, "[ Petite_Police %s ]\n", nom_petite_police);}
	if (!mod || taille_police_normale != 22)
	{fprintf(fconfig, "[ Taille_Police %d ]\n", taille_police_normale);}
	if (!mod || taille_petite_police != 19)
	{fprintf(fconfig, "[ Taille_Petite_Police %d ]\n\n", taille_petite_police);}
	
	if (!mod)
	{fprintf(fconfig, "Réglages Graphiques (autres réglages):\n");}
	if (!mod || strcmp(color_picker, "zenity --color-selection") != 0)
	{fprintf(fconfig, "[ Color_Picker %s ]\n", color_picker);}
	if (!mod || extraction_rgb != 1)
	{fprintf(fconfig, "[ Color_Picker_Extraction_RGB %d ] Ce réglage (modifiable seulement ici) contrôle le déformatage de l'output du color picker. Désactiver si le color picker fait crasher le programme.\n\n", extraction_rgb);}
	
	if (!mod)
	{fprintf(fconfig, "Réglages du jeu:\n");}
	if (!mod || afficher_zeros != 0)
	{fprintf(fconfig, "[ Afficher_Zéros %d ]\n", afficher_zeros);}
	if (!mod || debogage != 0)
	{fprintf(fconfig, "[ Debogage %d ] Activable depuis le command-line intégré du jeu (appuyez sur \"/\" pendant une partie!), permet de savoir où se trouve chaque bombe.\n\n", debogage);}
	
	if (!mod)
	{fprintf(fconfig, "Autres Réglages:\n");}
	if (!mod || largeur_fenetre[0] != 1000 && hauteur_fenetre[0] != 700)
	{fprintf(fconfig, "[ Taille_Fenetre_Principale %d x %d ]\n", largeur_fenetre[0], hauteur_fenetre[0]);}
	if (!mod || largeur_fenetre[1] != 800 && hauteur_fenetre[1] != 700)
	{fprintf(fconfig, "[ Taille_Fenetre_Reglages %d x %d ]\n", largeur_fenetre[1], hauteur_fenetre[1]);}
	if (!mod || largeur_fenetre[2] != 800 && hauteur_fenetre[2] != 700)
	{fprintf(fconfig, "[ Taille_Fenetre_Podium %d x %d ]\n", largeur_fenetre[2], hauteur_fenetre[2]);}
	
	fprintf(fconfig, "\n[ FIN ]\n\n");
	
	fprintf(fconfig, "Dernière Modification: %s", ctime(&date_heure));
	
	fclose(fconfig);
}


_Bool demander_txt (char titre[], char explications[], char input[], int max, SDL_Window* fenetre_source)
//Créé une nouvelle fenêtre de style "pop-up" pour demander un input de texte à l'utilisateur.
//Renvoie 1 en cas de succès et 0 en cas d'erreur.
/* Paramètres:	- titre = titre de la fenêtre (maximum 200 caractères)
				- explications = texte expliquant à l'utilisateur ce qu'il doit écrire
				- input = string où sera enregistré l'input de l'utilisateur
				- max = taille maximale de la string input (devrait donc toujours être "sizeof(input)")
				- fenetre_source = ptr vers la structure SDL_Window de la fenêtre à partir de laquelle est appelée cette fonction */
{
	SDL_Window* fenetre_d;
	SDL_Renderer* rend_d;
	Uint32 ID_fenetre_d;
	SDL_Event ev;
	char titre_fenetre[230] = "Minesweeper - ";
	char ancien_input[max];
	char focus = 0; //0 = nulle part, 'i' = boîte d'input, 'a' = bouton "annuler", 't' = bouton "terminé". S'applique à la sélection clavier "seulement".
	unsigned termine = 2; //0 ou 1 = valeur à retourner, 2 = pas terminé
	
	//Recopie de strings:
	strcat(titre_fenetre, titre);
	if (input[0] == '\000')
	{ancien_input[0] = '\000';}
	else
	{strcpy(ancien_input, input);}
	
	//Création de la fenêtre:
	fenetre_d = SDL_CreateWindow(titre_fenetre, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 650, 400, 0); //fenêtre centrée et non resizeable
	if (fenetre_d == NULL)
	{printf("Erreur lors de la création de la fenêtre SDL pour demander du texte:\n%s\n", SDL_GetError()); erreur = -52; return 0;}
	
	//Création du renderer:
	rend_d = SDL_CreateRenderer(fenetre_d, -1, 0);
	if (rend_d == NULL)
	{printf("Erreur lors de la création du renderer SDL de la fenêtre pour demander du texte:\n%s\n", SDL_GetError()); SDL_DestroyWindow(fenetre_d); erreur = -53; return 0;}
	SDL_SetRenderDrawBlendMode(rend_d, SDL_BLENDMODE_BLEND); //permet l'utilisation de couleurs semi-transparentes (et transparentes)
	
	//Taille minimale et ID de la fenêtre:
	ID_fenetre_d = SDL_GetWindowID(fenetre_d);
	SDL_SetWindowResizable(fenetre_source, SDL_FALSE); //la fenêtre source n'a plus d'affaire à se faire resizer...
	SDL_RaiseWindow(fenetre_d);
	
	//Dessin de la fenêtre et gestion de l'input utilisateur:
	while (termine >= 2)
	{
		//Remplissage avec la couleur du fond:
		SDL_SetColor(fond, rend_d);
		SDL_RenderClear(rend_d);
		
		//Affichage du titre:
		TTF_SetFontStyle(police, TTF_STYLE_UNDERLINE);
		afficher_txt_centre(titre, 0, 650, 5, police, couleur_timer, rend_d);
		TTF_SetFontStyle(police, TTF_STYLE_NORMAL);
		
		//Affichage des instructions:
		afficher_txt(explications, 30, 50, 590, petite_police, couleur_timer, rend_d);
		
		//Affichage de la boîte d'input:
		rectangle(30, 260, 590, 40, 0, couleur_boutons, fond, rend_d);
		if (focus == 'i')
		{rectangle(30, 260, 590, 40, 0, couleur_selection_curseur, fond, rend_d);}
		rectangle(30, 260, 590, 40, 4, couleur_txt_boutons, fond, rend_d);
		if (input[0] != '\000')
		{afficher_txt(input, 40, 270, 570, police, couleur_txt_boutons, rend_d);}
		
		//Affichage des boutons:
		rect_arrondi(370, 340, 120, 40, couleur_boutons, fond, rend_d);
		if (focus == 'a')
		{rect_arrondi(370, 340, 120, 40, couleur_selection_clavier, fond, rend_d);}
		if (ev.motion.x >= 370 && ev.motion.x <= 490 && ev.motion.y >= 340 && ev.motion.y <= 380)
		{rect_arrondi(370, 340, 120, 40, couleur_selection_curseur, fond, rend_d);}
		afficher_txt_centre("Annuler", 370, 490, 350, police, couleur_txt_boutons, rend_d);
		
		rect_arrondi(510, 340, 120, 40, couleur_boutons, fond, rend_d);
		if (focus == 't')
		{rect_arrondi(510, 340, 120, 40, couleur_selection_clavier, fond, rend_d);}
		if (ev.motion.x >= 510 && ev.motion.x <= 630 && ev.motion.y >= 340 && ev.motion.y <= 380)
		{rect_arrondi(510, 340, 120, 40, couleur_selection_curseur, fond, rend_d);}
		afficher_txt_centre("Terminé", 510, 630, 350, police, couleur_txt_boutons, rend_d);
		
		//Rendering et gestion de l'input utilisateur:
		SDL_RenderPresent(rend_d);
		SDL_WaitEvent(&ev);
		
		switch (ev.type)
		{
		case SDL_WINDOWEVENT:
			if (ev.window.windowID != ID_fenetre_d) //si l'utilisateur joue avec l'autre fenêtre (lui donnant ainsi le focus, déclenchant cet event), on veut le ramener à la bonne place
			{SDL_RaiseWindow(fenetre_d); SDL_FlashWindow(fenetre_d, SDL_FLASH_UNTIL_FOCUSED);}
			else if (ev.window.event == SDL_WINDOWEVENT_CLOSE) //SDL_QUIT ne fonctionne pas avec plusieurs fenêtres ouvertes...
			{termine = 0;}
			break;
		
		case SDL_MOUSEMOTION:
			if (ev.motion.x >= 30 && ev.motion.x <= 620 && ev.motion.y >= 260 && ev.motion.y <= 300)
			{SDL_SetCursor(curseur_txt);}
			else
			{SDL_SetCursor(curseur_normal);}
			//Le hovering des 2 autres boutons est géré avec l'affichage.
			break;
		
		case SDL_KEYDOWN:
			switch (ev.key.keysym.sym)
			{
			case SDLK_ESCAPE:
				if (!focus)
				{termine = 0;}
				else
				{focus = 0;}
				break;
			
			case SDLK_TAB:
				if (focus == 'i')
				{focus = 'a';}
				else
				{focus = 'i';}
				break;
			
			case SDLK_RIGHT:
			case SDLK_LEFT:
				if (focus == 'a')
				{focus = 't';}
				else if (focus == 't')
				{focus = 'a';}
				break;
			
			case SDLK_RETURN:
			case SDLK_KP_ENTER:
				if (focus == 'i')
				{focus = 0;}
				else if (focus == 'a')
				{termine = 0;}
				else if (focus == 't')
				{termine = 1;}
				break;
			
			case SDLK_BACKSPACE:
				if (!focus)
				{focus = 'i';}
				if (focus == 'i' && strlen(input) > 0)
				{tronquer(input);}
				break;
			}
			break;
		
		case SDL_MOUSEBUTTONDOWN:
			if (ev.button.x >= 30 && ev.button.x <= 620 && ev.button.y >= 250 && ev.button.y <= 290)
			{focus = 'i';} //boîte d'input
			else if (ev.button.x >= 370 && ev.button.x <= 490 && ev.button.y >= 340 && ev.button.y <= 380)
			{termine = 0;}
			else if (ev.button.x >= 510 && ev.button.x <= 630 && ev.button.y >= 340 && ev.button.y <= 380)
			{termine = 1;}
			else
			{focus = 0;}
			break;
		
		case SDL_TEXTINPUT:
			focus = 'i';
			if (strlen(input) < max - 1)
			{strcat(input, ev.text.text);}
			break;
		}
	}
	
	//Destruction de la fenêtre et du renderer et retour aux réglages:
	SDL_DestroyRenderer(rend_d);
	SDL_DestroyWindow(fenetre_d);
	SDL_SetWindowResizable(fenetre_source, SDL_TRUE);
	
	//Remise de l'ancien input si nécessaire:
	if (!termine)
	{
		if (ancien_input[0] == '\000')
		{input[0] = '\000';}
		else
		{strcpy(input, ancien_input);}
	}
	
	return (_Bool) termine;
}