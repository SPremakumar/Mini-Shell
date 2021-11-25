/*
– Compiler : gcc pipe-minishell.c -o pipe-minishell
– Exécuter : ./pipe-minishell 
- Compiler et Exécuter : gcc pipe-minishell.c -o pipe-minishell && ./pipe-minishell
*/

# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/wait.h>
# include <assert.h>
# include <string.h>
# include <fcntl.h> // open(),

enum {
    MaxLigne = 1024,              // longueur max d'une ligne de commandes
    MaxMot = MaxLigne / 2,        // nbre max de mot dans la ligne
    MaxDirs = 100,                // nbre max de repertoire dans PATH
    MaxPathLength = 512,          // longueur max d'un nom de fichier
};


void decouper(char *, char *, char **, int);
void executer_commande(char **);
int chercher(char *, char **);


# define PROMPT "? "


int main(int argc, char * argv[]){
    char ligne[MaxLigne];
    char pathname[MaxPathLength];
    char * mot[MaxMot];
    char * mot_pipe[MaxMot]; // + MODIF
    char * dirs[MaxDirs];
    int i, r, t, pfd[2], tmp, status, background, symbole_pipe;

    while(printf(PROMPT), fgets(ligne, sizeof ligne, stdin) != 0){
        decouper(ligne, " \t\n", mot, MaxMot);
        
        for(i = 0; mot[i] != 0 ; i++);
            if (mot[0] == 0) // ligne vide 
                continue;

        // Quitter le mini-shell 
        if(strcmp(mot[0], "quit") == 0 || strcmp(mot[0], "q") == 0){
            printf("Bye\n");
            exit(1);
        }

        // Détecter le symbole "|" :
        symbole_pipe = chercher("|", mot);

        if(symbole_pipe  > 0){
            // Copier la seconde commande :
            i = 0;
            while(symbole_pipe <= MaxMot) {
                mot_pipe[i] = mot[symbole_pipe];
                symbole_pipe++;
                i++;
            }
        }

        // lancement du processus : 
        tmp = fork();

        // Erreur  : 
        if (tmp < 0){
            perror("fork");
            continue;
        }

        // parent :
        if (tmp != 0){

            if (symbole_pipe > 0){

                // Creation d'un pipe() :
                t = pipe(pfd);

                // En cas d'erreur avec pipe() : 
                if(t < 0){
                    perror("Erreur dans la création Pipe()");
                    exit(EXIT_FAILURE);
                }

                // Création d'un fork() :
                pid_t p1 = fork();

                // Erreur : 
                if (p1 < 0){
                    perror("Erreur dans le fork()");
                    continue;
                }

                // Enfant du processus 1 : 
                if(p1 == 0){
                    close(pfd[0]);
                    dup2(pfd[1], STDOUT_FILENO);
                    close(pfd[1]);
                    executer_commande(mot);
                    exit(0);
                }

                // Parent du shell :
                pid_t p2 = fork();

                // Erreur : 
                if (p2 < 0){
                    perror("Erreur dans le fork()");
                    continue;
                }

                // Enfant du processus 2 : 
                if(p2 == 0){
                    close(pfd[1]);
                    dup2(pfd[0], STDIN_FILENO);
                    close(pfd[0]);
                    executer_commande(mot_pipe);
                    exit(0);
                } 

                // Parent du shell :
                close(pfd[0]);
                close(pfd[1]);
                waitpid(p1, &status, 0);
                waitpid(p2, &status, 0);
            }
        }

        // Pour les commandes normales (sans pipe) :
        if (tmp == 0 && symbole_pipe == 0){
            executer_commande(mot);
            wait(NULL);
        }
    }
    printf("Bye Bye\n");
    return 0;
}



void decouper(char * ligne, char * separ, char * mot[], int maxmot){
    int i;

    mot[0] = strtok(ligne, separ);
    for(i = 1; mot[i - 1] != 0; i++){
     if (i == maxmot){
        fprintf(stderr, "Erreur dans la fonction decouper: trop de mots\n");
        mot[i - 1] = 0;
        
     }
     mot[i] = strtok(NULL, separ);
    }
}


void executer_commande(char **commande){
    int i;
    char *dirs[MaxDirs];
    char pathname[MaxPathLength];

    decouper(getenv("PATH"), ":", dirs, MaxDirs);

    for (i = 0; dirs[i] != 0; i++){
        snprintf(pathname, sizeof pathname, "%s/%s", dirs[i], commande[0]);
        execvp(pathname, commande);
    }
    fprintf(stderr, "%s: not found\n", commande[0]);
    exit(1);
}


int chercher(char * mot_cherche, char ** mot){
    int i;
    for(i = 1; i <= MaxMot; i++){
        if (mot[i] == 0)
            break;

        if (strcmp(mot[i], mot_cherche) == 0){
            mot[i] = 0;
            return i+1;
        }
    }
    return 0;
}