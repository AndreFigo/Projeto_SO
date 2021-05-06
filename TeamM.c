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
    int elapsed;
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
    //repair car
    //fill tank
    //update box state and car state
    int time_box = 2, t_max = data->T_Box_Max, t_min = data->T_Box_min;
    while (1)
    {
        sem_wait(&(teams + num)->entered_box);

        int ind_car = ((teams + num)->ind_catual);
        int elapsed = data->tunits_passed;

        //check if it is a fake car
        //fake cars may be used to end the race
        if ((cars + ind_car)->num == -1)
        {
            break;
        }

        /* T_Box_min -  T_Box_max*/
        if (((cars + ind_car)->malfunc) == 1)
        {
            ((cars + ind_car)->malfunc) == 0;
            time_box = time_box + t_min + rand() % (t_max - t_min + 1);
        }

        (cars + ind_car)->fuel = data->fuel_tank;
        (cars + ind_car)->box_time = time_box;

        for (int i = 0; i < time_box; i++)
        {
            pthread_mutex_lock(&data->end_tunit_mutex);
            data->cars_ended_tunit += 1;
            pthread_cond_signal(&data->end_tunit);

            pthread_mutex_unlock(&data->end_tunit_mutex);

            // wait for a tunit to pass
            pthread_mutex_lock(&data->new_tunit_mutex);
            while (elapsed == data->tunits_passed)
            {
                pthread_cond_wait(&data->new_tunit, &data->new_tunit_mutex);
            }
            pthread_mutex_unlock(&data->new_tunit_mutex);
        }

        sem_post(&(teams + num)->box_finished);
    }

    //sleep(4);
    for (int i = 0; i < (teams + num)->n_cars; ++i)
        pthread_join((cars + num * data->n_teams + i)->tid, NULL);

    sprintf(msg, "saiu team %d\n", num);
    print_debug(msg);
}
