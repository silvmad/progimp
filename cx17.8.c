#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define max_mots 65536
#define maximum 2048
#define long_sl 1024

typedef unsigned idx;
typedef char * str;
typedef enum {INT, STR} type;
typedef enum {False, True} bool;
typedef struct node {void * car; type tcar; struct node * cdr;} node, * list;
typedef struct {str mot; list refs;} ndex;

ndex mots[max_mots];
idx mot_libre = 0;
const str split_chars = " .:,!?;(){}[]|\n-_><'\"\\/=&%#*1234567890";
list stoplist = NULL;

void usage(str);
void indexe(str, idx);
int indice(str);
bool pareil(str, str);
void ajoute_mot(idx, str, idx);
void ajoute_ref(idx, idx);
void dump(idx);
bool member_stoplist(str);
void extraire_stoplist(char*);
int compare(const void *, const void *);
list cons(void *, type, list);
void putlist(list);
void extraire_args(int, const str*, char**);

int main(int argc, const str argv[])
{
  /*noms_fichiers : vecteur contenant le nom de fichier à indexer en 0 et le
    nom du fichier stoplist en 1.*/
  char* noms_fichiers[2] = {0, "stoplist.txt"}; 
  extraire_args(argc, argv, noms_fichiers);
  char ligne[maximum];
  FILE * flux = fopen(noms_fichiers[0], "r");
  if (!flux) usage("Echec de l'ouverture du fichier à indexer.");
  idx ref = 0;
  extraire_stoplist(noms_fichiers[1]);
  while (fgets(ligne, maximum, flux))
    {
      indexe(ligne, ++ref);
    }
  fclose(flux);
  qsort(mots, mot_libre, sizeof(ndex), compare);
  dump(mot_libre);
  return 0;
}

void usage(str message)
{
  fprintf(stderr, "%s\n", message);
  exit(1);
}

void indexe(str ligne, idx ref)
{
  str mot = strtok(strdup(ligne), split_chars);
  while (mot)
    {
      int x = indice(mot);
      if (!member_stoplist(mot))
	{
	  if (x < 0) ajoute_mot(mot_libre, mot, ref);
	  else if (ref != *(int*)mots[x].refs -> car) ajoute_ref(x, ref);
	}
      mot = strtok(NULL, split_chars);
    }
}

int indice(str mot)
{
  idx x;
  for (x = 0; mots[x].mot; x++)
    {
      if(pareil(mot, mots[x].mot)) return x;
    }
  return -1;
}

bool pareil(str x, str y)
{
  return strcasecmp(x,y) ? False : True;
}

void ajoute_mot(idx x, str mot, idx ref)
{
  mots[x].mot = mot;
  int *nref = malloc(sizeof(int));
  if(!nref) usage("ajoute_mot : manque de RAM");
  *nref = ref;
  mots[x].refs = cons(nref, INT, NULL);
  ++mot_libre;
}

void ajoute_ref(idx x, idx ref)
{
  int *nref = malloc(sizeof(int));
  if(!nref) usage("ajoute_ref : manque de RAM");
  *nref = ref;
  mots[x].refs = cons(nref, INT, mots[x].refs);
}

void dump(idx k)
{
  idx x;
  for (x = 0; x < k; ++x)
    {
      if (mots[x].mot)
	{
	  printf("%s :", mots[x].mot);
	  putlist(mots[x].refs);
	  printf("\n");
	}
    }
}

bool member_stoplist(str mot)
/* 
Fonction déterminant si un mot appartient à la stoplist.
Elle renvoie True si son arguement appartient au vecteur stoplist et False 
sinon 
*/
{
  /*Permet d'avoir new = stoplist après le premier new = new -> cdr.*/
  list new = cons(NULL, INT, stoplist);
  while(new -> cdr)
    {
      /*On passe à l'élément suivant avant le test pareil afin d'être 
      sûr que le dernier élément soit bien testé*/
      new = new -> cdr;
      if (pareil(mot, new -> car))
	return True;
    }
  return False;
}

 void extraire_stoplist(char *sl_file)
/*
Fonction qui extrait le contenu du fichier stoplist.txt et le transfère dans
le vecteur stoplist.
*/
{
  FILE * fichier = fopen(sl_file, "r");
  if (!fichier) usage("Echec de l'ouverture du fichier stoplist");
  char sas[1024];
  int lu = fscanf(fichier, "%s", sas);
  while (lu != EOF)
    {
      stoplist = cons(strdup(sas), STR, stoplist);
      lu = fscanf(fichier, "%s", sas);
    }
}

int compare(const void * a, const void * b)
{
  const ndex * E1 = a;
  const ndex * E2 = b;
  return strcasecmp(E1 -> mot, E2 -> mot);
}

void extraire_args(int argc, const str argv[], char ** noms_fichiers)
/*
Fonction qui analyse la ligne de commande et ajoute les noms du fichier à
indexer et le nom du fichier contenant la stoplist dans le tableau
noms_fichiers.
*/
{
  int c;
  char format[512];
  while ((c = getopt(argc, argv, ":s:")) != -1)
    {;
      switch(c)
	{
	case 's' : noms_fichiers[1] = malloc(sizeof(char)*512);
                   strncpy(noms_fichiers[1], optarg, 512); break;
	case ':' : sprintf(format, "L'option %c a besoin d'une valeur", optopt); 
                   usage(format); break;
	case '?' : sprintf(format, "Option %c inconnue.", optopt); usage(format); break;
	}
    }
  if (optind == argc - 1) 
    {
      noms_fichiers[0] = malloc(sizeof(char)*512); 
      strncpy(noms_fichiers[0], argv[optind], 512);
    }
  else usage("Nombre d'arguments incorrect.");
}
  

  
