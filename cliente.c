#include "comum.h"

char pipe_cliente[100];
char pipe_veiculo_ativo[100] = "";
int fd_controlador;

// ============================================================================
// FUNÇÕES AUXILIARES
// ============================================================================

void sair() {
    printf("\n[CLIENTE] A desligar...\n");
    if (fd_controlador != -1) close(fd_controlador);
    unlink(pipe_cliente);
}

// Trata CTRL+C (SIGINT) e Encerramento do Servidor (SIGUSR1)
void trataSinais(int s) {
    if (s == SIGUSR1) {
        printf("\n[AVISO] O Controlador encerrou o sistema. A sair...\n");
    }
    exit(0); // Chama o atexit(sair)
}

// ============================================================================
// RECEÇÃO (FILHO)
// ============================================================================

void receberMensagens() {
    // ALTERAÇÃO AQUI: Usar O_RDWR em vez de O_RDONLY
    // Isto impede que o read retorne 0 (EOF) quando o veículo fecha a ligação
    int fd_recebe = open(pipe_cliente, O_RDWR); 
    
    if(fd_recebe == -1) {
        perror("[ERRO] Abrir pipe do cliente");
        exit(1);
    }

    Mensagem resp;
    // O ciclo agora mantém-se vivo mesmo que o veículo saia
    while (read(fd_recebe, &resp, sizeof(resp)) > 0) {
        
        if(strcmp(resp.comando, "status") == 0) {
            printf("[VEÍCULO] %s\n", resp.mensagem);
        } 
        else if(strstr(resp.mensagem, "concluída") != NULL || strcmp(resp.comando, "fim") == 0){
            pipe_veiculo_ativo[0] = '\0';
            printf("[VEÍCULO] Viagem terminada. (Podes agendar nova viagem)\n");
        }
        else {
            printf("[CONTROLADOR] %s\n", resp.mensagem);
        }
        
        printf("> "); 
        fflush(stdout);
    }
    
    close(fd_recebe);
}

// ============================================================================
// ENVIO (PAI)
// ============================================================================

void enviarComandos(const char *username) {
    char input[100];
    Mensagem msg;
    
    msg.pid = getpid();
    strcpy(msg.username, username);

    // MENU SIMPLIFICADO (Sem entrar/sair)
    printf("\n--- Comandos Disponíveis ---\n");
    printf(" agendar <hora> <local> <km>\n");
    printf(" terminar\n");
    printf("----------------------------\n");

    while (1) {   
        printf("> ");
        if (fgets(input, sizeof(input), stdin) == NULL) break;
        
        // Remover \n manualmente
        if (strlen(input) > 0 && input[strlen(input)-1] == '\n') {
            input[strlen(input)-1] = '\0';
        }

        if (strlen(input) == 0) continue;

        char *cmd = strtok(input, " ");
        char *args = strtok(NULL, ""); 

        if (cmd == NULL) continue;

        strcpy(msg.comando, cmd);
        
        if (args != NULL) strcpy(msg.mensagem, args);
        else msg.mensagem[0] = '\0';

        // Apenas aceita os comandos de gestão, já não aceita entrar/sair
        if(strcmp(cmd, "agendar") == 0 || strcmp(cmd, "cancelar") == 0 || strcmp(cmd, "terminar") == 0) {
            write(fd_controlador, &msg, sizeof(Mensagem));
            
            if(strcmp(cmd, "terminar") == 0) break;
        } else {
            printf("Comando desconhecido ou inválido.\n");
        }
    } 
}

int main(int argc, char *argv[]) {
    setbuf(stdout, NULL); 
    
    if (argc != 2) {
        printf("Uso: ./cliente <username>\n");
        return 1;
    }

    if (access(PIPE_CONTROLADOR, F_OK) == -1) {
        printf("[ERRO] Controlador inativo.\n");
        return 1;
    }

    fd_controlador = open(PIPE_CONTROLADOR, O_WRONLY);
    if (fd_controlador == -1) return 1;

    sprintf(pipe_cliente, PIPE_CLIENTE, getpid());
    if (mkfifo(pipe_cliente, 0666) == -1 && errno != EEXIST) return 1;

    // Configura SINAIS
    signal(SIGINT, trataSinais);
    signal(SIGUSR1, trataSinais); 
    atexit(sair);

    printf("[CLIENTE %s] PID %d\n", argv[1], getpid());

    // LOGIN AUTOMÁTICO
    Mensagem login;
    login.pid = getpid();
    strcpy(login.username, argv[1]);
    strcpy(login.comando, "login");
    strcpy(login.mensagem, "Entrei"); 
    write(fd_controlador, &login, sizeof(Mensagem));

    if (fork() == 0) {
        receberMensagens();
        exit(0);
    }

    enviarComandos(argv[1]);
    return 0;
}