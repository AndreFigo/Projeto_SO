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
    int elapsed = 0;

    while (1)
    {
        //inform that you are ready to wait for a tunit to pass
        pthread_mutex_lock(&data->end_tunit_mutex);

        data->cars_ended_tunit += 1;
        pthread_cond_signal(&data->end_tunit);

        pthread_mutex_unlock(&data->end_tunit_mutex);

        // wait for a tunit to pass
        pthread_mutex_lock(&data->new_tunit_mutex);
        while (elapsed != data->tunits_passed)
        {
            pthread_cond_wait(&data->new_tunit, &data->new_tunit_mutex);
        }
        elapsed += 1;
        pthread_mutex_unlock(&data->new_tunit_mutex);

        //update distance
        //update gas
        //check box
        //check status
        //communicate status changes
        //read message queue
    }

    //sleep(1);

    sprintf(msg, "Carro %d a sair da team %d\n", ind, team_num);
    print_debug(msg);

    pthread_exit(NULL);
}

void print_car_info(int ind, int team_num)
{
    printf("Car %d of team %s -> Number: %d, Speed: %d, Comsumption: %.2f, Reliability: %d\n", ind + 1, (teams + team_num)->name, (cars + team_num * data->n_teams + ind)->num, (cars + team_num * data->n_teams + ind)->speed, (cars + team_num * data->n_teams + ind)->consumption, (cars + team_num * data->n_teams + ind)->reliability);
}