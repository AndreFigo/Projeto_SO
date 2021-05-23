// Trabalho Prático - Simulador de Corridas
// André Carvalho 2019216156
// Sofia Alves 2019227240
// 20/21


#include "declarations.h"

void sigint(int signo)
{

    pthread_mutex_lock(&data->on_going_mutex);

    if (data->on_going == 0)
    {
        pthread_mutex_unlock(&data->on_going_mutex);
        sem_wait(pid_ready);
        sigint_before_race_Sim();
    }

    pthread_mutex_unlock(&data->on_going_mutex);

    app_log("^C pressed. Terminating race\n");

    pthread_mutex_lock(&data->forced_stop_mutex);
    data->stop = 1;
    pthread_mutex_unlock(&data->forced_stop_mutex);

    while (1)
    {
        if (sem_wait(end_simulator) == -1)
        {
            if (errno == EINTR)
                continue;
        }
        break;
    }

    // wait for child processes
    for (int i = 0; i < 2; i++)
        wait(NULL);

    app_log("SIMULATOR CLOSING\n");
    terminate();
    exit(0);
}

void sigint_before_race_Sim()
{

    app_log("^C PRESSED. SIMULATOR LEAVING\n");

    kill(data->malfunc_pid, SIGKILL);
    kill(data->manager_pid, SIGKILL);
    for (int i = 0; i < data->n_teams; ++i)
        kill((teams + i)->team_pid, SIGKILL);

    waitpid(data->malfunc_pid, NULL, 0);
    waitpid(data->manager_pid, NULL, 0);
    for (int i = 0; i < data->n_teams; ++i)
        waitpid((teams + i)->team_pid, NULL, 0);

    terminate();
    exit(0);
}

void sigusr1(int signo)
{

    pthread_mutex_lock(&data->interupt_mutex);
    data->interupt = 1;
    pthread_mutex_unlock(&data->interupt_mutex);

    app_log("SIGUSR1 call, interupting race\n");
}

void init_sigusr1()
{
    sigemptyset(&interupt_race.sa_mask);
    interupt_race.sa_flags = 0;
    interupt_race.sa_handler = &sigusr1;
    sigaction(SIGUSR1, &interupt_race, NULL);
}

void ignore_sigusr1()
{
    interupt_race.sa_handler = SIG_IGN;
    sigaction(SIGUSR1, &interupt_race, NULL);
}

void ignore_sigint()
{
    sigemptyset(&finish_race.sa_mask);
    finish_race.sa_flags = 0;
    finish_race.sa_handler = SIG_IGN;

    sigaction(SIGINT, &finish_race, NULL);
}

void ignore_sigtstp()
{
    sigemptyset(&print_est.sa_mask);
    print_est.sa_flags = 0;
    print_est.sa_handler = SIG_IGN;
    sigaction(SIGTSTP, &print_est, NULL);
}

void ignore_signals()
{
    ignore_sigint();
    ignore_sigtstp();
}

void init_sigint()
{

    sigemptyset(&finish_race.sa_mask);
    finish_race.sa_flags = 0;
    finish_race.sa_handler = &sigint;
    sigaction(SIGINT, &finish_race, NULL);
}

void init_sigtstp()
{
    sigemptyset(&print_est.sa_mask);
    sigaddset(&print_est.sa_mask, SIGINT);
    print_est.sa_flags = 0;

    print_est.sa_handler = &sigtstp;
    sigaction(SIGTSTP, &print_est, NULL);
}

void sigtstp(int signo)
{

    pthread_mutex_lock(&data->on_going_mutex);

    if (data->on_going == 0)
    {
        pthread_mutex_unlock(&data->on_going_mutex);
        return;
    }

    pthread_mutex_unlock(&data->on_going_mutex);

    //print estatisticas
    app_log("^Z pressed. Showing stats\n");

    pthread_mutex_lock(&data->stats_mutex);

    data->stats = 1;
    pthread_mutex_unlock(&data->stats_mutex);

    car *copy = (car *)malloc(sizeof(car) * data->max_car * data->n_teams);

    sem_wait(begin_copy);
    print_debug("Began copy\n");

    memcpy(copy, cars, sizeof(car) * data->max_car * data->n_teams);
    int n_malf = data->n_malfuncs;

    sem_post(ended_copy);
    print_debug("Ended copy\n");

    print_stats(copy, n_malf);

    free(copy);
}

void print_stats(car *c, int n_malf)
{

    int seen[5];
    for (int j = 0; j < 5; j++)
        seen[j] = -1;
    int ind;

    char separator[MAXSEPARATOR] = "----------------------------------------\n";
    char stats[7][200] = {0};

    for (int j = 0; j < 5; j++)
    {
        ind = max_distance(c, data->max_car * data->n_teams, seen, 5);
        seen[j] = ind;
        if (ind > -1)
        {
            // escrever os dados da equipa ind
            sprintf(stats[j], "%d place: Num-> %d, Team-> %s, Laps-> %d, Stops-> %d, Distance-> %d, Time-> %d\n", j + 1, (cars + ind)->num, (teams + (cars + ind)->ind_team)->name, (cars + ind)->laps_done, (cars + ind)->n_stops, (cars + ind)->distance, (cars + ind)->time_passed);
        }
        else
        {
            print_debug("Ha menos de 5 equipas, logo o top 5 esta incompleto\n");
            break;
        }
    }

    // escrever o pior
    strncat(stats[4], separator, MAXSEPARATOR);

    ind = last_place(c, data->max_car * data->n_teams);
    sprintf(stats[5], "Last Place: Num-> %d, Team-> %s, Laps-> %d, Stops-> %d, Distance-> %d, Time-> %d\n", (cars + ind)->num, (teams + (cars + ind)->ind_team)->name, (cars + ind)->laps_done, (cars + ind)->n_stops, (cars + ind)->distance, (cars + ind)->time_passed);
    strncat(stats[5], separator, MAXSEPARATOR);

    //escrevr os stops
    int n_stops = 0, on_track = 0;
    on_track_and_total_stops(&n_stops, &on_track, c, data->max_car * data->n_teams);
    sprintf(stats[6], "Total stops: %d\nTotal malfunctions: %d\nOn track: %d\n", n_stops, n_malf, on_track);

    strncat(stats[6], separator, MAXSEPARATOR);

    pthread_mutex_lock(&data->log_mutex);
    time_t now;
    time(&now);
    struct tm *timeinfo = localtime(&now);

    char date[20];
    sprintf(date, "%d:%d:%d STATS\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    write(1, date, strlen(date));
    write(data->logfile, date, strlen(date));

    for (int i = 0; i < 7; ++i)
    {
        write(1, stats[i], strlen(stats[i]));
        write(data->logfile, stats[i], strlen(stats[i]));
    }
    pthread_mutex_unlock(&data->log_mutex);
}

void on_track_and_total_stops(int *n_stops, int *on_track, car *copy, int len)
{
    for (int i = 0; i < len; ++i)
    {
        if ((copy + i)->num != -1)
        {
            if ((copy + i)->state == CORRIDA || (copy + i)->state == SEGURANCA)
                (*on_track)++;

            (*n_stops) += (copy + i)->n_stops;
        }
    }
}

int last_place(car *copy, int len)
{

    int min = data->distance * (data->n_laps + 1);
    int ind_min = -1, max_time = -1;
    ;
    for (int i = 0; i < len; i++)
    {
        if ((copy + i)->num != -1 && ((copy + i)->distance < min || ((copy + i)->distance == min && (copy + i)->time_passed > max_time)))
        {
            min = (copy + i)->distance;
            max_time = (copy + i)->time_passed;
            ind_min = i;
        }
    }
    return ind_min;
}

int max_distance(car *copy, int len, int *seen, int len2)
{
    int max = -1, used, ind_max = -1, min_time = 999999;

    for (int i = 0; i < len; i++)
    {
        if ((copy + i)->num == -1)
            continue;
        used = 0;
        for (int j = 0; j < len2; j++)
        {
            if (seen[j] == i)
            {
                used = 1;
                break;
            }
            if (seen[j] == -1)
                break;
        }
        if (!used && ((copy + i)->distance > max || ((copy + i)->distance == max && (copy + i)->time_passed < min_time)))
        {
            max = (copy + i)->distance;
            min_time = (copy + i)->time_passed;
            ind_max = i;
        }
    }
    return ind_max;
}