// Trabalho Prático - Simulador de Corridas
// André Carvalho 2019216156
// Sofia Alves 2019227240
// 20/21

#include "declarations.h"

void Team_manager(int num)
{
    
    info * ids= (info *) malloc (sizeof(info)* data->max_car);
    (teams + num)->box_state = LIVRE;
    char msg[MAXTAMLINE];
    sprintf(msg, "team %d entrou\n", num);
    print_debug(msg);

    for (int i = 0; i < data->max_car; ++i)
    {
        if (sem_wait(&((teams + num)->car_ready))==-1){
            printf("Erro no sem wait");
        }
        //printf("ENTROU team %d\nValor %d\n", num, valor);
        (ids +i)->ind_car = num * data->max_car + i;
        (ids +i)->team_num = num;

        if ((cars + num * data->max_car + i)->num != -1)
            pthread_create(&((cars + num * data->max_car + i)->tid), NULL, car_func, ids+i);
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
        sprintf(msg, "team %d waiting for car to enter the box\n", num);
        print_debug(msg);
        sem_wait(&(teams + num)->entered_box);


        int ind_car = ((teams + num)->ind_catual);
        int elapsed = data->tunits_passed;

        //check if it is a fake car
        //fake cars may be used to end the race
        if(ind_car==-1)
            break;

        /* T_Box_min -  T_Box_max*/
        if (((cars + ind_car)->malfunc) == 1)
        {
            ((cars + ind_car)->malfunc) = 0;
            time_box = time_box + t_min + rand() % (t_max - t_min + 1);
        }

        (cars + ind_car)->fuel = data->fuel_tank;
        (cars + ind_car)->box_time = time_box;

        for (int i = 0; i < time_box; i++)
        {   
            print_debug("AAAAAAAAAAAAAAAAAAAAAA\n");
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
            elapsed += 1;
            pthread_mutex_unlock(&data->new_tunit_mutex);
        }

        sem_wait(&(teams + num)->mutex_box_state);

        (teams + num)->n_cars_seg_mode -= 1;
        if ((teams + num)->n_cars_seg_mode ==0)
            (teams + num)->box_state = LIVRE;
        else
            (teams + num)->box_state = RESERVADO;
        sem_post(&(teams + num)->mutex_box_state);

        sem_post(&(teams + num)->box_finished);
    }

    //sleep(4);
    for (int i = 0; i < (teams + num)->n_cars; ++i)
        pthread_join((cars + num * data->n_teams + i)->tid, NULL);

    sprintf(msg, "saiu team %d\n", num);
    print_debug(msg);
}
