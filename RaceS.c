// Trabalho Prático - Simulador de Corridas
// André Carvalho 2019216156
// Sofia Alves 2019227240
// 20/21

#include "declarations.h"

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



void sigint_before_race_Sim(){

    app_log("^C PRESSED. SIMULATOR LEAVING\n");
    
    kill(data->malfunc_pid, SIGKILL);
    kill(data->manager_pid, SIGKILL);
    for (int i=0 ;i < data->n_teams;++i)
        kill((teams +i)->team_pid, SIGKILL);
    
    waitpid(data->malfunc_pid, NULL,0);
    waitpid(data->manager_pid, NULL,0);
    for (int i=0 ;i < data->n_teams;++i)
        waitpid((teams+i)-> team_pid, NULL,0);


    terminate();
    exit(0);

}

void sigint(int signo)
{

    pthread_mutex_lock(&data->on_going_mutex);

    if ( data-> on_going==0){
        pthread_mutex_unlock(&data->on_going_mutex);
        sem_wait(pid_ready);
        sigint_before_race_Sim();
    }

    pthread_mutex_unlock(&data->on_going_mutex);




    app_log("^C pressed. Terminating race\n");
    //finish race
    pthread_mutex_lock(&data->forced_stop_mutex);
    data->stop = 1;
    pthread_mutex_unlock(&data->forced_stop_mutex);

    //wait for everyone using cond variable

    while (1)
    {
        if (sem_wait(end_simulator) == -1)
        {
            if (errno == EINTR)
                continue;
        }
        break;
    }

    /*pthread_mutex_lock(&data->finish_mutex);

    while (data->cars_finished != data->total_cars)
    {
        print_debug("UM CARRO ACABOU\n");
        printf("%d\n", data->cars_finished);
        pthread_cond_wait(&data->all_finished, &data->finish_mutex);
    }

    pthread_mutex_unlock(&data->finish_mutex);*/

    // wait for child processes
    for (int i = 0; i < 2; i++)
        wait(NULL);

    app_log("SIMULATOR CLOSING\n");
    terminate();
    exit(0);
}

void sigtstp(int signo)
{   

    pthread_mutex_lock(&data->on_going_mutex);

    if ( data-> on_going==0){
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

    char separator[50] = "----------------------------------------\n";
    char stats[7][200] = {0};


    for (int j = 0; j < 5; j++)
    {
        ind = max_distance(c, data->max_car * data->n_teams, seen, 5);
        seen[j] = ind;
        if (ind > -1)
        {
            // escrever os dados da equipa ind
            sprintf(stats[j], "%do lugar: Num-> %d, Team-> %d, voltas-> %d, Stops-> %d, Distance-> %d, Time-> %d\n", j + 1, (cars + ind)->num, (cars + ind)->ind_team + 1, (cars + ind)->laps_done, (cars + ind)->n_stops, (cars + ind)->distance, (cars + ind)->time_passed);
            //strncat(tabela, stats[j],100);
        }
        else
        {
            print_debug("Ha menos de 5 equipas, logo o top 5 esta incompleto\n");
            break;
        }
    }

    // escrever o pior
    strncat(stats[4], separator, 50);

    ind = last_place(c, data->max_car * data->n_teams);
    sprintf(stats[5], "Ultimo lugar: Num-> %d, Team-> %d, voltas-> %d, Stops-> %d, Distance-> %d, Time-> %d\n", (cars + ind)->num, (cars + ind)->ind_team + 1, (cars + ind)->laps_done, (cars + ind)->n_stops, (cars + ind)->distance, (cars + ind)->time_passed);
    strncat(stats[5], separator, 50);

    //escrevr os stops
    int n_stops = 0, on_track = 0;
    on_track_and_total_stops(&n_stops, &on_track, c, data->max_car * data->n_teams);
    sprintf(stats[6], "Total de paragens: %d\nTotal de avarias: %d\nEm pista: %d\n", n_stops, n_malf, on_track);

    strncat(stats[6], separator, 50);

    pthread_mutex_lock(&data->log_mutex);
    time_t now;
    time(&now);
    struct tm *timeinfo = localtime(&now);

    char date[20];
    sprintf(date, "%d:%d:%d ESTATISTICAS\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
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

    // mudar para mutex
    /*for (int i = 0; i < data->n_teams * data->max_car; ++i)
    {   
        if (pthread_mutex_init(&((cars + i)->state_mutex), &(attrmutex)) != 0)
        {
            perror("Problemas a inicializar o mutex finish_mutex\n");
            exit(1);
        }
    }*/

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

    //to do BOX SEM, ...
    /* Initialize thread mutex. */
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

    //unnamed pipes creation
    for (int i = 0; i < data->n_teams; ++i)
    {
        if (pipe((teams + i)->fd) == -1)
        {
            perror("ERRO no pipe: ");
            exit(1);
        }
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

void ignore_sigint(){
    sigemptyset(&finish_race.sa_mask);
    finish_race.sa_flags = 0;
    finish_race.sa_handler = SIG_IGN;

    sigaction(SIGINT, &finish_race, NULL);

}

void ignore_sigtstp(){
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

void init_sigint(){

    sigemptyset(&finish_race.sa_mask);
    finish_race.sa_flags = 0;
    finish_race.sa_handler = &sigint;
    sigaction(SIGINT, &finish_race, NULL);

}

void init_sigtstp(){
    sigemptyset(&print_est.sa_mask);
    sigaddset(&print_est.sa_mask, SIGINT);
    print_est.sa_flags = 0;
    
    print_est.sa_handler = &sigtstp;
    sigaction(SIGTSTP, &print_est, NULL);


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
    /*
    for (int i = 0; i < data->n_teams * data->max_car; ++i)
    {
        if (pthread_mutex_destroy(&((cars + i)->state_mutex)) != 0)
        {
            perror("ERROR: Failed to destroy state_mutex mutex\n");
            exit(1);
        }
    }*/

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

    data-> manager_pid= aux;

    for (int i = 0; i < data->n_teams; ++i)
    {
        if (close((teams + i)->fd[0]) == -1)
        {
            print_debug("2\n");
            perror("ERROR: Failed to close unnamed pipe\n");  
            exit(1); 
        }
        if (close((teams + i)->fd[1]) == -1)
        {
            printf("3, %d\n", i);
            perror("ERROR: Failed to close unnamed pipe\n");
            exit(1);
        }
    }
    
    if ( (aux = fork() )== 0)   
    {
        Malfunction_manager(data->u_time_malfunc);
        exit(0);
    }

    data-> malfunc_pid= aux; 

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

    /*pthread_mutex_lock(&data->finish_mutex);

    while (data->cars_finished != data->total_cars)
    {
        //print_debug("UM CARRO ACABOU\n");
        //printf("%d\n", data->cars_finished);
        pthread_cond_wait(&data->all_finished, &data->finish_mutex);
    }

    pthread_mutex_unlock(&data->finish_mutex);*/

    for (int i = 0; i < 2; ++i)
        wait(NULL);

    print_debug("Terminating everything\n");

    app_log("SIMULATOR CLOSING\n");

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

void app_log(char *msg)
{

    pthread_mutex_lock(&data->log_mutex);
    time_t now;
    time(&now);
    struct tm *timeinfo = localtime(&now);
    char log_msg[MAXERRORMSG];
    sprintf(log_msg, "%d:%d:%d %s", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, msg);
    //char *date = ctime(&data->time);
    //date[(int)strlen(date) - 1] = ' ';
    //write(1, date, strlen(date));
    //write(data->logfile, date, strlen(date));

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