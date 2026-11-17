/*
 * estruturas.h
 * SINAL ZERO - Structs e constantes usadas pelo jogo todo.
 *
 * Aqui só tem declaração. As funções estão em src/jogo.c.
 */
#ifndef ESTRUTURAS_H
#define ESTRUTURAS_H

/* ---------------- Quantidades e limites ---------------- */
#define QTD_AMBIENTES     9
#define QTD_ITENS         6
#define QTD_ENIGMAS       4
#define QTD_OBJETOS      14     /* objetos fixos dos cenários (terminal, painel...) */
#define QTD_REGISTROS     6     /* textos que podem ir para o diário */
#define MAX_INVENTARIO    5     /* capacidade máxima do inventário   */
#define VIDA_MAXIMA     100
#define MAX_COMANDO     128     /* tamanho de uma linha digitada      */

/* ---------------- Direções (índices de saidas[4]) ---------------- */
#define NORTE 0
#define SUL   1
#define LESTE 2
#define OESTE 3
#define SEM_SAIDA (-1)

/* ---------------- Tipos de trava de uma passagem ----------------
 * Guardados em saidasBloqueadas[4]: 0 = livre, outro valor = motivo. */
#define TRAVA_NENHUMA 0
#define TRAVA_CARTAO  1     /* porta eletrônica do laboratório       */
#define TRAVA_ENERGIA 2     /* porta motorizada: precisa de energia  */
#define TRAVA_SINAL   3     /* doca: só abre com o sinal desativado  */

/* ---------------- IDs dos ambientes ---------------- */
#define AMB_DORMITORIO  0
#define AMB_CORREDOR    1
#define AMB_LABORATORIO 2
#define AMB_ARQUIVO     3
#define AMB_MAQUINAS    4
#define AMB_SONAR       5
#define AMB_MANUTENCAO  6
#define AMB_CONTENCAO   7
#define AMB_DOCA        8

/* ---------------- IDs dos itens ---------------- */
#define ITEM_CARTAO   0
#define ITEM_FUSIVEL  1
#define ITEM_MODULO   2
#define ITEM_CHAVE    3
#define ITEM_OXIGENIO 4
#define ITEM_AMOSTRA  5

/* Valores especiais de Item.localAtual */
#define LOCAL_INVENTARIO (-1)   /* item está com o jogador         */
#define LOCAL_CONSUMIDO  (-2)   /* item foi gasto/instalado        */
#define USO_QUALQUER     (-1)   /* item pode ser usado em qualquer lugar */

/* ---------------- IDs dos enigmas ---------------- */
#define ENIGMA_PORTA   0
#define ENIGMA_ENERGIA 1
#define ENIGMA_SONAR   2
#define ENIGMA_NUCLEO  3

/* ---------------- Valores de recuperação e dano ---------------- */
#define RECUPERACAO_OXIGENIO 40
#define DANO_MANUTENCAO      10   /* vapor ao entrar na passagem */

/* ---------------- Arquivos ---------------- */
#define ARQ_SAVE      "saves/partida.txt"
#define ARQ_SAVE_TEMP "saves/partida.tmp"
#define VERSAO_SAVE   1

/* Dados do protagonista. */
typedef struct {
    char nome[50];
    int  vida;
    int  localAtual;
    int  inventario[MAX_INVENTARIO];   /* ids dos itens carregados */
    int  quantidadeItens;
} Jogador;

/* Objeto que pode ser coletado e utilizado. */
typedef struct {
    int  id;
    char nome[50];
    char descricao[200];
    int  coletavel;
    int  utilizado;      /* já foi usado alguma vez       */
    int  coletado;       /* já foi pego alguma vez        */
    int  consumivel;     /* desaparece depois de usado    */
    int  localAtual;     /* ambiente, LOCAL_INVENTARIO ou LOCAL_CONSUMIDO */
    int  ambienteUso;    /* onde tem efeito (USO_QUALQUER = em qualquer lugar) */
} Item;

/* Ambiente da estação. */
typedef struct {
    int  id;
    char nome[60];
    char descricao[500];
    char arteASCII[1000];
    int  saidas[4];             /* id do vizinho em N, S, L, O ou SEM_SAIDA */
    int  visitado;
    int  bloqueado;             /* ambiente inteiro travado (1) ou livre (0) */
    int  saidasBloqueadas[4];   /* tipo de trava de cada saída (TRAVA_...)   */
} Ambiente;

/* Enigma ou obstáculo. */
typedef struct {
    int  id;
    char descricao[300];
    char resposta[100];
    int  resolvido;
    int  tentativas;
    int  ambienteID;
    int  danoErro;              /* dano aplicado em cada resposta errada */
} Enigma;

/* Objeto fixo do cenário: pode ser examinado e, às vezes, revela um registro. */
typedef struct {
    char nome[30];
    int  ambienteID;
    char descricao[300];
    int  registroID;            /* registro que vai para o diário, ou -1 */
} ObjetoCenario;

/* Texto encontrado durante a investigação. */
typedef struct {
    int  id;
    char titulo[60];
    char texto[400];
} Registro;

/* O que já foi feito na aventura (0 = não, 1 = sim). */
typedef struct {
    int energiaRestaurada;
    int sinalDesativado;
    int jogoFinalizado;
    int origemDescoberta;   /* amostra revelou a origem do sinal     */
    int painelAberto;       /* chave inglesa abriu o painel          */
    int fusivelInstalado;
    int moduloInstalado;
    int docaLiberada;
    int fugaRealizada;
} EstadoJogo;

/* Diário: vetor DINÂMICO com os ids dos registros encontrados. */
typedef struct {
    int *ids;
    int  quantidade;
    int  capacidade;
} Diario;

/* Comando digitado, separado em verbo e argumento. */
typedef struct {
    char verbo[20];
    char argumento[MAX_COMANDO];
} Comando;

/* Uma partida inteira. As funções recebem um ponteiro para ela, então
 * não é preciso copiar tudo isso a cada chamada. */
typedef struct {
    Jogador    jogador;
    Ambiente   ambientes[QTD_AMBIENTES];
    Item       itens[QTD_ITENS];
    Enigma     enigmas[QTD_ENIGMAS];
    EstadoJogo estado;
    Diario     diario;
} Jogo;

#endif /* ESTRUTURAS_H */
