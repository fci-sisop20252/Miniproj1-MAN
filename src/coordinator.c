#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include "hash_utils.h"

/**
 * PROCESSO COORDENADOR - Mini-Projeto 1: Quebra de Senhas Paralelo
 * 
 * Este programa coordena múltiplos workers para quebrar senhas MD5 em paralelo.
 * O MD5 JÁ ESTÁ IMPLEMENTADO - você deve focar na paralelização (fork/exec/wait).
 * 
 * Uso: ./coordinator <hash_md5> <tamanho> <charset> <num_workers>
 * 
 * Exemplo: ./coordinator "900150983cd24fb0d6963f7d28e17f72" 3 "abc" 4
 * 
 * SEU TRABALHO: Implementar os TODOs marcados abaixo
 */

#define MAX_WORKERS 16
#define RESULT_FILE "password_found.txt"

/**
 * Calcula o tamanho total do espaço de busca
 * 
 * @param charset_len Tamanho do conjunto de caracteres
 * @param password_len Comprimento da senha
 * @return Número total de combinações possíveis
 */
long long calculate_search_space(int charset_len, int password_len) {
    long long total = 1;
    for (int i = 0; i < password_len; i++) {
        total *= charset_len;
    }
    return total;
}

/**
 * Converte um índice numérico para uma senha
 * Usado para definir os limites de cada worker
 * 
 * @param index Índice numérico da senha
 * @param charset Conjunto de caracteres
 * @param charset_len Tamanho do conjunto
 * @param password_len Comprimento da senha
 * @param output Buffer para armazenar a senha gerada
 */
void index_to_password(long long index, const char *charset, int charset_len, 
                       int password_len, char *output) {
    for (int i = password_len - 1; i >= 0; i--) {
        output[i] = charset[index % charset_len];
        index /= charset_len;
    }
    output[password_len] = '\0';
}

/**
 * Função principal do coordenador
 * @return 0 em caso de sucesso, 1 em erro de uso, 2 caso senha não encontrada, -1 em caso de erro
 */
int main(int argc, char *argv[]) {
    // Verifica se o usurário passou 6 argumentos para o correto funcionamento do código
    // Argumentos a serem passados: ./@file <hash> <pass_len> <charset> <num_workers> <charset_len>
    if (argc != 6) {
        fprintf(stderr, "Uso interno: %s <hash> <pass_len> <charset> <num_workers> <charset_len>\n", argv[0]);
        return 1;
    }

    // Parsing dos argumentos (após validação)
    const char *target_hash = argv[1]; // <hash>
    int password_len = atoi(argv[2]);  // <pass_len>
    const char *charset = argv[3];     // <charset>
    int num_workers = atoi(argv[4]);   // <num_workers>
    int charset_len = strlen(charset); // <charset_len>



    // |==================================================|
    // |============ Validações dos parâmetros ===========|
    // |==================================================|

    // password_len deve estar entre 1 e 10     algebra
    if (password_len < 1 || password_len > 10) {
        fprintf(stderr, "Tamanho de senha invalido [%d]. Valor deve estar entre 1 e 10\n", password_len);
        return 1;
    }

    // num_workers deve estar entre 1 e MAX_WORKERS
    if (num_workers < 1 || num_workers > MAX_WORKERS) {
        fprintf(stderr, "Quantidade de workers invalida [%d]. Quantidade deve ser entre 1 e %d\n", num_workers, MAX_WORKERS);
        return 1;
    }

    // charset não pode ser vazio
    if (strlen(charset) == 0) {
        fprintf(stderr, "Charset nao pode estar vazio!\n");
        return 1;
    }



    // |==================================================|
    // |============== Inicio dos Processos ==============|
    // |==================================================|

    // Prints iniciais
    printf("=== Mini-Projeto 1: Quebra de Senhas Paralelo ===\n");
    printf("Hash MD5 alvo: %s\n", target_hash);
    printf("Tamanho da senha: %d\n", password_len);
    printf("Charset: %s (tamanho: %d)\n", charset, charset_len);
    printf("Número de workers: %d\n", num_workers);

    // Calcular espaço de busca total
    long long total_space = calculate_search_space(charset_len, password_len);
    printf("Espaço de busca total: %lld combinações\n\n", total_space);

    // Remover arquivo de resultado anterior se existir
    unlink(RESULT_FILE);

    // Registrar tempo de início
    time_t start_time = time(NULL);



    // |==================================================|
    // |===================== Workers ====================|
    // |==================================================|

    // Calculando quantas senhas devem ser testadas por worker e senhas restantes
    long long passwords_per_worker = total_space / num_workers;
    long long remaining            = total_space % num_workers;
    
    printf("Iniciando workers...\nSenhas por worker: %lld\nSenhas restantes:  %lld\n\n", passwords_per_worker, remaining);
    


    // Arrays para armazenar PIDs dos workers
    pid_t workers[MAX_WORKERS];
    
    // Criando processos filhos com fork(), cada um com uma parte das senhas a serem testadas:
    // 'for' que cria 'num_workers' workers
    for (int i = 0; i < num_workers; i++) {
        // |==================================================|
        // |======== Calculo de INICIO e FIM do worker =======|
        // |==================================================|

        // Calcula o index da senha de INICIO e FIM desse worker
        long long start_index = i * passwords_per_worker;
        long long end_index   = start_index + passwords_per_worker;

        // O ultimo worker deve pegar os casos que faltaram
        if (i == num_workers - 1) end_index += remaining - 1;

        // Prepara uma variavel tipo 'char' para receber a string da senha de INICIO e FIM desse worker
        char start_pass [password_len + 1];
        char end_pass [password_len + 1];

        // Gerando a string de INICIO e FIM a partir do start_index:
        index_to_password(start_index, charset, charset_len, password_len, start_pass); // String de INICIO: 'start_pass'
        index_to_password(end_index, charset, charset_len, password_len, end_pass); // String de FIM: 'end_pass'



        // |==================================================|
        // |=============== Criando os Workers ===============|
        // |==================================================|

        // Usar fork() para criar processo filho
        pid_t pid = fork(); // pid < 0:  Erro ao criar worker
                            // pid == 0: Processo filho
                            // pid > 0:  Processo pai
        
        // Erro ao criar worker
        if (pid < 0) {
            perror("frok");
            exit(1);
        }
        
        // Processo filho
        else if (pid == 0) {
            // Transformando 'password_len' em 'string' - Necessário para usar o 'execl'
            char password_len_str [16]; // String que vai receber (string)password_len
            snprintf(password_len_str, sizeof(password_len_str), "%d", password_len); // password_len_str = (string)password_len
            
            // Transformando o contador 'i' em 'string' - Necessário para usar o 'execl'
            char worker_id_str[16]; // String que vai receber (string)i
            snprintf(worker_id_str, sizeof(worker_id_str), "%d", i); // worker_id_str = (string)i
            
            // Executando @file 'worker' com parâmetros certos
            // Uso do worker: ./worker <hash> <start> <end> <charset> <len> <id>\n"
            execl("./worker", "worker", target_hash, start_pass, end_pass, charset, password_len_str, worker_id_str, (char*)NULL);

            // Se isso aqui executar eh porque algum erro aconteceu ao executar função no worker:
            perror("execl"); // Mensagem de erro
            exit(1); // Codigo de saida
        }

        // Processo pai
        else {
            printf("Iniciando worker %3d - pid: %d\n", i, pid);

            workers[i] = pid; // Salvando o pid ("id") do processo criado
        }

        usleep(100); // 0.0001 de dalay para não embolar os prints
    }
    printf("\nTodos os workers foram iniciados. Aguardando conclusão... \n\n");



    // |==================================================|
    // |========== Esperando workers terminarem ==========|
    // |==================================================|

    int finished_workers = 0;
    // Roda 'num_workers' vezes, ou seja, espera todos os workers
    while (finished_workers < num_workers) {
        int status;
        pid_t pid = wait(&status); // Espera o retorno do 'pid' de qualquer um dos workers

        // pid == -1: Erro ao receber o pid do worker ou erro com o worker
        if (pid == -1) {
            perror("wait");
            continue;
        }

        // Descobre qual o ID ('workers[>ID<]') do worker atraves de seu 'pid'
        int worker_id = -1; // Se o 'pid' nao for encontrado em 'workers[]' permanece em -1, indicando "'pid' desconhecido"
        for (int j = 0; j < MAX_WORKERS; j++) {
            if (workers[j] == pid) {
                worker_id = j;
                break;
            }
        }
        
        // 'pid' desconhecido: Não incrementa 'finished_workers'
        if (worker_id == -1) {
            fprintf(stderr, "Worker %d desconhecido", pid);
            continue;
        }
        
        // Caso worker tenha finalizado corretamente
        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status); // Pega o codigo de saida
            printf("Worker %3d [PID %d] terminou corretamente com codigo: %d\n", worker_id, pid, exit_code);
        }
        // Caso worker tenha finalizado com algum sinal específico (improvavel)
        else if (WIFSIGNALED(status)) {
            int exit_signal = WTERMSIG(status); // Pega o sinal retornado
            printf("Worker %3d [PID %d] terminou com um sinal: %d\n", worker_id, pid, exit_signal);
        }
        // Caso worker tenha finalizado por algum erro
        else {
            printf("Worker %3d [PID %d] terminou de forma inesperada\n", worker_id, pid);
        }

        finished_workers++; // Incrementa na quantidade de workers que retornaram
    }



    // |==================================================|
    // |=================== Resultados ===================|
    // |==================================================|

    // Registrar tempo de fim
    time_t end_time = time(NULL);
    // Calcula tempo total de execução do processo de quebra de senha
    double elapsed_time = difftime(end_time, start_time);



    // Print inicial dos resultados
    printf("\n\n\n|==================================================|\n");
    printf(      "|=================== Resultados ===================|\n");
    printf(      "|==================================================|\n\n");



    // Tenta abrir o arquivo onde a senha descriptografada foi salva
    int file = open(RESULT_FILE, O_RDONLY); // file < 0: Arquivo inexistente - Senha não encontrada

    // Caso senha não tenha sido encontrada
    if (file < 0) {
        printf("Senha nao encontrada!\nVerifique se as informacoes estao corretas:\n\n");
        printf("Hash: %s\n", target_hash);
        printf("Tamanho da senha: %d\n", password_len);
        printf("Charset: %s (tamanho: %d)\n", charset, charset_len);
        printf("Workers: %d\n", num_workers);
        return 2;
    }

    // |==================================================|
    // |===========    Leitura e tratamento    ===========|
    // |=========== das informações do arquivo ===========|
    // |==================================================|

    // Caso senha tenha sido encontrada, continua a ler o código:
    // Lê o arquivo 'RESULT_FILE'
    char buffer[1024];
    ssize_t bytes = read(file, buffer, sizeof(buffer) - 1); // Formato do arquivo: "<worker_id>:<senha_encontrada>"
    close(file);

    // byts <= 0: indica erro ao ler arquivo
    if (bytes <= 0 ) {
        perror("read");
        return -1;
    }

    buffer[bytes] = '\0'; // Adiciona o 'fim de string' ao buffer do arquivo (da ruim se não colocar)
                          // "<worker_id>:<senha_encontrada>" --> "<worker_id>:<senha_encontrada>\0"
                          //                               ^                                     ^^

    // Divide o arquivo lido onde for encontrado o char ':'
    char *infos = strchr(buffer, ':'); // 'buffer' continua apontando para o inicio da string
                                       // 'infos' aponta para onde está o ':'
                                       //
                                       // <worker_id>:<senha_encontrada>\0
                                       // ^          ^
                                       // buffer     infos

    // Caso o formato seja incompativel: Erro ao salvar arquivo, ou informações adulteradas
    if (!infos) {
        fprintf(stderr, "Formato incorreno no arquivo\n");
        return -1;
    }
    
    // Caso formato seja compativel, continua a ler o código:

    *infos = '\0'; // Adiciona o 'fim de string' onde estava o ':'
                   // "<worker_id>:<senha_encontrada>\0" --> "<worker_id>\0<senha_encontrada>\0"
                   //             ^                                      ^^

    char *worker_id = buffer;    // <worker_id>\0<senha_encontrada>\0
                                 // ^^^^^^^^^^^^^
    char *password  = infos + 1; // <worker_id>\0<senha_encontrada>\0
                                 //              ^^^^^^^^^^^^^^^^^^^^



    // |==================================================|
    // |================== Prints finais =================|
    // |==================================================|

    // Cria o hash da senha encontrada
    char hash_found[33];
    md5_string(password, hash_found);

    // Se 'hash_found' estiver igual 'target_hash',
    // ou seja, se a senha encontrada for a descriptgrafia de 'target_hash':
    if (strcmp(hash_found, target_hash) == 0)
        printf("Senha encontrada com sucesso!\n"
               "Worker: %s\n"
               "Hash:   %s\n"
               "Senha:  %s\n\n", worker_id, hash_found, password);
    // Se a senha encontrada NÃO for a descriptgrafia de 'target_hash':
    else
        printf("Senha encontrada incorreta\n"
               "Hash esperado:    %s\n"
               "Hash encontrado:  %s\n"
               "Senha encontrada: %s", target_hash, hash_found, password);



    // |==================================================|
    // |========== Print de estatísticas finais ==========|
    // |==================================================|

    // Print de tempo de execução
    printf("Tempo de execucao: %fsegundos\n", elapsed_time);

    // Retrun final
    return 0;
}