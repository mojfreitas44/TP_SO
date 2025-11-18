#include "comum.h"

char pipe_cliente[100];
char pipe_veiculo_ativo[100] = "";
int fd_controlador;

// Função para limpar o pipe ao sair
void sair() {
    printf("[CLIENTE] A desligar e limpar o pipe de %s\n", pipe_cliente);
    unlink(pipe_cliente);
    close(fd_controlador);
}

// Função para receber e processar mensagens do controlador(processo filho)
void receberMensagens() {
    int fd_recebe = open(pipe_cliente, O_RDONLY);
    if( fd_recebe == -1) {
        perror("Erro a abrir pipe do cliente para leitura");
        exit(1);
    }
    Mensagem resp;
    while (read(fd_recebe, &resp, sizeof(resp)) > 0) {
        if(strcmp(resp.comando, "aviso_chegada") == 0){
            strcpy(pipe_veiculo_ativo, resp.mensagem);
            printf("[CLIENTE] Veículo a chegar! Pipe do veículo: %s\n", pipe_veiculo_ativo);
        } else if(strcmp(resp.comando, "aviso_fim_viagem")==0){
            strcpy(pipe_veiculo_ativo, "");
            printf("[CLIENTE] Viagem terminada pelo veículo.\n");
        }else if(strcmp(resp.comando, "resposta")==0){
            printf("[CLIENTE] Resposta do controlador: %s\n", resp.mensagem);
        }else{
            printf("[CLIENTE] Mensagem desconhecida: %s - %s\n", resp.username, resp.mensagem);
        }
    fflush(stdout);
    }
    close(fd_recebe);
    printf("[CLIENTE-FILHO] O pipe filho foi fechado.\n");
}

// Função para enviar comandos ao controlador (processo pai)
void enviarComandos(const char *username) {
    char input[50];
    Mensagem msg;
    msg.pid = getpid();
    strcpy(msg.username, username);

    printf("Intruduz Comandos: <agendar/cancelar/consultar/entrar/sair/terminar>\n");
    while (1)
    {   
        printf("> ");
        fflush(stdout);
        if (fgets(input, sizeof(input), stdin) == NULL) 
            break;
    
        input[strcspn(input, "\n")] = 0;
        char *comando = strtok(input, " \n");
        char* args = strtok(NULL, "\n");
        if (comando == NULL) 
            continue;
        strcpy(msg.comando, comando);
        if (args != NULL) {
            strcpy(msg.mensagem, args);
        } else {
            msg.mensagem[0] = '\0';
        }
        //comandos  a enviar para controlador
        if(strcmp(comando, "agendar") == 0 || strcmp(comando, "cancelar") == 0 || strcmp(comando, "consultar")== 0 
        || strcmp(comando,"consultar") == 0 || strcmp(comando, "terminar")== 0){
            if(write(fd_controlador, &msg, sizeof(Mensagem))==-1){
                perror("Erro a enviar mensagem ao controlador");
            }
            if(strcmp(comando, "terminar")== 0){
                break;
            }
        //comandos a enviar para veiculo
        }else if(strcmp(comando, "entrar") == 0 || strcmp(comando, "sair") == 0){
            if(pipe_veiculo_ativo[0] == '\0'){
                printf("Nenhum veículo ativo para enviar o comando %s.\n", comando);
                continue;
            }
            int fd_veiculo = open(pipe_veiculo_ativo, O_WRONLY);
            if(fd_veiculo == -1){
                perror("Erro a abrir pipe do veículo");
                continue;
            }
            if(write(fd_veiculo, &msg, sizeof(Mensagem))==-1){
                perror("Erro a enviar mensagem ao veículo");
            }
            close(fd_veiculo);
        
        }else
            printf("comando desconhecido, itroduza novamente.\n");
    } 
}

// Função para inicializar a conexão com o controlador
int inicializarConexao() {
    int fd_envia = open(PIPE_CONTROLADOR, O_WRONLY);
    if (fd_envia == -1) {
        printf("O controlador não está a correr.\n");
        return -1;
    }
    return fd_envia;
}

int main(int argc, char *argv[]) {
    
    access(PIPE_CONTROLADOR, F_OK);
    fd_controlador = open(PIPE_CONTROLADOR, O_WRONLY);
    if (fd_controlador == -1) {
        printf("O controlador não está a correr.\n");
        return 1;
    }
    if (argc != 2) {
        printf("Uso: ./cliente <username>\n");
        return 1;
    }

    sprintf(pipe_cliente, PIPE_CLIENTE, getpid());
    
    mkfifo(pipe_cliente, 0666);
    atexit(sair);

    printf("[CLIENTE %s] Iniciado (PID=%d)\n", argv[1], getpid());

    int fd_envia = inicializarConexao();
    if (fd_envia == -1) return 1;

    // Thread ou processo para ouvir respostas
    pid_t pid = fork();
    if (pid == 0) {
        receberMensagens();
        exit(0);
    }

    // Processo principal para enviar comandos
    enviarComandos(argv[1]); 

    close(fd_envia);
    return 0;
}