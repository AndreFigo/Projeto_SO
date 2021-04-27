// Trabalho Prático - Simulador de Corridas
// André Carvalho 2019216156
// Sofia Alves 2019227240
// 20/21

#include "declarations.h"

void Malfunction_manager(int t_avaria)
{

    print_debug("Malfuncion_manager\n");

    sem_wait(start_race);
    int elapsed = 0;
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
        pthread_mutex_unlock(&data->new_tunit_mutex);
        elapsed += 1;

        // data->tunits_passed % data->u_time_malfunc ==0
        if (elapsed == data->u_time_malfunc)
        {

            // calcualte malfunc
            // send info to cars
            elapsed = 0;
        }
    }

    //to do

    //sleep(3);

    print_debug("Saiu malfunction manager\n");
}