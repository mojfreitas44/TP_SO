#include "comum.h"

//comunicção entre a viatura e cliente -> named pipe
//comunicação entre veiculo e controlador -> pipe anonimo e fork

typedef struct{  //não sei se a criação desta estrutura é necessária
    char username_cliente[50];
    int  pid_cliente;
    int  distancia_total;
    char local_partida[100];

    // Nomes dos pipes
    char pipe_cliente_nome[100]; // O pipe DO cliente (para onde envio avisos)
    char pipe_veiculo_nome[100]; // O MEU pipe (onde leio 'entrar' e 'sair')
}InfoVeiculo;

InfoVeiculo veiculo;

void limparPipeVeiculo() {
    unlink(veiculo.pipe_veiculo_nome);
}
void sinalSIGUSR1(int s) {
    // Handler para o sinal SIGUSR1 (pode ser usado para interrupções)
    printf("[VEÍCULO] Sinal SIGUSR1 recebido.\n");
    //enviar Mensagem ao cliente a informar que houve uma interrupção
}



int main(int argc, char *argv[]) {
    //config inicial: ler argv e garnatir que a comunicação está estabelecida (falta)
    if (argc != 5) {
        printf("Uso: ./veiculo <username> <pid_cliente> <distancia> <local>\n");
        return 1;
    }
    signal(SIGUSR1, sinalSIGUSR1);
    //argv
    strcpy(veiculo.username_cliente, argv[1]);
    veiculo.pid_cliente = atoi(argv[2]);
    veiculo.distancia_total = atoi(argv[3]);
    strcpy(veiculo.local_partida, argv[4]);


    printf("INICIO: Veículo (PID %d) lançado para %s (PID %d).\n", getpid(), veiculo.username_cliente, veiculo.pid_cliente);
    fflush(stdout);

    char pipe_cliente[100];
    sprintf(pipe_cliente, PIPE_CLIENTE, veiculo.pid_cliente);

    if(mkfifo(veiculo.pipe_veiculo_nome, 0666) == -1) {
        perror("mkfifo veiculo");
        fflush(stdout);
        exit (1);
    }

    // é melhor criar uma função enviar mensagem para cliente para evitar repetição de código
    sleep(2);
    int fd = open(pipe_cliente, O_WRONLY);
    if (fd != -1) {
        Mensagem m;
        sprintf(m.comando, "status");
        sprintf(m.mensagem, "Veículo a caminho de %s!", veiculo.username_cliente);
        write(fd, &m, sizeof(Mensagem));
        close(fd);
    }

    sleep(3);
    fd = open(pipe_cliente, O_WRONLY);
    if (fd != -1) {
        Mensagem m;
        sprintf(m.comando, "fim");
        sprintf(m.mensagem, "Chegámos ao destino, %s!", veiculo.username_cliente);
        write(fd, &m, sizeof(Mensagem));
        close(fd);
    }
    printf("[VEÍCULO] Viagem concluída.\n");
    return 0;
}