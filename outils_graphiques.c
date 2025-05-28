#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <math.h>


//Macro permettant de changer la couleur des formes dessinées avec SDL de manière plus simple et intelligente:
#define SDL_SetColor(couleur, renderer) 	SDL_SetRenderDrawColor(renderer, couleur.r, couleur.g, couleur.b, couleur.a)

//Macro permettant de dessiner un rectangle plein aux coins arrondis:
#define rect_arrondi(x, y, largeur, hauteur, couleur, fond, renderer)	rectangle(x + 1, y + 1, largeur, hauteur, 0, couleur, fond, renderer); rectangle(x+2, y+2, largeur-2, hauteur-2, 5, couleur, fond, renderer)

//Macros permettant de trouver la longueur d'une ligne de texte:
#define longueur_txt(txt, longueur_max, police)					afficher_txt(txt, 0, 0, longueur_max, police, transparent, NULL)
#define longueur_txt_centre(txt, x_gauche, x_droite, police)	afficher_txt_centre(txt, x_gauche, x_droite, 0, police, transparent, NULL)


//Liste des fonctions de ce fichier:
int afficher_txt(char[], int, int, int, TTF_Font*, SDL_Color, SDL_Renderer*);
int afficher_txt_centre(char[], int, int, int, TTF_Font*, SDL_Color, SDL_Renderer*);
void extraire_rgba(char[], char[], char[], char[], char[]);
void rectangle(int, int, int, int, int, SDL_Color, SDL_Color, SDL_Renderer*);
void tronquer(char[]);


//Palette de couleurs:
const SDL_Color transparent = {0, 0, 0, 0};
const SDL_Color noir = {0, 0, 0, 255};
const SDL_Color blanc = {255, 255, 255, 255};
const SDL_Color gris = {167, 170, 170, 255};
const SDL_Color gris_fonce = {108, 112, 116, 255};
const SDL_Color gris_pale = {209, 209, 209, 255};
const SDL_Color bleu = {36, 128, 206, 255};
const SDL_Color bleu_efface = {121, 187, 243, 100};
const SDL_Color vert_pale = {172, 212, 157, 255};
const SDL_Color jaune_pale = {252, 255, 145, 255};
const SDL_Color jaune_orange = {255, 229, 0, 255};
const SDL_Color orange = {255, 176, 21, 255};
const SDL_Color orange_fonce = {255, 148, 25, 255};
const SDL_Color rouge = {255, 95, 51, 255};
const SDL_Color rouge_symboles = {143, 23, 23, 255};
const SDL_Color rouge_fonce = {172, 26, 26, 255};
const SDL_Color rouge_tres_fonce = {80, 11, 11, 255};
//Ces couleurs ne devraient jamais être utilisées directement (passer plutôt par les différentes couleurs d'éléments définies dans "minesweeper.c").

//Autres variables globales liées à SDL:
SDL_Window* fenetre; //la fenêtre de l'application
SDL_Renderer* rend; //le renderer utilisé par l'application
TTF_Font* police; //la police utilisée par l'application
TTF_Font* petite_police; //la même police, mais de plus petite taille
SDL_Cursor* curseur_normal; //un curseur pointeur bien ordinaire...
SDL_Cursor* curseur_txt; //un curseur en forme de "I" pour éditer du texte
SDL_Cursor* curseur_clic; //un curseur en forme de main pour cliquer sur un lien (ou autre chose...)
int xmax = 1000; //largeur de la fenêtre
int ymax = 700; //hauteur de la fenêtre


void tronquer (char txt[])
//Enlève le dernier caractère d'une string (reçue en paramètre et modifiée directement à la source vu qu'une string est un array qui est en fait un pointeur...).
//Ce n'est pas vraiment un outil _graphique_, mais bon...
{
	int compteur;
	
	for (compteur = 0; txt[compteur] != '\000'; compteur++) {}
	txt[compteur - 1] = '\000';
}


void extraire_rgba (char src[], char r[], char g[], char b[], char a[])
//Extrait les valeurs r, g, b et a du texte reçu du color picker de zenity (src).
//Les valeurs sont storées directements dans les paramètres r, g, b et a de la fonction.
//Le paramètre scr doit donc être de format "rgb(R,G,B)" ou "rgba(R,G,B,A)", où R, G et B sont des nbres entiers positifs <= 255.
//Le alpha peut être fourni ou pas par src, mais son paramètre a devrait toujours être fourni au cas où.
//Le programme détectera si un alpha est intégré à src (indépendemment du "rgb" ou "rgba" du début).
//S'il y en a un, il doit être au format d'un chiffre à virgule entre 0 et 1, qui sera transformé en chiffre entier entre 0 et 255.
//C'est assez douteux de qualifier cette fonction d'outil _graphique_, mais bon...
{
	char buffer[20];
	double buffer_float = 0.0;
	
	//rgb ou rgba:
	if (strtok(src, "(,) \n") == NULL)
	{return;} //sert à éviter les segfault si le color picker n'a rien envoyé (annulé) ou si le format n'est pas le bon
	
	//rgb:
	strcpy(r, strtok(NULL, "(,) \n"));
	strcpy(g, strtok(NULL, "(,) \n"));
	strcpy(b, strtok(NULL, "(,) \n"));
	
	//a:
	sprintf(buffer, "%s", strtok(NULL, "(,) \n"));
	if (!strcmp(buffer, "(null)"))
	{return;}
	else
	{
		sscanf(buffer, "%lf", &buffer_float);
		sprintf(a, "%d", (int) rint(buffer_float * 255));
		//printf("%s -> %lf -> %lf -> %lf -> %d\n\n", buffer, buffer_float, buffer_float * 255, rint(buffer_float * 255), (int) rint(buffer_float * 255)); //débogage du alpha
	}
}


int afficher_txt (char txt[], int x, int y, int longueur_max, TTF_Font* police, SDL_Color couleur, SDL_Renderer* renderer)
//Affiche du texte dans une fenêtre.
//Renvoie la longueur du texte affiché.
/* Paramètres:	- txt = texte à afficher
				- x, y = coordonnées du coin supérieur gauche du texte
				- longueur_max = longueur maximale du texte (changera de ligne si plus long)
				- police = la police à utiliser
				- couleur = la couleur du texte
				- renderer = le renderer où s'affichera le texte ou NULL si on ne veut pas l'afficher */
{
	SDL_Surface* surface = TTF_RenderUTF8_Blended_Wrapped(police, txt, couleur, longueur_max); //Utiliser "blended" plutôt que "Solid" rend le txt bcp plus beau!!! (probablement à cause que je render avec du alpha blending...)
	SDL_Rect rect = {x, y, surface->w, surface->h};
	
	if (renderer != NULL)
	{
		SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_RenderCopy(renderer, texture, NULL, &rect);
		
		SDL_DestroyTexture(texture);
	}
	
	SDL_FreeSurface(surface);
	return rect.w;
}


int afficher_txt_centre (char txt[], int x_gauche, int x_droite, int y, TTF_Font* police, SDL_Color couleur, SDL_Renderer* renderer)
//Affiche du texte à l'écran en le centrant entre deux valeurs de x.
//Renvoie la longueur du texte affiché.
//ATTENTION: Le centrage ne fonctionne pas si le texte est sur plus d'une ligne!
/* Paramètres: 	- txt = texte à afficher
				- x_gauche, x_droite = limites gauche et droite de l'endroit où sera affiché le texte
				- y = hauteur du texte (le haut du texte)
				- police = la police à utiliser
				- couleur = la couleur du texte
				- renderer = le renderer à utiliser ou NULL si on ne veut pas afficher le texte */
{
	SDL_Surface* surface = TTF_RenderUTF8_Blended_Wrapped(police, txt, transparent, x_droite - x_gauche);
	int longueur = surface->w;
	
	afficher_txt(txt, x_gauche + (x_droite - x_gauche - longueur) / 2, y, x_droite - x_gauche, police, couleur, renderer);
	
	SDL_FreeSurface(surface);
	return longueur;
}


void rectangle (int x, int y, int largeur, int hauteur, int epaisseur, SDL_Color couleur, SDL_Color fond, SDL_Renderer* renderer)
//Affiche un rectangle à l'écran, selon les paramètres spécifiés:
/* Paramètres:	- x, y = coordonnées du coin supérieur gauche du rectangle à dessiner
				- largeur = largeur du rectangle
				- hauteur = hauteur du rectangle
				- epaisseur = épaisseur du contour du rectangle (de 1 à 5 pixels) (0 = rectangle plein)
				- couleur = couleur du rectangle
				- fond = couleur du fond ("background")
				- renderer = renderer à utiliser */
{
	SDL_Rect rect = {x, y, largeur, hauteur};
	
	SDL_SetColor(couleur, renderer);
	
	if (!epaisseur) //plein
	{SDL_RenderFillRect(renderer, &rect);}
		
	else //vide
	{
		SDL_RenderDrawRect(renderer, &rect); //rectangle aux coordonnées spécifiées
		if (epaisseur >= 2) //vers l'intérieur
		{
			rect.x++; rect.y++; rect.w -= 2; rect.h -= 2;
			SDL_RenderDrawRect(renderer, &rect);
		}
		if (epaisseur >= 3) //vers l'extérieur
		{
			rect.x -= 2; rect.y -= 2; rect.w += 4; rect.h += 4;
			SDL_RenderDrawRect(renderer, &rect);
			if (epaisseur <= 4)
			{
				SDL_SetColor(fond, renderer);
				SDL_RenderDrawPoint(renderer, x - 1, y - 1);
				SDL_RenderDrawPoint(renderer, x - 1, y + hauteur);
				SDL_RenderDrawPoint(renderer, x + largeur, y - 1);
				SDL_RenderDrawPoint(renderer, x + largeur, y + hauteur);
				SDL_SetColor(couleur, renderer);
			}
		}
		if (epaisseur >= 4) //vers l'intérieur x2
		{
			rect.x = x + 2; rect.y = y + 2; rect.w = largeur - 4; rect.h = hauteur - 4;
			SDL_RenderDrawRect(renderer, &rect);
		}
		if (epaisseur >= 5) //vers l'extérieur x2
		{
			rect.x -= 4; rect.y -= 4; rect.w += 8; rect.h += 8;
			SDL_RenderDrawRect(renderer, &rect);
			
			SDL_RenderDrawPoint(renderer, x + 3, y + 3);
			SDL_RenderDrawPoint(renderer, x + largeur - 4, y + 3);
			SDL_RenderDrawPoint(renderer, x + 3, y + hauteur - 4);
			SDL_RenderDrawPoint(renderer, x + largeur - 4, y + hauteur - 4);
			
			SDL_SetColor(fond, renderer);
			
			SDL_RenderDrawPoint(renderer, x - 2, y - 2);
			SDL_RenderDrawPoint(renderer, x - 2, y - 1);
			SDL_RenderDrawPoint(renderer, x - 1, y - 2);
			
			SDL_RenderDrawPoint(renderer, x - 2, y + hauteur);
			SDL_RenderDrawPoint(renderer, x - 2, y + hauteur + 1);
			SDL_RenderDrawPoint(renderer, x - 1, y + hauteur + 1);
			
			SDL_RenderDrawPoint(renderer, x + largeur, y - 2);
			SDL_RenderDrawPoint(renderer, x + largeur + 1, y - 2);
			SDL_RenderDrawPoint(renderer, x + largeur + 1, y - 1);
			
			SDL_RenderDrawPoint(renderer, x + largeur + 1, y + hauteur + 1);
			SDL_RenderDrawPoint(renderer, x + largeur, y + hauteur + 1);
			SDL_RenderDrawPoint(renderer, x + largeur + 1, y + hauteur);
		}
	}
}