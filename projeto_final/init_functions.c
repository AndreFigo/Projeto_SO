// Trabalho Prático - Simulador de Corridas
// André Carvalho 2019216156
// Sofia Alves 2019227240
// 20/21


#include "declarations.h"

void init_sem()
{

    /* Initialize attribute of mutex. */
    if (pthread_mutexattr_init(&(attrmutex)) != 0)
    {
        perror("ERROR: Failed to initialize mutex atribute\n");
        exit(1);
    }
    if (pthread_mutexattr_setpshared(&(attrmutex), PTHREAD_PROCESS_SHARED) != 0)
    {
        perror("ERROR: Failed to share mutex\n");
        exit(1);
    }

    /* Initialize attribute of condition variable. */
    if (pthread_condattr_init(&(attrcondv)) != 0)
    {
        perror("ERROR: Failed to initialize condition variable\n");
        exit(1);
    }
    if (pthread_condattr_setpshared(&(attrcondv), PTHREAD_PROCESS_SHARED) != 0)
    {
        perror("ERROR: Failed to share condition variable\n");
        exit(1);
    }

    print_debug_no_sem("Criando os semaforos\n");

    sem_unlink("START");
    if ((start_race = sem_open("START", O_CREAT | O_EXCL, 0766, 0)) == SEM_FAILED)
    {
        perror("ERROR: Failed to create semaphore\n");
        exit(1);
    }

    sem_unlink("END");
    if ((end_race = sem_open("END", O_CREAT | O_EXCL, 0766, 0)) == SEM_FAILED)
    {
        perror("ERROR: Failed to create semaphore\n");
        exit(1);
    }

    sem_unlink("END_SIM");
    if ((end_simulator = sem_open("END_SIM", O_CREAT | O_EXCL, 0766, 0)) == SEM_FAILED)
    {
        perror("ERROR: Failed to create semaphore\n");
        exit(1);
    }

    sem_unlink("BEG_COPY");
    if ((begin_copy = sem_open("BEG_COPY", O_CREAT | O_EXCL, 0766, 0)) == SEM_FAILED)
    {
        perror("ERROR: Failed to create semaphore\n");
        exit(1);
    }

    sem_unlink("END_COPY");
    if ((ended_copy = sem_open("END_COPY", O_CREAT | O_EXCL, 0766, 0)) == SEM_FAILED)
    {
        perror("ERROR: Failed to create semaphore\n");
        exit(1);
    }

    sem_unlink("PID");
    if ((pid_ready = sem_open("PID", O_CREAT | O_EXCL, 0766, 0)) == SEM_FAILED)
    {
        perror("ERROR: Failed to create semaphore\n");
        exit(1);
    }

    for (int i = 0; i < data->n_teams; ++i)
    {
        if (sem_init(&((teams + i)->car_ready), 1, 0) != 0)
        {
            fprintf(stderr, "ERROR: Failed to initialize %d car_ready\n", i);
            exit(1);
        }

        if (sem_init(&((teams + i)->box_finished), 1, 0) != 0)
        {
            fprintf(stderr, "ERROR: Failed to initialize %d box_finished\n", i);
            exit(1);
        }

        if (sem_init(&((teams + i)->entered_box), 1, 0) != 0)
        {
            fprintf(stderr, "ERROR: Failed to initialize %d entered_box\n", i);
            exit(1);
        }

        if (pthread_mutex_init(&((teams + i)->mutex_box_state), &(attrmutex)) != 0)
        {
            perror("ERROR: Failed to initialize mutex_box_state\n");
            exit(1);
        }

        if (pthread_mutex_init(&((teams + i)->pipe_write_mutex), &(attrmutex)) != 0)
        {
            perror("ERROR: Failed to initialize pipe_write_mutex\n");
            exit(1);
        }
    }

    /* Initialize mutex. */
    if (pthread_mutex_init(&(data->finish_mutex), &(attrmutex)) != 0)
    {
        perror("Problemas a inicializar o mutex finish_mutex\n");
        exit(1);
    }
    if (pthread_mutex_init(&(data->new_tunit_mutex), &(attrmutex)) != 0)
    {
        perror("Problemas a inicializar o mutex new_tunit_mutex\n");
        exit(1);
    }
    if (pthread_mutex_init(&(data->end_tunit_mutex), &(attrmutex)) != 0)
    {
        perror("Problemas a inicializar o mutex end_tunit_mutex\n");
        exit(1);
    }
    if (pthread_mutex_init(&(data->stats_mutex), &(attrmutex)) != 0)
    {
        perror("Problemas a inicializar o mutex stats_mutex\n");
        exit(1);
    }
    if (pthread_mutex_init(&(data->forced_stop_mutex), &(attrmutex)) != 0)
    {
        perror("Problemas a inicializar o mutex forced_stop_mutex\n");
        exit(1);
    }
    if (pthread_mutex_init(&(data->interupt_mutex), &(attrmutex)) != 0)
    {
        perror("Problemas a inicializar o mutex interupt_mutex\n");
        exit(1);
    }
    if (pthread_mutex_init(&(data->log_mutex), &(attrmutex)) != 0)
    {
        perror("Problemas a inicializar o mutex log_mutex\n");
        exit(1);
    }
    if (pthread_mutex_init(&(data->on_going_mutex), &(attrmutex)) != 0)
    {
        perror("Problemas a inicializar o mutex new_tunit_mutex\n");
        exit(1);
    }

    /* Initialize condition variables. */
    if (pthread_cond_init(&(data->new_tunit), &(attrcondv)) != 0)
    {
        perror("Problemas a inicializar a variavel de condicao new_tunit\n");
        exit(1);
    }
    if (pthread_cond_init(&(data->end_tunit), &(attrcondv)) != 0)
    {
        perror("Problemas a inicializar a variavel de condicao end_tunit\n");
        exit(1);
    }
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
        perror("Erro no shmget com IPC_CREAT: ");
        exit(1);
    }

    if ((data = (info_struct *)shmat(shmid, NULL, 0)) == (info_struct *)-1)
    {
        perror("ERRO no shmat: ");
        exit(1);
    }

    teams = (team *)(data + 1);
    cars = (car *)(teams + n_teams);

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
    data->stats = 0;
    data->stop = 0;

    free(config);

    for (int i = 0; i < data->n_teams; ++i)
    {
        for (int j = 0; j < data->max_car; ++j)
        {
            (cars + i * data->max_car + j)->ind_team = i;
            (cars + i * data->max_car + j)->num = -1;
        }
        //inicilizar o numero do carro ^

        //inicilizar o nome da equipa e o numero de carros
        strcpy((teams + i)->name, "");
        (teams + i)->n_cars = 0;
    }

    mqid = msgget(IPC_PRIVATE, IPC_CREAT | 0777);
    if (mqid < 0)
    {
        perror("Creating message queue: ");
        exit(1);
    }
    //named pipe creation
    unlink(PIPE_NAME);
    if ((mkfifo(PIPE_NAME, O_CREAT | O_EXCL | 0600) < 0) && (errno != EEXIST))
    {
        perror("Cannot create pipe: ");
        exit(1);
    }

    init_sem();
}

int verify(float *config)
{
    if (config[0] <= 0)
    {
        fprintf(stderr, "ERROR: Invalid time units.\n");
        return -1;
    }
    else if (config[1] <= 0)
    {
        fprintf(stderr, "ERROR: Invalide distance per lap.\n");
        return -1;
    }
    else if (config[2] <= 0)
    {
        fprintf(stderr, "ERROR: Invalid number of laps.\n");
        return -1;
    }
    else if (config[3] < 3)
    {
        fprintf(stderr, "ERROR: Number of teams must be at least 3.\n");
        return -1;
    }
    else if (config[4] < 1)
    {
        fprintf(stderr, "ERROR: Invalid number of cars per team.\n");
        return -1;
    }
    else if (config[5] <= 0)
    {
        fprintf(stderr, "ERROR: Invalid time units between the calculation of a new malfunction.\n");
        return -1;
    }
    else if (config[6] <= 0)
    {
        fprintf(stderr, "ERROR: Invalid choice for minimum time to repair.\n");
        return -1;
    }
    else if (config[7] <= 0)
    {
        fprintf(stderr, "ERROR: Invalid choice for maximum time to repair.\n");
        return -1;
    }
    else if (config[8] <= 0)
    {
        fprintf(stderr, "ERROR: Invalid choice for the capacity of gas for the tank.\n");
        return -1;
    }
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
