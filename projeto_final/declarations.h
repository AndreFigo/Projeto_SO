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
#define MAXTAMLINE 2000
#define MAXLOGMSG 400
#define MAXERRORMSG 200
#define MAXLOADMSG 200
#define MAXWARNINGMSG 200
#define MAXSEPARATOR 50

#define REFUEL_TIME 2

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

#define DEBUG 0

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
    int ind, last, current;
} state_change;

typedef struct
{
    pthread_t tid;
    //char equipa[MAXNOMEEQUIPA];
    int ind_team, num, speed, state, laps_done, distance, reliability, malfunc;
    int n_stops, time_passed, box_time;
    float consumption, fuel;
} car;

typedef struct
{
    pid_t team_pid;
    sem_t car_ready, entered_box, box_finished;
    pthread_mutex_t mutex_box_state, pipe_write_mutex;
    int box_state, n_cars, n_cars_seg_mode, ind_catual;
    char name[MAXNOMEEQUIPA];
    int fd[2];
} team;

typedef struct
{
    pid_t malfunc_pid, manager_pid;
    int n_laps, n_teams, max_car, logfile, u_time, distance, u_time_malfunc, T_Box_min, T_Box_Max;
    int total_cars, cars_finished, cars_waiting_tunit, cars_ended_tunit, tunits_passed;
    int on_going, stop, interupt, n_malfuncs, stats, on_track;
    float fuel_tank;
    pthread_mutex_t finish_mutex, new_tunit_mutex, end_tunit_mutex, stats_mutex;
    pthread_mutex_t forced_stop_mutex, interupt_mutex, log_mutex, on_going_mutex;
    pthread_cond_t new_tunit, end_tunit;

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

void print_status_changes(int ind, int last, int current);

int max_file();

void increment_cars_finished();

void sigint_before_race_Sim();

void init_sigint();

void init_sigtstp();

void init_sigusr1();

void ignore_sigint();

void ignore_sigtstp();

void ignore_sigusr1();

void sigusr1(int signo);

void print_stats(car *c, int n_malf);

void terminate_sem();

void sigtstp();

/* ================================= VARIAVEIS GLOBAIS ================================= */

info_struct *data;
team *teams;
car *cars;

int shmid, mqid;
int logfile, fd_named_pipe;

pthread_mutexattr_t attrmutex;
pthread_condattr_t attrcondv;
//mudar forced_stop
sem_t *start_race, *begin_copy, *ended_copy, *end_race, *end_simulator, *pid_ready;
struct sigaction print_est, finish_race, interupt_race, stop_commands;
sigset_t block_set_est, block_set_fin;

#endif
