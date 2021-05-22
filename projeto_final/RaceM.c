// Trabalho Prático - Simulador de Corridas
// André Carvalho 2019216156
// Sofia Alves 2019227240
// 20/21

#include "declarations.h"

void Race_manager(int n_equipas)
{
    print_debug("race manager!\n");
    ignore_signals();

    //unnamed pipes creation
    for (int i = 0; i < data->n_teams; ++i)
    {
        if (pipe((teams + i)->fd) == -1)
        {
            perror("ERRO na criacao do unnamed pipe: ");
            exit(1);
        }
    }

    pid_t aux;
    for (int i = 0; i < n_equipas; ++i)
    {
        if ((aux = fork()) == 0)
        {
            Team_manager(i);
            exit(0);
        }
        (teams + i)->team_pid = aux;
    }

    sem_post(pid_ready);

    for (int i = 0; i < n_equipas; ++i)
    {
        if (close((teams + i)->fd[1]) == -1)
        {
            print_debug("1\n");
            perror("ERROR: Failed to close unnamed pipe\n");
            exit(1);
        }
    }

    if ((fd_named_pipe = open(PIPE_NAME, O_RDWR)) < 0)
    {
        perror("Cannot open pipe for reading: ");
        exit(0);
    }

    read_commands();
    data->on_going = 1;
    print_debug("Primeiros comandos totalmente lidos\n");

    init_sigusr1();

    char log_msg[MAXERRORMSG];
    sprintf(log_msg, "ACCEPTING SIGUSR1 SINGAL IN PID %d\n", (int)getpid());
    app_log(log_msg);

    //read from all pipes
    fd_set read_set;
    state_change change;
    print_debug("Listening from all pipes\n");
    int nread, leave = 0;
    char line[MAXTAMCOMMANDS];

    int max_fd = max_file();
    int cars_finished = 0;

    while (1)
    {
        if (leave)
            break;
        FD_ZERO(&read_set);
        FD_SET(fd_named_pipe, &read_set);

        for (int i = 0; i < data->n_teams; ++i)
            FD_SET((teams + i)->fd[0], &read_set);

        //VERIFICAR SE O FD_NAMED_PIPE É EFETIVAMENTE O MAIOR
        if (select(max_fd + 1, &read_set, NULL, NULL, NULL) > 0)
        {
            if (FD_ISSET(fd_named_pipe, &read_set))
            {
                nread = read(fd_named_pipe, line, MAXTAMLINE);
                line[nread - 1] = 0;

                char *token = strtok(line, "\n");

                while (token != NULL)
                {
                    if (strcmp(token, "START RACE!") == 0)
                    {
                        pthread_mutex_lock(&data->on_going_mutex);
                        if (data->on_going)
                            sprintf(log_msg, "%s - Rejected, race is on going!\n", token);
                        else
                        {
                            //recomecar a corrida
                            for (int i = 0; i < (data->total_cars + 1); ++i)
                            {
                                sem_post(start_race);
                            }

                            sprintf(log_msg, "%s - Restarting race\n", token);
                            data->on_going = 1;
                        }
                        pthread_mutex_unlock(&data->on_going_mutex);
                    }
                    else
                    {
                        // dispose of addcar commands
                        char original[100];
                        strcpy(original, token);
                        char *token2 = strtok(original, " ");
                        if (strcmp(token2, "ADDCAR"))
                        {
                            // reject
                            pthread_mutex_lock(&data->on_going_mutex);

                            if (data->on_going)
                                sprintf(log_msg, "%s - Rejected, race already started!\n", token);
                            else
                                sprintf(log_msg, "%s - Rejected, cannot add cars in after interrupt or sigint!\n", token);
                            pthread_mutex_unlock(&data->on_going_mutex);
                        }
                        else
                        {
                            sprintf(log_msg, "%s - Rejected, wrong command!\n", token);
                        }
                    }

                    app_log(log_msg);
                    token = strtok(token, "\n");
                    token = strtok(NULL, "\n");
                }
            }
            for (int i = 0; i < data->n_teams; ++i)
            {
                if (FD_ISSET((teams + i)->fd[0], &read_set))
                {
                    do
                    {
                        nread = read((teams + i)->fd[0], &change, sizeof(state_change));

                        if (nread == -1)
                        {
                            //error
                            app_log("Unable to read from pipe.\n");
                        }
                    } while (nread < 0);

                    print_status_changes(change.ind, change.last, change.current);

                    if (change.current == TERMINADO || change.current == DESISTENCIA)
                        cars_finished++;

                    if (cars_finished == data->total_cars)
                    {

                        pthread_mutex_lock(&data->interupt_mutex);
                        pthread_mutex_lock(&data->forced_stop_mutex);

                        if (data->stop == 1 || data->interupt == 0)
                            leave = 1;
                        else
                            cars_finished = 0;

                        pthread_mutex_unlock(&data->forced_stop_mutex);
                        pthread_mutex_unlock(&data->interupt_mutex);
                    }
                }
            }
        }
        else
        {
            if (errno == EINTR)
                continue;
        }
    }

    ignore_sigusr1();

    for (int i = 0; i < data->n_teams; ++i)
    {
        (teams + i)->ind_catual = -1;
        sem_post(&(teams + i)->entered_box);
    }

    for (int i = 0; i < n_equipas; ++i)
        wait(NULL);

    for (int i = 0; i < data->n_teams; ++i)
    {
        if (close((teams + i)->fd[0]) == -1)
        {
            perror("ERROR: Failed to close unnamed pipe\n");
            exit(1);
        }
    }

    if (close(fd_named_pipe) == -1)
    {
        perror("ERROR: Failed to close unnamed pipe\n");
        exit(1);
    }

    print_debug("Saiu race manager\n");
}

void print_status_changes(int ind, int last, int current)
{
    char estados[5][20] = {"CORRIDA", "SEGURANCA", "BOX", "DESISTENCIA", "TERMINADO"};

    char msg[200];
    sprintf(msg, "CARRO %d PASSOU DO MODO %s PARA O MODO %s\n", (cars + ind)->num, estados[last], estados[current]);

    app_log(msg);
}

int max_file()
{
    int max = fd_named_pipe;
    for (int i = 0; i < data->n_teams; ++i)
    {
        if ((teams + i)->fd[0] > max)
            max = (teams + i)->fd[0];
    }
    return max;
}

void read_commands()
{
    char line_input[MAXTAMLINE] = "";
    int nread, leave = 0;

    while (1)
    {
        if (leave == 1)
            break;

        nread = read(fd_named_pipe, line_input, MAXTAMLINE);

        line_input[nread - 1] = 0;

        char *token = strtok(line_input, "\n");
        while (token != NULL)
        {

            if (leave == 1)
            {
                app_log("RECEIVED TOO MANY COMMANDS: STARTING RACE BEFORE READING MORE COMMANDS\n");
            }
            else if (strcasecmp(token, "START RACE!") == 0)
            {

                app_log("NEW COMMAND RECEIVED: START RACE\n");
                int ver = verify_all_teams();
                leave = 0;
                if (ver == 1)
                {
                    for (int i = 0; i < data->n_teams; ++i)
                    {
                        for (int j = (teams + i)->n_cars; j < data->max_car; ++j)
                        {
                            sem_post(&((teams + i)->car_ready));
                        }
                    }
                    leave = 1;
                }
                else
                    app_log("CANNOT START, NOT ENOUGH TEAMS\n");
            }
            else
            {
                add_car(token);
            }
            token = strtok(token, "\n");
            token = strtok(NULL, "\n");
        }
    }
    //informar todos os carros, todas as equipas e o malfunction manager
    for (int i = 0; i < (data->total_cars + data->n_teams + 2); ++i)
    {
        sem_post(start_race);
    }
}

int team_exists(char *team_input)
{
    //printf("Checking team %s\n", team_input);
    for (int i = 0; i < data->n_teams; i++)
    {
        if (strcmp(team_input, (teams + i)->name) == 0)
            return i; // posição da equipa no array
        else if (strcmp("", (teams + i)->name) == 0)
            return i; //primeira posicao livre
    }
    return -1; // nao ha lugares disponiveis
}

int car_exists(int car_num, int team_pos)
{
    //printf("Checking car %d\n", car_num);
    for (int i = 0; i < data->max_car * data->n_teams; i++)
    {
        if ((cars + i)->num == car_num)
            return -2;
    }
    for (int i = 0; i < data->max_car; ++i)
    {
        if ((cars + team_pos * data->max_car + i)->num == -1)
            return team_pos * data->max_car + i;
    }
    return -1;
}

int add_car(char *line)
{

    char team_name[MAXNOMEEQUIPA];
    int pos = -1, pos_car = -1, car_num = -1, car_speed = 0, car_reliability = 0;
    float car_consumption = 0;

    char original_line[MAXTAMLINE];
    // guardar uma copia da linha
    strcpy(original_line, line);

    //printf("original: %s", original_line);

    char *token = strtok(original_line, " ");
    if (strcasecmp(token, "ADDCAR") == 0)
    {
        token = strtok(NULL, " "); //TEAM
        if (strcasecmp(token, "TEAM:") != 0)
        {
            log_wrong_commands("TEAM FIELD INVALID", line);
            return -1;
        }

        token = strtok(NULL, " "); //B
        strcpy(team_name, token);

        //retirar a virgula
        team_name[(int)strlen(team_name) - 1] = 0;

        if ((pos = team_exists(team_name)) == -1)
        {
            //log erro - não há espaço
            log_wrong_commands("TEAM LIMIT EXCEEDED", line);
            return -1;
        }

        token = strtok(NULL, " "); //CAR
        if (strcasecmp(token, "CAR:") != 0)
        {
            //log erro - erro ao escrever car
            log_wrong_commands("CAR FIELD INVALID", line);
            return -1;
        }
        token = strtok(NULL, " "); //01
        car_num = atoi(token);
        if ((pos_car = car_exists(car_num, pos)) == -2)
        { //log erro - numero do carro já existe
            log_wrong_commands("CAR NUMBER ALREADY EXISTS", line);
            return -1;
        }
        else if (pos_car == -1)
        {
            //log erro - não há espaço
            log_wrong_commands("NUMBER OF CARS EXCEEDED", line);
            return -1;
        }

        token = strtok(NULL, " ");
        if (strcasecmp(token, "SPEED:") != 0)
        {
            log_wrong_commands("SPEED FIELD INVALID", line);
            return -1;
        }
        token = strtok(NULL, " ");
        car_speed = atoi(token); // verificar conversao para inteiro
        if (car_speed < 0)
        {
            log_wrong_commands("SPEED INVALID", line);
            return -1;
        }

        token = strtok(NULL, " ");
        if (strcasecmp(token, "CONSUMPTION:") != 0)
        {
            log_wrong_commands("CONSUPTION FIELD INVALID", line);
            return -1;
        }
        token = strtok(NULL, " ");
        car_consumption = atof(token); // verificar conversao para inteiro
        if (car_consumption < 0)
        {
            log_wrong_commands("CAR CONSUPTION INVALID", line);
            return -1;
        }

        token = strtok(NULL, " ");
        if (strcasecmp(token, "RELIABILITY:") != 0)
        {
            log_wrong_commands("RELIABILITY FIELD INVALID", line);
            return -1;
        }
        token = strtok(NULL, " ");
        car_reliability = atoi(token); // verificar conversao para inteiro
        if (car_reliability < 0)
        {
            //log erro - velocidade negativa
            log_wrong_commands("RELIABILITY INVALID", line);
            return -1;
        }

        if (strcmp((teams + pos)->name, "") == 0)
            strcpy((teams + pos)->name, team_name);

        //printf("POS %d\nNAME %s\nNUM %d\nPOSCAR %d\n", pos,(teams + pos)->name, car_num , pos_car);

        (teams + pos)->n_cars += 1;

        (data->total_cars)++;

        (cars + pos_car)->num = car_num;

        (cars + pos_car)->speed = car_speed;

        (cars + pos_car)->consumption = car_consumption;

        (cars + pos_car)->reliability = car_reliability;

        if (sem_post(&((teams + pos)->car_ready)) == -1)
        {
            printf("Erro no sem_post\n");
        }
        log_load_car(line);
        return 0;
    }
    log_wrong_commands("USE \'START RACE!\' OR \'ADDCAR ...\' ", line);
    return -1;
}

void log_wrong_commands(char *error_msg, char *command)
{

    char error[MAXERRORMSG];
    sprintf(error, "WRONG COMMAND - %s!! (%s).\n", error_msg, command);
    app_log(error);
}

void log_load_car(char *command)
{
    char load[MAXERRORMSG] = "NEW CAR LOADED => ";
    sprintf(load, "NEW CAR LOADED => %s\n", command);
    app_log(load);
}

//return  0 if there are not enough teams and 1 otherwise
int verify_all_teams()
{
    for (int i = 0; i < data->n_teams; ++i)
    {
        if (strcmp((teams + i)->name, "") == 0)
            return 0;
    }
    return 1;
}
