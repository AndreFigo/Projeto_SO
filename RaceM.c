// Trabalho Prático - Simulador de Corridas
// André Carvalho 2019216156
// Sofia Alves 2019227240
// 20/21

#include "declarations.h"

void Race_manager(int n_equipas)
{

    print_debug("race manager!\n");

    for (int i = 0; i < n_equipas; ++i)
    {
        if (fork() == 0)
        {
            close((teams + i)->fd[0]);
            Team_manager(i);
            exit(0);
        }
        close((teams + i)->fd[1]);
    }

    if ((fd_named_pipe = open(PIPE_NAME, O_RDWR)) < 0)
    {
        perror("Cannot open pipe for reading: ");
        exit(0);
    }

    read_commands();
    data->on_going = 1;
    print_debug("Comandos totalmente lidos\n");

    //read from all pipes
    fd_set read_set;
    print_debug("Listening from all pipes\n");
    int nread;
    char line[MAXTAMLINE];
    char log_msg[MAXERRORMSG];
    while (1)
    {
        FD_ZERO(&read_set);
        FD_SET(fd_named_pipe, &read_set);
        for (int i = 0; i < data->n_teams; ++i)
            FD_SET((teams + i)->fd[0], &read_set);

        //VERIFICAR SE O FD_NAMED_PIPE É EFETIVAMENTE O MAIOR
        if (select(fd_named_pipe + 1, &read_set, NULL, NULL, NULL) > 0)
        {

            if (FD_ISSET(fd_named_pipe, &read_set))
            {
                nread = read(fd_named_pipe, line, MAXTAMLINE);
                line[nread - 1] = 0;

                if (strcmp(line, "START RACE!") == 0)
                {
                    if (data->on_going)
                        sprintf(log_msg, "%s - Rejected, race is on going!\n", line);
                    else
                    {
                        //recomecar a corrida
                        //------------------------- to do -----------------------------//
                        sprintf(log_msg, "%s - Restarting race\n", line);
                    }
                }
                else
                {
                    // dispose of addcar commands
                    char original[MAXTAMLINE];
                    strcpy(original, line);
                    char *token = strtok(line, " ");
                    if (strcmp(token, "ADDCAR"))
                    {
                        // reject
                        if (data->on_going)
                            sprintf(log_msg, "%s - Rejected, race already started!\n", original);
                        else
                            sprintf(log_msg, "%s - Rejected, cannot add cars in after interrupt!\n", original);
                    }
                    else
                    {
                        sprintf(log_msg, "%s - Rejected, wrong command!\n", original);
                    }
                }

                log_entries(log_msg);
            }
            for (int i = 0; i < data->n_teams; ++i)
            {
                if (FD_ISSET((teams + i)->fd[0], &read_set))
                {
                    nread = read((teams + i)->fd[0], line, MAXTAMLINE);
                    log_entries(line);
                }
            }
        }
    }

    //sleep(1);
    for (int i = 0; i < n_equipas; ++i)
        wait(NULL);
    print_debug("Saiu race manager\n");
}

void read_commands()
{
    char line_input[MAXTAMLINE] = "";
    int nread;

    while (1)
    {
        nread = read(fd_named_pipe, line_input, MAXTAMLINE);
        line_input[nread - 1] = 0;
        /*int i = 0;
        while (line_input[i] != 0)
        {
            if (line_input[i] == '\n' || line_input[i] == '\r')
                line_input[i] = 0;
            i++;
        }*/

        if (strcasecmp(line_input, "START RACE!") == 0)
        {

            app_log("NEW COMMAND RECEIVED: START RACE\n");
            int ver = verify_all_teams();
            if (ver == 1)
            {
                for (int i = 0; i < data->n_teams; ++i)
                {
                    for (int j = (teams + i)->n_cars; j < data->max_car; ++j)
                    {
                        sem_post(&((teams + i)->car_ready));
                    }
                }
                break;
            }
            else
            {
                app_log("CANNOT START, NOT ENOUGH TEAMS\n");
                continue;
            }
        }
        else
        {
            add_car(line_input);
        }
    }
    //informar todos os carros, todas as equipas e o malfunction manager
    for (int i = 0; i < (data->total_cars + data->n_teams + 1); ++i)
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
    for (int i = 0; i < data->max_car; i++)
    {
        if ((cars + team_pos * data->n_teams + i)->num == car_num)
            return -2;
        else if ((cars + team_pos * data->n_teams + i)->num == -1)
            return i;
    }
    return -1;
}

int add_car(char *line)
{
    char team_name[MAXNOMEEQUIPA];
    int pos = 0, pos_car = 0, car_num = -1, car_speed = 0, car_reliability = 0;
    float car_consumption = 0;

    char original_line[MAXTAMLINE];
    // guardar uma copia da linha
    strcpy(original_line, line);

    char *token = strtok(line, " ");
    if (strcasecmp(token, "ADDCAR") == 0)
    {
        token = strtok(NULL, " ");
        if (strcasecmp(token, "TEAM:") != 0)
        {
            log_wrong_commands("TEAM FIELD INVALID", original_line);
            return -1;
        }

        token = strtok(NULL, " ");
        strcpy(team_name, token);

        //retirar a virgula
        team_name[(int)strlen(team_name) - 1] = 0;

        if ((pos = team_exists(team_name)) == -1)
        {
            //log erro - não há espaço
            log_wrong_commands("TEAM LIMIT EXCEEDED", original_line);
            return -1;
        }

        token = strtok(NULL, " ");
        if (strcasecmp(token, "CAR:") != 0)
        {
            //log erro - erro ao escrever car
            log_wrong_commands("CAR FIELD INVALID", original_line);
            return -1;
        }
        token = strtok(NULL, " ");
        car_num = atoi(token);
        if ((pos_car = car_exists(car_num, pos)) == -2)
        { //log erro - numero do carro já existe
            log_wrong_commands("CAR NUMBER ALREADY EXISTS", original_line);
            return -1;
        }
        else if (pos_car == -1)
        {
            //log erro - não há espaço
            log_wrong_commands("NUMBER OF CARS EXCEEDED", original_line);
            return -1;
        }

        token = strtok(NULL, " ");
        if (strcasecmp(token, "SPEED:") != 0)
        {
            log_wrong_commands("SPEED FIELD INVALID", original_line);
            return -1;
        }
        token = strtok(NULL, " ");
        car_speed = atoi(token); // verificar conversao para inteiro
        if (car_speed < 0)
        {
            log_wrong_commands("SPEED INVALID", original_line);
            return -1;
        }

        token = strtok(NULL, " ");
        if (strcasecmp(token, "CONSUMPTION:") != 0)
        {
            log_wrong_commands("CONSUPTION FIELD INVALID", original_line);
            return -1;
        }
        token = strtok(NULL, " ");
        car_consumption = atof(token); // verificar conversao para inteiro
        if (car_consumption < 0)
        {
            log_wrong_commands("CAR CONSUPTION INVALID", original_line);
            return -1;
        }

        token = strtok(NULL, " ");
        if (strcasecmp(token, "RELIABILITY:") != 0)
        {
            log_wrong_commands("RELIABILITY FIELD INVALID", original_line);
            return -1;
        }
        token = strtok(NULL, " ");
        car_reliability = atoi(token); // verificar conversao para inteiro
        if (car_reliability < 0)
        {
            //log erro - velocidade negativa
            log_wrong_commands("RELIABILITY INVALID", original_line);
            return -1;
        }

        if (strcmp((teams + pos)->name, "") == 0)
            strcpy((teams + pos)->name, team_name);
        (teams + pos)->n_cars += 1;

        (data->total_cars)++;

        (cars + pos * data->n_teams + pos_car)->num = car_num;

        (cars + pos * data->n_teams + pos_car)->speed = car_speed;

        (cars + pos * data->n_teams + pos_car)->consumption = car_consumption;

        (cars + pos * data->n_teams + pos_car)->reliability = car_reliability;

        sem_post(&((teams + pos)->car_ready));
        log_load_car(original_line);
        return 0;
    }
    log_wrong_commands("USE \'START RACE!\' OR \'ADDCAR ...\' ", original_line);
    return -1;
}

void log_wrong_commands(char *error_msg, char *command)
{
    //sprintf ???
    char error[MAXERRORMSG] = "WRONG COMMAND - ";
    strcat(error, error_msg);
    strcat(error, "!! (");
    strcat(error, command);
    strcat(error, ").\n");
    app_log(error);
}

void log_load_car(char *command)
{
    char load[MAXERRORMSG] = "NEW CAR LOADED => ";
    strcat(load, command + 7);
    strcat(load, "\n");
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
