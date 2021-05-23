// Trabalho Prático - Simulador de Corridas
// André Carvalho 2019216156
// Sofia Alves 2019227240
// 20/21


#include "declarations.h"

void terminate()
{
    /* Destroy semaphores */
    terminate_sem();

    if (close(data->logfile) == -1)
    {
        perror("ERROR: Failed to close logfile\n");
        exit(1);
    }
    //unlink(LOGFILE);
    //^this deletes the files

    if (shmdt(data) == -1)
    {
        perror("ERROR: Failed to dettach shared memory\n");
        exit(1);
    }
    if (shmctl(shmid, IPC_RMID, NULL) == -1)
    {
        perror("ERROR: Failed to remove shared memory\n");
        exit(1);
    }

    /* Destroying Named Pipe */
    if (unlink(PIPE_NAME) == -1)
    {
        perror("ERROR: Failed to unlink named pipe\n");
        exit(1);
    }

    /* Destroying Message Queue */
    if (msgctl(mqid, IPC_RMID, 0) == -1)
    {
        perror("ERROR: Failed to destroy message queue\n");
        exit(1);
    }
}

void terminate_sem()
{

    sem_unlink("START");
    if (sem_close(start_race) == -1)
    {
        perror("ERROR: Failed to close semaphore START\n");
        exit(1);
    }
    sem_unlink("END");
    if (sem_close(end_race) == -1)
    {
        perror("ERROR: Failed to close semaphore END\n");
        exit(1);
    }

    sem_unlink("END_SIM");
    if (sem_close(end_simulator) == -1)
    {
        perror("ERROR: Failed to close semaphore END_SIM\n");
        exit(1);
    }

    sem_unlink("BEG_COPY");
    if (sem_close(begin_copy) == -1)
    {
        perror("ERROR: Failed to close semaphore BEG_COPY\n");
        exit(1);
    }

    sem_unlink("END_COPY");
    if (sem_close(ended_copy) == -1)
    {
        perror("ERROR: Failed to close semaphore END_COPY\n");
        exit(1);
    }

    sem_unlink("PID");
    if (sem_close(pid_ready) == -1)
    {
        perror("ERROR: Failed to close semaphore END_COPY\n");
        exit(1);
    }

    for (int i = 0; i < data->n_teams; ++i)
    {
        if (sem_destroy(&((teams + i)->car_ready)) == -1)
        {
            perror("ERROR: Failed to destroy car_ready semaphore\n");
            exit(1);
        }

        if (sem_destroy(&((teams + i)->box_finished)) == -1)
        {
            perror("ERROR: Failed to destroy box_finished semaphore\n");
            exit(1);
        }

        if (sem_destroy(&((teams + i)->entered_box)) == -1)
        {
            perror("ERROR: Failed to destroy entered_box semaphore\n");
            exit(1);
        }
        if (pthread_mutex_destroy(&((teams + i)->mutex_box_state)) != 0)
        {
            perror("ERROR: Failed to destroy mutex_box_state mutex\n");
            exit(1);
        }
        if (pthread_mutex_destroy(&((teams + i)->pipe_write_mutex)) != 0)
        {
            perror("ERROR: Failed to destroy pipe_write_mutex mutex\n");
            exit(1);
        }
    }

    /* Destroying mutexes */
    if (pthread_mutex_destroy(&(data->finish_mutex)) != 0)
    {
        perror("ERROR: Failed to destroy finish_mutex mutex\n");
        exit(1);
    }
    if (pthread_mutex_destroy(&(data->new_tunit_mutex)) != 0)
    {
        perror("ERROR: Failed to destroy new_tunit_mutex mutex\n");
        exit(1);
    }
    if (pthread_mutex_destroy(&(data->end_tunit_mutex)) != 0)
    {
        perror("ERROR: Failed to destroy end_tunit_mutex mutex\n");
        exit(1);
    }
    if (pthread_mutex_destroy(&(data->stats_mutex)) != 0)
    {
        perror("ERROR: Failed to destroy stats_mutex mutex\n");
        exit(1);
    }
    if (pthread_mutex_destroy(&(data->forced_stop_mutex)) != 0)
    {
        perror("ERROR: Failed to destroy forced_stop_mutex mutex\n");
        exit(1);
    }
    if (pthread_mutex_destroy(&(data->interupt_mutex)) != 0)
    {
        perror("ERROR: Failed to destroy forced_stop_mutex mutex\n");
        exit(1);
    }
    if (pthread_mutex_destroy(&(data->log_mutex)) != 0)
    {
        perror("ERROR: Failed to destroy log_mutex mutex\n");
        exit(1);
    }
    if (pthread_mutex_destroy(&(data->on_going_mutex)) != 0)
    {
        perror("ERROR: Failed to destroy log_mutex mutex\n");
        exit(1);
    }

    /* Destroying mutex attribute */
    if (pthread_mutexattr_destroy(&attrmutex) != 0)
    {
        perror("ERROR: Failed to destroy mutex attribute\n");
        exit(1);
    }

    /* Destroying condition variables */
    if (pthread_cond_destroy(&(data->new_tunit)) != 0)
    {
        perror("ERROR: Failed to destroy condition variable new_tunit\n");
        exit(1);
    }
    if (pthread_cond_destroy(&(data->end_tunit)) != 0)
    {
        perror("ERROR: Failed to destroy condition variable end_tunit\n");
        exit(1);
    }

    if (pthread_condattr_destroy(&attrcondv) != 0)
    {
        perror("ERROR: Failed to destroy condition variable\n");
        exit(1);
    }
}
