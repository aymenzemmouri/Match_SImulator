#include "fonctions.h"
/**
 * @file main.c
 * @brief Programme principal pour la simulation du tournoi
 * Ce programme simule un tournoi entre plusieurs équipes de façon concurrente ou manuelle.
 * Les equipes ainsi que la duree du match sont lus à partir d'un fichier passé en argument
 * Le résultat des matchs est enregistré dans un fichier de sortie.
 * @author Ayoub SALAH
 * @author Chakib MOUSSAOUI
 * @author Aymen ZEMMOURI
 * @date 2023
 * @version 1.0
*/

/**
 *@brief Durée des matchs en secondes
*/
int match_duration = DURATION;

/**
 *@brief Nombre d'équipes participant au tournoi
*/
int num_teams;

/**
 *@brief Tableau des noms des équipes, indicé par les numeros des equipes
*/
char **team_names;

/**
 *@brief Tableau des équipes restantes, indicé par les numeros des equipes, contient (-1: si l'equipe est eliminé | 0: si l'equipe est entrain de jouer | n: si l'equipe est au tour n, prete a jouer)
*/
int * teams_remaining;

/**
 *@brief Mutex utilisé pour la synchronisation des threads dans le mode "Simulation concurrente"
*/
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


/**
 *@brief Fonction principale
 *Cette fonction lit le nom des équipes depuis un fichier, initialise les tableaux et les structures nécessaires pour la simulation, et lance la simulation du tournoi en mode concurrent ou manuel.
 *@param argc Nombre d'arguments passés au programme
 *@param argv Tableau de chaînes de caractères contenant les arguments passés au programme
 *@return Toujours 1
*/
int main(int argc, char *argv[])
{
    char * filename;
    if (argc != 2) {
        filename = FILENAME;
    }else {
        filename = argv[1];
    }

    //Creation d'une liste randomise avec les equipes
    read_team_names(filename, &num_teams, &team_names);

    //Allocation du tableau teams_remaining
    teams_remaining = (int *)malloc(num_teams*sizeof(int));

    //Initialisation du tableau a 1, toutes les equipes sont au debut au tour 1
    for (int i = 0; i < num_teams; i++)
    {
        teams_remaining[i] = 1;
    }

    int num_matchs_total = num_teams-1;
    Match matchs[num_matchs_total];
    int num_match = 0;
    int team1 = 0;
    int team2 = 0;
    int tour = 1;
    int manual = 1;
    do {
        printf("Choisir le mode de jeu : [1]:Mode simulation concurrente | [2]:Mode manuel \n");
        scanf("%d",&manual);
    }while(manual != 1 && manual != 2);

    if (manual == 1) { //Mode "Simulation concurrente"
        pthread_mutex_init(&mutex,NULL);

        pthread_t threads[num_matchs_total]; //Tableau de threads de taille egale au nombre de match totale
        while(num_match < num_matchs_total){
            for (team1 = 0; team1 < num_teams; team1++)
            {
                pthread_mutex_lock(&mutex); //Verrouiler le mutex avant d'acceder au tableau teams_ramining
                if(teams_remaining[team1] > 0){ //L'equipe team1 est prete a jouer le tour=teams_remaining[team1]
                    tour = teams_remaining[team1];
                    team2 = team1+1;
                    while(team2 < num_teams){ //Rrouver l'equipe 2 qui a le meme numero de tour que team1
                        if(teams_remaining[team2] == tour){
                            //Creaion d'un match entre team1 et team2
                            matchs[num_match] = (Match)malloc(sizeof(struct Match));
                            matchs[num_match]->team1 = team1;
                            matchs[num_match]->team2 = team2;
                            matchs[num_match]->score1 = 0;
                            matchs[num_match]->score2 = 0;
                            matchs[num_match]->tour = teams_remaining[team2];
                            //Mettre a 0 les cases de teams_remaining d'indices team1 et team2, signifie qu'ils sont en train de jouer un match
                            teams_remaining[team1] = 0;
                            teams_remaining[team2] = 0;
                            //Creation d'un thread pour ce match
                            pthread_create(&threads[num_match], NULL, simulate_match, matchs[num_match]);
                            num_match++;
                            team2++;
                            break;
                        }
                        team2++;
                    }
                }
                pthread_mutex_unlock(&mutex);//Deverouillage du mutex
            }
        }

        for (int i = 0; i < num_matchs_total; i++)
        {
            pthread_join(threads[i], NULL);
        }
        pthread_mutex_destroy(&mutex);
    }else { //Mode Manuel
        while(num_match<num_matchs_total) {
            team1=0;
            team2=0;
            while(team1<num_teams && team2 < num_teams) {
                while (teams_remaining[team1] <= 0) { team1++; }
                team2 = team1 + 1;
                while (teams_remaining[team2] <= 0) { team2++; }
                matchs[num_match] = (Match) malloc(sizeof(struct Match));
                matchs[num_match]->team1 = team1;
                matchs[num_match]->team2 = team2;
                matchs[num_match]->score1 = 0;
                matchs[num_match]->score2 = 0;
                matchs[num_match]->tour = tour;
                play_match(matchs[num_match]);
                num_match++;
                team1= team2 + 1;
            }
            tour++;
        }
    }

    //Ecriture du resumé sur fichier
    save_matchs(team_names, matchs, num_match);

    //Liberation memoire des deux tableaux
    free_memory();

    return 1;
}