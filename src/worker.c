#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <time.h>
#include "hash_utils.h"

/**
 * PROCESSO TRABALHADOR - Mini-Projeto 1: Quebra de Senhas Paralelo
 * 
 * Este programa verifica um subconjunto do espaço de senhas, usando a biblioteca
 * MD5 FORNECIDA para calcular hashes e comparar com o hash alvo.
 * 
 * Uso: ./worker <hash_alvo> <senha_inicial> <senha_final> <charset> <tamanho> <worker_id>
 * 
 * EXECUTADO AUTOMATICAMENTE pelo coordinator através de fork() + execl()
 * SEU TRABALHO: Implementar os TODOs de busca e comunicação
 */

#define RESULT_FILE "password_found.txt"
#define PROGRESS_INTERVAL 100000  // Reportar progresso a cada N senhas

/**
 * Incrementa uma senha para a próxima na ordem lexicográfica (aaa -> aab -> aac...)
 * 
 * @param password Senha atual (será modificada)
 * @param charset Conjunto de caracteres permitidos
 * @param charset_len Tamanho do conjunto
 * @param password_len Comprimento da senha
 * @return 1 se incrementou com sucesso, 0 se chegou ao limite (overflow)
 */
int increment_password(char *password, const char *charset, int charset_len, int password_len) {
    
    // TODO 1: Implementar o algoritmo de incremento de senha
    // OBJETIVO: Incrementar senha como um contador (ex: aaa -> aab -> aac -> aad...)
    // DICA: Começar do último caractere, como somar 1 em um número
    // DICA: Se um caractere "estoura", volta ao primeiro e incrementa o caracter a esquerda (aay -> aaz -> aba)
    for(int t = password_len - 1; t >= 0; t--){
        int index_charset = 0;
        
        while(password[t] != charset[index_charset] && index_charset < charset_len){
            index_charset++; //incrementa
        }
        
        if(index_charset == charset_len){
            return 0;
        } else if(index_charset < charset_len - 1){
            password[t] = charset[index_charset + 1];
            return 1;
        } else {
            password[t] = charset[0];
        }
    
    }

    // IMPLEMENTE AQUI:
    // - Percorrer password de trás para frente
    // - Para cada posição, encontrar índice atual no charset
    // - Incrementar índice
    // - Se não estourou: atualizar caractere e retornar 1
    // - Se estourou: definir como primeiro caractere e continuar loop
    // - Se todos estouraram: retornar 0 (fim do espaço)

    return 0;  // SUBSTITUA por sua implementação
}   

/**
 * Compara duas senhas lexicograficamente
 * 
 * @return -1 se a < b, 0 se a == b, 1 se a > b
 */
int password_compare(const char *a, const char *b) {
    if(strcmp(a, b) == 0){
        return 0;
    } else if(strcmp(a, b) < 0){
        return -1;
    } else {
        return 1;
    }
}

/**
 * Verifica se o arquivo de resultado já existe
 * Usado para parada antecipada se outro worker já encontrou a senha
 */
int check_result_exists() {
    if(access(RESULT_FILE, F_OK) == 0){
        if(access(RESULT_FILE, F_OK) == 0){
            return 1;
        } else {
            return 0;
        }   
    }
}
/**
 * Salva a senha encontrada no arquivo de resultado
 * Usa O_CREAT | O_EXCL para garantir escrita atômica (apenas um worker escreve)
 */
void save_result(int worker_id, const char *password) {
    // TODO 2: Implementar gravação atômica do resultado
    // OBJETIVO: Garantir que apenas UM worker escreva no arquivo
    // DICA: Use O_CREAT | O_EXCL - falha se arquivo já existe
    // FORMATO DO ARQUIVO: "worker_id:password\n"
    
    int open_arquivo = open(RESULT_FILE O_CREAT O_EXCL O_WRONLY 0644);

    if(open_arquivo >= 0){
        char buffer[100];
        int tamanho = snprintf(buffer, sizeof(buffer), "%d:%s\n", worker_id, password);

        write(open_arquivo, buffer, tamanho);
        close(open_arquivo);
    } else {
        printf("[Worker %d]Outro worker já encontrou a senha\n", worker_id);
    }
    // IMPLEMENTE AQUI:
    // - Tentar abrir arquivo com O_CREAT | O_EXCL | O_WRONLY
    // - Se sucesso: escrever resultado e fechar
    // - Se falhou: outro worker já encontrou
}

/**
 * Função principal do worker
 */
int main(int argc, char *argv[]) {
    // Validar argumentos
    if (argc != 7) {
        fprintf(stderr, "Uso interno: %s <hash> <start> <end> <charset> <len> <id>\n", argv[0]);
        return 1;
    }
    
    // Parse dos argumentos
    const char *target_hash = argv[1];
    char *start_password = argv[2];
    const char *end_password = argv[3];
    const char *charset = argv[4];
    int password_len = atoi(argv[5]);
    int worker_id = atoi(argv[6]);
    int charset_len = strlen(charset);
    
    printf("[Worker %d] Iniciado: %s até %s\n", worker_id, start_password, end_password);
    
    // Buffer para a senha atual
    char current_password[11];
    strcpy(current_password, start_password);
    
    // Buffer para o hash calculado
    char computed_hash[33];
    
    // Contadores para estatísticas
    long long passwords_checked = 0;
    time_t start_time = time(NULL);
    time_t last_progress_time = start_time;
    
    // Loop principal de verificação
    while (1) {
        // TODO 3: Verificar periodicamente se outro worker já encontrou a senha
        // DICA: A cada PROGRESS_INTERVAL senhas, verificar se arquivo resultado existe
        if(passwords_checked % PROGRESS_INTERVAL == 0){
            if(check_result_exists){
                break;    
        }
        // TODO 4: Calcular o hash MD5 da senha atual
        // IMPORTANTE: Use a biblioteca MD5 FORNECIDA - md5_string(senha, hash_buffer)
        md5_string(current_password, computed_hash);

        // TODO 5: Comparar com o hash alvo
        // Se encontrou: salvar resultado e terminar
        if(strcmp(computed_hash, target_hash) == 0){
            save_result(worker_id, current_password);
            break;
        }
        
        // TODO 6: Incrementar para a próxima senha
        // DICA: Use a função increment_password implementada acima
        int prox_senha = increment_password(current_password, charset, charset_len, password_len);
        
        // TODO: Verificar se chegou ao fim do intervalo
        // Se sim: terminar loop
        if(prox_senha == 0){
            break;
        }
        passwords_checked++;
    }
    
    // Estatísticas finais
    time_t end_time = time(NULL);
    double total_time = difftime(end_time, start_time);
    
    printf("[Worker %d] Finalizado. Total: %lld senhas em %.2f segundos", 
           worker_id, passwords_checked, total_time);
    if (total_time > 0) {
        printf(" (%.0f senhas/s)", passwords_checked / total_time);
    }
    printf("\n");
    
    return 0;
}
