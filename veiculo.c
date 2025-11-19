#include "comum.h"

// ============================================================================
// VARIÁVEIS GLOBAIS
// ============================================================================
char pipe_cliente_nome[100];
int fd_cliente_pipe = -1;

// ============================================================================
// GESTÃO DE RECURSOS E SINAIS
// ============================================================================

void limpar_recursos() {
    if (fd_cliente_pipe != -1) close(fd_cliente_pipe);
    // Já não há pipe do veículo para apagar!
}

void trata_sinal_cancelar(int s) {
    // Caso receba o sinal SIGUSR1 deve cancelar o serviço
    printf("Viagem cancelada pelo controlador!\n"); 
    fflush(stdout);
    
    if (fd_cliente_pipe != -1) {
        Mensagem m;
        m.pid = getpid();
        strcpy(m.comando, "fim");
        strcpy(m.mensagem, "Viagem cancelada pela central!");
        write(fd_cliente_pipe, &m, sizeof(Mensagem));
    }
    exit(0);
}

void setup_ambiente(int pid_cliente) {
    setbuf(stdout, NULL);
    atexit(limpar_recursos);
    signal(SIGUSR1, trata_sinal_cancelar);
    signal(SIGINT, trata_sinal_cancelar);

    // Apenas precisamos de saber o nome do pipe do cliente para o avisar
    sprintf(pipe_cliente_nome, PIPE_CLIENTE, pid_cliente);
}

// ============================================================================
// FASES DO SERVIÇO
// ============================================================================

void iniciar_viagem(const char *local) {
    // 1. Contactar Cliente
    fd_cliente_pipe = open(pipe_cliente_nome, O_WRONLY);
    if (fd_cliente_pipe == -1) {
        printf("Erro: Cliente incontactável. Abortar.\n"); // Sai no stdout para o controlador ver
        exit(1);
    }

    Mensagem msg;
    msg.pid = getpid();
    strcpy(msg.comando, "status");
    
    // Nova lógica: Chegou = Começou
    sprintf(msg.mensagem, "Veículo chegou a %s. A iniciar viagem...", local);
    write(fd_cliente_pipe, &msg, sizeof(Mensagem));
}

void realizar_viagem_simulada(int distancia_total) {
    printf("Início da viagem de %dkm.\n", distancia_total); // -> Controlador

    int perc = 0;
    int km_percorridos = 0;
    
    // Loop simples de simulação (sem leituras, apenas escrita)
    while (km_percorridos < distancia_total) {
        sleep(1); // Avanço do tempo (1s = 1 unidade de tempo)
        km_percorridos++;
        
        int nova_perc = (km_percorridos * 100) / distancia_total;
        
        // Reporta a cada 10% ao Controlador
        if (nova_perc / 10 > perc / 10) {
            printf("Progresso: %d%% (%d/%d km)\n", nova_perc, km_percorridos, distancia_total);
            fflush(stdout); // Importante para o pipe anónimo não ficar preso no buffer
        }
        perc = nova_perc;
    }

    // Conclusão normal
    printf("Viagem concluída com sucesso.\n"); // -> Controlador
    
    Mensagem msg_fim;
    msg_fim.pid = getpid();
    strcpy(msg_fim.comando, "fim");
    strcpy(msg_fim.mensagem, "Chegámos ao destino.");
    write(fd_cliente_pipe, &msg_fim, sizeof(Mensagem));
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Erro args: ./veiculo <user> <pid_cli> <dist> <local>\n");
        return 1;
    }

    // Parsing dos argumentos
    // char *username = argv[1]; // Não usado diretamente aqui, mas útil para debug se quiseres
    int pid_cliente = atoi(argv[2]);
    int distancia = atoi(argv[3]);
    char *local = argv[4];

    // 1. Configuração
    setup_ambiente(pid_cliente);

    // 2. Avisar chegada e início automático (Simplificação do Prof)
    iniciar_viagem(local);

    // 3. Simular o percurso
    realizar_viagem_simulada(distancia);

    return 0;
}