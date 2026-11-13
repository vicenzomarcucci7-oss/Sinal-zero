/*
 * funcoes.c
 * SINAL ZERO - Implementação das funções do jogo.
 *
 * Organização do arquivo:
 *   1. Utilitários de terminal e strings
 *   2. Inicialização, leitura dos CSV e memória dinâmica
 *   3. Buscas (inclusive a busca recursiva)
 *   4. Inventário e registros do jogador
 *   5. Interface (telas e menus)
 *   6. Ações do jogador (examinar, coletar, usar, mover...)
 *   7. Salvar / carregar progresso e final do jogo
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>   /* apenas para SetConsoleOutputCP (acentos no Windows) */
#endif
#include "funcoes.h"

/* =====================================================================
 * 1. UTILITÁRIOS DE TERMINAL E STRINGS
 * ===================================================================== */

/* No Windows, o terminal precisa usar UTF-8 para exibir os acentos. */
void configurarTerminal(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
}

/* Lê uma linha do teclado com fgets (seguro contra estouro de buffer).
 * Retorna 0 se a entrada terminou (EOF), 1 caso contrário. */
int lerLinha(char *destino, int tamanho)
{
    if (fgets(destino, tamanho, stdin) == NULL) {
        destino[0] = '\0';
        return 0;
    }
    /* Se a linha era maior que o buffer, descarta o restante. */
    if (strchr(destino, '\n') == NULL) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {
            /* descarta */
        }
    }
    removerQuebraLinha(destino);
    return 1;
}

/* Lê um número de menu. Letras, vazio ou números negativos viram
 * OPCAO_INVALIDA, então o programa nunca trava com entrada errada. */
int lerOpcao(void)
{
    char buffer[32];
    char *fim;
    long valor;

    if (!lerLinha(buffer, sizeof(buffer)))
        return OPCAO_FIM_ENTRADA;
    aparar(buffer);
    if (strlen(buffer) == 0)
        return OPCAO_INVALIDA;

    valor = strtol(buffer, &fim, 10);   /* converte e indica onde parou */
    if (*fim != '\0' || valor < 0 || valor > 99)
        return OPCAO_INVALIDA;
    return (int)valor;
}

/* Remove "\n" e "\r" do final (arquivos salvos no Windows usam \r\n). */
void removerQuebraLinha(char *texto)
{
    texto[strcspn(texto, "\r\n")] = '\0';
}

/* Remove espaços do início e do fim da string. */
void aparar(char *texto)
{
    size_t inicio = 0;
    size_t tamanho = strlen(texto);

    while (tamanho > 0 && isspace((unsigned char)texto[tamanho - 1]))
        texto[--tamanho] = '\0';
    while (isspace((unsigned char)texto[inicio]))
        inicio++;
    if (inicio > 0)
        memmove(texto, texto + inicio, tamanho - inicio + 1);
}

/* Converte para maiúsculas percorrendo a string com aritmética de ponteiros. */
void paraMaiusculas(char *texto)
{
    for (; *texto != '\0'; texto++)
        *texto = (char)toupper((unsigned char)*texto);
}

/* Copia uma string respeitando o tamanho do destino.
 * Usa strlen para decidir se cabe; se couber usa strcpy, senão trunca. */
void copiarTexto(char *destino, const char *origem, size_t tamanho)
{
    if (strlen(origem) < tamanho) {
        strcpy(destino, origem);
    } else {
        strncpy(destino, origem, tamanho - 1);
        destino[tamanho - 1] = '\0';
    }
}

/* Troca a primeira ocorrência de 'marcador' por 'valor' usando strstr.
 * Exemplo: "Assinado: [NOME]" -> "Assinado: Ana". Retorna 1 se trocou. */
int substituirMarcador(char *texto, size_t tamanho, const char *marcador, const char *valor)
{
    char restante[MAX_TEXTO];
    char *posicao = strstr(texto, marcador);

    if (posicao == NULL)
        return 0;
    if (strlen(texto) - strlen(marcador) + strlen(valor) >= tamanho)
        return 0;                               /* não caberia: não altera */

    copiarTexto(restante, posicao + strlen(marcador), sizeof(restante));
    *posicao = '\0';                            /* corta o texto no marcador */
    strcat(texto, valor);
    strcat(texto, restante);
    return 1;
}

/* Divide uma linha CSV no separador, sem copiar: troca cada separador por
 * '\0' e guarda em campos[] um ponteiro para o início de cada campo.
 * (Diferente de strtok, aceita campos vazios.) Retorna a quantidade. */
int dividirCampos(char *linha, char separador, char *campos[], int maximo)
{
    int quantidade = 0;
    char *atual = linha;

    campos[quantidade++] = atual;
    while (quantidade < maximo && (atual = strchr(atual, separador)) != NULL) {
        *atual = '\0';
        atual++;
        campos[quantidade++] = atual;
    }
    return quantidade;
}

/* Imprime um texto do CSV: o caractere '|' representa quebra de linha. */
void imprimirTexto(const char *texto)
{
    const char *p;

    printf("  ");
    for (p = texto; *p != '\0'; p++) {
        if (*p == '|')
            printf("\n  ");
        else
            putchar(*p);
    }
    putchar('\n');
}

/* Acrescenta uma linha com data/hora ao diário de eventos (modo "a").
 * Usa argumentos variáveis (stdarg.h) para funcionar como um printf. */
void registrarEvento(const char *formato, ...)
{
    static int avisou = 0;
    char dataHora[32];
    time_t agora = time(NULL);
    va_list argumentos;
    FILE *arquivo = fopen(ARQ_EVENTOS, "a");

    if (arquivo == NULL) {
        if (!avisou)
            printf("[AVISO] Não foi possível gravar em %s.\n", ARQ_EVENTOS);
        avisou = 1;
        return;
    }
    strftime(dataHora, sizeof(dataHora), "%d/%m/%Y %H:%M:%S", localtime(&agora));
    fprintf(arquivo, "[%s] ", dataHora);

    va_start(argumentos, formato);
    vfprintf(arquivo, formato, argumentos);
    va_end(argumentos);

    fputc('\n', arquivo);
    fclose(arquivo);
}

/* =====================================================================
 * 2. INICIALIZAÇÃO, LEITURA DOS CSV E MEMÓRIA DINÂMICA
 * ===================================================================== */

/* Prepara a struct Jogo e carrega os três arquivos CSV. */
int inicializarJogo(Jogo *jogo)
{
    int i, j;

    jogo->itens = NULL;
    jogo->registros = NULL;
    jogo->qtdItens = 0;
    jogo->qtdRegistros = 0;
    jogo->qtdCenarios = 0;
    jogo->ativo = 0;

    /* Matriz de adjacência começa toda com 0 (nenhuma passagem). */
    for (i = 0; i < MAX_CENARIOS; i++)
        for (j = 0; j < MAX_CENARIOS; j++)
            jogo->conexoes[i][j] = 0;

    iniciarJogador(&jogo->jogador, "Técnico");

    if (!carregarCenarios(jogo)) return 0;
    if (!carregarItens(jogo))    return 0;
    if (!carregarRegistros(jogo)) return 0;

    printf("[SISTEMA] %d cenários, %d objetos e %d registros carregados.\n",
           jogo->qtdCenarios, jogo->qtdItens, jogo->qtdRegistros);
    return 1;
}

/* Recebe o jogador por ponteiro e zera todo o progresso. */
void iniciarJogador(Jogador *jogador, const char *nome)
{
    copiarTexto(jogador->nome, nome, sizeof(jogador->nome));
    jogador->cenarioAtual = CEN_DORMITORIO;
    jogador->qtdInventario = 0;
    jogador->qtdRegistros = 0;
    jogador->movimentos = 0;
    jogador->laboratorioLiberado = 0;
    jogador->energiaRestaurada = 0;
    jogador->moduloInstalado = 0;
    jogador->origemDescoberta = 0;
    jogador->escapeLiberado = 0;
}

/* Lê dados/cenarios.csv. Formato (separador ';'):
 * id;nome;requisito;conexoes;descricao;descricao_energia
 * O campo conexoes ("0|2|3") é usado para montar a matriz de adjacência. */
int carregarCenarios(Jogo *jogo)
{
    char linha[MAX_LINHA];
    char *campos[6];
    char *destino;
    int numeroLinha = 0;
    FILE *arquivo = fopen(ARQ_CENARIOS, "r");

    if (arquivo == NULL) {
        printf("[ERRO] Arquivo %s não encontrado.\n", ARQ_CENARIOS);
        printf("       Execute o jogo a partir da pasta SINAL_ZERO.\n");
        return 0;
    }

    while (fgets(linha, sizeof(linha), arquivo) != NULL) {
        int id;
        Cenario *cenario;

        numeroLinha++;
        removerQuebraLinha(linha);
        if (numeroLinha == 1 || strlen(linha) == 0)
            continue;                       /* pula cabeçalho e linhas vazias */

        if (dividirCampos(linha, ';', campos, 6) != 6) {
            printf("[AVISO] %s linha %d ignorada (formato inválido).\n", ARQ_CENARIOS, numeroLinha);
            continue;
        }
        id = atoi(campos[0]);
        if (id < 0 || id >= MAX_CENARIOS) {
            printf("[AVISO] %s linha %d: id %d fora do intervalo.\n", ARQ_CENARIOS, numeroLinha, id);
            continue;
        }

        cenario = jogo->cenarios + id;      /* aritmética de ponteiros */
        cenario->id = id;
        copiarTexto(cenario->nome, campos[1], sizeof(cenario->nome));
        copiarTexto(cenario->requisito, campos[2], sizeof(cenario->requisito));
        copiarTexto(cenario->descricao, campos[4], sizeof(cenario->descricao));
        copiarTexto(cenario->descricaoEnergia, campos[5], sizeof(cenario->descricaoEnergia));

        /* Preenche a linha 'id' da matriz: 1 = existe passagem. */
        destino = strtok(campos[3], "|");
        while (destino != NULL) {
            int d = atoi(destino);
            if (d >= 0 && d < MAX_CENARIOS && d != id)
                jogo->conexoes[id][d] = 1;
            destino = strtok(NULL, "|");
        }
        jogo->qtdCenarios++;
    }
    fclose(arquivo);

    if (jogo->qtdCenarios != MAX_CENARIOS) {
        printf("[ERRO] %s deveria ter %d cenários, mas tem %d.\n",
               ARQ_CENARIOS, MAX_CENARIOS, jogo->qtdCenarios);
        return 0;
    }
    return 1;
}

/* Lê dados/itens.csv para um vetor DINÂMICO.
 * Formato: id;chave;nome;cenario_inicial;cenario_uso;descricao
 * Como o programa não sabe quantos itens o arquivo tem, começa com malloc()
 * e dobra a capacidade com realloc() sempre que o vetor enche. */
int carregarItens(Jogo *jogo)
{
    char linha[MAX_LINHA];
    char *campos[6];
    int capacidade = 0;
    int numeroLinha = 0;
    FILE *arquivo = fopen(ARQ_ITENS, "r");

    if (arquivo == NULL) {
        printf("[ERRO] Arquivo %s não encontrado.\n", ARQ_ITENS);
        return 0;
    }

    while (fgets(linha, sizeof(linha), arquivo) != NULL) {
        Item *item;

        numeroLinha++;
        removerQuebraLinha(linha);
        if (numeroLinha == 1 || strlen(linha) == 0)
            continue;
        if (dividirCampos(linha, ';', campos, 6) != 6) {
            printf("[AVISO] %s linha %d ignorada.\n", ARQ_ITENS, numeroLinha);
            continue;
        }

        /* Garante espaço para mais um item. */
        if (jogo->qtdItens == capacidade) {
            Item *novo;
            if (capacidade == 0) {
                capacidade = CAPACIDADE_INICIAL;
                novo = (Item *)malloc(capacidade * sizeof(Item));
            } else {
                capacidade *= 2;
                novo = (Item *)realloc(jogo->itens, capacidade * sizeof(Item));
            }
            if (novo == NULL) {               /* o vetor antigo continua válido */
                printf("[ERRO] Memória insuficiente para os itens.\n");
                fclose(arquivo);
                return 0;
            }
            jogo->itens = novo;
        }

        item = jogo->itens + jogo->qtdItens;      /* ponteiro para a próxima posição */
        item->id = atoi(campos[0]);
        copiarTexto(item->chave, campos[1], sizeof(item->chave));
        copiarTexto(item->nome, campos[2], sizeof(item->nome));
        item->localOriginal = atoi(campos[3]);
        item->localizacao = item->localOriginal;
        item->cenarioUso = atoi(campos[4]);
        copiarTexto(item->descricao, campos[5], sizeof(item->descricao));
        jogo->qtdItens++;
    }
    fclose(arquivo);

    if (jogo->qtdItens == 0) {
        printf("[ERRO] Nenhum item foi carregado de %s.\n", ARQ_ITENS);
        return 0;
    }
    return 1;
}

/* Lê dados/registros.csv para um vetor dinâmico (mesma técnica dos itens).
 * Formato: id;cenario;condicao;titulo;texto */
int carregarRegistros(Jogo *jogo)
{
    char linha[MAX_LINHA];
    char *campos[5];
    int capacidade = 0;
    int numeroLinha = 0;
    FILE *arquivo = fopen(ARQ_REGISTROS, "r");

    if (arquivo == NULL) {
        printf("[ERRO] Arquivo %s não encontrado.\n", ARQ_REGISTROS);
        return 0;
    }

    while (fgets(linha, sizeof(linha), arquivo) != NULL) {
        Registro *registro;

        numeroLinha++;
        removerQuebraLinha(linha);
        if (numeroLinha == 1 || strlen(linha) == 0)
            continue;
        if (dividirCampos(linha, ';', campos, 5) != 5) {
            printf("[AVISO] %s linha %d ignorada.\n", ARQ_REGISTROS, numeroLinha);
            continue;
        }

        if (jogo->qtdRegistros == capacidade) {
            Registro *novo;
            if (capacidade == 0) {
                capacidade = CAPACIDADE_INICIAL;
                novo = (Registro *)malloc(capacidade * sizeof(Registro));
            } else {
                capacidade *= 2;
                novo = (Registro *)realloc(jogo->registros, capacidade * sizeof(Registro));
            }
            if (novo == NULL) {
                printf("[ERRO] Memória insuficiente para os registros.\n");
                fclose(arquivo);
                return 0;
            }
            jogo->registros = novo;
        }

        registro = jogo->registros + jogo->qtdRegistros;
        registro->id = atoi(campos[0]);
        registro->cenario = atoi(campos[1]);
        copiarTexto(registro->condicao, campos[2], sizeof(registro->condicao));
        copiarTexto(registro->titulo, campos[3], sizeof(registro->titulo));
        copiarTexto(registro->texto, campos[4], sizeof(registro->texto));
        jogo->qtdRegistros++;
    }
    fclose(arquivo);

    if (jogo->qtdRegistros == 0) {
        printf("[ERRO] Nenhum registro foi carregado de %s.\n", ARQ_REGISTROS);
        return 0;
    }
    return 1;
}

/* Devolve todos os itens aos cenários iniciais (usado em "nova aventura"). */
void reiniciarItens(Jogo *jogo)
{
    int i;
    for (i = 0; i < jogo->qtdItens; i++)
        (jogo->itens + i)->localizacao = (jogo->itens + i)->localOriginal;
}

/* Libera toda a memória alocada dinamicamente. Pode ser chamada mesmo
 * se a carga falhou no meio (free(NULL) é seguro). */
void liberarJogo(Jogo *jogo)
{
    free(jogo->itens);
    free(jogo->registros);
    jogo->itens = NULL;
    jogo->registros = NULL;
    jogo->qtdItens = 0;
    jogo->qtdRegistros = 0;
}

/* =====================================================================
 * 3. BUSCAS
 * ===================================================================== */

/* Procura um item pelo id e devolve um PONTEIRO para ele dentro do vetor,
 * permitindo que quem chamou altere o item diretamente. */
Item *buscarItemPorId(const Jogo *jogo, int id)
{
    int i;
    for (i = 0; i < jogo->qtdItens; i++)
        if ((jogo->itens + i)->id == id)
            return jogo->itens + i;
    return NULL;
}

Registro *buscarRegistroPorId(const Jogo *jogo, int id)
{
    int i;
    for (i = 0; i < jogo->qtdRegistros; i++)
        if ((jogo->registros + i)->id == id)
            return jogo->registros + i;
    return NULL;
}

/* Verifica o inventário usando *(vetor + i) em vez de vetor[i]. */
int temItem(const Jogador *jogador, int idItem)
{
    int i;
    for (i = 0; i < jogador->qtdInventario; i++)
        if (*(jogador->inventario + i) == idItem)
            return 1;
    return 0;
}

int registroJaEncontrado(const Jogador *jogador, int idRegistro)
{
    int i;
    for (i = 0; i < jogador->qtdRegistros; i++)
        if (*(jogador->registros + i) == idRegistro)
            return 1;
    return 0;
}

/* Verifica se o título ou o texto contém o termo (comparação sem
 * diferenciar maiúsculas: copia os textos e converte para maiúsculas). */
int registroContem(const Registro *registro, const char *termoMaiusculo)
{
    char titulo[MAX_NOME];
    char texto[MAX_TEXTO];

    copiarTexto(titulo, registro->titulo, sizeof(titulo));
    copiarTexto(texto, registro->texto, sizeof(texto));
    paraMaiusculas(titulo);
    paraMaiusculas(texto);
    return strstr(titulo, termoMaiusculo) != NULL || strstr(texto, termoMaiusculo) != NULL;
}

/* BUSCA RECURSIVA nos registros já encontrados pelo jogador.
 * Retorna a posição (no vetor jogador.registros) do primeiro registro, a
 * partir de 'posicao', que contém o termo; ou -1 se nenhum contém.
 *   - Condição de parada 1: posicao chegou ao fim do vetor -> -1.
 *   - Condição de parada 2: o registro da posição contém o termo -> posicao.
 *   - Chamada recursiva: procura no restante do vetor (posicao + 1). */
int buscarRegistroRecursivo(const Jogo *jogo, int posicao, const char *termoMaiusculo)
{
    const Registro *registro;

    if (posicao >= jogo->jogador.qtdRegistros)
        return -1;

    registro = buscarRegistroPorId(jogo, *(jogo->jogador.registros + posicao));
    if (registro != NULL && registroContem(registro, termoMaiusculo))
        return posicao;

    return buscarRegistroRecursivo(jogo, posicao + 1, termoMaiusculo);
}

/* =====================================================================
 * 4. INVENTÁRIO E REGISTROS DO JOGADOR
 *    (o Jogador é recebido por ponteiro, então as alterações valem fora)
 * ===================================================================== */

int adicionarAoInventario(Jogador *jogador, int idItem)
{
    if (jogador->qtdInventario >= MAX_INVENTARIO)
        return 0;
    jogador->inventario[jogador->qtdInventario] = idItem;
    jogador->qtdInventario++;
    return 1;
}

/* Remove o item deslocando os elementos seguintes uma posição à esquerda. */
int removerDoInventario(Jogador *jogador, int idItem)
{
    int i, j;
    for (i = 0; i < jogador->qtdInventario; i++) {
        if (jogador->inventario[i] == idItem) {
            for (j = i; j < jogador->qtdInventario - 1; j++)
                jogador->inventario[j] = jogador->inventario[j + 1];
            jogador->qtdInventario--;
            return 1;
        }
    }
    return 0;
}

int adicionarRegistroEncontrado(Jogador *jogador, int idRegistro)
{
    if (registroJaEncontrado(jogador, idRegistro))
        return 0;
    if (jogador->qtdRegistros >= MAX_REG_ENCONTRADOS)
        return 0;
    jogador->registros[jogador->qtdRegistros++] = idRegistro;
    return 1;
}

/* =====================================================================
 * 5. INTERFACE
 * ===================================================================== */

void mostrarAbertura(void)
{
    printf("\n");
    printf("  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("        .   o   .      ((( ))) <- sinal      .   o\n");
    printf("            S  I  N  A  L       Z  E  R  O\n");
    printf("        o      .       ~~~~~~~~~~~      o      .\n");
    printf("  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("     Estação científica submarina NEREIDA-7\n");
    printf("     Profundidade: 2.140 metros | Contato com a superfície: PERDIDO\n");
    printf("  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");
}

/* Menu antes da aventura: nova partida ou carregar. Retorna 0 para sair. */
int menuInicial(Jogo *jogo)
{
    char nome[MAX_NOME];
    int opcao;

    while (1) {
        printf("[1] Nova aventura\n[2] Carregar progresso\n[0] Sair\nEscolha uma opção: ");
        opcao = lerOpcao();

        if (opcao == 1) {
            printf("\nIdentifique-se, técnico (nome): ");
            if (!lerLinha(nome, sizeof(nome)))
                return 0;
            aparar(nome);
            if (strlen(nome) == 0)
                copiarTexto(nome, "Técnico", sizeof(nome));
            iniciarJogador(&jogo->jogador, nome);
            reiniciarItens(jogo);
            registrarEvento("Nova aventura iniciada por %s.", nome);
            printf("\n%s, você foi enviado para investigar o desaparecimento da equipe.\n", nome);
            printf("A descida durou horas. Então, o silêncio.\n");
            jogo->ativo = 1;
            return 1;
        } else if (opcao == 2) {
            if (carregarProgresso(jogo)) {
                jogo->ativo = 1;
                return 1;
            }
        } else if (opcao == 0 || opcao == OPCAO_FIM_ENTRADA) {
            return 0;
        } else {
            printf("Opção inválida. Digite 1, 2 ou 0.\n\n");
        }
    }
}

/* Barra de status no estilo dos painéis da estação. */
void mostrarStatus(const Jogo *jogo)
{
    const Jogador *j = &jogo->jogador;

    printf("\n==============================================================\n");
    printf(" [ SISTEMA DE ENERGIA: %s ]  [ SONAR: %s ]  [ ESCAPE: %s ]\n",
           j->energiaRestaurada ? "ONLINE" : "OFFLINE",
           j->moduloInstalado ? "ATIVO" : "INATIVO",
           j->escapeLiberado ? "LIBERADO" : "BLOQUEADO");
    printf(" LOCALIZAÇÃO: %s\n", jogo->cenarios[j->cenarioAtual].nome);
    printf("==============================================================\n");
}

/* Mostra o status e a descrição do cenário atual (varia com a energia). */
void mostrarCenario(const Jogo *jogo)
{
    const Cenario *c = &jogo->cenarios[jogo->jogador.cenarioAtual];

    mostrarStatus(jogo);
    imprimirTexto(jogo->jogador.energiaRestaurada ? c->descricaoEnergia : c->descricao);
}

void mostrarMenu(const Jogo *jogo)
{
    printf("\n--- %s --- O que deseja fazer?\n", jogo->cenarios[jogo->jogador.cenarioAtual].nome);
    printf("[1] Examinar o ambiente      [6] Usar objeto\n");
    printf("[2] Procurar objetos         [7] Registros encontrados\n");
    printf("[3] Consultar inventário     [8] Salvar progresso\n");
    printf("[4] Acessar terminal         [9] Carregar progresso\n");
    printf("[5] Movimentar-se            [0] Sair\n");
    printf("Escolha uma opção: ");
}

/* Encaminha a opção escolhida para a função correspondente. */
void processarOpcao(Jogo *jogo, int opcao)
{
    switch (opcao) {
        case 1: examinarAmbiente(jogo); break;
        case 2: procurarObjetos(jogo); break;
        case 3: mostrarInventario(jogo); break;
        case 4: acessarTerminal(jogo); break;
        case 5: moverJogador(jogo); break;
        case 6: escolherItemParaUsar(jogo); break;
        case 7: menuRegistros(jogo); break;
        case 8: salvarProgresso(jogo); break;
        case 9:
            if (carregarProgresso(jogo))
                mostrarCenario(jogo);
            break;
        case 0:
            printf("\nVocê se senta no chão frio e espera. O sinal continua...\n");
            registrarEvento("Jogador saiu do jogo em %s.",
                            jogo->cenarios[jogo->jogador.cenarioAtual].nome);
            jogo->ativo = 0;
            break;
        case OPCAO_FIM_ENTRADA:
            printf("\n[Entrada encerrada]\n");
            jogo->ativo = 0;
            break;
        default:
            printf("\n[!] Opção inválida. Digite um número de 0 a 9.\n");
    }
}

/* =====================================================================
 * 6. AÇÕES DO JOGADOR
 * ===================================================================== */

/* Descrição completa + saídas (lidas da matriz) + objetos e terminais. */
void examinarAmbiente(const Jogo *jogo)
{
    int atual = jogo->jogador.cenarioAtual;
    int i, objetos = 0, terminais = 0;

    mostrarCenario(jogo);

    printf("\n  Saídas:\n");
    for (i = 0; i < jogo->qtdCenarios; i++) {
        if (jogo->conexoes[atual][i] == 1)
            printf("   -> %s%s\n", jogo->cenarios[i].nome,
                   podeAcessar(jogo, i) ? "" : " [BLOQUEADO]");
    }

    for (i = 0; i < jogo->qtdItens; i++)
        if ((jogo->itens + i)->localizacao == atual)
            objetos++;
    for (i = 0; i < jogo->qtdRegistros; i++)
        if ((jogo->registros + i)->cenario == atual)
            terminais++;

    if (objetos > 0)
        printf("\n  Algo chama sua atenção por aqui. Talvez valha a pena procurar objetos.\n");
    if (terminais > 0)
        printf("  Existe um terminal ou registro neste local [4].\n");
}

/* Lista os objetos do cenário atual e permite coletar um. */
void procurarObjetos(Jogo *jogo)
{
    int atual = jogo->jogador.cenarioAtual;
    int encontrados[MAX_INVENTARIO];
    int quantidade = 0;
    int i, opcao;

    for (i = 0; i < jogo->qtdItens && quantidade < MAX_INVENTARIO; i++) {
        Item *item = jogo->itens + i;
        if (item->localizacao == atual)
            encontrados[quantidade++] = item->id;
    }

    if (quantidade == 0) {
        printf("\n  Você revira o local, mas não encontra nada útil.\n");
        return;
    }

    printf("\n  Você encontra:\n");
    for (i = 0; i < quantidade; i++)
        printf("  [%d] %s\n", i + 1, buscarItemPorId(jogo, encontrados[i])->nome);
    printf("  [0] Deixar como está\nQual objeto deseja coletar? ");

    opcao = lerOpcao();
    if (opcao >= 1 && opcao <= quantidade)
        coletarItem(jogo, encontrados[opcao - 1]);
    else if (opcao != 0)
        printf("  [!] Opção inválida.\n");
}

/* Move o item do cenário para o inventário (alterando a struct Item e a
 * struct Jogador através de ponteiros). */
int coletarItem(Jogo *jogo, int idItem)
{
    Item *item = buscarItemPorId(jogo, idItem);

    if (item == NULL || item->localizacao != jogo->jogador.cenarioAtual) {
        printf("  Esse objeto não está aqui.\n");
        return 0;
    }
    if (!adicionarAoInventario(&jogo->jogador, idItem)) {
        printf("  Seu inventário está cheio.\n");
        return 0;
    }
    item->localizacao = LOCAL_INVENTARIO;
    printf("\n  >> Você coletou: %s\n", item->nome);
    imprimirTexto(item->descricao);
    registrarEvento("Item coletado: %s.", item->nome);
    return 1;
}

void mostrarInventario(const Jogo *jogo)
{
    const Jogador *j = &jogo->jogador;
    int i;

    printf("\n  === INVENTÁRIO (%d/%d) ===\n", j->qtdInventario, MAX_INVENTARIO);
    if (j->qtdInventario == 0) {
        printf("  Vazio. Apenas suas mãos geladas.\n");
        return;
    }
    for (i = 0; i < j->qtdInventario; i++) {
        const Item *item = buscarItemPorId(jogo, *(j->inventario + i));
        if (item != NULL) {
            printf("  - %s\n", item->nome);
            imprimirTexto(item->descricao);
        }
    }
}

/* Mostra o inventário numerado e usa o item escolhido. */
void escolherItemParaUsar(Jogo *jogo)
{
    Jogador *j = &jogo->jogador;
    int i, opcao;

    if (j->qtdInventario == 0) {
        printf("\n  Você não tem nenhum objeto para usar.\n");
        return;
    }
    printf("\n  Usar qual objeto?\n");
    for (i = 0; i < j->qtdInventario; i++)
        printf("  [%d] %s\n", i + 1, buscarItemPorId(jogo, j->inventario[i])->nome);
    printf("  [0] Cancelar\nEscolha: ");

    opcao = lerOpcao();
    if (opcao >= 1 && opcao <= j->qtdInventario)
        usarItem(jogo, j->inventario[opcao - 1]);
    else if (opcao != 0)
        printf("  [!] Opção inválida.\n");
}

/* Aplica o efeito do item. O item só funciona no cenário indicado na
 * coluna cenario_uso do CSV; a chave (strcmp) define o efeito. */
int usarItem(Jogo *jogo, int idItem)
{
    Jogador *j = &jogo->jogador;
    Item *item = buscarItemPorId(jogo, idItem);

    if (item == NULL || !temItem(j, idItem)) {
        printf("  Você não possui esse objeto.\n");
        return 0;
    }
    if (item->cenarioUso != j->cenarioAtual) {
        printf("\n  Você procura onde usar %s, mas nada aqui reage.\n", item->nome);
        return 0;
    }

    if (strcmp(item->chave, "cartao") == 0) {
        if (!j->laboratorioLiberado) {
            j->laboratorioLiberado = 1;
            printf("\n  Você passa o cartão no leitor do LABORATÓRIO.\n");
            printf("  Bip. A luz fica verde e a trava se solta com um estalo.\n");
            registrarEvento("Porta do laboratório liberada com o cartão.");
        } else if (j->escapeLiberado) {
            printf("\n  A câmara de escape já está liberada.\n");
        } else if (j->energiaRestaurada && j->origemDescoberta) {
            j->escapeLiberado = 1;
            printf("\n  Você passa o cartão no leitor da CÂMARA DE ESCAPE.\n");
            printf("  QUARENTENA ENCERRADA. ORIGEM DO SINAL IDENTIFICADA.\n");
            printf("  A porta pressurizada se abre lentamente.\n");
            registrarEvento("Câmara de escape liberada.");
        } else {
            printf("\n  Leitor da CÂMARA DE ESCAPE: ACESSO NEGADO - QUARENTENA ATIVA\n");
            printf("   Energia restabelecida ...... %s\n", j->energiaRestaurada ? "OK" : "PENDENTE");
            printf("   Origem do sinal descoberta . %s\n", j->origemDescoberta ? "OK" : "PENDENTE");
        }
        return 1;
    }

    if (strcmp(item->chave, "fusivel") == 0) {
        j->energiaRestaurada = 1;
        item->localizacao = LOCAL_UTILIZADO;
        removerDoInventario(j, idItem);
        printf("\n  Você encaixa o fusível no painel e puxa a alavanca.\n");
        printf("  O gerador tosse, engasga... e ruge. A NEREIDA-7 volta a respirar.\n");
        printf("  [ SISTEMA DE ENERGIA: ONLINE ]\n");
        registrarEvento("Energia restabelecida com o fusível.");
        return 1;
    }

    if (strcmp(item->chave, "modulo") == 0) {
        j->moduloInstalado = 1;
        item->localizacao = LOCAL_UTILIZADO;
        removerDoInventario(j, idItem);
        printf("\n  Você insere o módulo de memória no console de decodificação.\n");
        printf("  O sonar dispara um pulso grave. Você o sente nos dentes.\n");
        printf("  Novas transmissões aparecem no terminal [4].\n");
        registrarEvento("Módulo de memória instalado no sonar.");
        return 1;
    }

    printf("  Nada acontece.\n");
    return 0;
}

/* Mostra um registro substituindo o marcador [NOME] pelo nome do jogador. */
void exibirRegistro(const Jogo *jogo, const Registro *registro)
{
    char texto[MAX_TEXTO];

    copiarTexto(texto, registro->texto, sizeof(texto));
    substituirMarcador(texto, sizeof(texto), "[NOME]", jogo->jogador.nome);
    printf("\n  >>> %s\n", registro->titulo);
    imprimirTexto(texto);
}

int condicaoAtendida(const Jogo *jogo, const char *condicao)
{
    if (strcmp(condicao, "NENHUM") == 0)  return 1;
    if (strcmp(condicao, "ENERGIA") == 0) return jogo->jogador.energiaRestaurada;
    if (strcmp(condicao, "MODULO") == 0)  return jogo->jogador.moduloInstalado;
    return 0;   /* condição desconhecida: não mostra */
}

/* Lê os registros disponíveis no cenário atual e os guarda no vetor de
 * registros do jogador. Na central de sonar, abre o enigma. */
void acessarTerminal(Jogo *jogo)
{
    int i, exibidos = 0;
    int atual = jogo->jogador.cenarioAtual;

    for (i = 0; i < jogo->qtdRegistros; i++) {
        const Registro *registro = jogo->registros + i;
        if (registro->cenario != atual || !condicaoAtendida(jogo, registro->condicao))
            continue;
        exibirRegistro(jogo, registro);
        if (adicionarRegistroEncontrado(&jogo->jogador, registro->id))
            registrarEvento("Registro lido: %s.", registro->titulo);
        exibidos++;
    }

    if (exibidos == 0)
        printf("\n  Não há nenhum terminal funcionando aqui.\n");

    if (atual == CEN_SONAR && jogo->jogador.moduloInstalado && !jogo->jogador.origemDescoberta)
        resolverEnigma(jogo);
}

/* ENIGMA: as iniciais das três transmissões formam a palavra ECO. */
int resolverEnigma(Jogo *jogo)
{
    char resposta[MAX_NOME];

    printf("\n  === DECODIFICADOR DE ORIGEM ===\n");
    printf("  As três transmissões escondem uma palavra: a natureza do sinal.\n");
    printf("  Digite a palavra (ou apenas ENTER para tentar depois): ");

    if (!lerLinha(resposta, sizeof(resposta)))
        return 0;
    aparar(resposta);
    if (strlen(resposta) == 0) {
        printf("  O cursor continua piscando. Você pode voltar ao terminal quando quiser.\n");
        return 0;
    }
    paraMaiusculas(resposta);

    if (strcmp(resposta, RESPOSTA_ENIGMA) == 0) {
        jogo->jogador.origemDescoberta = 1;
        printf("\n  PALAVRA ACEITA: %s\n", RESPOSTA_ENIGMA);
        printf("  ORIGEM DO SINAL: CÂMARA DE ESCAPE - NEREIDA-7\n");
        printf("  REMETENTE: %s\n\n", jogo->jogador.nome);
        printf("  Não é um pedido de socorro vindo do oceano. É um eco.\n");
        printf("  As mensagens foram gravadas por você, no gravador da cápsula,\n");
        printf("  e o sonar as devolveu sete minutos ao passado.\n");
        printf("  O sistema de quarentena agora aceita um cartão autorizado.\n");
        registrarEvento("Enigma resolvido: origem do sinal descoberta.");
        return 1;
    }
    printf("  RESPOSTA INCORRETA. Dica: observe a primeira letra de cada transmissão.\n");
    registrarEvento("Tentativa errada no enigma: %s.", resposta);
    return 0;
}

/* Verifica o requisito (texto vindo do CSV) do cenário de destino. */
int podeAcessar(const Jogo *jogo, int destino)
{
    const char *requisito = jogo->cenarios[destino].requisito;
    const Jogador *j = &jogo->jogador;

    if (strcmp(requisito, "NENHUM") == 0)     return 1;
    if (strcmp(requisito, "CARTAO") == 0)     return j->laboratorioLiberado;
    if (strcmp(requisito, "ENERGIA") == 0)    return j->energiaRestaurada;
    if (strcmp(requisito, "QUARENTENA") == 0) return j->escapeLiberado;
    return 0;
}

void explicarBloqueio(const Jogo *jogo, int destino)
{
    const char *requisito = jogo->cenarios[destino].requisito;

    printf("\n  [BLOQUEADO] %s: ", jogo->cenarios[destino].nome);
    if (strcmp(requisito, "CARTAO") == 0)
        printf("a porta exige um cartão de acesso no leitor do corredor.\n");
    else if (strcmp(requisito, "ENERGIA") == 0)
        printf("a porta é motorizada e não há energia.\n");
    else if (strcmp(requisito, "QUARENTENA") == 0)
        printf("protocolo de quarentena ativo. Leia o aviso no terminal do corredor.\n");
    else
        printf("acesso impossível.\n");
}

/* Mostra apenas os destinos com 1 na matriz de adjacência. */
void moverJogador(Jogo *jogo)
{
    int destinos[MAX_CENARIOS];
    int quantidade = 0;
    int atual = jogo->jogador.cenarioAtual;
    int i, opcao;

    printf("\n  Para onde deseja ir?\n");
    for (i = 0; i < jogo->qtdCenarios; i++) {
        if (jogo->conexoes[atual][i] == 1) {
            destinos[quantidade++] = i;
            printf("  [%d] %s%s\n", quantidade, jogo->cenarios[i].nome,
                   podeAcessar(jogo, i) ? "" : " [BLOQUEADO]");
        }
    }
    printf("  [0] Ficar aqui\nEscolha: ");

    opcao = lerOpcao();
    if (opcao >= 1 && opcao <= quantidade)
        moverPara(jogo, destinos[opcao - 1]);
    else if (opcao != 0)
        printf("  [!] Opção inválida.\n");
}

/* Efetua o movimento conferindo a matriz e os requisitos. */
int moverPara(Jogo *jogo, int destino)
{
    int atual = jogo->jogador.cenarioAtual;

    if (destino < 0 || destino >= jogo->qtdCenarios || jogo->conexoes[atual][destino] != 1) {
        printf("  Não existe passagem direta para lá.\n");
        return 0;
    }
    if (!podeAcessar(jogo, destino)) {
        explicarBloqueio(jogo, destino);
        return 0;
    }
    jogo->jogador.cenarioAtual = destino;
    jogo->jogador.movimentos++;
    registrarEvento("Movimento: %s -> %s.", jogo->cenarios[atual].nome, jogo->cenarios[destino].nome);
    mostrarCenario(jogo);
    return 1;
}

void menuRegistros(const Jogo *jogo)
{
    const Jogador *j = &jogo->jogador;
    int i, opcao;

    printf("\n  === REGISTROS ENCONTRADOS (%d de %d) ===\n", j->qtdRegistros, jogo->qtdRegistros);
    if (j->qtdRegistros == 0) {
        printf("  Nenhum registro lido ainda. Procure terminais [4].\n");
        return;
    }
    for (i = 0; i < j->qtdRegistros; i++) {
        const Registro *r = buscarRegistroPorId(jogo, *(j->registros + i));
        printf("  [%d] %s\n", i + 1, r != NULL ? r->titulo : "(registro corrompido)");
    }
    printf("\n  [1] Ler um registro  [2] Pesquisar palavra-chave  [0] Voltar\nEscolha: ");
    opcao = lerOpcao();

    if (opcao == 1) {
        printf("  Número do registro: ");
        opcao = lerOpcao();
        if (opcao >= 1 && opcao <= j->qtdRegistros) {
            const Registro *r = buscarRegistroPorId(jogo, j->registros[opcao - 1]);
            if (r != NULL)
                exibirRegistro(jogo, r);
        } else {
            printf("  [!] Número inválido.\n");
        }
    } else if (opcao == 2) {
        pesquisarRegistros(jogo);
    } else if (opcao != 0) {
        printf("  [!] Opção inválida.\n");
    }
}

/* Usa a busca recursiva repetidamente para listar TODOS os resultados:
 * depois de achar na posição p, continua a busca a partir de p + 1. */
void pesquisarRegistros(const Jogo *jogo)
{
    char termo[MAX_NOME];
    int posicao, total = 0;

    printf("  Palavra-chave (ex.: sinal, fusível, sonar): ");
    if (!lerLinha(termo, sizeof(termo)))
        return;
    aparar(termo);
    if (strlen(termo) < 2) {
        printf("  [!] Digite pelo menos 2 caracteres.\n");
        return;
    }
    paraMaiusculas(termo);

    posicao = buscarRegistroRecursivo(jogo, 0, termo);
    while (posicao != -1) {
        const Registro *r = buscarRegistroPorId(jogo, jogo->jogador.registros[posicao]);
        printf("  - encontrado em: %s\n", r->titulo);
        total++;
        posicao = buscarRegistroRecursivo(jogo, posicao + 1, termo);
    }
    printf("  %d registro(s) contém \"%s\".\n", total, termo);
}

/* =====================================================================
 * 7. SALVAR / CARREGAR PROGRESSO E FINAL
 * ===================================================================== */

/* Grava o progresso em formato texto "chave=valor".
 * Para não corromper o save anterior se algo falhar no meio, grava
 * primeiro em progresso.tmp e só depois substitui progresso.txt. */
int salvarProgresso(const Jogo *jogo)
{
    const Jogador *j = &jogo->jogador;
    int i, erro;
    FILE *arquivo = fopen(ARQ_PROGRESSO_TMP, "w");

    if (arquivo == NULL) {
        printf("\n  [ERRO] Não foi possível criar %s (a pasta saves existe?).\n", ARQ_PROGRESSO_TMP);
        return 0;
    }

    fprintf(arquivo, "%s\n", CABECALHO_SAVE);
    fprintf(arquivo, "nome=%s\n", j->nome);
    fprintf(arquivo, "cenario=%d\n", j->cenarioAtual);
    fprintf(arquivo, "movimentos=%d\n", j->movimentos);
    fprintf(arquivo, "laboratorioLiberado=%d\n", j->laboratorioLiberado);
    fprintf(arquivo, "energiaRestaurada=%d\n", j->energiaRestaurada);
    fprintf(arquivo, "moduloInstalado=%d\n", j->moduloInstalado);
    fprintf(arquivo, "origemDescoberta=%d\n", j->origemDescoberta);
    fprintf(arquivo, "escapeLiberado=%d\n", j->escapeLiberado);

    fprintf(arquivo, "inventario=");
    for (i = 0; i < j->qtdInventario; i++)
        fprintf(arquivo, "%s%d", i > 0 ? "," : "", j->inventario[i]);
    fprintf(arquivo, "\nregistros=");
    for (i = 0; i < j->qtdRegistros; i++)
        fprintf(arquivo, "%s%d", i > 0 ? "," : "", j->registros[i]);
    fprintf(arquivo, "\n");

    /* Localização de cada item: id:local (-1 inventário, -2 utilizado). */
    for (i = 0; i < jogo->qtdItens; i++)
        fprintf(arquivo, "item=%d:%d\n", (jogo->itens + i)->id, (jogo->itens + i)->localizacao);

    erro = ferror(arquivo);
    if (fclose(arquivo) != 0)
        erro = 1;
    if (erro) {
        printf("\n  [ERRO] Falha ao gravar o progresso.\n");
        remove(ARQ_PROGRESSO_TMP);
        return 0;
    }

    /* No Windows, rename() falha se o destino existe: remove antes. */
    remove(ARQ_PROGRESSO);
    if (rename(ARQ_PROGRESSO_TMP, ARQ_PROGRESSO) != 0) {
        printf("\n  [ERRO] Não foi possível substituir %s.\n", ARQ_PROGRESSO);
        return 0;
    }
    printf("\n  [ PROGRESSO SALVO EM %s ]\n", ARQ_PROGRESSO);
    registrarEvento("Progresso salvo em %s.", jogo->cenarios[j->cenarioAtual].nome);
    return 1;
}

/* Lê o progresso. Os dados são montados em variáveis temporárias e só
 * substituem o jogo atual se o arquivo for válido. */
int carregarProgresso(Jogo *jogo)
{
    char linha[MAX_LINHA];
    Jogador novo;
    int *locais;      /* localização de cada item lida do arquivo */
    long tamanho;
    int i;
    FILE *arquivo = fopen(ARQ_PROGRESSO, "r");

    if (arquivo == NULL) {
        printf("\n  Nenhum arquivo de progresso encontrado (%s).\n", ARQ_PROGRESSO);
        return 0;
    }

    /* fseek/ftell: descobre o tamanho do arquivo para detectar save vazio. */
    fseek(arquivo, 0, SEEK_END);
    tamanho = ftell(arquivo);
    rewind(arquivo);
    if (tamanho <= 0) {
        printf("\n  O arquivo de progresso está vazio.\n");
        fclose(arquivo);
        return 0;
    }

    if (fgets(linha, sizeof(linha), arquivo) == NULL) {
        fclose(arquivo);
        return 0;
    }
    removerQuebraLinha(linha);
    if (strcmp(linha, CABECALHO_SAVE) != 0) {
        printf("\n  Nenhum progresso salvo válido em %s.\n", ARQ_PROGRESSO);
        fclose(arquivo);
        return 0;
    }

    /* Vetor temporário com o tamanho exato do vetor de itens. */
    locais = (int *)malloc(jogo->qtdItens * sizeof(int));
    if (locais == NULL) {
        printf("\n  [ERRO] Memória insuficiente.\n");
        fclose(arquivo);
        return 0;
    }
    for (i = 0; i < jogo->qtdItens; i++)
        *(locais + i) = (jogo->itens + i)->localOriginal;

    iniciarJogador(&novo, "Técnico");

    while (fgets(linha, sizeof(linha), arquivo) != NULL) {
        char *chave, *valor, *igual, *parte;

        removerQuebraLinha(linha);
        igual = strchr(linha, '=');
        if (igual == NULL)
            continue;
        *igual = '\0';
        chave = linha;
        valor = igual + 1;

        if (strcmp(chave, "nome") == 0)                     copiarTexto(novo.nome, valor, sizeof(novo.nome));
        else if (strcmp(chave, "cenario") == 0)             novo.cenarioAtual = atoi(valor);
        else if (strcmp(chave, "movimentos") == 0)          novo.movimentos = atoi(valor);
        else if (strcmp(chave, "laboratorioLiberado") == 0) novo.laboratorioLiberado = atoi(valor);
        else if (strcmp(chave, "energiaRestaurada") == 0)   novo.energiaRestaurada = atoi(valor);
        else if (strcmp(chave, "moduloInstalado") == 0)     novo.moduloInstalado = atoi(valor);
        else if (strcmp(chave, "origemDescoberta") == 0)    novo.origemDescoberta = atoi(valor);
        else if (strcmp(chave, "escapeLiberado") == 0)      novo.escapeLiberado = atoi(valor);
        else if (strcmp(chave, "inventario") == 0) {
            for (parte = strtok(valor, ","); parte != NULL; parte = strtok(NULL, ","))
                if (buscarItemPorId(jogo, atoi(parte)) != NULL)
                    adicionarAoInventario(&novo, atoi(parte));
        } else if (strcmp(chave, "registros") == 0) {
            for (parte = strtok(valor, ","); parte != NULL; parte = strtok(NULL, ","))
                if (buscarRegistroPorId(jogo, atoi(parte)) != NULL)
                    adicionarRegistroEncontrado(&novo, atoi(parte));
        } else if (strcmp(chave, "item") == 0) {
            int id, local;
            if (sscanf(valor, "%d:%d", &id, &local) == 2)
                for (i = 0; i < jogo->qtdItens; i++)
                    if ((jogo->itens + i)->id == id)
                        *(locais + i) = local;
        }
    }
    fclose(arquivo);

    if (novo.cenarioAtual < 0 || novo.cenarioAtual >= jogo->qtdCenarios) {
        printf("\n  [ERRO] Progresso corrompido (cenário inválido).\n");
        free(locais);
        return 0;
    }

    /* Arquivo válido: aplica os dados ao jogo. */
    jogo->jogador = novo;                       /* cópia de struct inteira */
    for (i = 0; i < jogo->qtdItens; i++)
        (jogo->itens + i)->localizacao = *(locais + i);
    free(locais);

    printf("\n  [ PROGRESSO CARREGADO: %s em %s ]\n",
           novo.nome, jogo->cenarios[novo.cenarioAtual].nome);
    registrarEvento("Progresso carregado (%s).", novo.nome);
    return 1;
}

/* Vitória: sistemas restabelecidos, origem descoberta, escape liberado e
 * jogador dentro da câmara de escape. */
int verificarVitoria(const Jogo *jogo)
{
    const Jogador *j = &jogo->jogador;
    return j->cenarioAtual == CEN_ESCAPE && j->energiaRestaurada &&
           j->origemDescoberta && j->escapeLiberado;
}

void mostrarFinal(Jogo *jogo)
{
    Registro *gravador = NULL;
    int i;

    for (i = 0; i < jogo->qtdRegistros; i++)
        if ((jogo->registros + i)->cenario == CEN_ESCAPE)
            gravador = jogo->registros + i;

    if (gravador != NULL) {
        exibirRegistro(jogo, gravador);
        adicionarRegistroEncontrado(&jogo->jogador, gravador->id);
    }

    printf("\n  Você entende o que precisa fazer. Aperta o botão do gravador e diz:\n");
    printf("  \"SE VOCÊ ESTÁ LENDO ISSO, NÃO ATIVE O SONAR.\"\n\n");
    printf("  A cápsula se desprende da NEREIDA-7 e sobe em direção à superfície.\n");
    printf("  Lá embaixo, em um dormitório escuro, um terminal acaba de acender.\n");
    printf("\n==============================================================\n");
    printf("              FIM DA TRANSMISSÃO - VOCÊ ESCAPOU\n");
    printf("==============================================================\n");
    printf("  Movimentos: %d | Registros encontrados: %d de %d\n",
           jogo->jogador.movimentos, jogo->jogador.qtdRegistros, jogo->qtdRegistros);
    registrarEvento("VITÓRIA: %s escapou da NEREIDA-7 com %d movimentos.",
                    jogo->jogador.nome, jogo->jogador.movimentos);
}
