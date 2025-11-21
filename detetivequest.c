/*
 Detective Quest - Sistema de mapas, pistas, BST e tabela hash
 Nível: Mestre
 Autor: Enigma Studios (exercício)
 Descrição:
  - Árvore binária (mapa da mansão) com salas que podem conter pistas.
  - BST armazena as pistas coletadas (em ordem alfabética). Cada nó tem contagem para pistas repetidas.
  - Tabela hash associa cada pista a um suspeito (chave = pista string, valor = nome do suspeito).
  - Navegação interativa a partir do Hall de Entrada: esquerda (e), direita (d), sair (s).
  - Ao final, jogador acusa um suspeito; se >= 2 pistas coletadas apontam para esse suspeito => acusação sustentada.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* =========================
   Definições básicas
   ========================= */
typedef struct Sala {
    char *nome;            // nome da sala
    char *pista;           // pista associada (NULL se não houver)
    int pistaColetada;     // flag para indicar se a pista já foi coletada
    struct Sala *esq;      // caminho esquerdo
    struct Sala *dir;      // caminho direito
} Sala;

/* Nó da BST para pistas coletadas */
typedef struct PistaNode {
    char *pista;
    int contador;               // número de vezes que a pista foi coletada (pode ser 1)
    struct PistaNode *esq;
    struct PistaNode *dir;
} PistaNode;

/* Entrada da tabela hash (lista encadeada para tratamento de colisões) */
typedef struct HashEntry {
    char *pista;            // chave
    char *suspeito;         // valor associado
    struct HashEntry *prox;
} HashEntry;

#define HASH_SIZE 31       // tamanho da tabela hash (primo pequeno)
HashEntry *tabelaHash[HASH_SIZE];

/* =========================
   Funções auxiliares de string
   ========================= */
char *strdup_safe(const char *s) {
    if (!s) return NULL;
    char *r = (char *)malloc(strlen(s) + 1);
    if (!r) {
        fprintf(stderr, "Erro de memória\n");
        exit(EXIT_FAILURE);
    }
    strcpy(r, s);
    return r;
}

/* trim newline */
void trim_nl(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    if (n == 0) return;
    if (s[n-1] == '\n') s[n-1] = '\0';
}

/* transforma para minúsculas para comparação case-insensitive */
void to_lower_inplace(char *s) {
    if (!s) return;
    for (; *s; ++s) *s = (char) tolower((unsigned char)*s);
}

/* =========================
   Funções para criar salas (árvore binária)
   ========================= */
/* criarSala: cria dinamicamente uma sala com nome e pista opcional */
Sala *criarSala(const char *nome, const char *pista) {
    Sala *s = (Sala *) malloc(sizeof(Sala));
    if (!s) {
        fprintf(stderr, "Falha ao alocar memória para sala\n");
        exit(EXIT_FAILURE);
    }
    s->nome = strdup_safe(nome);
    s->pista = pista ? strdup_safe(pista) : NULL;
    s->pistaColetada = 0;
    s->esq = s->dir = NULL;
    return s;
}

/* libera memória da árvore de salas */
void liberarSalas(Sala *root) {
    if (!root) return;
    liberarSalas(root->esq);
    liberarSalas(root->dir);
    free(root->nome);
    if (root->pista) free(root->pista);
    free(root);
}

/* =========================
   Funções BST (pistas coletadas)
   ========================= */
/* criar nó da BST */
PistaNode* novoNoPista(const char *pista) {
    PistaNode *n = (PistaNode *) malloc(sizeof(PistaNode));
    if (!n) {
        fprintf(stderr, "Falha ao alocar memória para nó de pista\n");
        exit(EXIT_FAILURE);
    }
    n->pista = strdup_safe(pista);
    n->contador = 1;
    n->esq = n->dir = NULL;
    return n;
}

/* inserirPista: insere ou incrementa contador se já existir (ordenado alfabeticamente) */
PistaNode* inserirPista(PistaNode *root, const char *pista) {
    if (!root) return novoNoPista(pista);
    int cmp = strcmp(pista, root->pista);
    if (cmp == 0) {
        root->contador++;
    } else if (cmp < 0) {
        root->esq = inserirPista(root->esq, pista);
    } else {
        root->dir = inserirPista(root->dir, pista);
    }
    return root;
}

/* exibirPistas: percurso em-ordem (alfabético) com contadores */
void exibirPistasInOrder(PistaNode *root) {
    if (!root) return;
    exibirPistasInOrder(root->esq);
    printf(" - \"%s\" (vezes coletada: %d)\n", root->pista, root->contador);
    exibirPistasInOrder(root->dir);
}

/* liberar BST */
void liberarPistas(PistaNode *root) {
    if (!root) return;
    liberarPistas(root->esq);
    liberarPistas(root->dir);
    free(root->pista);
    free(root);
}

/* =========================
   Funções da tabela hash
   ========================= */
/* hash djb2 simples */
unsigned long hash_djb2(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

/* inserirNaHash: associa pista -> suspeito */
void inserirNaHash(const char *pista, const char *suspeito) {
    unsigned long h = hash_djb2(pista) % HASH_SIZE;
    HashEntry *entry = (HashEntry *) malloc(sizeof(HashEntry));
    if (!entry) {
        fprintf(stderr, "Falha ao alocar memória para hash entry\n");
        exit(EXIT_FAILURE);
    }
    entry->pista = strdup_safe(pista);
    entry->suspeito = strdup_safe(suspeito);
    entry->prox = tabelaHash[h];
    tabelaHash[h] = entry;
}

/* encontrarSuspeito: retorna nome do suspeito associado à pista (ou NULL se não existir) */
char *encontrarSuspeito(const char *pista) {
    unsigned long h = hash_djb2(pista) % HASH_SIZE;
    HashEntry *cur = tabelaHash[h];
    while (cur) {
        if (strcmp(cur->pista, pista) == 0) {
            return cur->suspeito;
        }
        cur = cur->prox;
    }
    return NULL;
}

/* liberar tabela hash */
void liberarHash() {
    for (int i = 0; i < HASH_SIZE; ++i) {
        HashEntry *cur = tabelaHash[i];
        while (cur) {
            HashEntry *next = cur->prox;
            free(cur->pista);
            free(cur->suspeito);
            free(cur);
            cur = next;
        }
        tabelaHash[i] = NULL;
    }
}

/* =========================
   Exploração das salas e coleta de pistas
   ========================= */
/*
 explorarSalasComPistas:
  - navega interativamente a partir do nó inicial
  - comandos: e (esquerda), d (direita), s (sair)
  - ao visitar sala com pista não coletada: exibe e adiciona à BST
*/
void explorarSalasComPistas(Sala *inicio, PistaNode **rootPistas) {
    Sala *atual = inicio;
    char linha[128];

    printf("Começando a investigação a partir do Hall de Entrada.\n");
    while (1) {
        printf("\nVocê está na sala: %s\n", atual->nome);
        if (atual->pista && !atual->pistaColetada) {
            printf("Pista encontrada: \"%s\"\n", atual->pista);
            *rootPistas = inserirPista(*rootPistas, atual->pista);
            atual->pistaColetada = 1;
        } else if (atual->pista && atual->pistaColetada) {
            printf("Esta sala já teve sua pista coletada anteriormente.\n");
        } else {
            printf("Nenhuma pista nesta sala.\n");
        }

        printf("\nOpções: (e) esquerda, (d) direita, (s) sair da exploração\n");
        printf("Escolha: ");
        if (!fgets(linha, sizeof(linha), stdin)) break;
        trim_nl(linha);
        if (strlen(linha) == 0) continue;
        char cmd = (char) tolower((unsigned char)linha[0]);
        if (cmd == 's') {
            printf("Você optou por encerrar a exploração.\n");
            break;
        } else if (cmd == 'e') {
            if (atual->esq) {
                atual = atual->esq;
            } else {
                printf("Caminho à esquerda não existe a partir daqui.\n");
            }
        } else if (cmd == 'd') {
            if (atual->dir) {
                atual = atual->dir;
            } else {
                printf("Caminho à direita não existe a partir daqui.\n");
            }
        } else {
            printf("Comando inválido. Use 'e', 'd' ou 's'.\n");
        }
    }
}

/* =========================
   Função que percorre a BST e conta quantas pistas apontam para suspeito alvo
   ========================= */
int contarPistasQueApontam(PistaNode *root, const char *suspeitoAlvo) {
    if (!root) return 0;
    int total = 0;
    // verificar nó atual
    char *sus = encontrarSuspeito(root->pista);
    if (sus && strcmp(sus, suspeitoAlvo) == 0) {
        total += root->contador;
    }
    total += contarPistasQueApontam(root->esq, suspeitoAlvo);
    total += contarPistasQueApontam(root->dir, suspeitoAlvo);
    return total;
}

/* =========================
   Função principal (main)
   ========================= */
int main() {
    /* Inicializações */
    for (int i = 0; i < HASH_SIZE; ++i) tabelaHash[i] = NULL;
    PistaNode *rootPistas = NULL;

    /* Montagem manual do mapa da mansão (árvore binária)
       Exemplo de mapa (pode ser alterado):
                 Hall de Entrada
                 /           \
             Biblioteca    Sala de Estar
             /     \          /     \
         Cozinha  Jardim   Corredor  Oficina
    Cada sala pode ter uma pista (string) ou NULL.
    */
    Sala *hall = criarSala("Hall de Entrada", NULL);
    Sala *biblioteca = criarSala("Biblioteca", "Marca de luva com poeira");
    Sala *salaEstar = criarSala("Sala de Estar", "Copo quebrado com pegadas");
    Sala *cozinha = criarSala("Cozinha", "resto de chá de ervas");
    Sala *jardim = criarSala("Jardim", NULL);
    Sala *corredor = criarSala("Corredor", "notas rasgadas com iniciais A.B.");
    Sala *oficina = criarSala("Oficina", "peça de chave inglesa com verniz");

    /* ligações */
    hall->esq = biblioteca;
    hall->dir = salaEstar;

    biblioteca->esq = cozinha;
    biblioteca->dir = jardim;

    salaEstar->esq = corredor;
    salaEstar->dir = oficina;

    /* Preencher tabela hash: pista -> suspeito
       Regras da história (exemplo):
       - "Marca de luva com poeira" -> "Sr. Almeida"
       - "Copo quebrado com pegadas" -> "Sra. Beatriz"
       - "resto de chá de ervas" -> "Srta. Camila"
       - "notas rasgadas com iniciais A.B." -> "Sra. Beatriz"
       - "peça de chave inglesa com verniz" -> "Sr. Almeida"
    */
    inserirNaHash("Marca de luva com poeira", "Sr. Almeida");
    inserirNaHash("Copo quebrado com pegadas", "Sra. Beatriz");
    inserirNaHash("resto de chá de ervas", "Srta. Camila");
    inserirNaHash("notas rasgadas com iniciais A.B.", "Sra. Beatriz");
    inserirNaHash("peça de chave inglesa com verniz", "Sr. Almeida");

    /* Início da exploração */
    printf("=== Detective Quest - Investigação na Mansão ===\n");
    printf("Instruções: navegue entre salas com 'e' (esq), 'd' (dir) e saia com 's'.\n");
    explorarSalasComPistas(hall, &rootPistas);

    /* Exibir pistas coletadas em ordem alfabética */
    printf("\n=== PISTAS COLETADAS (ordem alfabética) ===\n");
    if (!rootPistas) {
        printf("Nenhuma pista coletada durante a investigação.\n");
    } else {
        exibirPistasInOrder(rootPistas);
    }

    /* Fase de acusação */
    char acusacao[128];
    printf("\nAgora, indique o nome do suspeito que deseja acusar (ex: \"Sra. Beatriz\").\n");
    printf("Nome do acusado: ");
    if (!fgets(acusacao, sizeof(acusacao), stdin)) {
        strcpy(acusacao, "");
    }
    trim_nl(acusacao);
    if (strlen(acusacao) == 0) {
        printf("Nenhum nome fornecido. Encerrando sem acusação.\n");
    } else {
        /* comparar com as strings dos suspeitos na tabela hash - contagem de pistas que apontam para o acusado */
        int totalQueApontam = contarPistasQueApontam(rootPistas, acusacao);
        printf("\nVocê acusou: %s\n", acusacao);
        printf("Número de pistas coletadas que apontam para %s: %d\n", acusacao, totalQueApontam);
        if (totalQueApontam >= 2) {
            printf("Resultado: ACUSAÇÃO SUSTENTADA! Existem evidências suficientes (>= 2 pistas).\n");
        } else {
            printf("Resultado: ACUSAÇÃO FRACA. Não há pistas suficientes para sustentar a acusação.\n");
        }
    }

    /* limpeza */
    liberarPistas(rootPistas);
    liberarHash();
    liberarSalas(hall);

    printf("\nEncerrando Detective Quest. Obrigado por jogar!\n");
    return 0;
}