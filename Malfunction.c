// Trabalho Prático - Simulador de Corridas
// André Carvalho 2019216156
// Sofia Alves 2019227240
// 20/21

#include "declarations.h"

void Malfunction_manager(int t_avaria)
{

    print_debug("Malfuncion_manager\n");

    sem_wait(start_race);

    //to do

    //sleep(3);

    print_debug("Saiu malfunction manager\n");
}