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
    sprintf(msg, "Novo carro criado na team %d (Carro %d)\n", team_num, ind);
    print_debug(msg);

    //print_car_info(ind, team_num);
    float fuel_2laps = (float)(ceil((double)data->distance / (cars + ind)->speed) * (cars + ind)->consumption * 2);
    float fuel_4laps = 2 * fuel_2laps;

    while(1){


        //inicializacao do carro

        (cars + ind)->fuel = data->fuel_tank;
        (cars + ind)->distance = 0;
        (cars + ind)->malfunc = 0;
        (cars + ind)->laps_done = 0;
        (cars + ind)->n_stops = 0;
        (cars + ind)->state = CORRIDA;

        

        sem_wait(start_race);
        sprintf(msg, "carro %d entered race\n", ind);
        print_debug(msg);
        int elapsed = 0, dist_curr = 0, current_speed = (cars + ind)->speed;
        float current_cons = (cars + ind)->consumption;
        warning_message warning;

        while (1)
        {
            //inform that you are ready to wait for a tunit to pass
            sprintf(msg, "carro %d waiting for time unit (atual %d)\n", ind, elapsed);
            print_debug(msg);
            pthread_mutex_lock(&data->end_tunit_mutex);

            data->cars_ended_tunit += 1;
            pthread_cond_signal(&data->end_tunit);

            pthread_mutex_unlock(&data->end_tunit_mutex);

            if ((cars + ind)->state == TERMINADO || (cars + ind)->state == DESISTENCIA)
                break;

            // wait for a tunit to pass
            pthread_mutex_lock(&data->new_tunit_mutex);
            while (elapsed == data->tunits_passed)
            {
                pthread_cond_wait(&data->new_tunit, &data->new_tunit_mutex);
            }
            elapsed += 1;
            pthread_mutex_unlock(&data->new_tunit_mutex);
            sprintf(msg, "carro %d just started a new tunit\n", ind);
            print_debug(msg);

            //check malfunctions

            /*if (msgrcv(mqid, &warning, sizeof(warning_message) - sizeof(long), ind, IPC_NOWAIT) != -1)
            {
                //malfunc
                (cars + ind)->malfunc = 1;
                enter_safe_mode(team_num, ind, &current_speed, &current_cons);
                sprintf(msg, "carro %d just had a malfunction\n", ind);
                app_log(msg);
            }*/

            if (elapsed % data->u_time_malfunc == 0)
            {
                sprintf(msg, "carro %d checking malfunction\n", ind);
                print_debug(msg);
                pthread_mutex_lock(&(data->check_malf_mutex));

                if (msgrcv(mqid, &warning, sizeof(warning_message) - sizeof(long), ind + 1, IPC_NOWAIT) != -1)
                {
                    //malfunc
                    (cars + ind)->malfunc = 1;
                    enter_safe_mode(team_num, ind, &current_speed, &current_cons);
                    sprintf(msg, "carro %d just had a malfunction\n", ind);
                    app_log(msg);
                }
                //perror("Na message queue");
                else if (errno == ENOMSG)
                {
                }

                pthread_mutex_unlock(&(data->check_malf_mutex));
            }
            //sprintf(msg, "carro %d malf seen\n", ind);
            //print_debug(msg);

            //update fuel
            (cars + ind)->fuel -= current_cons;

            if ((cars + ind)->fuel < 0)
            {
                (cars + ind)->state = DESISTENCIA;
                sprintf(msg, "carro %d just gave up\n", ind);
                print_debug(msg);
                // comunicate
                communicate_status_changes(team_num, ind, SEGURANCA, DESISTENCIA);

                increment_cars_finished();

                continue;
            }

            //sprintf(msg, "carro %d fuel seen %f\n", ind, fuel_2laps);
            //print_debug(msg);
            if ((cars + ind)->fuel < fuel_2laps)
                enter_safe_mode(team_num, ind, &current_speed, &current_cons);

            //sprintf(msg, "carro %d distance begin\n", ind);
            //print_debug(msg);
            //update distance
            (cars + ind)->distance += current_speed;
            dist_curr += current_speed;

            if (dist_curr >= data->distance)
            {
                print_debug("Completed one more lap\n");
                (cars + ind)->laps_done += 1;
                pthread_mutex_lock(&data->interupt_mutex);
                pthread_mutex_lock(&data->forced_stop_mutex);

                if ((cars + ind)->laps_done == data->n_laps || data->stop == 1|| data->interupt==1)
                {
                    pthread_mutex_unlock(&data->forced_stop_mutex);
                    pthread_mutex_unlock(&data->interupt_mutex);

                    int last = (cars + ind)->state;
                    (cars + ind)->state = TERMINADO;
                    increment_cars_finished();
                    communicate_status_changes(team_num, ind, last, TERMINADO);
                    continue;
                }
                pthread_mutex_unlock(&data->forced_stop_mutex);
                pthread_mutex_unlock(&data->interupt_mutex);
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
            //sprintf(msg, "carro %d distance seen\n", ind);
            //print_debug(msg);

            //check box
            //check status
            //communicate status changes
        }

        sem_wait(end_race);

        pthread_mutex_lock(&data->interupt_mutex);
        pthread_mutex_lock(&data->forced_stop_mutex);

        if (data->stop==0 && data->interupt==1){
            pthread_mutex_unlock(&data->forced_stop_mutex);
            pthread_mutex_unlock(&data->interupt_mutex);
            continue;
        }
        pthread_mutex_unlock(&data->forced_stop_mutex);
        pthread_mutex_unlock(&data->interupt_mutex);
        break;
        


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
    char msg[MAXTAMLINE];

    if ((cars + ind)->state == CORRIDA)
    {
        (cars + ind)->state = SEGURANCA;

        *(cur_cons) = 0.4 * (cars + ind)->consumption;
        *(cur_speed) = (int)round(0.3 * (cars + ind)->speed);

        //print_debug("atualizou estado e speed\n");

        reserve_box(team_num);
        sprintf(msg, "Carro %d reservou a box %d\n", ind, team_num);
        print_debug(msg);

        //print_debug("Reserved box\n");

        //comunicar com a race manager
        communicate_status_changes(team_num, ind, CORRIDA, SEGURANCA);

        //print_debug("Communicated changes\n");

    }
}

void increment_cars_finished()
{

    pthread_mutex_lock(&data->finish_mutex);
    data->cars_finished += 1;
    pthread_mutex_unlock(&data->finish_mutex);
}

void reserve_box(int team_num)
{
    //lock
    sem_wait(&(teams + team_num)->mutex_box_state);
    //print_debug("Entrou mutex\n");

    (teams + team_num)->n_cars_seg_mode += 1; //check
    if ((teams + team_num)->box_state == LIVRE)
        (teams + team_num)->box_state = RESERVADO;

    sem_post(&(teams + team_num)->mutex_box_state);
    //print_debug("saiu mutex\n");
}

void enter_box(int team_num, int ind, int last)
{
    //check if it is
    (cars + ind)->state = BOX;
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
    if ((cars + ind)->state == BOX){
        (cars + ind)->state = CORRIDA;
        communicate_status_changes(team_num, ind, BOX, CORRIDA);

    }
    //communicate status change
}

void communicate_status_changes(int team, int ind, int last, int current)
{

    state_change change;
    change.current=current;
    change.last=last;
    change.ind=ind;

    write((teams + team)->fd[1], &change, sizeof(state_change));
}
