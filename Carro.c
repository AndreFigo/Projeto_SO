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
    (cars + ind)->malfunc = 0;
    (cars + ind)->laps_done = 0;
    (cars + ind)->n_stops = 0;
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
            if (msgrcv(mqid, &warning, sizeof(warning_message) - sizeof(long), ind, IPC_NOWAIT) != -1)
            {
                //malfunc
                (cars + ind)->malfunc = 1;
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
        {
            (cars + ind)->state == DESISTENCIA;
            pthread_cond_broadcast(&data->all_finished);
            // comunicate
            communicate_status_changes(team_num, ind, SEGURANCA, DESISTENCIA);

            break;
        }

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
                int last = (cars + ind)->state;
                (cars + ind)->state == TERMINADO;
                communicate_status_changes(team_num, ind, last, TERMINADO);
                pthread_mutex_lock(&data->finish_mutex);
                data->cars_finished += 1;
                pthread_cond_broadcast(&data->all_finished);
                pthread_mutex_unlock(&data->finish_mutex);
                break;
            }
            sem_post(forced_stop);
            dist_curr -= data->distance;

            sem_wait(&(teams + team_num)->mutex_box_state);

            if (((cars + ind)->state == SEGURANCA && (teams + team_num)->box_state < OCUPADO) || ((cars + ind)->fuel < fuel_4laps && (teams + team_num)->box_state == LIVRE))
            {
                int last = CORRIDA;
                if ((cars + ind)->state == SEGURANCA)
                    last = SEGURANCA;
                sem_post(&(teams + team_num)->mutex_box_state);

                enter_box(team_num, ind, last);
                elapsed += (cars + ind)->box_time;
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
        communicate_status_changes(team_num, ind, CORRIDA, SEGURANCA);
        //unnamed pipe
    }
}

void reserve_box(int team_num)
{
    //lock
    sem_wait(&(teams + team_num)->mutex_box_state);

    (teams + team_num)->n_cars_seg_mode += 1; //check
    if ((teams + team_num)->box_state == LIVRE)
        (teams + team_num)->box_state = RESERVADO;

    sem_wait(&(teams + team_num)->mutex_box_state);
}

void enter_box(int team_num, int ind, int last)
{
    //check if it is
    (cars + ind)->state == BOX;
    (teams + team_num)->ind_catual = ind;

    sem_wait(&(teams + team_num)->mutex_box_state);
    (teams + team_num)->box_state = OCUPADO;
    sem_post(&(teams + team_num)->mutex_box_state);

    //communicate status change
    communicate_status_changes(team_num, ind, last, BOX);

    sem_post(&(teams + team_num)->entered_box);

    pthread_mutex_lock(&(cars + ind)->n_stops_mutex);

    (cars + ind)->n_stops += 1;

    pthread_mutex_unlock(&(cars + ind)->n_stops_mutex);

    // wait for the box to do its job
    sem_wait(&(teams + team_num)->box_finished);
    (cars + ind)->malfunc = 0;

    //car can leave
    (cars + ind)->state == CORRIDA;
    communicate_status_changes(team_num, ind, BOX, CORRIDA);
    //communicate status change
}

void communicate_status_changes(int team, int ind, int last, int current)
{
    char estados[5][20] = {"CORRIDA", "SEGURANCA", "BOX", "DESISTENCIA", "TERMINADO"};

    char msg[100];
    sprintf(msg, "Carro %d passou do modo %s para o modo %s\n", (cars + ind)->num, estados[last], estados[current]);

    write((teams + team)->fd[1], msg, strlen(msg));
}