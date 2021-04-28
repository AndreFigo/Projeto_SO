// Trabalho Prático - Simulador de Corridas
// André Carvalho 2019216156
// Sofia Alves 2019227240
// 20/21

#include "declarations.h"

void Team_manager(int num)
{
    info ids[data->max_car];
    (teams + num)->box_state = LIVRE;
    char msg[MAXTAMLINE];
    sprintf(msg, "team %d entrou\n", num);
    print_debug(msg);

    for (int i = 0; i < data->max_car; ++i)
    {
        sem_wait(&((teams + num)->car_ready));
        ids[i].ind_car = num * data->n_teams + i;
        ids[i].team_num = num;

        if ((cars + num * data->n_teams + i)->num != -1)
            pthread_create(&((cars + num * data->n_teams + i)->tid), NULL, car_func, &(ids[i]));
        else
            print_debug("Car ignored\n");
    }

    sem_wait(start_race);

    //box
    sem_wait(&((teams + num)->box_access));
    //repair car
    //fill tank
    //update box state and car state
    sem_post(&((teams + num)->box_access));
    if ((teams + num)->box_state >= OCUPADO)
    {
        for (int i = 0; i < data->max_car; i++)
        {
            if ((cars + i)->num != -1 && (cars + i)->ind_team == num && ((cars + i)->distance) % data->distance == 0)
            {
                if ((teams + num)->box_state == LIVRE && (cars + i)->state == SEGURANCA)
                {
                    (teams + num)->box_state == RESERVADO;
                    (cars + i)->state == SEGURANCA;
                }
            }
        }
    }

    //sleep(4);
    for (int i = 0; i < (teams + num)->n_cars; ++i)
        pthread_join((cars + num * data->n_teams + i)->tid, NULL);

    sprintf(msg, "saiu team %d\n", num);
    print_debug(msg);
}