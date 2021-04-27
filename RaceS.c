// Trabalho Prático - Simulador de Corridas
// André Carvalho 2019216156
// Sofia Alves 2019227240
// 20/21

#include "declarations.h"

int verify(float *config)
{
    if (config[0] <= 0)
    {
        fprintf(stderr, "Erro na colocacao unidades de tempo.");
        return -1;
    }
    else if (config[1] <= 0)
    {
        fprintf(stderr, "Erro na colocacao da distancia de cada volta.");
        return -1;
    }
    else if (config[2] <= 0)
    {
        fprintf(stderr, "Erro na colocacao do numero de voltas.");
        return -1;
    }
    else if (config[3] < 3)
    {
        fprintf(stderr, "Numero de equipas devera ser no minimo 3.");
        return -1;
    }
    else if (config[4] < 1)
    {
        fprintf(stderr, "Erro na colocacao do numero maximo de carros por equipa.");
        return -1;
    }
    else if (config[5] <= 0)
    {
        fprintf(stderr, "Erro na colocacao das unidades de tempo entre o novo calculo de uma avaria.");
        return -1;
    }
    else if (config[6] <= 0)
    {
        fprintf(stderr, "Erro na colocacao do tempo minimo de reparacao.");
        return -1;
    }
    else if (config[7] <= 0)
    {
        fprintf(stderr, "Erro na colocacao do tempo maximo de reparacao.");
        return -1;
    }
    else if (config[8] <= 0)
    {
        fprintf(stderr, "Erro na colocacao da capacidade do deposito de combustivel.");
        return -1;
    }
    return 0;
}

void init_sem()
{
    print_debug_no_sem("Criando os semaforos\n");
    sem_unlink("LOG_MUTEX");
    if ((log_mutex = sem_open("LOG_MUTEX", O_CREAT | O_EXCL, 0766, 1)) == SEM_FAILED)
    {
        perror("ERROR: Failed to create semaphore\n");
        exit(1);
    }

    for (int i = 0; i < data->n_teams; ++i)
    {
        if (sem_init(&((teams + i)->car_ready), 1, 0) != 0)
        {
            fprintf(stderr, "Problemas a inicializar o semaforo %d das equipas\n", i);
            exit(1);
        }
    }

    sem_unlink("START");
    if ((start_race = sem_open("START", O_CREAT | O_EXCL, 0766, 0)) == SEM_FAILED)
    {
        perror("ERROR: Failed to create semaphore\n");
        exit(1);
    }

    /* Initialize attribute of mutex. */
    pthread_mutexattr_init(&(data->attrmutex));
    pthread_mutexattr_setpshared(&(data->attrmutex), PTHREAD_PROCESS_SHARED);

    /* Initialize attribute of condition variable. */
    pthread_condattr_init(&(data->attrcondv));
    pthread_condattr_setpshared(&(data->attrcondv), PTHREAD_PROCESS_SHARED);

    /* Initialize mutex. */
    pthread_mutex_init(&(data->finish_mutex), &(data->attrcondv));

    /* Initialize condition variables. */
    pthread_cond_init(&(data->all_finished), &(data->attrcondv));

    //to do
}

void sigint(int signo)
{

    //finish race
    //wait for everyone using cond variable

    pthread_mutex_lock(&data->finished_mutex);

    while (data->cars_finished != data->total_cars)
    {
        pthread_cond_wait(&data->all_finished, &data->finished_mutex);
    }

    pthread_mutex_unlock(&data->finished_mutex);

    // wait for child processes
    for (int i = 0; i < 2; i++)
        wait(NULL);

    terminate();
}

void sgtstp(int signo)
{

    //print estatisticas
    //top 5
    //last place
    // total avarias
    //total abastecimentos
    // numero de carros em pista
}

void init(float *config)
{
    if (verify(config) == -1)
        exit(1);

    int n_teams = (int)config[3];
    int max_car = (int)config[4];

    print_debug_no_sem("Criando a memoria partilhada\n");
    if ((shmid = shmget(IPC_PRIVATE, sizeof(info_struct) + n_teams * sizeof(teams) + max_car * n_teams * sizeof(car), IPC_CREAT | 0777)) < 0)
    {
        perror("Erro no shmget com IPC_CREAT\n");
        exit(1);
    }

    if ((data = (info_struct *)shmat(shmid, NULL, 0)) == (info_struct *)-1)
    {
        perror("ERRO no shmat.\n");
        exit(1);
    }

    teams = (team *)data + 1;
    cars = (car *)teams + n_teams;

    data->logfile = logfile;

    data->u_time = (int)config[0];
    data->distance = (int)config[1];
    data->n_laps = (int)config[2];
    data->u_time_malfunc = (int)config[5];
    data->T_Box_min = (int)config[6];
    data->T_Box_Max = (int)config[7];
    data->fuel_tank = config[8];
    data->n_teams = n_teams;
    data->max_car = max_car;
    data->total_cars = 0;
    data->cars_finished = 0;

    free(config);

    for (int i = 0; i < data->n_teams; ++i)
    {
        for (int j = 0; j < data->max_car; ++j)
        {
            (cars + i * data->n_teams + j)->ind_team = i;
            (cars + i * data->n_teams + j)->num = -1;
        }
        //inicilizar o numero do carro ^

        //inicilizar o nome da equipa e o numero de carros
        strcpy((teams + i)->name, "");
        (teams + i)->n_cars = 0;
    }

    unlink(PIPE_NAME);

    if ((mkfifo(PIPE_NAME, O_CREAT | O_EXCL | 0600) < 0) && (errno != EEXIST))
    {
        perror("Cannot create pipe: ");
        exit(0);
    }

    sigemptyset(&print_est.sa_mask);
    print_est.sa_flags = 0;
    print_est.sa_handler = &sigtstp;

    sigemptyset(&finish_race.sa_mask);
    finish_race.sa_flags = 0;
    finish_race.sa_handler = &sigint;

    sigaction(SIGINT, &print_est, NULL);
    sigaction(SIGTSTP, &finish_race, NULL);

    init_sem();
}

void terminate()
{

    if (sem_close(log_mutex) == -1)
    {
        perror("ERROR: Failed to close semaphore");
        exit(1);
    }
    sem_unlink("LOG_MUTEX");
    if (sem_close(start_race) == -1)
    {
        perror("ERROR: Failed to close semaphore");
        exit(1);
    }
    sem_unlink("START");

    for (int i = 0; i < data->n_teams; ++i)
    {
        if (sem_destroy(&((teams + i)->car_ready)) == -1)
        {
            perror("ERROR: Failed to destroy semaphore");
            exit(1);
        }
    }

    if (close(data->logfile) == -1)
    {
        perror("ERROR: Failed to close logfile");
        exit(1);
    }
    //unlink(LOGFILE);

    if (shmdt(data) == -1)
    {
        perror("ERROR: Failed to dettach shared memory");
        exit(1);
    }
    if (shmctl(shmid, IPC_RMID, NULL) == -1)
    {
        perror("ERROR: Failed to remove shared memory");
        exit(1);
    }
}

int main()
{
    if ((logfile = creat(LOGFILE, S_IRWXU | S_IROTH | S_IWOTH | S_IRGRP | S_IWGRP)) == -1)
    {
        perror("Erro a criar o logfile\n");
        exit(1);
    }

    float *config = configurationRead();
    print_debug_no_sem("Ficheiro de configuracoes lido\n");

    init(config);

    for (int i = 0; i < 2; ++i)
    {
        if (fork() == 0)
        {
            if (i == 0)
            {
                Malfunction_manager(data->u_time_malfunc);
                exit(0);
            }
            else
            {
                Race_manager(data->n_teams);
                exit(0);
            }
        }
    }

    // sinais

    // a mudar
    for (int i = 0; i < 2; ++i)
        wait(NULL);

    print_debug("Terminating everything\n");

    terminate();

    return 0;
}

int read1f(float *config, int pos, FILE *fich)
{
    int read = fscanf(fich, "%f", &(config[pos]));
    if (read != 1)
        return 0;
    else
        return 1;
}

int read1int(float *config, int pos, FILE *fich)
{
    int a;
    int read = fscanf(fich, "%d", &a);
    config[pos] = (float)a;
    if (read != 1)
        return 0;
    else
        return 1;
}

int read2int(float *config, int pos, FILE *fich)
{
    int a, b;
    int read = fscanf(fich, "%d, %d", &a, &b);
    config[pos] = (float)a;
    config[pos + 1] = (float)b;

    if (read != 2)
        return 0;
    else
        return 1;
}

float *configurationRead()
{

    float *config = (float *)malloc(sizeof(int) * NINPUTS);

    FILE *fich = fopen("config.txt", "r");
    int read = 0;

    read += read1int(config, 0, fich);
    read += read2int(config, 1, fich);
    read += read1int(config, 3, fich);
    read += read1int(config, 4, fich);
    read += read1int(config, 5, fich);
    read += read2int(config, 6, fich);
    read += read1f(config, 8, fich);
    char lixo[100];
    if (fscanf(fich, "%s", lixo) >= 0)
    {
        fprintf(stderr, "Erro na leitura do ficheiro de configurações.\n");
        exit(-1);
    }

    if (read == 7)
        return config;

    fprintf(stderr, "Erro na leitura do ficheiro de configurações.\n");
    exit(1);
}

void log_errors(char *msg)
{

    sem_wait(log_mutex);

    time(&data->time);
    char *date = ctime(&data->time);
    date[(int)strlen(date) - 1] = ' ';
    write(1, date, strlen(date));
    write(data->logfile, date, strlen(date));

    write(1, msg, strlen(msg));
    write(data->logfile, msg, strlen(msg));

    sem_post(log_mutex);
}

void print_debug(char *msg)
{
#if DEBUG
    sem_wait(log_mutex);
    printf("%s", msg);
    sem_post(log_mutex);
#endif
}

void print_debug_no_sem(char *msg)
{
#if DEBUG
    printf("%s", msg);
#endif
}
