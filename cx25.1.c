/* 
Programme qui émule le fonctionnement de l'ordinateur en papier. Prend un 
fichier contenant un programme pour l'ordinateur en papier comme argument. 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef enum {False, True} bool;
char ram[256];
char A;
//PC ne doit pas être signé car il est utilisé comme index du vecteur ram.
unsigned char PC;
//Vecteur qui recueille les adresses dont l'utilisateur a demandé l'affichage
unsigned char add_display[256] = {0};
/*Variable qui conserve l'indice de la prochaine case libre dans le vecteur
add_display */
int next_add_dis = 0;
//Variable qui permet d'activer ou non le stepper.
bool stepper_on;

void usage(char *);
void scan_ldc(int, char**, char**);
void charger_hexcode(char *);
void run(void);
void executer(unsigned char *, unsigned char *);
char strtoc(char*);
char recup_entree(void);
void step(unsigned char, unsigned char);
void get_user_input(void);
void parse_input(char *);
void print_mem(void);
bool adresse(char *);
void print_instruction(unsigned char, char);

int main(int argc, char *argv[])
{
  char *fichier;
  scan_ldc(argc, argv, &fichier);
  charger_hexcode(fichier);
  run();
}

void usage(char *message)
/*
Fonction qui affiche un message d'erreur puis arrête le programme.
*/
{
  fprintf(stderr, "%s\n", message);
  exit(1);
}

void scan_ldc(int argc, char * argv[], char ** fichier)
/*
Fonction qui analyse la ligne de commande. Permet d'activer ou non le stepper
et fait pointer fichier vers le nom du fichier à exécuter.
*/
{
  int c;
  char format[512];
  while ((c = getopt(argc, argv, "s")) != -1)
    {
      switch (c)
	{
	case 's' : stepper_on = True;
	  puts("\nStepper activé, commandes disponibles :\n"
	       "memory : affiche l'intégralité de la mémoire\n"
	       "display X : affiche le contenu de l'adresse X à chaque pas."
	       "\nX : affiche le contenu de l'adresse X.\n"
	       "X Y : affiche le contenu de toutes les adresse de X à Y.\n"
	       "quit : arrête l'exécution\n"
	       "Les adresses doivent être données en hexadécimal\n");
	       break;
	case '?' : sprintf(format, "Option %c inconnue.", optopt);
	  usage(format); break;
	}
    }
  if (optind == argc - 1) //Le nombre d'arguments en correct.
    {
      *fichier = malloc(sizeof(char)*512);
      strncpy(*fichier, argv[optind], 511);
    }
  else usage("Usage : ./cx25.1 fichier\nL'option -s active le stepper");
}

void charger_hexcode(char *nom_fichier)
/*
Fonction qui lit un fichier, et charge le code qu'il contient dans le vecteur
ram.
Si un offset est précisé le code est chargé à partir de l'indice donné, 
sinon il est chargé à partir de l'indice 32.
Mis à part le premier mot qui peut être "offset", le code doit être composé 
uniquement de nombres hexadécimaux compris entre 00 et FF. Ils sont convertis 
en char avant d'être rangés dans le vecteur ram.
*/
{
  FILE *flux = fopen(nom_fichier, "r");
  //Si on a pas pu ouvrir le fichier, on arrête le programme.
  if (!flux)
    {
      char tampon[1024];
      snprintf(tampon, 1023, "Impossible d'ouvrir le fichier %s",
	       nom_fichier);
      usage(tampon);
    }
  char tampon[8];
  int i; //indice à partir duquel on charge
  /*On regarde s'il y a un offset, il suffit de scanner 7 caractères, en effet
  la chaine offset faisant 6 caractères, si l'on obtient une chaine de 7 
  caractères ou plus on sait qu'il y a une erreur. */
  fscanf(flux, "%7s", tampon); //scan de la chaine "offset" ou du 1er code
  if (!strcmp(tampon, "offset"))
    {
      /* De la même façon il suffit de scanner 3 caractères car comme l'on 
      s'attend à avoir des codes de deux caractères, si l'on en obtient 3 il y 
      a forcément une erreur. */
      fscanf(flux, "%3s", tampon); //scan de l'offset
      PC = strtoc(tampon);
      i = PC;
      fscanf(flux, "%3s", tampon); //scan du 1er code
    }
  else // S'il n'y a pas d'offset, on charge à partir de l'indice 32.
    {
      PC = 32;
      i = 32;
    }
  //Boucle qui permet de scanner l'intégralité des codes du fichier.
  do
    {
      ram[i++] = strtoc(tampon);
    }
  while (fscanf(flux, "%3s", tampon) != EOF);
}

void run(void)
{
/*
Fonction qui contient la boucle d'exécution de l'ordinateur en papier.
Elle lit un opcode et son argument, incrémente le compteur PC et exécute
ensuite l'opération correspondante.
*/
  unsigned char op, arg;
  /* op est un opcode, c'est un unsigned char pour faciliter la conversion 
     décimal => hexadécimal dans la fonction executer (évite les case 
     négatifs).
     arg est un argument, c'est un unsigned char car arg est utilisé comme 
     index de ram il ne peut donc être négatif. */
  while (1)
    {
      op = ram[PC];
      arg = ram[PC + 1];
      if (stepper_on)
	step(op, arg);
      PC += 2;
      executer(&op, &arg);
    }
}

 void executer(unsigned char *op,unsigned char *arg)
/*
Fonction qui exécute une instruction en fonction de l'opcode et de l'argument
fourni.
op étant un unsigned char, les opcodes ont été traduits en décimal dans les 
case.
*/
{
  char c;
  switch (*op)
    {
    case 0 : A = *arg; break;
    case 16 : PC = *arg; break;
    case 17 : if (A < 0) PC = *arg; break;
    case 18 : if (!A) PC = *arg; break;
    case 32 : A += *arg; break;
    case 33 : A -= *arg; break;
    case 34 : A = ~(A & *arg); break;
    case 64 : A = ram[*arg]; break;
    case 65 : printf("out : %d\n", ram[*arg]);
      /*arrête la boucle d'exécution le temps pour l'utilisateur de voir la 
        et vide le flux entrant */
      while ((c = getchar() !='\n') && c != EOF) {}; break;
    case 72 : ram[*arg] = A; break;
    case 73 : ram[*arg] = recup_entree(); break;
    case 96 : A += ram[*arg]; break;
    case 97 : A -= ram[*arg]; break;
    case 98 : A = ~(A & ram[*arg]); break;
    case 192 : A = ram[(unsigned char)ram[*arg]]; break;
    case 193 : printf("out : %d\n", ram[(unsigned char)ram[*arg]]);
      while ((c = getchar() !='\n') && c != EOF) {}; break;
    case 200 : ram[(unsigned char)ram[*arg]] = A; break;
    case 201 : ram[(unsigned char)ram[*arg]] = recup_entree(); break;
    case 224 : A += ram[(unsigned char)ram[*arg]]; break;
    case 225 : A -= ram[(unsigned char)ram[*arg]]; break;
    case 226 : A = ~(A & ram[(unsigned char)ram[*arg]]); break;
    default : printf("ligne %x, op : %x\n", PC, *op);
      usage("Erreur : opcode inconnu."); break;
    }
}

char strtoc(char * chaine)
/*
Fonction qui convertit un code ou une adresse (nombre hexadécimal entre 00 et
FF) en un char. Provoque une erreur si l'on essaie de convertir autre
chose.

Prend une chaine en argument et renvoie celle-ci convertie en char.
 */
{
  if (strlen(chaine) != 2)
    usage("Contenu du fichier incorrect : les codes doivent avoir une "
	  "longueur de deux caractères");
  char c;
  char * end;
  c = strtol(chaine, &end, 16);
  /* Lorsque strtol rencontre un caractère qu'elle ne peut convertir, elle 
  fait pointer end vers ce caractère, on sait donc que si end pointe vers
  autre chose que 0 la chaine que l'on a essayé de convertir n'était pas
  uniquement en hexadécimal. */
  if (*end != 0)
    usage("Contenu du fichier incorrect : les codes doivent être en "
	  "hexadécimal");
  return c;
}

char recup_entree(void)
/*
Fonction qui permet de récupérer l'entrée de l'utilisateur à l'aide de scanf, 
s'il ne propose pas une entrée valide (un nombre entre -128 et 127) celui-ci 
devra recommencer.

Renvoie le nombre entré par l'utilisateur.
*/
{
  printf("in ? ");
  char * end;
  long int i;
  char c;
  char tampon[32];
  /* On scanne 31 caractères pour le cas où l'utilisateur entre des zéros non
  significatifs à gauche du nombre. S'il entre moins de 29 zéros non 
  significatifs, son nombre sera tout de même pris en compte par le 
  programme, au delà non mais on considérera qu'il l'a bien mérité. */
  scanf(" %31s", tampon);
  while ((c = getchar() !='\n') && c != EOF) {}; //vide le flux entrant
  i = strtol(tampon, &end, 10);
  if ((i < -128 || i > 127) || (*end != '\n' && *end != 0))
    {
      puts("Saisissez un nombre entre -128 et 127");
      return recup_entree();
    }
  c = (char) i;
  puts("");
  return c;
}
 
void step(unsigned char op, unsigned char arg)
/* 
Fonction qui affiche les valeurs de A et PC, les adresses dont l'utilisateur
a demandé le display puis l'instruction en cours.
Elle propose ensuite à l'utilisateur d'entrer une commande.
*/
{
  printf("A : %x, PC : %x\n", A, PC);
  for (int i = 0; add_display[i]; i++)
    printf("%x : %x\n", add_display[i], ram[add_display[i]]);
  print_instruction(op, arg);
  get_user_input();
  puts("");
}

void get_user_input(void)
/*
Fonction qui permet à l'utilisateur d'entrer une commande dans le stepper.
Il peut entrer une commande ou appuyer sur entrée pour passer à l'instruction
suivante. 
*/
{
  char tampon[1024] = {0};
  //boucle tant que l'utilisateur entre quelquechose
  while (strcmp(tampon, "\n"))
    {
      printf("(step) ? ");
      fgets(tampon, 1023, stdin);
      parse_input(tampon);
    }
}

void parse_input(char *tampon)
/*
Fonction qui analyse les entrées de l'utilisateur lors du stepper et effectue
le traitement adéquat.
*/
{
  char *elt = strtok(tampon, " ");
  if (!strcmp(elt, "display"))
    {
      char *suite = strtok(NULL, " ");
      //Si l'utilisateur a entré display [une adresse valide]
      if (adresse(suite))
	//On ajoute cette adresse au vecteur des adresse à afficher
	add_display[next_add_dis++] = strtol(suite, NULL, 16);
    }
  //Si l'entrée est memory, on affiche l'ensemble de la mémoire.
  else if (!strcmp(elt, "memory\n"))
    print_mem();
  else if (!strcmp(elt, "quit\n"))
    usage("Arrêt de l'exécution");
  else if (adresse(elt))
    
    {
      char *suite = strtok(NULL, " ");
      /*Si l'utilisateur a entré une adresse valide, on affiche le contenu
      de cette adresse. */
      if (!suite)
	{
	  unsigned char nb = strtol(elt, NULL, 16);
	  printf("%x : %x\n", nb, ram[nb]);
	}
      /*Si l'utilisateur a entré deux adresses valides, on affiche le contenu
        de toutes les adresses entre les deux. */
      else if (adresse(suite))
	{
	  unsigned char n = strtol(suite, NULL, 16);
	  /* i est un long int et non un unsigned char pour éviter une boucle
	     infinie si l'utilateur entre "0 ff" */
	  for (long int i = strtol(elt, NULL, 16); i <= n; i++)
	    printf("%lx : %x\n", i, ram[i]);
	}
    }
}

void print_mem(void)
/*
Affiche l'intégralité de la mémoire sous forme d'un carré de 16*16.
*/
{
  for (int i = 0; i < 16; i++)
    printf("\t%x", i);
  printf("\n  ");
  //printf("\n\t_\t_\t_\t_\t_\t_\t_\t_\t_\t_\t_\t_\t_\t_\t_\t_\n");
  for (int i = 0; i < 127; i++)
    printf("-");
  puts("");
  for (int i = 0; i <= 240; i += 16)
    {
      printf("%x |\t", i / 16);
      for (int j = i; j < i + 16; j++)
	printf("%x\t", ram[j]);
      puts("");
    }
}

bool adresse(char *chaine)
/*
Fonction qui détermine si une chaîne est une adresse valide ou non.
Prend en argument une chaine et renvoie True si cette chaîne est une adresse
valide et False sinon.
*/
{
  char * endptr;
  if (*chaine == '\n')
    return False;
  long int nb = strtol(chaine, &endptr, 16);
  if ((*endptr != '\0') && (*endptr != '\n'))
    return False;
  else if (nb >= 0 && nb < 256)
      return True;
  return False;
}

void print_instruction(unsigned char op, char arg)
/*
Fonction qui affiche l'instruction correspondant à l'opcode en cours et son
argument.
Prend en argument deux long int correspondant à l'opcode et son argument.
*/
{
    switch (op)
    {
    case 0 : printf("LOAD #%x : A <- %x\n", arg, arg); break;
    case 16 : printf("JUMP %x : PC <- %x\n", arg, arg); break;
    case 17 : printf("BRN %x : if (A < 0) PC <- %x\n", arg, arg); break;
    case 18 : printf("BRZ %x : if (A = 0) PC <- %x\n", arg, arg); break;
    case 32 : printf("ADD #%x : A <- A + %x\n", arg, arg); break;
    case 33 : printf("SUB #%x : A <- A - %x\n", arg, arg); break;
    case 34 : printf("NAND #%x : A <- ~(A & %x)\n", arg, arg); break;
    case 64 : printf("LOAD %x : A <- (%x)\n", arg, arg); break;
    case 65 : printf("OUT %x : out <- (%x)\n", arg, arg); break;
    case 72 : printf("STORE %x : (%x) <- A\n", arg, arg); break;
    case 73 : printf("IN %x : (%x) <- in\n", arg, arg); break;
    case 96 : printf("ADD %x : A <- A + (%x)\n", arg, arg); break;
    case 97 : printf("SUB %x : A <- A - (%x)\n", arg, arg); break;
    case 98 : printf("NAND %x : A <- ~ (A & (%x))\n", arg, arg); break;
    case 192 : printf("LOAD *%x : A <- *(%x)\n", arg, arg); break;
    case 193 : printf("OUT *%x : out <- *(%x)\n", arg, arg); break;
    case 200 : printf("STORE *%x : *(%x) <- A\n", arg, arg); break;
    case 201 : printf("IN *%x : *(%x) <- in\n", arg, arg); break;
    case 224 : printf("ADD *%x : A <- A + *(%x)\n", arg, arg); break;
    case 225 : printf("SUB *%x : A <- A - *(%x)\n", arg, arg); break;
    case 226 : printf("NAND *%x : A <- ~(A & *(%x))\n", arg, arg); break;
    default : printf("ligne %x, op : %x", PC, op);
      usage("Erreur : opcode inconnu."); break;
    }
}
