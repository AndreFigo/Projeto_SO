// Trabalho Prático - Simulador de Corridas
// André Carvalho 2019216156
// Sofia Alves 2019227240
// 20/21

#include "declarations.h"

void Malfunction_manager(int t_avaria)
{

    print_debug("Malfuncion_manager\n");

    srand(time(NULL));

    sem_wait(start_race);
    int elapsed = 0, send_damage;
    char warning[MAXWARNINGMSG];
    while (1)
    {
        pthread_mutex_lock(&data->end_tunit_mutex);
        while (data->cars_ended_tunit != data->total_cars)
        {
            pthread_cond_wait(&data->end_tunit, &data->end_tunit_mutex);
        }
        data->cars_ended_tunit = 0;
        pthread_mutex_unlock(&data->end_tunit_mutex);

        //sleep in microseconds
        usleep(1000000 / data->u_time);

        //maybe use a sem instead
        pthread_mutex_lock(&data->new_tunit_mutex);
        data->tunits_passed += 1;
        pthread_cond_broadcast(&data->new_tunit);
        elapsed += 1;

        elapsed += 1;

        // data->tunits_passed % t_avaria ==0
        if (elapsed == t_avaria)
        {
            elapsed = 0;
            warning_message message;
            message.warning = 1;
            for (int i = 0; i < data->n_teams * data->max_car; i++)
            {
                if ((cars + i)->num != -1)
                {
                    message.mtype = i;
                    // calcualte malfunc
                    send_damage = rand() % 100 + 1;
                    // send info to cars
                    if (send_damage > (cars + i)->reliability)
                    {
                        msgsnd(mqid, &message, sizeof(message) - sizeof(long), 0);

                        sprintf(warning, "MALFUNCTION DETECTED - car %d\n", i);
                        app_log(warning);
                    }
                }
            }
        }
        pthread_mutex_unlock(&data->new_tunit_mutex);

        //to do

        //sleep(3);

        print_debug("Saiu malfunction manager\n");
    }