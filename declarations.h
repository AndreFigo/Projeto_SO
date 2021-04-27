// Trabalho Prático - Simulador de Corridas
// André Carvalho 2019216156
// Sofia Alves 2019227240
// 20/21

#ifndef DECLARATIONS
#define DECLARATIONS

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <semaphore.h>

#define NINPUTS 9
#define MAXTAMLINE 200
#define MAXNOMEEQUIPA 20
#define MAXERRORMSG 200
#define MAXLOADMSG 200
#define PIPE_NAME "my_pipe"

#define Corrida 0
#define Seguranca 1
#define Box 2
#define Desistencia 3
#define Terminado 4

#define Livre 0
#define Ocupado 1
#define Reservado 2

#define LOGFILE "log.txt"
#define DEBUG 1

void Team_manager(int num);

void Malfunction_manager(int);

void Race_manager(int);

float *configurationRead();

int verify(float *config);

void init(float *config);

void *car_func(void *p);

void log_errors(char *msg);

int add_car(char *line);

int verify_all_teams();

void terminate();

void read_commands();

void init_sem();

void print_car_info(int ind, int team_num);

void log_wrong_commands(char *error_msg, char *command);

void log_load_car(char *command);

int car_exists(int car_num, int team_pos);

int team_exists(char *team_input);

int read1int(float *config, int pos, FILE *fich);

int read2int(float *config, int pos, FILE *fich);

int read1f(float *config, int pos, FILE *fich);

void print_debug(char *msg);

void print_debug_no_sem(char *msg);

typedef struct
{
    int team_num, ind_car;
} info;

typedef struct
{
    pthread_t tid;
    //char equipa[MAXNOMEEQUIPA];
    int ind_team, num, speed, state, laps_done, distance, reliability, n_stops;
    float consumption, gas;
} car;

typedef struct
{
    sem_t car_ready;
    int box, n_cars;
    char name[MAXNOMEEQUIPA];
} team;

typedef struct
{
    time_t time;
    int n_laps, n_teams, max_car, logfile, u_time, distance, u_time_malfunc, T_Box_min, T_Box_Max, total_cars, cars_finished;
    float fuel_tank;
    pthread_mutex_t finish_mutex;
    pthread_cond_t all_finished;
    pthread_mutexattr_t attrmutex;
    pthread_condattr_t attrcondv;
} info_struct;

info_struct *data;
team *teams;
car *cars;

int shmid;
int logfile;

sem_t *log_mutex, *start_race;
struct sigaction print_est, finish_race;
sigset_t block_set_est, block_set_fin;

#endif
