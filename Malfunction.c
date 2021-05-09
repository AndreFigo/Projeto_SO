// Trabalho Prático - Simulador de Corridas
// André Carvalho 2019216156
// Sofia Alves 2019227240
// 20/21

#include "declarations.h"

void Malfunction_manager(int t_avaria)
{
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    print_debug("Malfuncion_manager\n");

    srand(time(NULL));



    while(1){


        data->interupt=0;
        data->tunits_passed = 0;
        data->cars_finished = 0;
        data->cars_ended_tunit = 0;
        data->n_malfuncs = 0;

        sem_wait(start_race);
        int send_damage;
        char warning[MAXWARNINGMSG], msg[MAXWARNINGMSG];


        while (1)
        {
            print_debug("Malfunction manager waiting for all cars\n");
            pthread_mutex_lock(&data->end_tunit_mutex);
            while (data->cars_ended_tunit != data->total_cars)
            {   
                pthread_cond_wait(&data->end_tunit, &data->end_tunit_mutex);
            }
            
            pthread_mutex_lock(&data->finish_mutex);
            data->cars_ended_tunit = data->cars_finished;

            sprintf(msg, "cars finished %d \n", data->cars_finished);
            print_debug(msg);

            if (data->cars_finished == data->total_cars){
                pthread_mutex_unlock(&data->finish_mutex);
                pthread_mutex_unlock(&data->end_tunit_mutex);
                break;
            }

            pthread_mutex_unlock(&data->finish_mutex);
            

            pthread_mutex_unlock(&data->end_tunit_mutex);

            //sleep in microseconds
            usleep(1000000 / data->u_time);

            // se houver sinal de estatisticas, esperamos que se faca uma copia
            pthread_mutex_lock(&data->stats_mutex);
            if (data->stats == 1)
            {
                sem_post(begin_copy);
                sem_wait(ended_copy);
                data->stats = 0;
            }
            pthread_mutex_unlock(&data->stats_mutex);

            //maybe use a sem instead
            pthread_mutex_lock(&data->new_tunit_mutex);
            data->tunits_passed += 1;
            pthread_cond_broadcast(&data->new_tunit);

            if (data->tunits_passed % t_avaria == 0)
            {
                print_debug("Malfunction manager generating\n");
                warning_message message;
                message.warning = 1;
                for (int i = 0; i < data->n_teams * data->max_car; i++)
                {
                    // no need because it hasnt passed a second yet
                    //sem_wait(&(cars+i)->state_mutex);
                    if ((cars + i)->num != -1 && (cars + i)->state < BOX && (cars + i)->malfunc ==0)
                    {   
                        // ind + 1 because mtype cant be zero
                        message.mtype = i+1;
                        // calcualte malfunc
                        send_damage = rand() % 100 + 1;
                        // send info to cars
                        if (send_damage > (cars + i)->reliability)
                        {
                            msgsnd(mqid, &message, sizeof(message) - sizeof(long), 0);
                            data->n_malfuncs++;
                            sprintf(warning, "MALFUNCTION DETECTED in car number %d\n", i);
                            app_log(warning);
                        }
                    }
                    //sem_post(&(cars+i)->state_mutex);
                }
            }
            print_debug("Malfunction ended new tunit\n");
            pthread_mutex_unlock(&data->new_tunit_mutex);
        }


        for(int i=0; i< data->total_cars;++i)
            sem_post(end_race);

        print_stats(cars,data->n_malfuncs);

        pthread_mutex_lock(&data->interupt_mutex);
        pthread_mutex_lock(&data->forced_stop_mutex);
        if (data->interupt==1 && data->stop==0){
            pthread_mutex_unlock(&data->forced_stop_mutex);
            pthread_mutex_unlock(&data->interupt_mutex);

            pthread_mutex_lock(& data->on_going_mutex);
            data->on_going=0;
            app_log("RACE INTERRUPTED SUCCESFULLY. READY TO START AGAIN\n");
            pthread_mutex_unlock(& data->on_going_mutex);
            continue;
        }else{
            pthread_mutex_unlock(&data->forced_stop_mutex);
            pthread_mutex_unlock(&data->interupt_mutex);
            break;
        }
    }


    for (int i=0; i< data->n_teams;++i){
        (teams+i)->ind_catual=-1;
        sem_post(&(teams+i)->entered_box);
    }

    sem_post(end_simulator);



    print_debug("Saiu malfunction manager\n");
}