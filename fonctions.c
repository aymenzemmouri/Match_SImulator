#include "fonctions.h"

/**
*@brief Lit les noms d'équipe à partir d'un fichier et les stocke dans un tableau de chaînes alloué dynamiquement.
*@param filename Le nom du fichier contenant les noms des équipes.
*@param num_teams Un pointeur vers un entier qui stockera le nombre d'équipes lues à partir du fichier.
*@param team_names Un pointeur vers un pointeur qui stockera le tableau des noms d'équipe.
*@pre Le paramètre filename doit être un chemin d'accès valide vers un fichier contenant des noms d'équipe, et les paramètres num_teams et team_names doivent être non nuls.
*@post Le paramètre num_teams sera mis à jour avec le nombre d'équipes lues à partir du fichier, et le paramètre team_names pointera vers un tableau de chaînes alloué dynamiquement contenant les noms des équipes.
*@return vide.
*/
void read_team_names(char* filename, int* num_teams, char *** team_names) {
    FILE *file;
    *team_names = (char**) malloc(MAX_TEAMS * sizeof(char*));
    char line[MAX_TEAM_NAME_LEN];

    for (int i = 0; i < MAX_TEAMS; i++) {
        (*team_names)[i] = (char*) malloc(MAX_TEAM_NAME_LEN * sizeof(char));
    }

    file = fopen(filename, "r");
    if (file == NULL) {
        printf("Erreur lors de l'ouverture du fichier.");
        exit(1);
    }

    // Lecture de la durée du match sur la première ligne
    fgets(line, sizeof(line), file);

    //Verification si une duree est introduite
    int num;
    if (sscanf(line, "%d", &num) == 1) {
        match_duration = atoi(line);
    }else {
        match_duration = DURATION;
    }

    // Lecture des noms d'équipes sur les lignes suivantes
    while (fgets(line, sizeof(line), file)) {
        // Supprime le caractère de fin de ligne
        line[strcspn(line, "\r\n")] = 0;

        // Supprime les espaces inutiles à la fin de la chaîne de caractères
        int len = strlen(line);
        while (len > 0 && line[len-1] == ' ') {
            line[len-1] = '\0';
            len--;
        }

        // Vérifie si la ligne contient que des vides
        int only_spaces = 1;
        for (int i = 0; i < len; i++) {
            if (line[i] != ' ') {
                only_spaces = 0;
                break;
            }
        }

        // Si la ligne ne contient pas que des vides, ajoute le nom de l'équipe à la liste
        if (!only_spaces) {
            strcpy((*team_names)[*num_teams], line);
            (*num_teams)++;
        }
    }
    if (((*num_teams) & ((*num_teams) - 1)) != 0 || (*num_teams) > MAX_TEAMS)
    {
        printf("Number of teams must be a power of 2 and less than %d\n", MAX_TEAMS);
        exit(EXIT_FAILURE);
    }
    fclose(file);

    // Mélange les noms d'équipes de manière aléatoire
    srand(time(NULL));

    // Shuffle
    for (int i = *num_teams - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        char temp[MAX_TEAM_NAME_LEN];
        strcpy(temp, (*team_names)[i]);
        strcpy((*team_names)[i], (*team_names)[j]);
        strcpy((*team_names)[j], temp);
    }
}
/**
*@brief La fonction simule un match pour le mode "Simulation concurrente", cette fonction s'execute dans un thread, ce qui permet d'executer plusieurs matchs en meme temps avec un mecanisme de verrouillage avec mutex pour eviter les problemes liées aux acces concurrents.
* Elle simule le match en générant des scores aléatoires pour chaque équipe en utilisant la fonction rand().
* Le match est simulé pendant un certain temps défini par la constante match_duration, et si les scores sont égaux à la fin du temps réglementaire, une séance de tirs au but est effectuée pour déterminer le vainqueur.
* Si une équipe gagne le match, la fonction met à jour le tableau teams_remaining qui indique quelles équipes sont encore en compétition et à quel tour. Elle utilise également un verrou (mutex) pour éviter les conflits d'accès au tableau par plusieurs threads en même temps.
*@param ma Pointeur qui est ensuite casté en une structure Match. Corresspond au match qui va etre simulé
*@return void*
*/
void *simulate_match(void *ma)
{
    Match match = (Match) ma;

    int duration = 0;
    int action;

    // Debut de la simulation
    printf("DEBUT %s %d - %d %s [TOUR %d]\n",team_names[match->team1],match->score1, match->score2,team_names[match->team2],match->tour);
    while (duration < match_duration)
    {
        action = rand() % 100; //Simule une action aleatoire
        if (action < 98)
        { // 98% de chance de ne pas marquer pour les deux equipes
            duration++;
        }
        else if (action == 99 )
        { // 1% de chance de marquer pour l'equipe 1
            match->score1++;
            duration++;
            printf("(%d') %.20s %d - %d %-20s\n",duration, team_names[match->team1], match->score1, match->score2, team_names[match->team2]);
        }
        else //action == 100
        { // 1% de chance de marquer pour l'equipe 2
            match->score2++;
            duration++;
            printf("(%d') %.20s %d - %d %-20s\n",duration, team_names[match->team1], match->score1, match->score2, team_names[match->team2]);
        }
        sleep(0.005); //mettre en pause pendant 1 seconde
    }
    //Si le score reste nul, alors execution de la séance de tirs au buts
    int nTab = 5;
    while(match->score1 == match->score2){
        int tab;
        while(nTab > 0){
            tab = rand() % 100;
            if(tab > 20){ //80% de chance de marquer
                match->score1++;
                printf("%.20s (%d) - (%d) %-20s\n", team_names[match->team1], match->score1, match->score2, team_names[match->team2]);
            }
            tab = rand() % 100;
            if(tab < 40){//60% de chance de marquer
                match->score2++;
                printf("%.20s (%d) - (%d) %-20s\n", team_names[match->team1], match->score1, match->score2, team_names[match->team2]);
            }
            nTab--;
        } //Si apres les 5 TAB, le score reste vierge, alors chacune des deux equipes tir une fois, la premiere qui rate perd le match
        nTab=1;
    }
    if(match->score1 > match->score2){ // Si l'équipe 1 a gagné
        pthread_mutex_lock(&mutex); // Verrouillage du mutex pour accéder à la variable partagée
        teams_remaining[match->team1] = match->tour+1; // On met à jour le tableau des équipes restantes en compétition
        teams_remaining[match->team2] = -1; // On indique que l'équipe 2 est éliminée
        pthread_mutex_unlock(&mutex); // Déverrouillage du mutex
        printf("FIN %s* %d - %d %s\n",team_names[match->team1], match->score1, match->score2, team_names[match->team2]); // On affiche le résultat du match avec une astérisque à côté du nom de l'équipe gagnante
    }
    else{ // Si l'équipe 2 a gagné ou s'il y a match nul
        pthread_mutex_lock(&mutex); // Verrouillage du mutex pour accéder à la variable partagée
        teams_remaining[match->team2] = match->tour+1; // On met à jour le tableau des équipes restantes en compétition
        teams_remaining[match->team1] = -1; // On indique que l'équipe 1 est éliminée
        pthread_mutex_unlock(&mutex); // Déverrouillage du mutex
        printf("FIN %s %d - %d %s*\n",team_names[match->team1], match->score1, match->score2, team_names[match->team2]); // On affiche le résultat du match avec une astérisque à côté du nom de l'équipe gagnante
    }

    pthread_exit(0);
}
/**
*@brief Simule un match pour le mode manuel, la fonction propose à l'utilisateur de choisir entre : [1] Simuler le deroulement du match en temps reel minute par minute, avec possibilité d'interruption en cliquant sur une touche afin de choisir quelle equipe marque [2]: Choisir directement un score pour le match
*@param ma Pointeur sur la structure Match
*@return void*
*/
void play_match(Match match){
    int valid = 0; //choix valide ou pas
    int sleep = 1000000; //temps d'arret a chaque boucle, en microsecondes
    int action;
    int mode ; //Choix du mode de simulation manuel
    int res = -1;

    printf("Match %s VS %s [TOUR %d]\n",team_names[match->team1],team_names[match->team2],match->tour);
    do {
        printf("Choisir le mode de jeu : [1]:Simuler | [2]:Choisir un score \n");
        scanf("%d",&mode);
    }while(mode != 1 && mode != 2);

    if(mode==1){ //Mode "Simuler"
        printf("DEBUT %s %d - %d %s [TOUR %d]\n",team_names[match->team1],match->score1, match->score2,team_names[match->team2],match->tour);
        for (int duration = 0; duration <= 90; duration++) {
            usleep(sleep);
            // Check for input
            fd_set set;
            FD_ZERO(&set);
            FD_SET(STDIN_FILENO, &set);
            struct timeval timeout = {0, 0};
            int rv = select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout);
            if (rv == 1) {
                getchar();
                printf("Que voulez-vous faire ? (0: Accelerer le match, 1: L'equipe 1 marque, 2: L'equipe 2 marque) ");
                duration++;
                valid=0;
                res=-1;
                while(valid==0){
                    int c;
                    while ((c = getchar()) != '\n' && c != EOF) {}
                    scanf("%d",&res);
                    if(res==0){
                        sleep = 0; //accelerer le match en mettant le temps d'arret a chaque boucle a 0
                        valid = 1;
                    }else if(res==1){ //l'equipe 1 marque
                        match->score1++;
                        printf("(%d') %.20s %d - %d %-20s\n",duration, team_names[match->team1], match->score1, match->score2,
                               team_names[match->team2]);
                        valid = 1;
                    }else if(res==2){//l'equipe 2 marque
                        match->score2++;
                        printf("(%d') %.20s %d - %d %-20s\n",duration, team_names[match->team1], match->score1, match->score2,
                               team_names[match->team2]);
                        valid = 1;
                    }else{printf("Veuillez choisir entre 0, 1 et 2\n");}
                }
            }else{
                action = rand() % 100; // Simule une action aleatoire
                if (action < 98) { // 98% de chance de ne pas marquer
                    printf("(%d')\n", duration);
                } else if (action == 99) { // 1% de chance de marquer pour l'equipe 1
                    match->score1++;
                    printf("(%d') %.20s %d - %d %-20s\n", duration, team_names[match->team1], match->score1, match->score2,
                           team_names[match->team2]);
                } else //action == 100
                { // 1% de chance de marquer pour l'equipe 2
                    match->score2++;
                    printf("(%d') %.20s %d - %d %-20s\n", duration, team_names[match->team1], match->score1, match->score2,
                           team_names[match->team2]);
                }
            }
        }
        //Si le score reste nul, alors execution de la séance de tirs au buts
        int nTab = 5;
        while (match->score1 == match->score2) {
            int tab;
            while (nTab > 0) {
                tab = rand() % 100;
                if (tab > 20) { //80% de chance de marquer
                    match->score1++;
                    printf("%.20s (%d) - (%d) %-20s\n", team_names[match->team1], match->score1, match->score2,
                           team_names[match->team2]);
                }
                tab = rand() % 100;
                if (tab < 40) {//60% de chance de marquer
                    match->score2++;
                    printf("%.20s (%d) - (%d) %-20s\n", team_names[match->team1], match->score1, match->score2,
                           team_names[match->team2]);
                }
                nTab--;
            }
            nTab = 1; //Si apres les 5 TAB, le score reste vierge, alors chacune des deux equipes tir une fois, la premiere qui rate perd le match
        }
    }

    if(mode==2){ //Mode "Choisir un score"
        int score1 = 0, score2 = 0;
        do {
            printf("Score %s: ",team_names[match->team1]);
            scanf("%d",&score1);
            printf("Score %s: ",team_names[match->team2]);
            scanf("%d",&score2);
            if(score1 == score2){
                printf("Veuillez choisir un score non nul \n");
            }
        }while(score1 <= 0 || score2 <= 0 || score1==score2);
        match->score1 = score1;
        match->score2 = score2;
    }

    if(match->score1 > match->score2){
        teams_remaining[match->team1] = match->tour+1;
        teams_remaining[match->team2] = -1;
        printf("FIN %s* %d - %d %s\n",team_names[match->team1], match->score1, match->score2, team_names[match->team2]);
    }
    else{
        teams_remaining[match->team2] = match->tour+1;
        teams_remaining[match->team1] = -1;
        printf("FIN %s %d - %d %s*\n",team_names[match->team1], match->score1, match->score2, team_names[match->team2]);
    }
}
/**
 *@brief Enregistre les informations des matchs dans un fichier texte.
 Cette fonction ouvre un fichier texte en mode écriture, puis écrit les informations
 de chaque match dans le fichier sous la forme suivante :
 "Match [numéro] : [nom de l'équipe 1] ([score de l'équipe 1]) : ([score de l'équipe 2]) [nom de l'équipe 2] | Tour [numéro du tour]"
 Les informations sont extraites du tableau "matchs" et du tableau "team_names".
 Le fichier est ensuite fermé avant la fin de la fonction.
 *@param team_names Le tableau de noms des équipes.
 *@param matchs Le tableau de matchs à enregistrer.
 *@param num_match Le nombre de matchs à enregistrer.
 *@return void
*/
void save_matchs(char **team_names, Match *matchs, int num_match) {
    // Ouverture du fichier en mode écriture
    FILE *fp = fopen("matchs.txt", "w");
    if (fp == NULL) {
        printf("Erreur d'ouverture du fichier.");
        return;
    }

    // Écriture des informations de chaque match dans le fichier texte
    for (int i = 0; i < num_match; i++) {
        fprintf(fp, "Match %d : %s [%d] : [%d] %s | Tour %d\n", i + 1, team_names[matchs[i]->team1], matchs[i]->score1, matchs[i]->score2, team_names[matchs[i]->team2], matchs[i]->tour);
    }

    fclose(fp); //Fermeture du fichier
}
/**
@brief Libérez toute la mémoire allouée dynamiquement utilisée dans le programme.
Cette fonction libère la mémoire utilisée pour les noms d'équipe et le tableau des équipes restantes dans le tournoi.
@retour vide
*/
void free_memory() {
    //Liberation du tableau des noms des equipes
    for (int i = 0; i < num_teams; i++) {
        free(team_names[i]);
    }
    free(team_names);
    //Liberation du tableau des equipes restantes
    free(teams_remaining);
}

