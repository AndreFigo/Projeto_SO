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

    //inicializacao do carro

    (cars + ind)->fuel = data->fuel_tank;
    (cars + ind)->distance = 0;
    (cars + ind)->n_stops = 0;
    (cars + ind)->laps_done = 0;
    (cars + ind)->state = CORRIDA;

    float fuel_2laps = (float)(ceil((double)data->distance / (cars + ind)->speed) * (cars + ind)->consumption * 2);
    float fuel_4laps = 2 * fuel_2laps;

    sem_wait(start_race);
    int elapsed = 0, dist_curr = 0, current_speed = (cars + ind)->speed;
    float current_cons = (cars + ind)->consumption;
    warning_message warning;

    while (1)
    {
        //inform that you are ready to wait for a tunit to pass
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

        //check malfunctions
        if (elapsed % data->u_time_malfunc == 0)
        {
            if (msgrcv(mqid, &warning, sizeof(warning_message), IPC_NOWAIT) != -1)
            {
                //malfunc
                enter_safe_mode(team_num, ind, &current_speed, &current_cons);
            }
            if (errno == ENOMSG)
            {
                //all good
            }
        }

        //update fuel
        (cars + ind)->fuel -= current_cons;

        if ((cars + ind)->fuel < 0)
            (cars + ind)->state == DESISTENCIA;

        if ((cars + ind)->fuel < fuel_2laps)
            enter_safe_mode(team_num, ind, &current_speed, &current_cons);

        //update distance
        (cars + ind)->distance += current_speed;
        dist_curr += current_speed;
        if (dist_curr >= data->distance)
        {
            (cars + ind)->laps_done += 1;
            sem_wait(forced_stop);
            if ((cars + ind)->laps_done == data->n_laps || data->stop == 1)
            {
                sem_post(forced_stop);

                (cars + ind)->state == TERMINADO;
                pthread_mutex_lock(&data->finish_mutex);
                data->cars_finished += 1;
                pthread_cond_broadcast(&data->all_finished);
                pthread_mutex_unlock(&data->finish_mutex);
                break;
            }
            sem_post(forced_stop);
            dist_curr -= data->distance;

            sem_wait(&(teams + team_num)->mutex_box_state);

            if ((cars + ind)->state == SEGURANCA && (teams + team_num)->box_state < OCUPADO || (cars + ind)->fuel < fuel_4laps && (teams + team_num)->box_state == LIVRE)
            {
                sem_post(&(teams + team_num)->mutex_box_state);
                try_enter_box(team_num, ind);
            }
            else
                sem_post(&(teams + team_num)->mutex_box_state);
        }

        //check box
        //check status
        //communicate status changes
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

void enter_safe_mode(int team_num, int ind, int *cur_speed, float *cur_cons)
{

    if ((cars + ind)->state == CORRIDA)
    {
        (cars + ind)->state == SEGURANCA;

        *(cur_cons) = 0.4 * (cars + ind)->consumption;
        *(cur_speed) = (int)round(0.3 * (cars + ind)->speed);
        reserve_box(team_num);

        //comunicar com a race manager
        //unnamed pipe
    }
}

void reserve_box(int team_num)
{
    sem_wait(&(teams + team_num)->mutex_box_state);

    (teams + team_num)->n_cars_seg_mode += 1; //check
    if ((teams + team_num)->box_state == LIVRE)
        (teams + team_num)->box_state = RESERVADO;

    sem_wait(&(teams + team_num)->mutex_box_state);
}

void try_enter_box(int team_num, int ind)
{
    //check if it is
    if ((sem_trywait(&(teams + team_num)->box_access)) == 0)
    {
        (cars + ind)->state == BOX;
        sem_post(&(teams + team_num)->entered_box);

        // wait for the box to do its job
        sem_wait(&(teams + team_num)->box_finished);

        //car can leave
        (cars + ind)->state == CORRIDA;
        sem_post(&(teams + team_num)->box_access);
    }
}