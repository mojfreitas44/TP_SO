#include "comum.h"

// ============================================================================
// ESTRUTURAS DE DADOS
// ============================================================================

#define MAX_AGENDAMENTOS 50 // Limite de pedidos em espera

typedef struct {
    pid_t pid;
    int fd_leitura;     
    int ocupado;
    pid_t pid_cliente;  // Guardar de quem é o veiculo (para o cancelar)
} Veiculo;

typedef struct {
    char username[50];
    pid_t pid_cliente;
    int hora;
    int distancia;
    char local[100];
    int ativo; // 1 = Pendente, 0 = Vazio/Executado
} Agendamento;

typedef struct {
    Veiculo frota[NVEICULOS];
    pid_t clientes[NUTILIZADORES]; 
    Agendamento agenda[MAX_AGENDAMENTOS];
    int num_veiculos;   
    int fd_clientes;    
    int tempo;          
    int total_km;       
} Controlador;

static Controlador ctrl;

// ============================================================================
// FUNÇÕES AUXILIARES GERAIS
// ============================================================================

void log_msg(const char *tag, const char *msg) {
    printf("[TEMPO %03d] %-12s %s\n", ctrl.tempo, tag, msg);
    fflush(stdout);
}

void registar_cliente(pid_t pid) {
    for (int i = 0; i < NUTILIZADORES; i++) {
        if (ctrl.clientes[i] == 0) {
            ctrl.clientes[i] = pid;
            return;
        }
    }
}

void remover_cliente(pid_t pid) {
    for (int i = 0; i < NUTILIZADORES; i++) {
        if (ctrl.clientes[i] == pid) {
            ctrl.clientes[i] = 0;
            return;
        }
    }
}

void limpar_recursos() {
    printf("\n[SISTEMA] A encerrar controlador e notificar todos...\n");
    
    // Notificar Clientes
    for (int i = 0; i < NUTILIZADORES; i++) {
        if (ctrl.clientes[i] > 0) kill(ctrl.clientes[i], SIGUSR1);
    }

    // Notificar Veículos
    for (int i = 0; i < ctrl.num_veiculos; i++) {
        if (ctrl.frota[i].pid > 0) {
            kill(ctrl.frota[i].pid, SIGKILL); 
            close(ctrl.frota[i].fd_leitura);
        }
    }
    
    if (ctrl.fd_clientes != -1) close(ctrl.fd_clientes);
    unlink(PIPE_CONTROLADOR);
}

void handler_sinal(int s) {
    exit(0); 
}

void setup_inicial() {
    setbuf(stdout, NULL); 
    memset(&ctrl, 0, sizeof(Controlador));
    ctrl.fd_clientes = -1;

    signal(SIGINT, handler_sinal);
    atexit(limpar_recursos);

    if (mkfifo(PIPE_CONTROLADOR, 0666) == -1 && errno != EEXIST) {
        perror("[ERRO] Falha no mkfifo");
        exit(1);
    }

    ctrl.fd_clientes = open(PIPE_CONTROLADOR, O_RDONLY | O_NONBLOCK);
    if (ctrl.fd_clientes == -1) {
        perror("[ERRO] Falha no open do FIFO");
        exit(1);
    }

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    printf("\n=== CONTROLADOR DE TÁXIS ===\n");
    printf("--- Comandos Disponíveis ---\n");
    printf(" listar   -> Ver agendamentos\n");
    printf(" utiliz   -> Ver utilizadores ligados\n");
    printf(" frota    -> Ver estado dos veículos\n");
    printf(" km       -> Ver total de KMs\n");
    printf(" hora     -> Ver tempo simulado\n");
    printf(" cancelar -> Cancelar serviço (ID/PID)\n");
    printf(" terminar -> Encerrar sistema\n");
    printf("----------------------------\n");

    log_msg("[SISTEMA]", "Controlador iniciado.");
}

// ============================================================================
// GESTÃO DE AGENDAMENTOS (Definido ANTES de ser usado)
// ============================================================================

void registar_agendamento(char* user, pid_t pid, int h, int d, char* loc) {
    for (int i = 0; i < MAX_AGENDAMENTOS; i++) {
        if (ctrl.agenda[i].ativo == 0) {
            strcpy(ctrl.agenda[i].username, user);
            ctrl.agenda[i].pid_cliente = pid;
            ctrl.agenda[i].hora = h;
            ctrl.agenda[i].distancia = d;
            strcpy(ctrl.agenda[i].local, loc);
            ctrl.agenda[i].ativo = 1; // Marca como ocupado
            
            char msg[100];
            sprintf(msg, "Agendado para t=%d (Slot %d)", h, i);
            log_msg("[AGENDA]", msg);
            return;
        }
    }
    log_msg("[ERRO]", "Lista de agendamentos cheia!");
}

// ============================================================================
// GESTÃO DE VEÍCULOS
// ============================================================================

void lancar_veiculo(char* user, int pid_cli, int dist, char* local) {
    int p[2];
    pid_t pid;
    char str_pid[20], str_dist[20];
    char buffer[100];

    if (ctrl.num_veiculos >= NVEICULOS) {
        log_msg("[AVISO]", "Frota cheia. Pedido recusado (ou adiado).");
        return;
    }

    if (pipe(p) == -1) {
        log_msg("[ERRO]", "Falha pipe anónimo");
        return;
    }

    pid = fork();

    if (pid == 0) { 
        // --- FILHO (VEÍCULO) ---
        close(p[0]); 
        close(STDOUT_FILENO); 
        dup(p[1]);            
        close(p[1]);          

        sprintf(str_pid, "%d", pid_cli);
        sprintf(str_dist, "%d", dist);

        execl("./veiculo", "veiculo", user, str_pid, str_dist, local, NULL);
        perror("[ERRO] execl falhou");
        _exit(1);

    } else if (pid > 0) { 
        // --- PAI (CONTROLADOR) ---
        close(p[1]); 
        int flags = fcntl(p[0], F_GETFL, 0);
        fcntl(p[0], F_SETFL, flags | O_NONBLOCK);

        int idx = ctrl.num_veiculos;
        ctrl.frota[idx].pid = pid;
        ctrl.frota[idx].pid_cliente = pid_cli; // Guardar dono
        ctrl.frota[idx].fd_leitura = p[0];
        ctrl.frota[idx].ocupado = 1;
        ctrl.num_veiculos++;

        sprintf(buffer, "Veículo %d enviado para %s", pid, user);
        log_msg("[FROTA]", buffer);
    }
}

// Verifica se algum agendamento chegou à hora
void verificar_agendamentos() {
    for (int i = 0; i < MAX_AGENDAMENTOS; i++) {
        if (ctrl.agenda[i].ativo == 1 && ctrl.agenda[i].hora <= ctrl.tempo) {
            
            if (ctrl.num_veiculos < NVEICULOS) {
                lancar_veiculo(ctrl.agenda[i].username, 
                             ctrl.agenda[i].pid_cliente, 
                             ctrl.agenda[i].distancia, 
                             ctrl.agenda[i].local);
                
                // Remove da lista
                ctrl.agenda[i].ativo = 0;
            }
        }
    }
}

void verificar_frota() {
    char buffer[256];
    char tag[20];
    int i, n;

    for (i = 0; i < ctrl.num_veiculos; i++) {
        if (ctrl.frota[i].pid > 0) { // Só verifica ativos
            n = read(ctrl.frota[i].fd_leitura, buffer, sizeof(buffer) - 1);
            if (n > 0) {
                buffer[n] = '\0';
                if (buffer[strlen(buffer)-1] == '\n') buffer[strlen(buffer)-1] = '\0';
                
                sprintf(tag, "[TAXI-%d]", ctrl.frota[i].pid);
                printf("[TEMPO %03d] %-12s %s\n", ctrl.tempo, tag, buffer);
                fflush(stdout);

                if (strstr(buffer, "concluída") != NULL) {
                    ctrl.total_km += 10; // Simplificado
                    // Num sistema real, aqui marcarias o veículo como livre ou removerias da lista
                }
            }
        }
    }
}

// ============================================================================
// GESTÃO DE PEDIDOS (CLIENTES)
// ============================================================================

void processar_comando_cliente(Mensagem *m) {
    char msg_buf[300];
    int h, d;
    char loc[100];

    if (strcmp(m->comando, "login") == 0) {
        registar_cliente(m->pid); 
        sprintf(msg_buf, "Cliente %s (PID %d) entrou.", m->username, m->pid);
        log_msg("[LOGIN]", msg_buf);
    }
    else if (strcmp(m->comando, "agendar") == 0) {
        if (sscanf(m->mensagem, "%d %99s %d", &h, loc, &d) == 3) {
            sprintf(msg_buf, "Agendar: %s, %dkm, %dh", loc, d, h);
            log_msg("[PEDIDO]", msg_buf);
            
            // Se for para AGORA e houver vaga
            if (h <= ctrl.tempo && ctrl.num_veiculos < NVEICULOS) {
                lancar_veiculo(m->username, m->pid, d, loc);
            } else {
                // Guarda para depois
                registar_agendamento(m->username, m->pid, h, d, loc);
            }
        } else {
            log_msg("[ERRO]", "Formato incorreto.");
        }
    } 
    else if (strcmp(m->comando, "cancelar") == 0) {
        // Lógica de cancelamento simplificada (apenas log por enquanto se preferires)
        // Ou podes descomentar a lógica de matar o processo que fizémos antes
        sprintf(msg_buf, "Cliente %s pediu cancelamento (Funcionalidade WIP).", m->username);
        log_msg("[CANCELAR]", msg_buf);
    }
    else if (strcmp(m->comando, "terminar") == 0) {
        remover_cliente(m->pid); 
        sprintf(msg_buf, "Cliente %s saiu.", m->username);
        log_msg("[LOGOUT]", msg_buf);
    }
}

void verificar_clientes() {
    Mensagem m;
    int n = read(ctrl.fd_clientes, &m, sizeof(Mensagem));
    
    if (n > 0) {
        processar_comando_cliente(&m);
    } 
    else if (n == 0) {
        close(ctrl.fd_clientes);
        ctrl.fd_clientes = open(PIPE_CONTROLADOR, O_RDONLY | O_NONBLOCK);
    }
}

// ============================================================================
// INTERFACE ADMIN
// ============================================================================

void verificar_admin() {
    char cmd[100];
    int n = read(STDIN_FILENO, cmd, sizeof(cmd)-1);
    if (n > 0) {
        cmd[n] = '\0';
        if (cmd[strlen(cmd)-1] == '\n') cmd[strlen(cmd)-1] = '\0';

        if (strcmp(cmd, "listar") == 0) {
            printf("--- STATUS: Tempo: %ds | Veículos: %d/%d | KM: %d ---\n", 
                   ctrl.tempo, ctrl.num_veiculos, NVEICULOS, ctrl.total_km);
            fflush(stdout);
        } 
        else if (strcmp(cmd, "terminar") == 0) {
            exit(0);
        }
    }
}

int main() {
    setup_inicial();

    while (1) {
        verificar_clientes(); 
        verificar_frota();
        verificar_agendamentos(); // Verificar lista de espera
        verificar_admin();    

        sleep(1); 
        ctrl.tempo++;
    }
    return 0;
}