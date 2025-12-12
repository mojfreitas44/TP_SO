#ifndef COMUM_H
#define COMUM_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <sys/wait.h>

#define PIPE_CONTROLADOR "controlador_fifo"
#define PIPE_CLIENTE "pipe%d"
#define NVEICULOS 2 // 10 por default
#define NUTILIZADORES 3 // 30 por default
#define MAX_AGENDAMENTOS 3 // 50 por default

typedef struct {
    pid_t pid;             // PID do cliente
    char comando[50];      // tipo de comando
    char username[50];     // nome do utilizador
    char mensagem[256];    // mensagem adicional
} Mensagem;

#endif 