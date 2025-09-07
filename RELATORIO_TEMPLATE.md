# Relatório: Mini-Projeto 1 - Quebra-Senhas Paralelo

**Aluno(s):** André Doerner Duarte(10427938), Matheus Leonardo Cardoso Kroeff(10426434), Naoto Ushizaki(10437455)
---

## 1. Estratégia de Paralelização

**Como você dividiu o espaço de busca entre os workers?**

<div align="justify">
  O espaco de busca foi dividido da seguinte forma: o total de senhas possíveis foi calculado com base na combinação de caracteres (total *= charset_len) e, em seguida, distribuído igualmente entre os processos (workers) na variável "password_per_worker". Cada worker é responsável por testar uma faixa específica de senhas, definida por um intervalo de índices numéricos. Como os processos não trabalham diretamente com índices, esses valores são convertidos em senhas reais utilizando o conjunto de caracteres (charset). Dessa forma, cada worker sabe exatamente quais combinações testar dentro de sua faixa designada.
  
</div>

##
  
**Código relevante:** Cole aqui a parte do coordinator.c onde você calcula a divisão:
```c
long long passwords_per_worker = total_space / num_workers;
long long remaining            = total_space % num_workers;

```
---

## 2. Implementação das System Calls

**Descreva como você usou fork(), execl() e wait() no coordinator:**
<div align="justify">
A gente usou o fork() para criar um processo filho. No filho (quando pid == 0), os argumentos necessários (hash, senha inicial, senha final, charset, comprimento da senha e id do worker) são convertidos para strings quando necessário e passados ao execl(), que substitui o processo filho pela execução do binário worker com esses parâmetros. Se o execl() falhar, o filho imprime um erro e encerra. Já o processo pai guarda o PID do worker criado e continua criando os demais. Após iniciar todos os workers, o coordenador usa wait() em um laço para aguardar a finalização de cada filho, capturando o status de saída ou sinal de término. Assim, fork() cria os processos, execl() define a função específica de cada worker e wait() sincroniza o coordenador com o término dos filhos.
</div>

**Código do fork/exec:**
```c
// pid_t pid = fork();

if (pid < 0) {
    perror("fork");
    exit(1);
}
else if (pid == 0) {  
    // Processo filho executa o worker
    execl("./worker", "worker", target_hash, start_pass, end_pass, charset, 
          password_len_str, worker_id_str, (char*)NULL);
    perror("execl");
    exit(1);
}
else {
    // Processo pai armazena o PID do worker
    workers[i] = pid;
}

while (finished_workers < num_workers) {
    int status;
    pid_t pid = wait(&status);
    ...
}
```

---

## 3. Comunicação Entre Processos

**Como você garantiu que apenas um worker escrevesse o resultado?**
<div align="justify">
No projeto, o mecanismo escolhido foi o arquivo password_found.txt como ponto de comunicação. Para evitar condições de corrida, não existe compartilhamento simultâneo de memória entre workers, e a escrita é feita de forma atômica, cada worker, ao encontrar a senha, escreve no arquivo e encerra. Como a criação do arquivo é única e não há necessidade de múltiplos workers continuarem escrevendo, basta que o primeiro a encontrar crie o arquivo. Os demais, ao tentarem abrir o arquivo, encontram que ele já existe e não sobrescrevem o resultado. Isso garante que apenas o primeiro worker vencedor salva a senha, evitando inconsistências.
</div>

**Como o coordinator consegue ler o resultado?**

<div align="justify">
Após todos os workers terminarem (coordenados com wait()), o processo principal (coordinator) abre o arquivo password_found.txt usando open() em modo somente leitura (O_RDONLY). Ele lê todo o conteúdo com read() em um buffer e garante o término da string com '\0'.

O coordinator então usa strchr(buffer, ':') para localizar o separador. Assim, divide a string em duas partes: antes do : está o ID do worker, e depois está a senha encontrada. Por fim, ele calcula o hash MD5 da senha, compara com o hash alvo e imprime o resultado.
</div>

---

## 4. Análise de Performance
Complete a tabela com tempos reais de execução:
O speedup é o tempo do teste com 1 worker dividido pelo tempo com 4 workers.

| Teste | 1 Worker | 2 Workers | 4 Workers | Speedup (4w) |
|-------|----------|-----------|-----------|--------------|
| Hash: 202cb962ac59075b964b07152d234b70<br>Charset: "0123456789"<br>Tamanho: 3<br>Senha: "123" | __0.001__s | __0.05__s | __0.001__s | __1.000__ |
| Hash: 5d41402abc4b2a76b9719d911017c592<br>Charset: "abcdefghijklmnopqrstuvwxyz"<br>Tamanho: 5<br>Senha: "hello" | __5.000__s | __8.000__s | __2.000__s | __0.400__ |

**O speedup foi linear? Por quê?**
  <div align="justify">
  O speedup não foi linear pelo fato de, ao aumentar o número de 'worker' de 1 para 2 em ambos os testes, foi observado um aumento no tempo de execução em vez de uma redução. Isso se deve ao 'overhead de processos', que inclui o alto consumo de memória pelos 'workers', o custo de mútiplas chamadas fork() (acaba consumindo muito tempo) e a latência envolvida na comunicação entre o coordinator e os workers — tanto no envio de tarefas quanto na coleta de resultados. 
  </div>

---

## 5. Desafios e Aprendizados
**Qual foi o maior desafio técnico que você enfrentou?**
[Descreva um problema e como resolveu. Ex: "Tive dificuldade com o incremento de senha, mas resolvi tratando-o como um contador em base variável"]

---

## Comandos de Teste Utilizados

```bash
# Teste básico
./coordinator "900150983cd24fb0d6963f7d28e17f72" 3 "abc" 2

# Teste de performance
time ./coordinator "202cb962ac59075b964b07152d234b70" 3 "0123456789" 1
time ./coordinator "202cb962ac59075b964b07152d234b70" 3 "0123456789" 4

# Teste com senha maior
time ./coordinator "5d41402abc4b2a76b9719d911017c592" 5 "abcdefghijklmnopqrstuvwxyz" 4
```
---

**Checklist de Entrega:**
- [ ] Código compila sem erros
- [ ] Todos os TODOs foram implementados
- [ ] Testes passam no `./tests/simple_test.sh`
- [ ] Relatório preenchido
