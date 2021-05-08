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
#include <errno.h>
#include <semaphore.h>
#include <math.h>
#include <signal.h>
#include <sys/msg.h>

/* ========================================= DEFINES ========================================= */

#define NINPUTS 9
#define MAXNOMEEQUIPA 20
#define MAXTAMCOMMANDS 1000
#define MAXTAMLINE 200
#define MAXLOGMSG 400
#define MAXERRORMSG 200
#define MAXLOADMSG 200
#define MAXWARNINGMSG 200
#define MAXTABELA 2000

#define CORRIDA 0
#define SEGURANCA 1
#define BOX 2
#define DESISTENCIA 3
#define TERMINADO 4

#define LIVRE 0
#define RESERVADO 1
#define OCUPADO 2

#define PIPE_NAME "my_pipe"
#define LOGFILE "log.txt"

#define DEBUG 1

/* ======================================== ESTRUTURAS ======================================== */

typedef struct
{
    long mtype;
    int warning;
} warning_message;

typedef struct
{
    int team_num, ind_car;
} info;

typedef struct
{
    pthread_t tid;
    //char equipa[MAXNOMEEQUIPA];
    int ind_team, num, speed, state, laps_done, distance, reliability, malfunc;
    int n_stops, seconds_taken, last_state, box_time;
    float consumption, fuel;
    sem_t state_mutex;
    pthread_mutex_t n_stops_mutex;
} car;

typedef struct
{
    sem_t car_ready,  entered_box, box_finished, mutex_box_state;
    int box_state, n_cars, n_cars_seg_mode, ind_catual;
    char name[MAXNOMEEQUIPA];
    int fd[2];
} team;

typedef struct
{
    time_t time;

    int n_laps, n_teams, max_car, logfile, u_time, distance, u_time_malfunc, T_Box_min, T_Box_Max;
    int total_cars, cars_finished, cars_waiting_tunit, cars_ended_tunit, tunits_passed;
    int on_going, stop, n_malfuncs, stats, on_track;
    float fuel_tank;
    pthread_mutex_t finish_mutex, new_tunit_mutex, end_tunit_mutex, stats_mutex;
    pthread_cond_t all_finished, new_tunit, end_tunit;

} info_struct;

/*======================================== FUNÇÕES ======================================== */

void Team_manager(int num);

void Malfunction_manager(int);

void Race_manager(int);

float *configurationRead();

int verify(float *config);

void init(float *config);

void *car_func(void *p);

void app_log(char *msg);

int add_car(char *line);

int verify_all_teams();

void terminate();

void read_commands();

void init_sem();

void ignore_signals();

void init_signal();

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

void enter_safe_mode(int team_num, int ind, int *cur_speed, float *cur_cons);

void reserve_box(int team_num);

void enter_box(int team_num, int ind, int last);

int max_distance(car *copy, int len, int *seen, int len2);

int last_place(car *copy, int len);

void on_track_and_total_stops(int *n_stops, int *on_track, car *copy, int len);

void communicate_status_changes(int team, int ind, int last, int current);

int max_file();

/* ================================= VARIAVEIS GLOBAIS ================================= */

info_struct *data;
team *teams;
car *cars;

int shmid, mqid;
int logfile, fd_named_pipe;

pthread_mutexattr_t attrmutex;
pthread_condattr_t attrcondv;
//mudar forced_stop
sem_t *log_mutex, *start_race, *forced_stop, *begin_copy, *ended_copy;
struct sigaction print_est, finish_race;
sigset_t block_set_est, block_set_fin;

#endif
