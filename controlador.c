#include "comum.h"


int tempo_simulado = 0;
int total_km = 0;

// ...existing code...
void limparPipes()
{
    unlink(PIPE_CONTROLADOR);
}

// Envia uma resposta ao cliente que enviou 'msg'

// Trata os comandos recebidos numa Mensagem
void processa_comandos(char* linha)
{
    char comando[50], arg[50];
    int num_args = sscanf(linha, "%49s %49s", comando, arg);
    if(num_args == 0) return;
    printf("[CONTROLADOR] Comando recebido: %s\n", linha);
    

    if (strcmp(comando, "listar") == 0)
    {
        // Exemplo: listar utilizadores ou serviços
        printf("[CONTROLADOR] Comando 'listar' recebido\n");
        // ... Itera pelo teu array 'agendamentos' e imprime ...
    }
    else if (strcmp(comando, "utiliz") == 0)
    // ... Itera pelo teu array 'utilizadores' e imprime ...
    {
    }
    else if (strcmp(comando, "frota") == 0)
    {
        // ... Itera pelo teu array 'frota' e imprime progresso ...
    }

    else if (strcmp(comando, "cancelar") == 0 && num_args == 2)
    {
        int id= atoi(arg);
        if( id == 0){
            printf("[CONTROLADOR] Comando 'cancelar' inválido: %s\n", arg);
            // ... Limpa array 'agendamentos' ...
            // ... Itera pelo array 'frota' e envia kill(pid, SIGUSR1) a todos
        }else{
            // ... Procura 'id' em 'agendamentos' e remove ...
            // ... Procura 'id' em 'frota' e envia kill(pid, SIGUSR1)[
        }
    }else if (strcmp(comando, "km") == 0)
    {
        printf("[ADMIN] Total de KMs: %d\n", total_km);
    }
    else if (strcmp(comando, "hora") == 0)
    {
        printf("[ADMIN] Tempo Simulado: %d segundos\n", tempo_simulado);
    }
    else if (strcmp(comando, "terminar") == 0)
    {
        printf("[ADMIN] A terminar o sistema...\n");
        unlink(PIPE_CONTROLADOR);
        exit(0);
    }
    else
    {
        // Outros comandos: apenas log por agora
        printf("[CONTROLADOR] Comando desconhecido: %s\n", comando);
    }
}

void Resposta(const Mensagem *msg)
{
    char pipe_cliente[100];
    snprintf(pipe_cliente, sizeof(pipe_cliente), PIPE_CLIENTE, msg->pid);

    int fd_cliente = open(pipe_cliente, O_WRONLY);
    if (fd_cliente == -1)
    {
        // Não conseguimos abrir o pipe do cliente; apenas log
        perror("Resposta: erro a abrir pipe do cliente");
        return;
    }

    Mensagem resposta;
    memset(&resposta, 0, sizeof(Mensagem));
    resposta.pid = getpid();
    snprintf(resposta.comando, sizeof(resposta.comando), "resposta");
    snprintf(resposta.mensagem, sizeof(resposta.mensagem), "Olá %s, recebi o teu pedido!", msg->username);

    if (write(fd_cliente, &resposta, sizeof(Mensagem)) == -1)
    {
        perror("Resposta: erro a escrever para pipe do cliente");
    }
    close(fd_cliente);
}

void exec_veiculos(const Mensagem *msg_agendamento, int hora, const char *local, int distancia)
{
    int veiculos_ativos;

    if (veiculos_ativos >= NVEICULOS)
    {
        printf("[CONTROLADOR] Número máximo de veículos ativos atingido.\n");
        return;
    }
    
    
    int pipe_telemetria[2];
    if (pipe(pipe_telemetria) == -1)
    {
        perror("lancar_veiculo: pipe");
        return;
    }
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("comandos: fork falhou");

        return;
    }
    if (pid == 0)
    {
        // Filho = veículo
        close(pipe_telemetria[0]);
        dup2(pipe_telemetria[1], STDOUT_FILENO);
        close(pipe_telemetria[1]);

        char pid_cliente[20], distancia_str[20], hora_str[20];
        sprintf(pid_cliente, "%d", msg_agendamento->pid);
        sprintf(distancia_str, "%d", distancia);
        printf("[VEÍCULO] Lançado  user=%s PID=%s, distancia:%d, local=%s\n", msg_agendamento->username, pid_cliente, distancia, local);
        fflush(stdout);
        execl("./veiculo", "veiculo", msg_agendamento->username, pid_cliente, distancia_str, local, (char *)NULL);
        perror("comandos: erro no exec do veículo");
        _exit(1);
    }
    else // Pai = controlador
    {
        close(pipe_telemetria[1]);
        printf("[CONTROLADOR] Veículo lançado (pid=%d) para cliente %s\n", pid, msg_agendamento->username);
        // **CRÍTICO**: Guardar o FD de leitura e o PID do veículo
        // ... Adiciona 'pid' e 'pipe_telemetria[0]' ao teu array 'frota' ...
        // ... Define o 'pipe_telemetria[0]' como O_NONBLOCK ...
        // fcntl(pipe_telemetria[0], F_SETFL, O_NONBLOCK);
        // num_veiculos_ativos++;
    }
}

void processar_pedido_cliente(Mensagem* msg) {
    int tempo_simulado = 0; // Deves manter isto atualizado no controlador
    
    printf("[CONTROLADOR-CLIENTE] Recebido '%s' de %s\n", msg->comando, msg->username);

    // (Aqui deves ter a lógica de registar novo utilizador se for a 1ª vez)
    // ...
    
    if (strcmp(msg->comando, "agendar") == 0) {
        // [cite: 138]
        int hora, distancia;
        char local[100];
        if (sscanf(msg->mensagem, "%d %99s 
            %d", &hora, local, &distancia) == 3) {
            printf("[CONTROLADOR] Agendamento: H: %d, L: %s, D: %d\n", hora, local, distancia);
            
            // Lógica de agendamento:
            if (hora == tempo_simulado) {
                // Se for para "agora", lança já
                exec_veiculos(msg, hora, local, distancia);
            } else {
                // Se for para o futuro, guarda no array 'agendamentos'
                // ... (adicionar ao array 'agendamentos') ...
            }
            // ... (enviar resposta de sucesso ao cliente) ...
            
        } else {
            // ... (enviar resposta de erro ao cliente) ...
        }

    } else if (strcmp(msg->comando, "cancelar") == 0) {
        // [cite: 142]
        int id = atoi(msg->mensagem);
        printf("[CONTROLADOR] %s pede para cancelar ID %d\n", msg->username, id);
        // ... Procura no array 'agendamentos' pelo 'id' E 'msg->username' ...
        // ... Se encontrar, remove e envia sucesso ...
        // ... Se não, envia erro ...
        
    } else if (strcmp(msg->comando, "consultar") == 0) {
        // [cite: 144]
        printf("[CONTROLADOR] %s pede para consultar\n", msg->username);
        // ... Itera no array 'agendamentos' por 'msg->username' ...
        // ... Constrói uma string com os resultados ...
        // ... Envia a string de resposta ao cliente ...
        
    } else if (strcmp(msg->comando, "terminar") == 0) {
        // [cite: 158]
        printf("[CONTROLADOR] %s está a terminar\n", msg->username);
        // ... Cancela todos os serviços agendados deste user ('cancelar 0' só para ele) ...
        // ... Remove o user do array 'utilizadores' ...
        // ... Envia "Adeus" ...
    }
}

void verificar_agendamentos(int tempo_atual)
{
    // Itera pelo array 'agendamentos'
    // Se algum agendamento tiver hora == tempo_simulado:
    //    lancar_veiculo(...)
    //    remover do array 'agendamentos'
}

void processa_telemetria_veiculo(int indice_frota, char*dados){
    
}


int main()
{
    printf("[CONTROLADOR] A iniciar...\n");
    atexit(limparPipes);

    // Cria pipe principal
    if (mkfifo(PIPE_CONTROLADOR, 0666) == -1 && errno != EEXIST)
    {
        perror("Erro a criar FIFO do controlador");
        exit(1);
    }

    int fd_controlador = open(PIPE_CONTROLADOR, O_RDONLY | O_NONBLOCK);
    if (fd_controlador == -1)
    {
        perror("Erro a abrir FIFO do controlador");
        exit(1);
    }

    Mensagem msg;
    printf("[CONTROLADOR] A aguardar mensagens...\n");

    while (1)
    {
        int n = read(fd_controlador, &msg, sizeof(Mensagem));
        if (n > 0)
        {
            printf("[CONTROLADOR] Recebido de %s: %s\n", msg.username, msg.comando);
            processar_pedido_cliente(&msg);
        }
        else if (n == 0)
        {
            // EOF: reabrir o pipe para continuar a ler sem bloquear
            close(fd_controlador);
            fd_controlador = open(PIPE_CONTROLADOR, O_RDONLY);
            if (fd_controlador == -1)
            {
                perror("Erro a reabrir FIFO do controlador");
                exit(1);
            }
        }
        else
        {
            perror("Erro a ler do pipe do controlador");
        }
    }

    close(fd_controlador);
    unlink(PIPE_CONTROLADOR);
    return 0;
}