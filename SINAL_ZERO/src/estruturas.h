/*
 * estruturas.h
 * SINAL ZERO - Declaração das structs e das definições compartilhadas.
 *
 * Todas as constantes do jogo ficam aqui para que main.c e funcoes.c
 * usem exatamente os mesmos tamanhos, caminhos e identificadores.
 */
#ifndef ESTRUTURAS_H
#define ESTRUTURAS_H

/* ---------- Tamanhos máximos (vetores de tamanho fixo) ---------- */
#define MAX_NOME            64    /* nomes de cenários, itens e títulos       */
#define MAX_CHAVE           24    /* identificadores curtos: "cartao", "NENHUM" */
#define MAX_TEXTO          600    /* descrições e textos de registros         */
#define MAX_LINHA         1024    /* uma linha lida de um arquivo CSV         */
#define MAX_CENARIOS         6    /* quantidade de ambientes da estação       */
#define MAX_INVENTARIO      10    /* espaços no inventário do jogador         */
#define MAX_REG_ENCONTRADOS 32    /* registros que o jogador pode coletar     */
#define CAPACIDADE_INICIAL   2    /* primeiro malloc() dos vetores dinâmicos  */

/* ---------- Identificadores dos cenários (mesmos ids do CSV) ---------- */
#define CEN_DORMITORIO 0
#define CEN_CORREDOR   1
#define CEN_LABORATORIO 2
#define CEN_MAQUINAS   3
#define CEN_SONAR      4
#define CEN_ESCAPE     5

/* ---------- Localização especial de um item ---------- */
#define LOCAL_INVENTARIO (-1)   /* item está com o jogador             */
#define LOCAL_UTILIZADO  (-2)   /* item foi instalado e saiu do jogo   */

/* ---------- Valores especiais devolvidos por lerOpcao() ---------- */
#define OPCAO_INVALIDA     (-1)
#define OPCAO_FIM_ENTRADA  (-2)

/* ---------- Enigma ---------- */
#define RESPOSTA_ENIGMA "ECO"

/* ---------- Caminhos (relativos à pasta SINAL_ZERO) ---------- */
#define ARQ_CENARIOS      "dados/cenarios.csv"
#define ARQ_ITENS         "dados/itens.csv"
#define ARQ_REGISTROS     "dados/registros.csv"
#define ARQ_PROGRESSO     "saves/progresso.txt"
#define ARQ_PROGRESSO_TMP "saves/progresso.tmp"
#define ARQ_EVENTOS       "saves/diario_eventos.txt"
#define CABECALHO_SAVE    "SINAL_ZERO_SAVE 1"

/* Objeto que pode ser encontrado, coletado e utilizado. */
typedef struct {
    int  id;
    char chave[MAX_CHAVE];      /* identificador usado no código: "cartao"  */
    char nome[MAX_NOME];        /* nome exibido: "Cartão de acesso"         */
    char descricao[MAX_TEXTO];
    int  localizacao;           /* id do cenário, LOCAL_INVENTARIO ou LOCAL_UTILIZADO */
    int  localOriginal;         /* cenário inicial (usado ao reiniciar/carregar) */
    int  cenarioUso;            /* único cenário onde o item tem efeito     */
} Item;

/* Ambiente da estação NEREIDA-7. */
typedef struct {
    int  id;
    char nome[MAX_NOME];
    char requisito[MAX_CHAVE];          /* NENHUM, CARTAO, ENERGIA ou QUARENTENA */
    char descricao[MAX_TEXTO];          /* texto com a estação sem energia       */
    char descricaoEnergia[MAX_TEXTO];   /* texto depois que a energia volta      */
} Cenario;

/* Registro/mensagem que pode ser lido em um terminal. */
typedef struct {
    int  id;
    int  cenario;                /* onde o registro está                     */
    char condicao[MAX_CHAVE];    /* NENHUM, ENERGIA ou MODULO                */
    char titulo[MAX_NOME];
    char texto[MAX_TEXTO];
} Registro;

/* Dados do protagonista e do progresso da aventura. */
typedef struct {
    char nome[MAX_NOME];
    int  cenarioAtual;
    int  inventario[MAX_INVENTARIO];        /* ids dos itens carregados      */
    int  qtdInventario;
    int  registros[MAX_REG_ENCONTRADOS];    /* ids dos registros já lidos    */
    int  qtdRegistros;
    int  movimentos;
    /* Objetivos (0 = pendente, 1 = concluído) */
    int  laboratorioLiberado;
    int  energiaRestaurada;
    int  moduloInstalado;
    int  origemDescoberta;     /* enigma resolvido */
    int  escapeLiberado;
} Jogador;

/* Agrupa todo o estado do jogo para ser passado às funções por ponteiro. */
typedef struct {
    Jogador  jogador;
    Cenario  cenarios[MAX_CENARIOS];               /* vetor estático       */
    int      qtdCenarios;
    int      conexoes[MAX_CENARIOS][MAX_CENARIOS]; /* matriz de adjacência */
    Item    *itens;            /* vetor dinâmico (malloc/realloc) */
    int      qtdItens;
    Registro *registros;       /* vetor dinâmico (malloc/realloc) */
    int      qtdRegistros;
    int      ativo;            /* 1 enquanto o laço principal roda */
} Jogo;

#endif /* ESTRUTURAS_H */
