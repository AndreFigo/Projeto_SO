// Trabalho Prático - Simulador de Corridas
// André Carvalho 2019216156
// Sofia Alves 2019227240
// 20/21

#include "declarations.h"

void *car_func(void *p)
{

    int ind = ((info *)p)->ind_car;
    int team_num = ((info *)p)->team_num;
    //pthread_t id = pthread_self();

    char msg[MAXTAMLINE];
    sprintf(msg, "Novo carro criado na team %d (Carro %d)\n", ind, team_num);
    print_debug(msg);

    //print_car_info(ind, team_num);

    sem_wait(start_race);


    //sleep(1);

    sprintf(msg, "Carro %d a sair da team %d\n", ind, team_num);
    print_debug(msg);

    pthread_exit(NULL);
}

void print_car_info(int ind, int team_num)
{
    printf("Car %d of team %s -> Number: %d, Speed: %d, Comsumption: %.2f, Reliability: %d\n", ind + 1, (teams + team_num)->name, (cars + team_num * data->n_teams + ind)->num, (cars + team_num * data->n_teams + ind)->speed, (cars + team_num * data->n_teams + ind)->consumption, (cars + team_num * data->n_teams + ind)->reliability);
}