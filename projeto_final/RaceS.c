// Trabalho Prático - Simulador de Corridas
// André Carvalho 2019216156
// Sofia Alves 2019227240
// 20/21

#include "declarations.h"

int main()
{
    ignore_sigtstp();

    if ((logfile = creat(LOGFILE, S_IRWXU | S_IROTH | S_IWOTH | S_IRGRP | S_IWGRP)) == -1)
    {
        perror("Erro a criar o logfile\n");
        exit(1);
    }

    float *config = configurationRead();
    print_debug_no_sem("Ficheiro de configuracoes lido\n");

    init(config);

    app_log("SIMULATOR STARTING\n");
    pid_t aux;
    if ((aux = fork()) == 0)
    {
        Race_manager(data->n_teams);
        exit(0);
    }

    data->manager_pid = aux;

    if ((aux = fork()) == 0)
    {
        Malfunction_manager(data->u_time_malfunc);
        exit(0);
    }

    data->malfunc_pid = aux;

    init_sigint();

    sem_wait(start_race);

    print_debug("Simulator entered race\n");
    init_sigtstp();

    // sinais
    while (1)
    {
        if (sem_wait(end_simulator) == -1)
        {
            if (errno == EINTR)
                continue;
            else
            {
                //true error
            }
        }
        break;
    }

    ignore_signals();

    for (int i = 0; i < 2; ++i)
        wait(NULL);

    print_debug("Terminating everything\n");

    app_log("SIMULATOR CLOSING\n");

    terminate();

    return 0;
}

void app_log(char *msg)
{

    pthread_mutex_lock(&data->log_mutex);
    time_t now;
    time(&now);
    struct tm *timeinfo = localtime(&now);
    char log_msg[MAXERRORMSG];
    sprintf(log_msg, "%d:%d:%d %s", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, msg);

    write(1, log_msg, strlen(log_msg));
    write(data->logfile, log_msg, strlen(log_msg));
    pthread_mutex_unlock(&data->log_mutex);
}

void print_debug(char *msg)
{
#if DEBUG
    pthread_mutex_lock(&data->log_mutex);
    printf("%s", msg);
    pthread_mutex_unlock(&data->log_mutex);

#endif
}

void print_debug_no_sem(char *msg)
{
#if DEBUG
    printf("%s", msg);
#endif
}