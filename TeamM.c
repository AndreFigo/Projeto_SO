// Trabalho Prático - Simulador de Corridas
// André Carvalho 2019216156
// Sofia Alves 2019227240
// 20/21

#include "declarations.h"

void Team_manager(int num)
{
    info ids[data->max_car];

    char msg[MAXTAMLINE];
    sprintf(msg, "team %d entrou\n", num);
    print_debug(msg);

    for (int i = 0; i < data->max_car; ++i)
    {
        sem_wait(&((teams + num)->car_ready));
        ids[i].ind_car = i;
        ids[i].team_num = num;

        if ((cars + num * data->n_teams + i)->num != -1)
            pthread_create(&((cars + num * data->n_teams + i)->tid), NULL, car_func, &(ids[i]));
        else
            print_debug("Car ignored\n");
    }

    sem_wait(start_race);

    //sleep(4);
    for (int i = 0; i < (teams + num)->n_cars; ++i)
        pthread_join((cars + num * data->n_teams + i)->tid, NULL);

    sprintf(msg, "saiu team %d\n", num);
    print_debug(msg);
}