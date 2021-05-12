// Trabalho Prático - Simulador de Corridas
// André Carvalho 2019216156
// Sofia Alves 2019227240
// 20/21

#include "declarations.h"

void Team_manager(int num)
{

    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    for (int i = 0; i < data->n_teams; ++i)
    {
        if (i != num && close((teams + i)->fd[1]) == -1)
        {
            perror("ERROR: Failed to close unnamed pipe\n");
            exit(1);
        }
        if (close((teams + i)->fd[0]) == -1)
        {
            perror("ERROR: Failed to close unnamed pipe\n");
            exit(1);
        }
    }

    info *ids = (info *)malloc(sizeof(info) * data->max_car);
    (teams + num)->box_state = LIVRE;
    char msg[MAXWARNINGMSG];
    sprintf(msg, "team %d entrou\n", num);
    print_debug(msg);

    for (int i = 0; i < data->max_car; ++i)
    {
        if (sem_wait(&((teams + num)->car_ready)) == -1)
        {
            printf("Erro no sem wait");
        }
        //printf("ENTROU team %d\nValor %d\n", num, valor);
        (ids + i)->ind_car = num * data->max_car + i;
        (ids + i)->team_num = num;

        if ((cars + num * data->max_car + i)->num != -1)
            pthread_create(&((cars + num * data->max_car + i)->tid), NULL, car_func, ids + i);
        else
            print_debug("Car ignored\n");
    }

    sem_wait(start_race);

    //box
    //repair car
    //fill tank
    //update box state and car state
    int time_box = REFUEL_TIME, t_max = data->T_Box_Max, t_min = data->T_Box_min;
    srand(time(NULL));
    while (1)
    {
        sprintf(msg, "team %d waiting for car to enter the box\n", num);
        print_debug(msg);
        sem_wait(&(teams + num)->entered_box);

        int ind_car = ((teams + num)->ind_catual);
        int elapsed = data->tunits_passed;

        //check if it is a fake car
        //fake cars may be used to end the race
        if (ind_car == -1)
            break;

        /* T_Box_min -  T_Box_max*/
        if (((cars + ind_car)->malfunc) == 1)
        {
            ((cars + ind_car)->malfunc) = 0;
            time_box = time_box + t_min + rand() % (t_max - t_min + 1);
        }

        (cars + ind_car)->fuel = data->fuel_tank;
        (cars + ind_car)->box_time = 0;
        (cars + ind_car)->distance = (cars + ind_car)->laps_done * data->distance;

        for (int i = 0; i < time_box; i++)
        {
            pthread_mutex_lock(&data->interupt_mutex);
            pthread_mutex_lock(&data->forced_stop_mutex);
            if (data->stop == 1 || data->interupt == 1)
            {
                pthread_mutex_unlock(&data->forced_stop_mutex);
                pthread_mutex_unlock(&data->interupt_mutex);

                (cars + ind_car)->state = TERMINADO;
                increment_cars_finished();

                communicate_status_changes(num, ind_car, BOX, TERMINADO);
                break;
            }
            pthread_mutex_unlock(&data->forced_stop_mutex);
            pthread_mutex_unlock(&data->interupt_mutex);

            print_debug("BOX just passed another second\nAAAAAAAAAAAAAAAAAAAAA\n");
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
            (cars + ind_car)->box_time += 1;
        }

        pthread_mutex_lock(&(teams + num)->mutex_box_state);

        (teams + num)->n_cars_seg_mode -= 1;
        if ((teams + num)->n_cars_seg_mode == 0)
            (teams + num)->box_state = LIVRE;
        else
            (teams + num)->box_state = RESERVADO;
        pthread_mutex_unlock(&(teams + num)->mutex_box_state);

        sem_post(&(teams + num)->box_finished);
    }

    //sleep(4);
    sprintf(msg, "team %d a espera dos seus carros\n", num);
    print_debug(msg);
    free(ids);
    for (int i = 0; i < (teams + num)->n_cars; ++i)
        pthread_join((cars + num * data->max_car + i)->tid, NULL);

    if (close((teams + num)->fd[1]) == -1)
    {
        perror("ERROR: Failed to close unnamed pipe\n");
        exit(1);
    }

    sprintf(msg, "saiu team %d\n", num);
    print_debug(msg);
}
