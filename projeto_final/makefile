# Trabalho Prático - Simulador de Corridas
# André Carvalho 2019216156
# Sofia Alves 2019227240
# 20/21

CC= gcc
FLAGS= -Wall -Wextra -g -pthread 
FLAGS2= -lm
OBJS = RaceS.o Malfunction.o RaceM.o TeamM.o Carro.o signal_functions.o init_functions.o terminate_functions.o
PROG = race

#GENERIC

all:		${PROG}

clean:
		rm ${OBJS} *~ ${PROG}

${PROG}:	${OBJS}
		${CC} ${FLAGS} ${OBJS} ${FLAGS2} -o $@

.c.o:
		${CC} ${FLAGS} $< -c -o  $@


################################

Carro.o: Carro.c declarations.h

RaceS.o: RaceS.c declarations.h

TeamM.o: TeamM.c declarations.h

RaceM.o: RaceM.c declarations.h

Malfunction.o: Malfunction.c declarations.h

signal_functions.o: signal_functions.c declarations.h

terminate_functions.o: terminate_functions.c declarations.h

init_functions.o: init_functions.c declarations.h

corrida: RaceS.o Malfunction.o RaceM.o TeamM.o Carro.o signal_functions.o init_functions.o terminate_functions.o
