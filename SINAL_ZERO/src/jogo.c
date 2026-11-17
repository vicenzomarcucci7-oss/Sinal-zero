/*
 * jogo.c
 * SINAL ZERO - Implementação das regras do jogo.
 *
 * Organização do arquivo:
 *   1. Tabelas fixas (objetos de cenário, registros, nomes)
 *   2. Utilitários de entrada e texto (string.h)
 *   3. Inicialização do mundo e memória dinâmica (diário)
 *   4. Telas: ambiente, mapa, inventário, status, diário, ajuda
 *   5. Vida: aplicarDano / restaurarVida (ponteiros)
 *   6. Movimentação e passagens bloqueadas
 *   7. Itens: coletar, descartar, examinar, usar
 *   8. Enigmas e interações
 *   9. Busca recursiva de caminhos (DFS com backtracking)
 *  10. Interpretador de comandos
 *  11. Salvar e carregar (arquivo TXT)
 *  12. Vitória, derrota e menu inicial
 *
 * O texto que aparece na tela não tem acento de propósito: assim ele
 * sai certo em qualquer terminal do Windows, sem mexer em página de código.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "jogo.h"

/* =====================================================================
 * 1. TABELAS FIXAS
 * ===================================================================== */

static const char *NOMES_DIRECOES[4] = { "NORTE", "SUL", "LESTE", "OESTE" };

/* Direção contrária: NORTE<->SUL e LESTE<->OESTE. Usada para travar e
 * destravar os dois lados de uma mesma porta. */
static const int DIRECAO_OPOSTA[4] = { SUL, NORTE, OESTE, LESTE };

static const char *TITULOS_ENIGMAS[QTD_ENIGMAS] = {
    "Seguranca do laboratorio",
    "Sistema de energia",
    "Calibracao do sonar",
    "Nucleo transmissor (desafio final)"
};

static const char *MENSAGENS_ERRO_ENIGMA[QTD_ENIGMAS] = {
    "",
    "Uma descarga eletrica sobe pelas alavancas e atravessa seu braco!",
    "O sonar devolve um pulso ensurdecedor que faz seus ouvidos sangrarem!",
    "O nucleo libera uma descarga de energia que te arremessa contra a parede!"
};

/* Objeto que o jogador usa com INTERAGIR para tentar cada enigma de novo. */
static const char *OBJETOS_ENIGMA[QTD_ENIGMAS] = { "LEITOR", "PAINEL", "CONSOLE", "NUCLEO" };

/* Objetos fixos de cada ambiente (podem ser examinados e/ou usados com
 * INTERAGIR). registroID indica o texto que vai para o diario. */
static const ObjetoCenario OBJETOS[QTD_OBJETOS] = {
    { "TERMINAL", AMB_DORMITORIO,
      "Um terminal antigo, ainda ligado na bateria de emergencia.\nHa uma mensagem na tela.", 0 },
    { "ARMARIO", AMB_DORMITORIO,
      "Uniformes encharcados. Uma das etiquetas tem o seu nome, mas a letra parece mais velha.", -1 },
    { "LEITOR", AMB_CORREDOR,
      "Leitor de cartao da porta NORTE, que leva ao laboratorio.", -1 },
    { "PLACA", AMB_CORREDOR,
      "NORTE: laboratorio | LESTE: arquivo tecnico | SUL: sala de maquinas | OESTE: dormitorio.", -1 },
    { "DIARIO", AMB_LABORATORIO,
      "Diario de pesquisa da Dra. Ilse Varga, preso a bancada com fita isolante.", 1 },
    { "TANQUE", AMB_LABORATORIO,
      "Tanque de cultivo rachado. Os organismos brilham mais forte quando o chiado aumenta.", -1 },
    { "MANUAL", AMB_ARQUIVO,
      "Manual de religamento do gerador principal, protegido por um saco plastico.", 2 },
    { "RELATORIO", AMB_ARQUIVO,
      "Relatorio 7-B sobre as amostras do laboratorio.", 3 },
    { "PAINEL", AMB_MAQUINAS,
      "Painel do gerador com quatro alavancas numeradas:\n"
      "  1-TURBINA   2-REDE   3-REFRIGERACAO   4-BOMBA\n"
      "A tampa e presa por parafusos isolados e ha um encaixe para fusivel.", -1 },
    { "GERADOR", AMB_MAQUINAS,
      "Gerador principal da estacao. Sem fusivel e sem a sequencia correta, nao liga.", -1 },
    { "CONSOLE", AMB_SONAR,
      "Console de calibracao do sonar. Uma transmissao codificada aparece na tela.", 4 },
    { "TUBULACAO", AMB_MANUTENCAO,
      "Canos estourados. Na parede, riscado com a SUA letra: 'EU JA PASSEI POR AQUI'.", -1 },
    { "NUCLEO", AMB_CONTENCAO,
      "Algo grande zumbe no centro da camara. No escuro, voce so sente a vibracao.", -1 },
    { "CAPSULA", AMB_DOCA,
      "Capsula de fuga para uma pessoa. O painel pisca: PRONTA PARA LANCAMENTO.", -1 }
};

/* Registros que podem ser encontrados e guardados no diario. */
static const Registro REGISTROS[QTD_REGISTROS] = {
    { 0, "Mensagem no terminal do dormitorio",
      "\"SE VOCE ESTA LENDO ISSO, NAO ATIVE O SONAR.\"\n"
      "A porta do laboratorio aceita o cartao magnetico.\n"
      "-- assinado: [NOME]" },
    { 1, "Diario da Dra. Ilse Varga",
      "Dia 41. O sinal se repete a cada 7 minutos, sempre identico.\n"
      "Ele nao vem do oceano: vem de dentro da estacao.\n"
      "A voz nas gravacoes pertence a alguem que ainda nao chegou aqui." },
    { 2, "Manual de religamento do gerador",
      "Para religar o gerador sem sobrecarga, acione as alavancas na ordem:\n"
      "REFRIGERACAO, BOMBA, TURBINA e, por ultimo, REDE.\n"
      "Qualquer outra ordem provoca descarga eletrica." },
    { 3, "Relatorio 7-B",
      "As amostras bioluminescentes brilham na presenca do sinal.\n"
      "A Camara de Contencao nao tem iluminacao eletrica:\n"
      "leve uma amostra para enxergar o nucleo." },
    { 4, "Transmissao codificada do sonar",
      "MENSAGEM INTERCEPTADA: HFR\n"
      "Chave de decodificacao: armazenada no modulo de memoria." },
    { 5, "Cifra da equipe (modulo de memoria)",
      "Nossas mensagens usam deslocamento: cada letra avanca TRES posicoes\n"
      "no alfabeto (A vira D, B vira E, C vira F...).\n"
      "Para decodificar, volte tres posicoes em cada letra." }
};

/* Siglas usadas no mapa. */
static const char *SIGLAS[QTD_AMBIENTES] = {
    "DORM", "CORR", "LABO", "ARQV", "MAQS", "SONR", "MANT", "CONT", "DOCA"
};

/* =====================================================================
 * 2. UTILITÁRIOS DE ENTRADA E TEXTO
 * ===================================================================== */

/* Lê uma linha com fgets. Se a linha for maior que o buffer, descarta o
 * resto. Retorna 0 quando a entrada termina (EOF). */
int lerLinha(char *destino, int tamanho)
{
    if (destino == NULL || tamanho <= 0)
        return 0;
    if (fgets(destino, tamanho, stdin) == NULL) {
        destino[0] = '\0';
        return 0;
    }
    if (strchr(destino, '\n') == NULL) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {
            /* descarta o excesso */
        }
    }
    removerQuebraLinha(destino);
    return 1;
}

/* Remove \n e \r do final (strcspn devolve a posição do primeiro). */
void removerQuebraLinha(char *texto)
{
    if (texto != NULL)
        texto[strcspn(texto, "\r\n")] = '\0';
}

/* Deixa o texto em MAIUSCULAS, sem espaços nas pontas e com apenas um
 * espaço entre as palavras. Usa dois ponteiros: um lê e outro escreve. */
void normalizarTexto(char *texto)
{
    char *leitura;
    char *escrita;
    int espacoPendente = 0;

    if (texto == NULL)
        return;
    leitura = texto;
    escrita = texto;
    while (*leitura != '\0') {
        unsigned char c = (unsigned char)*leitura;
        if (isspace(c)) {
            if (escrita != texto)
                espacoPendente = 1;
        } else {
            if (espacoPendente) {
                *escrita++ = ' ';
                espacoPendente = 0;
            }
            *escrita++ = (char)toupper(c);
        }
        leitura++;
    }
    *escrita = '\0';
}

/* Mantém apenas letras e números, em maiúsculas.
 * Assim "3-4-1-2", "3 4 1 2" e "3412" viram a mesma resposta. */
void normalizarResposta(const char *origem, char *destino, size_t tamanho)
{
    size_t n = 0;

    if (destino == NULL || tamanho == 0)
        return;
    if (origem != NULL) {
        for (; *origem != '\0' && n + 1 < tamanho; origem++) {
            unsigned char c = (unsigned char)*origem;
            if (isalnum(c))
                destino[n++] = (char)toupper(c);
        }
    }
    destino[n] = '\0';
}

/* Cópia que não estoura o destino. Se o texto cabe (strlen), uso strcpy.
 * Se não cabe, strncpy corta e eu coloco o '\0' no fim na mão. */
void copiarTexto(char *destino, const char *origem, size_t tamanho)
{
    if (destino == NULL || origem == NULL || tamanho == 0)
        return;
    if (strlen(origem) < tamanho) {
        strcpy(destino, origem);
    } else {
        strncpy(destino, origem, tamanho - 1);
        destino[tamanho - 1] = '\0';
    }
}

/* Troca a primeira ocorrência de 'marcador' (achada com strstr) por 'valor'. */
int substituirMarcador(char *texto, size_t tamanho, const char *marcador, const char *valor)
{
    char restante[512];
    char *posicao;

    if (texto == NULL || marcador == NULL || valor == NULL)
        return 0;
    posicao = strstr(texto, marcador);
    if (posicao == NULL)
        return 0;
    if (strlen(texto) - strlen(marcador) + strlen(valor) >= tamanho)
        return 0;
    copiarTexto(restante, posicao + strlen(marcador), sizeof(restante));
    *posicao = '\0';
    strcat(texto, valor);
    strcat(texto, restante);
    return 1;
}

/* O nome digitado corresponde ao nome do objeto?
 * Com 3 letras ou mais aceita parte do nome (strstr): "CHAVE" encontra
 * "CHAVE INGLESA ISOLADA". Com menos letras exige o nome exato. */
int textoCorresponde(const char *nome, const char *busca)
{
    if (nome == NULL || busca == NULL || busca[0] == '\0')
        return 0;
    if (strlen(busca) < 3)
        return strcmp(nome, busca) == 0;
    return strstr(nome, busca) != NULL;
}

/* Converte texto em inteiro, recusando valores vazios ou com lixo. */
static int lerInteiro(const char *texto, int *valor)
{
    char *fim;
    long numero;

    if (texto == NULL || valor == NULL || *texto == '\0')
        return 0;
    numero = strtol(texto, &fim, 10);
    if (*fim != '\0' || numero < -1000000 || numero > 1000000)
        return 0;
    *valor = (int)numero;
    return 1;
}

/* Lê um valor que só pode ser 0 ou 1. */
static int lerFlag(const char *texto, int *valor)
{
    int numero;
    if (!lerInteiro(texto, &numero) || (numero != 0 && numero != 1))
        return 0;
    *valor = numero;
    return 1;
}

/* Remove espaços das pontas sem mudar maiúsculas (usado no nome). */
static void aparar(char *texto)
{
    size_t tamanho = strlen(texto);
    size_t inicio = 0;

    while (tamanho > 0 && isspace((unsigned char)texto[tamanho - 1]))
        texto[--tamanho] = '\0';
    while (isspace((unsigned char)texto[inicio]))
        inicio++;
    if (inicio > 0)
        memmove(texto, texto + inicio, tamanho - inicio + 1);
}

/* =====================================================================
 * 3. INICIALIZAÇÃO DO MUNDO E MEMÓRIA DINÂMICA
 * ===================================================================== */

/* Recebe o jogador por endereço e coloca os valores iniciais. */
void iniciarJogo(Jogador *jogador)
{
    int i;

    if (jogador == NULL)
        return;
    copiarTexto(jogador->nome, "TECNICO", sizeof(jogador->nome));
    jogador->vida = VIDA_MAXIMA;
    jogador->localAtual = AMB_DORMITORIO;
    jogador->quantidadeItens = 0;
    for (i = 0; i < MAX_INVENTARIO; i++)
        *(jogador->inventario + i) = -1;      /* posição vazia */
}

static void definirAmbiente(Ambiente *ambiente, int id, const char *nome,
                            const char *descricao, const char *arte,
                            int norte, int sul, int leste, int oeste)
{
    int d;

    ambiente->id = id;
    copiarTexto(ambiente->nome, nome, sizeof(ambiente->nome));
    copiarTexto(ambiente->descricao, descricao, sizeof(ambiente->descricao));
    copiarTexto(ambiente->arteASCII, arte, sizeof(ambiente->arteASCII));
    ambiente->saidas[NORTE] = norte;
    ambiente->saidas[SUL]   = sul;
    ambiente->saidas[LESTE] = leste;
    ambiente->saidas[OESTE] = oeste;
    ambiente->visitado = 0;
    ambiente->bloqueado = 0;
    for (d = 0; d < 4; d++)
        ambiente->saidasBloqueadas[d] = TRAVA_NENHUMA;
}

/* Trava (ou destrava, com TRAVA_NENHUMA) os DOIS lados de uma passagem. */
static void definirTrava(Ambiente ambientes[], int origem, int direcao, int tipo)
{
    int destino = ambientes[origem].saidas[direcao];

    ambientes[origem].saidasBloqueadas[direcao] = tipo;
    if (destino != SEM_SAIDA)
        ambientes[destino].saidasBloqueadas[DIRECAO_OPOSTA[direcao]] = tipo;
}

static void definirItem(Item *item, int id, const char *nome, const char *descricao,
                        int consumivel, int local, int ambienteUso)
{
    item->id = id;
    copiarTexto(item->nome, nome, sizeof(item->nome));
    copiarTexto(item->descricao, descricao, sizeof(item->descricao));
    item->coletavel = 1;
    item->utilizado = 0;
    item->coletado = 0;
    item->consumivel = consumivel;
    item->localAtual = local;
    item->ambienteUso = ambienteUso;
}

static void definirEnigma(Enigma *enigma, int id, const char *descricao,
                          const char *resposta, int ambienteID, int danoErro)
{
    enigma->id = id;
    copiarTexto(enigma->descricao, descricao, sizeof(enigma->descricao));
    copiarTexto(enigma->resposta, resposta, sizeof(enigma->resposta));
    enigma->resolvido = 0;
    enigma->tentativas = 0;
    enigma->ambienteID = ambienteID;
    enigma->danoErro = danoErro;
}

/* Monta a estação do jeito que ela está no começo do jogo.
 * O diário começa vazio, sem nenhuma memória alocada (ids = NULL). */
void inicializarMundo(Jogo *jogo)
{
    Ambiente *a;

    if (jogo == NULL)
        return;
    a = jogo->ambientes;
    iniciarJogo(&jogo->jogador);

    /*            id               nome / descricao / arte                         N                S               L               O */
    definirAmbiente(a + AMB_DORMITORIO, AMB_DORMITORIO, "DORMITORIO",
        "Voce desperta apos um ruido ensurdecedor.\n"
        "As luzes de emergencia piscam e ha agua escorrendo por baixo da porta.\n"
        "Um terminal permanece ligado no canto da sala, perto do armario.",
        "       __________________________\n"
        "      |                          |\n"
        "      |        DORMITORIO        |\n"
        "      |  [ CAMA ]     [ARMARIO]  |\n"
        "      |  ~~~~~~~~     [TERMINAL] |\n"
        "      |__________[PORTA]_________|\n",
        SEM_SAIDA, SEM_SAIDA, AMB_CORREDOR, SEM_SAIDA);

    definirAmbiente(a + AMB_CORREDOR, AMB_CORREDOR, "CORREDOR CENTRAL",
        "O corredor central liga os setores da NEREIDA-7.\n"
        "O casco range com a pressao do oceano e a agua bate nos tornozelos.\n"
        "Uma placa indica as direcoes. A porta norte tem um leitor de cartao.",
        "   ==========================================\n"
        "    ||   N: LAB   |  L: ARQ   |  S: MAQ   ||\n"
        "   ------------------------------------------\n"
        "    >>   o    o    o  [LEITOR]  o    o   >>\n"
        "   ==========================================\n",
        AMB_LABORATORIO, AMB_MAQUINAS, AMB_ARQUIVO, AMB_DORMITORIO);

    definirAmbiente(a + AMB_LABORATORIO, AMB_LABORATORIO, "LABORATORIO ABISSAL",
        "Bancadas viradas, frascos quebrados e um tanque de cultivo rachado.\n"
        "Organismos minusculos brilham em azul dentro do tanque.\n"
        "Um diario de pesquisa continua preso a uma bancada.",
        "      ________________________________\n"
        "     |  [TANQUE]   o  .  o            |\n"
        "     |  ( ~*~*~ )     o    [BANCADA]  |\n"
        "     |  (_______)          [DIARIO ]  |\n"
        "     |________________________________|\n",
        SEM_SAIDA, AMB_CORREDOR, AMB_SONAR, SEM_SAIDA);

    definirAmbiente(a + AMB_ARQUIVO, AMB_ARQUIVO, "ARQUIVO TECNICO",
        "Estantes de metal cheias de pastas encharcadas.\n"
        "Quase tudo esta ilegivel, mas um manual e um relatorio\n"
        "foram guardados em sacos plasticos.",
        "      _______________________________\n"
        "     | [===] [===] [===] [===] [===] |\n"
        "     | [===] [===] [===] [===] [===] |\n"
        "     |  [MANUAL]         [RELATORIO] |\n"
        "     |_______________________________|\n",
        SEM_SAIDA, SEM_SAIDA, SEM_SAIDA, AMB_CORREDOR);

    definirAmbiente(a + AMB_MAQUINAS, AMB_MAQUINAS, "SALA DE MAQUINAS",
        "O gerador principal esta silencioso. Cheiro forte de fio queimado.\n"
        "Ao lado dele, um painel com quatro alavancas numeradas.",
        "       _______________________________\n"
        "      |   ( GERADOR )     [ PAINEL ]  |\n"
        "      |   (  o   o  )      | | | |    |\n"
        "      |   (_________)      1 2 3 4    |\n"
        "      |_______________________________|\n",
        AMB_CORREDOR, SEM_SAIDA, AMB_MANUTENCAO, SEM_SAIDA);

    definirAmbiente(a + AMB_SONAR, AMB_SONAR, "CENTRAL DE SONAR",
        "Telas enormes mostram o fundo do oceano em tons de verde.\n"
        "Os alto-falantes repetem um chiado que lembra uma voz humana.\n"
        "No centro, o console de calibracao pisca.",
        "          .   (   (   ( O )   )   )   .\n"
        "       _________________________________\n"
        "      |  [ TELA ]  [ TELA ]  [CONSOLE]  |\n"
        "      |   ~^~^~     ~^~^~     [=====]   |\n"
        "      |_________________________________|\n",
        SEM_SAIDA, AMB_MANUTENCAO, SEM_SAIDA, AMB_LABORATORIO);

    definirAmbiente(a + AMB_MANUTENCAO, AMB_MANUTENCAO, "PASSAGEM DE MANUTENCAO",
        "Um tunel estreito de manutencao. Canos estourados soltam vapor quente.\n"
        "Cada passo aqui dentro queima a pele.",
        "     ====================================\n"
        "      ||  ~ vapor ~    ||    ~ vapor ~ ||\n"
        "     =##====##====##====##====##====##==\n"
        "        (canos estourados) [TUBULACAO]\n"
        "     ====================================\n",
        AMB_SONAR, SEM_SAIDA, AMB_CONTENCAO, AMB_MAQUINAS);

    definirAmbiente(a + AMB_CONTENCAO, AMB_CONTENCAO, "CAMARA DE CONTENCAO",
        "Escuridao total: a luz eletrica nunca chegou a esta camara.\n"
        "Um zumbido grave vem do centro da sala e vibra no seu peito.",
        "            _________________\n"
        "           |    .-------.    |\n"
        "           |    | NUCLEO|    |\n"
        "           |    '-------'    |\n"
        "           |_________________|\n"
        "              (( ))   (( ))\n",
        SEM_SAIDA, AMB_DOCA, SEM_SAIDA, AMB_MANUTENCAO);

    definirAmbiente(a + AMB_DOCA, AMB_DOCA, "DOCA DE ESCAPE",
        "A doca de escape esta inundada ate a metade.\n"
        "Uma capsula de fuga flutua presa ao trilho de lancamento.",
        "        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
        "              ____________\n"
        "             |  CAPSULA   |>\n"
        "             |____________|\n"
        "        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n",
        AMB_CONTENCAO, SEM_SAIDA, SEM_SAIDA, SEM_SAIDA);

    a[AMB_DORMITORIO].visitado = 1;

    /* Passagens bloqueadas no início (os dois lados de cada porta). */
    definirTrava(a, AMB_CORREDOR, NORTE, TRAVA_CARTAO);     /* corredor <-> laboratório */
    definirTrava(a, AMB_LABORATORIO, LESTE, TRAVA_ENERGIA); /* laboratório <-> sonar    */
    definirTrava(a, AMB_CONTENCAO, SUL, TRAVA_SINAL);       /* contenção <-> doca       */
    a[AMB_MANUTENCAO].bloqueado = 1;                         /* ambiente inteiro travado */

    /* Itens: id, nome, descricao, consumivel, local inicial, onde usar */
    definirItem(jogo->itens + ITEM_CARTAO, ITEM_CARTAO, "CARTAO MAGNETICO",
        "Cartao da equipe tecnica. O nome foi raspado, mas a foto se parece com voce.",
        0, AMB_DORMITORIO, AMB_CORREDOR);
    definirItem(jogo->itens + ITEM_FUSIVEL, ITEM_FUSIVEL, "FUSIVEL INDUSTRIAL",
        "Fusivel de alta tensao compativel com o gerador principal.",
        1, AMB_LABORATORIO, AMB_MAQUINAS);
    definirItem(jogo->itens + ITEM_MODULO, ITEM_MODULO, "MODULO DE MEMORIA",
        "Modulo com registros cientificos da equipe. Encaixa no console do sonar.",
        1, AMB_ARQUIVO, AMB_SONAR);
    definirItem(jogo->itens + ITEM_CHAVE, ITEM_CHAVE, "CHAVE INGLESA ISOLADA",
        "Ferramenta com cabo de borracha, segura para mexer em mecanismos eletricos.",
        0, AMB_MAQUINAS, AMB_MAQUINAS);
    definirItem(jogo->itens + ITEM_OXIGENIO, ITEM_OXIGENIO, "CILINDRO DE OXIGENIO",
        "Cilindro portatil. Recupera 40 pontos de vida (maximo 100).",
        1, AMB_DORMITORIO, USO_QUALQUER);
    definirItem(jogo->itens + ITEM_AMOSTRA, ITEM_AMOSTRA, "AMOSTRA BIOLUMINESCENTE",
        "Frasco com organismos que brilham em azul na presenca do sinal.",
        1, AMB_LABORATORIO, AMB_CONTENCAO);

    /* Enigmas: id, descricao, resposta, ambiente, dano por erro */
    definirEnigma(jogo->enigmas + ENIGMA_PORTA, ENIGMA_PORTA,
        "Porta eletronica do laboratorio: exige um cartao autorizado.",
        "CARTAO MAGNETICO", AMB_CORREDOR, 0);
    definirEnigma(jogo->enigmas + ENIGMA_ENERGIA, ENIGMA_ENERGIA,
        "Painel do gerador: acione as quatro alavancas na ordem correta.\n"
        "  1-TURBINA   2-REDE   3-REFRIGERACAO   4-BOMBA",
        "3412", AMB_MAQUINAS, 15);
    definirEnigma(jogo->enigmas + ENIGMA_SONAR, ENIGMA_SONAR,
        "Console do sonar: digite a transmissao HFR decodificada.",
        "ECO", AMB_SONAR, 10);
    definirEnigma(jogo->enigmas + ENIGMA_NUCLEO, ENIGMA_NUCLEO,
        "SEQUENCIA DE DESLIGAMENTO DO NUCLEO.\n"
        "  Informe a PALAVRA de origem do sinal e o INTERVALO de repeticao\n"
        "  em minutos, no formato PALAVRA-NUMERO.",
        "ECO-7", AMB_CONTENCAO, 25);

    jogo->estado.energiaRestaurada = 0;
    jogo->estado.sinalDesativado = 0;
    jogo->estado.jogoFinalizado = 0;
    jogo->estado.origemDescoberta = 0;
    jogo->estado.painelAberto = 0;
    jogo->estado.fusivelInstalado = 0;
    jogo->estado.moduloInstalado = 0;
    jogo->estado.docaLiberada = 0;
    jogo->estado.fugaRealizada = 0;

    jogo->diario.ids = NULL;
    jogo->diario.quantidade = 0;
    jogo->diario.capacidade = 0;
}

int diarioContem(const Diario *diario, int idRegistro)
{
    int i;

    if (diario == NULL)
        return 0;
    for (i = 0; i < diario->quantidade; i++)
        if (*(diario->ids + i) == idRegistro)
            return 1;
    return 0;
}

/* Acrescenta um registro ao diário (vetor dinâmico).
 * Primeiro uso: malloc para 2 posições. Quando enche: realloc dobrando.
 * O realloc é feito num ponteiro TEMPORÁRIO: se falhar, o vetor antigo
 * continua válido e nada é perdido.
 * Retorna 1 = adicionado, 0 = já existia/inválido, -1 = sem memória. */
int adicionarAoDiario(Diario *diario, int idRegistro)
{
    int *temporario;
    int novaCapacidade;

    if (diario == NULL || idRegistro < 0 || idRegistro >= QTD_REGISTROS)
        return 0;
    if (diarioContem(diario, idRegistro))
        return 0;

    if (diario->quantidade == diario->capacidade) {
        if (diario->ids == NULL) {
            novaCapacidade = 2;
            temporario = (int *)malloc(novaCapacidade * sizeof(int));
        } else {
            novaCapacidade = diario->capacidade * 2;
            temporario = (int *)realloc(diario->ids, novaCapacidade * sizeof(int));
        }
        if (temporario == NULL) {
            printf("[ERRO] Memoria insuficiente para o diario.\n");
            return -1;
        }
        diario->ids = temporario;
        diario->capacidade = novaCapacidade;
    }
    *(diario->ids + diario->quantidade) = idRegistro;
    diario->quantidade++;
    return 1;
}

void liberarDiario(Diario *diario)
{
    if (diario == NULL)
        return;
    free(diario->ids);          /* free(NULL) é seguro */
    diario->ids = NULL;
    diario->quantidade = 0;
    diario->capacidade = 0;
}

void liberarJogo(Jogo *jogo)
{
    if (jogo != NULL)
        liberarDiario(&jogo->diario);
}

/* =====================================================================
 * 4. TELAS
 * ===================================================================== */

void mostrarTitulo(void)
{
    printf("\n==================================================\n");
    printf("                    SINAL ZERO\n");
    printf("               ESTACAO NEREIDA-7\n");
    printf("==================================================\n");
    printf("          .   o    ((( ( ) )))    o   .\n");
    printf("     profundidade: 2140 m | contato: PERDIDO\n");
    printf("==================================================\n\n");
}

/* Mostra arte e descrição de um ambiente (recebido por ponteiro const). */
void mostrarAmbiente(const Ambiente *ambiente)
{
    if (ambiente == NULL)
        return;
    printf("\n%s\n", ambiente->arteASCII);
    printf("%s\n", ambiente->descricao);
}

static const char *textoComunicacao(const EstadoJogo *estado)
{
    if (estado->sinalDesativado)
        return "SILENCIOSA";
    if (estado->moduloInstalado)
        return "DECODIFICANDO";
    return "INSTAVEL";
}

/* Linhas extras que mudam conforme o progresso. */
static void mostrarSituacao(const Jogo *jogo)
{
    const EstadoJogo *e = &jogo->estado;

    switch (jogo->jogador.localAtual) {
        case AMB_CORREDOR:
            printf("Leitor da porta NORTE: %s\n",
                   jogo->enigmas[ENIGMA_PORTA].resolvido ? "luz VERDE (liberada)" : "luz VERMELHA (travada)");
            break;
        case AMB_MAQUINAS:
            printf("Tampa do painel: %s | Fusivel: %s | Gerador: %s\n",
                   e->painelAberto ? "ABERTA" : "FECHADA",
                   e->fusivelInstalado ? "INSTALADO" : "AUSENTE",
                   e->energiaRestaurada ? "LIGADO" : "DESLIGADO");
            break;
        case AMB_SONAR:
            if (jogo->enigmas[ENIGMA_SONAR].resolvido)
                printf("SONAR CALIBRADO. As travas da manutencao foram liberadas.\n");
            else if (e->moduloInstalado)
                printf("Console: chave de decodificacao CARREGADA. Mensagem na tela: HFR\n");
            else
                printf("Console: mensagem codificada HFR. Chave de decodificacao: AUSENTE\n");
            break;
        case AMB_CONTENCAO:
            if (e->sinalDesativado)
                printf("O nucleo esta desligado. Pela primeira vez, silencio total.\n");
            else if (e->origemDescoberta)
                printf("A luz azul da amostra revela o NUCLEO transmissor e um gravador de bordo.\n");
            else
                printf("Voce nao enxerga nada. Algo que brilhe ajudaria aqui.\n");
            break;
        case AMB_DOCA:
            printf("A CAPSULA aguarda. Use INTERAGIR CAPSULA para fugir.\n");
            break;
        default:
            break;
    }
}

int passagemLivre(const Ambiente ambientes[], int origem, int direcao)
{
    int destino;

    if (ambientes == NULL || origem < 0 || origem >= QTD_AMBIENTES || direcao < 0 || direcao > 3)
        return 0;
    destino = ambientes[origem].saidas[direcao];
    return destino != SEM_SAIDA &&
           ambientes[origem].saidasBloqueadas[direcao] == TRAVA_NENHUMA &&
           !ambientes[destino].bloqueado;
}

static void mostrarSaidas(const Jogo *jogo)
{
    const Ambiente *atual = &jogo->ambientes[jogo->jogador.localAtual];
    int d, primeira = 1;

    printf("SAIDAS: ");
    for (d = 0; d < 4; d++) {
        int destino = atual->saidas[d];
        if (destino == SEM_SAIDA)
            continue;
        printf("%s%s (%s)%s", primeira ? "" : " | ", NOMES_DIRECOES[d],
               jogo->ambientes[destino].nome,
               passagemLivre(jogo->ambientes, atual->id, d) ? "" : " [BLOQUEADA]");
        primeira = 0;
    }
    printf("\n");
}

static void mostrarObjetosDoLocal(const Jogo *jogo)
{
    int i, quantidade = 0;
    int local = jogo->jogador.localAtual;

    printf("OBJETOS: ");
    for (i = 0; i < QTD_ITENS; i++) {
        const Item *item = jogo->itens + i;          /* (vetor + i) */
        if (item->localAtual == local) {
            printf("%s%s", quantidade > 0 ? ", " : "", item->nome);
            quantidade++;
        }
    }
    if (quantidade == 0)
        printf("nenhum");

    quantidade = 0;
    printf("\nPONTOS DE INTERESSE: ");
    for (i = 0; i < QTD_OBJETOS; i++) {
        if ((OBJETOS + i)->ambienteID == local) {
            printf("%s%s", quantidade > 0 ? ", " : "", (OBJETOS + i)->nome);
            quantidade++;
        }
    }
    if (quantidade == 0)
        printf("nenhum");
    printf("\n");
}

/* Limpa o console. O C99 não tem um comando padrão para isso, então uso o
 * comando do sistema: "cls" no Windows e "clear" no Linux/macOS. */
void limparTela(void)
{
    fflush(stdout);   /* manda antes o texto que ainda está no buffer do printf */
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

/* Próximo passo da aventura, calculado pelo progresso. Diz para onde ir e
 * o que fazer, mas não entrega a resposta dos enigmas. */
const char *objetivoAtual(const Jogo *jogo)
{
    const EstadoJogo *e = &jogo->estado;

    if (!jogo->enigmas[ENIGMA_PORTA].resolvido) {
        if (possuiItem(&jogo->jogador, ITEM_CARTAO))
            return "Usar o cartao no leitor da porta norte do Corredor Central.";
        return "Achar um jeito de abrir a porta do laboratorio (norte do corredor).";
    }
    if (!e->energiaRestaurada) {
        if (!e->painelAberto)
            return "Abrir a tampa do painel na Sala de Maquinas. Precisa de uma ferramenta isolada.";
        if (!e->fusivelInstalado)
            return "Instalar um fusivel no painel do gerador (Sala de Maquinas).";
        return "Acionar as alavancas na ordem certa (INTERAGIR PAINEL). A ordem esta no manual do Arquivo.";
    }
    if (!e->moduloInstalado)
        return "Levar o modulo de memoria ate a Central de Sonar e usa-lo no console.";
    if (!jogo->enigmas[ENIGMA_SONAR].resolvido)
        return "Decifrar a mensagem do sonar (INTERAGIR CONSOLE).";
    if (!e->origemDescoberta)
        return "Chegar a Camara de Contencao pela manutencao e iluminar o escuro.";
    if (!e->sinalDesativado)
        return "Desligar o nucleo transmissor (INTERAGIR NUCLEO).";
    return "Fugir: ir para a Doca de Escape e entrar na capsula (INTERAGIR CAPSULA).";
}

/* Resumo da aventura: onde o personagem está, o que leva, o que já fez,
 * quantas pistas tem e qual o próximo objetivo. */
void mostrarResumo(const Jogo *jogo)
{
    const Jogador *j;
    const EstadoJogo *e;
    const char *feitos[10];
    int quantidade = 0, i;

    if (jogo == NULL)
        return;
    j = &jogo->jogador;
    e = &jogo->estado;

    /* Monta a lista do que já foi feito, na ordem da história. */
    if (diarioContem(&jogo->diario, 0))           feitos[quantidade++] = "leu o aviso no terminal do dormitorio";
    if (jogo->enigmas[ENIGMA_PORTA].resolvido)    feitos[quantidade++] = "abriu a porta do laboratorio";
    if (diarioContem(&jogo->diario, 1))           feitos[quantidade++] = "leu o diario da Dra. Varga";
    if (diarioContem(&jogo->diario, 2))           feitos[quantidade++] = "achou o manual do gerador";
    if (e->energiaRestaurada)                     feitos[quantidade++] = "religou o gerador";
    if (e->moduloInstalado)                       feitos[quantidade++] = "instalou o modulo no sonar";
    if (jogo->enigmas[ENIGMA_SONAR].resolvido)    feitos[quantidade++] = "decifrou o sonar e abriu a manutencao";
    if (e->origemDescoberta)                      feitos[quantidade++] = "descobriu a origem do sinal";
    if (e->sinalDesativado)                       feitos[quantidade++] = "desligou o nucleo e liberou a doca";

    printf("\n------------------------------- RESUMO -------------------------------\n");
    printf("Voce esta em: %s | Vida: %d/%d\n",
           jogo->ambientes[j->localAtual].nome, j->vida, VIDA_MAXIMA);

    printf("Levando: ");
    if (j->quantidadeItens == 0)
        printf("nada");
    for (i = 0; i < j->quantidadeItens; i++)
        printf("%s%s", i > 0 ? ", " : "", jogo->itens[*(j->inventario + i)].nome);
    printf("\n");

    printf("Ja fez: ");
    if (quantidade == 0)
        printf("nada ainda. Voce acabou de acordar");
    for (i = 0; i < quantidade; i++)
        printf("%s%s", i > 0 ? ", " : "", feitos[i]);
    printf(".\n");

    printf("Pistas no diario: %d de %d (DIARIO para reler)\n", jogo->diario.quantidade, QTD_REGISTROS);
    printf("OBJETIVO: %s\n", objetivoAtual(jogo));
    printf("----------------------------------------------------------------------\n");
}

/* Linha que antecede cada comando, lembrando onde o jogador está. */
void mostrarPrompt(const Jogo *jogo)
{
    printf("\n[%s] O que deseja fazer?\n> ", jogo->ambientes[jogo->jogador.localAtual].nome);
}

/* Tela do local atual: cabeçalho curto, arte, texto, saídas, objetos e resumo. */
void mostrarTela(const Jogo *jogo)
{
    const Jogador *j;
    int i, barras;

    if (jogo == NULL)
        return;
    j = &jogo->jogador;

    printf("======================================================================\n");
    printf("SINAL ZERO - NEREIDA-7 | ENERGIA: %s | COMUNICACAO: %s\n",
           jogo->estado.energiaRestaurada ? "ONLINE" : "OFFLINE", textoComunicacao(&jogo->estado));
    barras = j->vida / 10;
    printf("LOCAL: %s | VIDA: %d/%d [", jogo->ambientes[j->localAtual].nome, j->vida, VIDA_MAXIMA);
    for (i = 0; i < 10; i++)
        putchar(i < barras ? '#' : '-');
    printf("]\n");
    printf("======================================================================\n");

    mostrarAmbiente(&jogo->ambientes[j->localAtual]);
    mostrarSituacao(jogo);
    printf("\n");
    mostrarSaidas(jogo);
    mostrarObjetosDoLocal(jogo);
    mostrarResumo(jogo);
}

/* Lista os itens do inventário percorrendo o vetor com *(vetor + i). */
void mostrarInventario(const Jogador *jogador, const Item itens[])
{
    int i;

    if (jogador == NULL || itens == NULL)
        return;
    printf("\n=== INVENTARIO (%d/%d) ===\n", jogador->quantidadeItens, MAX_INVENTARIO);
    if (jogador->quantidadeItens == 0) {
        printf("Vazio.\n");
        return;
    }
    for (i = 0; i < jogador->quantidadeItens; i++) {
        int id = *(jogador->inventario + i);
        if (id >= 0 && id < QTD_ITENS)
            printf("- %s: %s\n", (itens + id)->nome, (itens + id)->descricao);
    }
}

void mostrarStatus(const Jogo *jogo)
{
    const Jogador *j = &jogo->jogador;
    const EstadoJogo *e = &jogo->estado;
    int i;

    printf("\n=== STATUS ===\n");
    printf("Tecnico: %s | Vida: %d/%d | Local: %s\n",
           j->nome, j->vida, VIDA_MAXIMA, jogo->ambientes[j->localAtual].nome);
    printf("Itens: %d/%d | Registros no diario: %d\n",
           j->quantidadeItens, MAX_INVENTARIO, jogo->diario.quantidade);
    printf("Energia: %s | Comunicacao: %s\n",
           e->energiaRestaurada ? "ONLINE" : "OFFLINE", textoComunicacao(e));

    printf("\nDesafios:\n");
    for (i = 0; i < QTD_ENIGMAS; i++) {
        const Enigma *enigma = jogo->enigmas + i;
        printf("  [%c] %-36s tentativas erradas: %d\n",
               enigma->resolvido ? 'X' : ' ', TITULOS_ENIGMAS[i], enigma->tentativas);
    }

    printf("\nObjetivos para a vitoria:\n");
    printf("  [%c] Descobrir a origem do sinal\n", e->origemDescoberta ? 'X' : ' ');
    printf("  [%c] Desativar a transmissao\n", e->sinalDesativado ? 'X' : ' ');
    printf("  [%c] Desbloquear a Doca de Escape\n", e->docaLiberada ? 'X' : ' ');
    printf("  [%c] Executar a fuga\n", e->fugaRealizada ? 'X' : ' ');
}

/* Monta o rótulo de um ambiente no mapa:
 * <SIGLA> = você está aqui, [SIGLA] = visitado, [????] = desconhecido. */
static void rotuloMapa(const Ambiente ambientes[], int id, int localAtual, char rotulo[7])
{
    if (id == localAtual)
        sprintf(rotulo, "<%s>", SIGLAS[id]);
    else if (ambientes[id].visitado)
        sprintf(rotulo, "[%s]", SIGLAS[id]);
    else
        sprintf(rotulo, "[????]");
}

/* 'X' no mapa quando a passagem está bloqueada. */
static char conector(const Ambiente ambientes[], int origem, int direcao, char livre)
{
    return passagemLivre(ambientes, origem, direcao) ? livre : 'X';
}

void mostrarMapa(const Ambiente ambientes[], int quantidade, int localAtual)
{
    char r[QTD_AMBIENTES][7];
    int i;

    if (ambientes == NULL || quantidade != QTD_AMBIENTES || localAtual < 0 || localAtual >= quantidade)
        return;
    for (i = 0; i < quantidade; i++)
        rotuloMapa(ambientes, i, localAtual, r[i]);

    printf("\n=============== MAPA DA NEREIDA-7 ===============\n\n");
    printf("              %s----%c-----%s\n", r[AMB_LABORATORIO],
           conector(ambientes, AMB_LABORATORIO, LESTE, '-'), r[AMB_SONAR]);
    printf("                %c               %c\n",
           conector(ambientes, AMB_CORREDOR, NORTE, '|'), conector(ambientes, AMB_SONAR, SUL, '|'));
    printf("  %s--%c---%s-%c--%s  %c\n", r[AMB_DORMITORIO],
           conector(ambientes, AMB_DORMITORIO, LESTE, '-'), r[AMB_CORREDOR],
           conector(ambientes, AMB_CORREDOR, LESTE, '-'), r[AMB_ARQUIVO],
           conector(ambientes, AMB_SONAR, SUL, '|'));
    printf("                %c               %c\n",
           conector(ambientes, AMB_CORREDOR, SUL, '|'), conector(ambientes, AMB_SONAR, SUL, '|'));
    printf("              %s----%c-----%s--%c---%s\n", r[AMB_MAQUINAS],
           conector(ambientes, AMB_MAQUINAS, LESTE, '-'), r[AMB_MANUTENCAO],
           conector(ambientes, AMB_MANUTENCAO, LESTE, '-'), r[AMB_CONTENCAO]);
    printf("                                            %c\n",
           conector(ambientes, AMB_CONTENCAO, SUL, '|'));
    printf("                                          %s\n", r[AMB_DOCA]);
    printf("\n  <XXXX> voce esta aqui   [????] nao visitado   X passagem bloqueada\n");
    printf("  DORM dormitorio  CORR corredor  LABO laboratorio  ARQV arquivo\n");
    printf("  MAQS maquinas    SONR sonar     MANT manutencao   CONT contencao  DOCA doca\n");
}

/* Mostra um registro trocando [NOME] pelo nome do jogador (strstr). */
void mostrarRegistro(const Jogo *jogo, int idRegistro)
{
    char texto[512];
    const Registro *registro;

    if (jogo == NULL || idRegistro < 0 || idRegistro >= QTD_REGISTROS)
        return;
    registro = REGISTROS + idRegistro;
    copiarTexto(texto, registro->texto, sizeof(texto));
    substituirMarcador(texto, sizeof(texto), "[NOME]", jogo->jogador.nome);
    printf("\n  >>> %s\n%s\n", registro->titulo, texto);
}

void mostrarDiario(const Jogo *jogo)
{
    int i;

    printf("\n=== DIARIO DE INVESTIGACAO (%d de %d registros) ===\n",
           jogo->diario.quantidade, QTD_REGISTROS);
    if (jogo->diario.quantidade == 0) {
        printf("Nenhum registro ainda. Examine terminais, documentos e consoles.\n");
        return;
    }
    for (i = 0; i < jogo->diario.quantidade; i++)
        mostrarRegistro(jogo, *(jogo->diario.ids + i));
}

void mostrarAjuda(void)
{
    printf("\n=== COMANDOS ===\n");
    printf("  NORTE, SUL, LESTE, OESTE (ou N, S, L, O)   movimentar-se\n");
    printf("  OLHAR                 descreve o local atual\n");
    printf("  EXAMINAR <OBJETO>     examina um item ou ponto de interesse\n");
    printf("  PEGAR <ITEM>          coleta um item (limite de %d)\n", MAX_INVENTARIO);
    printf("  USAR <ITEM>           usa um item do inventario\n");
    printf("  DESCARTAR <ITEM>      deixa um item no local atual\n");
    printf("  INTERAGIR <OBJETO>    mexe em paineis, consoles, capsula...\n");
    printf("  INVENTARIO            lista seus itens\n");
    printf("  DIARIO                le os registros encontrados\n");
    printf("  MAPA                  mostra o mapa da estacao\n");
    printf("  BUSCAR <DESTINO>      procura um caminho livre (busca recursiva)\n");
    printf("  RESUMO                onde voce esta, o que ja fez e o proximo objetivo\n");
    printf("  STATUS                vida, desafios e objetivos\n");
    printf("  SALVAR / CARREGAR     arquivo %s\n", ARQ_SAVE);
    printf("  AJUDA                 esta lista\n");
    printf("  SAIR                  encerra o jogo\n");
    printf("Enigmas: digite INTERAGIR <OBJETO> (painel, console, nucleo) e responda\n");
    printf("quando o jogo pedir. Errou? Digite INTERAGIR de novo para outra tentativa.\n");
    printf("Dica: nomes podem ser parciais (PEGAR CHAVE) e artigos sao ignorados\n");
    printf("(USAR O CARTAO, INTERAGIR COM O PAINEL). Digite sem acentos.\n");
    printf("Variacoes tambem valem: PEGA, USA, OLHA, VAI PRO NORTE, LER O MANUAL,\n");
    printf("ENCERRAR, FECHAR...\n");
}

/* =====================================================================
 * 5. VIDA (passagem por endereço)
 * ===================================================================== */

/* Diminui a vida do jogador ORIGINAL: a função recebe o endereço (&jogador)
 * e altera o campo com ->. Nunca deixa a vida negativa. */
void aplicarDano(Jogador *jogador, int dano)
{
    if (jogador == NULL || dano <= 0)
        return;
    jogador->vida -= dano;
    if (jogador->vida < 0)
        jogador->vida = 0;
}

/* Recupera vida respeitando o máximo de 100. */
void restaurarVida(Jogador *jogador, int quantidade)
{
    if (jogador == NULL || quantidade <= 0)
        return;
    jogador->vida += quantidade;
    if (jogador->vida > VIDA_MAXIMA)
        jogador->vida = VIDA_MAXIMA;
}

/* =====================================================================
 * 6. MOVIMENTAÇÃO E PASSAGENS
 * ===================================================================== */

void explicarTrava(int tipoTrava)
{
    switch (tipoTrava) {
        case TRAVA_CARTAO:
            printf("A porta eletronica exige um CARTAO MAGNETICO autorizado no leitor.\n");
            break;
        case TRAVA_ENERGIA:
            printf("A porta e motorizada e nao ha energia. Religue o gerador na sala de maquinas.\n");
            break;
        case TRAVA_SINAL:
            printf("Bloqueio de quarentena: a doca so abre com o sinal desativado no nucleo.\n");
            break;
        default:
            printf("A passagem esta bloqueada.\n");
    }
}

/* Libera os dois lados de uma passagem (altera o vetor recebido). */
void desbloquearPassagem(Ambiente ambientes[], int origem, int direcao)
{
    if (ambientes == NULL || origem < 0 || origem >= QTD_AMBIENTES || direcao < 0 || direcao > 3)
        return;
    definirTrava(ambientes, origem, direcao, TRAVA_NENHUMA);
}

/* Move o jogador verificando, nesta ordem: direção válida, existência da
 * saída, destino existente, trava da porta e bloqueio do ambiente.
 * As conexões vêm do vetor saidas[4]; não há if por ambiente. */
int moverJogador(Jogador *jogador, Ambiente ambientes[], int quantidade, int direcao)
{
    Ambiente *atual;
    Ambiente *destino;
    int idDestino;

    if (jogador == NULL || ambientes == NULL)
        return MOV_INVALIDO;
    if (direcao < NORTE || direcao > OESTE) {
        printf("Direcao invalida.\n");
        return MOV_INVALIDO;
    }

    atual = ambientes + jogador->localAtual;          /* ponteiro para o ambiente atual */
    idDestino = atual->saidas[direcao];
    if (idDestino == SEM_SAIDA) {
        printf("Nao ha saida para o %s.\n", NOMES_DIRECOES[direcao]);
        return MOV_SEM_SAIDA;
    }
    if (idDestino < 0 || idDestino >= quantidade) {
        printf("[ERRO] Conexao para um ambiente inexistente.\n");
        return MOV_INVALIDO;
    }
    if (atual->saidasBloqueadas[direcao] != TRAVA_NENHUMA) {
        printf("\n[BLOQUEADO] ");
        explicarTrava(atual->saidasBloqueadas[direcao]);
        return MOV_BLOQUEADO;
    }

    destino = ambientes + idDestino;
    if (destino->bloqueado) {
        printf("\n[BLOQUEADO] As travas de seguranca da %s estao acionadas.\n", destino->nome);
        printf("O sistema de manutencao so libera as travas depois da calibracao do sonar.\n");
        return MOV_BLOQUEADO;
    }

    jogador->localAtual = idDestino;
    destino->visitado = 1;
    return MOV_OK;
}

static void comandoMover(Jogo *jogo, int direcao)
{
    if (moverJogador(&jogo->jogador, jogo->ambientes, QTD_AMBIENTES, direcao) != MOV_OK)
        return;

    limparTela();                                       /* sala nova: tela limpa */
    if (jogo->jogador.localAtual == AMB_MANUTENCAO) {
        aplicarDano(&jogo->jogador, DANO_MANUTENCAO);   /* passagem por endereço */
        mostrarTela(jogo);
        printf("\n[PERIGO] Um jato de vapor queima seu ombro! -%d de vida (vida: %d).\n",
               DANO_MANUTENCAO, jogo->jogador.vida);
        return;
    }
    mostrarTela(jogo);
}

/* =====================================================================
 * 7. ITENS
 * ===================================================================== */

/* Procura um item pelo nome entre os que estão em 'local' (um ambiente ou
 * LOCAL_INVENTARIO). Devolve um PONTEIRO para o item dentro do vetor, ou NULL. */
Item *buscarItem(Item itens[], int quantidade, const char *nome, int local)
{
    int i;

    if (itens == NULL || nome == NULL)
        return NULL;
    for (i = 0; i < quantidade; i++) {
        Item *item = itens + i;
        if (item->localAtual == local && textoCorresponde(item->nome, nome))
            return item;
    }
    return NULL;
}

static const ObjetoCenario *buscarObjetoCenario(int ambiente, const char *nome)
{
    int i;

    for (i = 0; i < QTD_OBJETOS; i++) {
        const ObjetoCenario *objeto = OBJETOS + i;
        if (objeto->ambienteID == ambiente && textoCorresponde(objeto->nome, nome))
            return objeto;
    }
    return NULL;
}

int possuiItem(const Jogador *jogador, int idItem)
{
    int i;

    if (jogador == NULL)
        return 0;
    for (i = 0; i < jogador->quantidadeItens; i++)
        if (*(jogador->inventario + i) == idItem)
            return 1;
    return 0;
}

/* Pega um item. Pelos dois ponteiros a função muda o inventário do
 * jogador e o local do item ao mesmo tempo. */
int coletarItem(Jogador *jogador, Item *item)
{
    if (jogador == NULL || item == NULL)
        return 0;
    if (!item->coletavel) {
        printf("Nao e possivel carregar isso.\n");
        return 0;
    }
    if (item->localAtual == LOCAL_INVENTARIO || possuiItem(jogador, item->id)) {
        printf("Voce ja carrega %s.\n", item->nome);
        return 0;
    }
    if (item->localAtual != jogador->localAtual) {
        printf("%s nao esta aqui.\n", item->nome);
        return 0;
    }
    if (jogador->quantidadeItens >= MAX_INVENTARIO) {
        printf("Inventario cheio (%d/%d). Use DESCARTAR <ITEM> para liberar espaco.\n",
               jogador->quantidadeItens, MAX_INVENTARIO);
        return 0;
    }

    jogador->inventario[jogador->quantidadeItens] = item->id;
    jogador->quantidadeItens++;
    item->localAtual = LOCAL_INVENTARIO;
    item->coletado = 1;
    return 1;
}

/* Remove o id do vetor deslocando os seguintes para a esquerda. */
int removerDoInventario(Jogador *jogador, int idItem)
{
    int i, j;

    if (jogador == NULL)
        return 0;
    for (i = 0; i < jogador->quantidadeItens; i++) {
        if (jogador->inventario[i] == idItem) {
            for (j = i; j < jogador->quantidadeItens - 1; j++)
                jogador->inventario[j] = jogador->inventario[j + 1];
            jogador->quantidadeItens--;
            jogador->inventario[jogador->quantidadeItens] = -1;
            return 1;
        }
    }
    return 0;
}

/* Deixa o item no chão do ambiente atual. */
int descartarItem(Jogador *jogador, Item *item)
{
    if (jogador == NULL || item == NULL)
        return 0;
    if (!removerDoInventario(jogador, item->id))
        return 0;
    item->localAtual = jogador->localAtual;
    return 1;
}

void examinarObjeto(const Item *item)
{
    if (item == NULL) {
        printf("Nao ha nada para examinar.\n");
        return;
    }
    printf("\n[ %s ]\n%s\n", item->nome, item->descricao);
    printf("Situacao: %s%s\n",
           item->localAtual == LOCAL_INVENTARIO ? "no seu inventario" : "neste local",
           item->utilizado ? " (ja utilizado)" : "");
}

/* Item usado e gasto: sai do inventário e do jogo. */
static void consumirItem(Jogo *jogo, Item *item)
{
    removerDoInventario(&jogo->jogador, item->id);
    item->localAtual = LOCAL_CONSUMIDO;
    item->utilizado = 1;
}

/* Aplica o efeito de um item do inventário no local atual. */
int usarItem(Jogo *jogo, Item *item)
{
    Jogador *jogador;
    EstadoJogo *estado;

    if (jogo == NULL || item == NULL)
        return 0;
    jogador = &jogo->jogador;
    estado = &jogo->estado;

    if (!possuiItem(jogador, item->id)) {
        printf("Voce nao possui %s.\n", item->nome);
        return 0;
    }
    if (item->ambienteUso != USO_QUALQUER && item->ambienteUso != jogador->localAtual) {
        printf("Voce procura onde usar %s, mas nada aqui reage.\n", item->nome);
        return 0;
    }

    switch (item->id) {
        case ITEM_CARTAO:
            if (jogo->enigmas[ENIGMA_PORTA].resolvido) {
                printf("A porta do laboratorio ja esta liberada.\n");
                return 0;
            }
            /* Enigma 1: o "segredo" da porta é o próprio cartão. */
            if (resolverEnigma(&jogo->enigmas[ENIGMA_PORTA], item->nome)) {
                desbloquearPassagem(jogo->ambientes, AMB_CORREDOR, NORTE);
                item->utilizado = 1;
                printf("Voce passa o cartao no leitor. Bip. A luz fica VERDE.\n");
                printf("A porta NORTE do laboratorio destrava com um estalo.\n");
            }
            return 1;

        case ITEM_CHAVE:
            if (estado->painelAberto) {
                printf("A tampa do painel ja esta aberta.\n");
                return 0;
            }
            estado->painelAberto = 1;
            item->utilizado = 1;
            printf("Com a chave inglesa isolada voce solta os parafusos da tampa do painel.\n");
            printf("O encaixe do fusivel e as alavancas agora estao acessiveis.\n");
            return 1;

        case ITEM_FUSIVEL:
            if (!estado->painelAberto) {
                printf("A tampa do painel esta presa por parafusos. Abra-a antes (ferramenta isolada).\n");
                return 0;
            }
            estado->fusivelInstalado = 1;
            consumirItem(jogo, item);
            printf("Voce encaixa o fusivel industrial. Uma luz amarela acende no painel.\n");
            printf("Agora falta acionar as alavancas na ordem correta (INTERAGIR PAINEL).\n");
            return 1;

        case ITEM_MODULO:
            estado->moduloInstalado = 1;
            consumirItem(jogo, item);
            printf("Voce encaixa o modulo de memoria no console. Registros sao recuperados...\n");
            if (adicionarAoDiario(&jogo->diario, 5) == 1)
                printf("(registro adicionado ao diario)\n");
            mostrarRegistro(jogo, 5);
            printf("\nAgora use INTERAGIR CONSOLE para calibrar o sonar.\n");
            return 1;

        case ITEM_OXIGENIO:
            if (jogador->vida >= VIDA_MAXIMA) {
                printf("Sua respiracao esta normal. Melhor guardar o oxigenio.\n");
                return 0;
            }
            restaurarVida(jogador, RECUPERACAO_OXIGENIO);
            consumirItem(jogo, item);
            printf("Voce respira o oxigenio puro. Vida: %d/%d.\n", jogador->vida, VIDA_MAXIMA);
            return 1;

        case ITEM_AMOSTRA:
            estado->origemDescoberta = 1;
            consumirItem(jogo, item);
            printf("\nVoce abre o frasco. A amostra se acende em um azul intenso.\n");
            printf("A luz revela o NUCLEO: um transmissor ligado a um gravador de bordo.\n");
            printf("Na tela do gravador: ULTIMA GRAVACAO - VOZ: %s.\n", jogador->nome);
            printf("\nORIGEM DO SINAL DESCOBERTA: ESTA CAMARA.\n");
            printf("O sonar captou mensagens gravadas por voce no futuro e as devolveu\n");
            printf("ao passado, a cada 7 minutos. O pedido de socorro sempre foi seu.\n");
            printf("Use INTERAGIR NUCLEO para tentar desliga-lo.\n");
            return 1;

        default:
            printf("Nada acontece.\n");
            return 0;
    }
}

static void comandoPegar(Jogo *jogo, const char *nome)
{
    Item *item;

    if (nome[0] == '\0') {
        printf("Pegar o que? Use: PEGAR <ITEM>\n");
        return;
    }
    item = buscarItem(jogo->itens, QTD_ITENS, nome, jogo->jogador.localAtual);
    if (item == NULL) {
        Item *carregado = buscarItem(jogo->itens, QTD_ITENS, nome, LOCAL_INVENTARIO);
        if (carregado != NULL)
            printf("Voce ja carrega %s.\n", carregado->nome);   /* impede duplicidade */
        else if (buscarObjetoCenario(jogo->jogador.localAtual, nome) != NULL)
            printf("Isso esta preso a estacao. Nao da para carregar.\n");
        else
            printf("Nao ha '%s' aqui.\n", nome);
        return;
    }
    if (coletarItem(&jogo->jogador, item))
        printf("Voce pegou: %s (%d/%d)\n", item->nome, jogo->jogador.quantidadeItens, MAX_INVENTARIO);
}

static void comandoDescartar(Jogo *jogo, const char *nome)
{
    Item *item;

    if (nome[0] == '\0') {
        printf("Descartar o que? Use: DESCARTAR <ITEM>\n");
        return;
    }
    item = buscarItem(jogo->itens, QTD_ITENS, nome, LOCAL_INVENTARIO);
    if (item == NULL) {
        printf("Voce nao carrega '%s'.\n", nome);
        return;
    }
    if (descartarItem(&jogo->jogador, item))
        printf("Voce deixou %s em %s.\n", item->nome, jogo->ambientes[jogo->jogador.localAtual].nome);
}

/* =====================================================================
 * 8. ENIGMAS E INTERAÇÕES
 * ===================================================================== */

/* Compara a resposta com a solução ignorando espaços, hífens e maiúsculas.
 * Altera o enigma original pelo ponteiro: resolvido ou tentativas. */
int resolverEnigma(Enigma *enigma, const char *resposta)
{
    char tentativa[100];
    char solucao[100];

    if (enigma == NULL || resposta == NULL)
        return 0;
    if (enigma->resolvido)
        return 1;
    normalizarResposta(resposta, tentativa, sizeof(tentativa));
    normalizarResposta(enigma->resposta, solucao, sizeof(solucao));
    if (strlen(tentativa) > 0 && strcmp(tentativa, solucao) == 0) {
        enigma->resolvido = 1;
        return 1;
    }
    enigma->tentativas++;
    return 0;
}

/* Pede a resposta ao jogador. Resposta errada causa dano (danoErro).
 * ENTER vazio cancela sem penalidade. */
int tentarEnigma(Jogo *jogo, int idEnigma, const char *instrucao)
{
    Enigma *enigma;
    char resposta[MAX_COMANDO];

    if (jogo == NULL || idEnigma < 0 || idEnigma >= QTD_ENIGMAS)
        return 0;
    enigma = jogo->enigmas + idEnigma;
    if (enigma->resolvido)
        return 1;

    printf("\n=== %s ===\n  %s\n", TITULOS_ENIGMAS[idEnigma], enigma->descricao);
    if (instrucao != NULL)
        printf("  %s\n", instrucao);
    printf("Resposta (ENTER para cancelar): ");
    if (!lerLinha(resposta, sizeof(resposta)))
        return 0;
    normalizarTexto(resposta);
    if (strlen(resposta) == 0) {
        printf("Voce se afasta. Quando quiser responder, digite INTERAGIR %s.\n",
               OBJETOS_ENIGMA[idEnigma]);
        return 0;
    }

    if (resolverEnigma(enigma, resposta)) {
        printf("\n[ CORRETO ]\n");
        return 1;
    }
    printf("\n[ INCORRETO ] (tentativa errada n. %d)\n", enigma->tentativas);
    if (enigma->danoErro > 0) {
        printf("%s\n", MENSAGENS_ERRO_ENIGMA[idEnigma]);
        aplicarDano(&jogo->jogador, enigma->danoErro);
        printf("-%d de vida. Vida atual: %d/%d\n", enigma->danoErro, jogo->jogador.vida, VIDA_MAXIMA);
    }
    if (jogo->jogador.vida > 0)
        printf("Para tentar de novo, digite INTERAGIR %s.\n", OBJETOS_ENIGMA[idEnigma]);
    return 0;
}

/* Examina item (inventário ou chão) ou ponto de interesse do local. */
void examinarAlvo(Jogo *jogo, const char *nome)
{
    const ObjetoCenario *objeto;
    Item *item;
    int local = jogo->jogador.localAtual;

    if (nome[0] == '\0') {
        printf("Examinar o que? Use: EXAMINAR <OBJETO>\n");
        return;
    }
    item = buscarItem(jogo->itens, QTD_ITENS, nome, LOCAL_INVENTARIO);
    if (item == NULL)
        item = buscarItem(jogo->itens, QTD_ITENS, nome, local);
    if (item != NULL) {
        examinarObjeto(item);
        return;
    }

    objeto = buscarObjetoCenario(local, nome);
    if (objeto == NULL) {
        printf("Voce nao ve '%s' aqui.\n", nome);
        return;
    }
    if (local == AMB_CONTENCAO && !jogo->estado.origemDescoberta) {
        printf("Esta escuro demais para examinar qualquer coisa. Voce so sente a vibracao.\n");
        return;
    }
    printf("\n[ %s ]\n%s\n", objeto->nome, objeto->descricao);
    if (objeto->registroID >= 0) {
        int resultado = adicionarAoDiario(&jogo->diario, objeto->registroID);
        mostrarRegistro(jogo, objeto->registroID);
        if (resultado == 1)
            printf("(registro adicionado ao diario)\n");
    }
}

/* INTERAGIR com pontos de interesse: painéis, console, núcleo, cápsula. */
void interagirObjeto(Jogo *jogo, const char *nome)
{
    const ObjetoCenario *objeto;
    EstadoJogo *estado = &jogo->estado;

    if (nome[0] == '\0') {
        printf("Interagir com o que? Use: INTERAGIR <OBJETO>\n");
        return;
    }
    objeto = buscarObjetoCenario(jogo->jogador.localAtual, nome);
    if (objeto == NULL) {
        if (buscarItem(jogo->itens, QTD_ITENS, nome, LOCAL_INVENTARIO) != NULL)
            printf("Para itens use: USAR <ITEM>\n");
        else
            printf("Nao ha '%s' aqui.\n", nome);
        return;
    }

    if (strcmp(objeto->nome, "PAINEL") == 0) {
        /* ENIGMA 2: chave inglesa + fusível + sequência do manual */
        if (estado->energiaRestaurada) {
            printf("O gerador ja esta funcionando.\n");
        } else if (!estado->painelAberto) {
            printf("A tampa esta presa por parafusos isolados. Mexer com as maos seria suicidio.\n");
        } else if (!estado->fusivelInstalado) {
            printf("O encaixe do fusivel esta vazio. As alavancas nao respondem.\n");
        } else if (tentarEnigma(jogo, ENIGMA_ENERGIA,
                   "Pista: o MANUAL do Arquivo Tecnico diz a ordem pelos NOMES das alavancas.\n"
                   "  Troque cada nome pelo numero dele no painel e digite os 4 numeros\n"
                   "  separados por espaco. (DIARIO mostra o manual, se voce ja leu.)")) {
            estado->energiaRestaurada = 1;
            desbloquearPassagem(jogo->ambientes, AMB_LABORATORIO, LESTE);
            printf("O gerador tosse, engasga... e ruge. As luzes da estacao acendem!\n");
            printf("[ SISTEMA DE ENERGIA: ONLINE ] A porta motorizada para o sonar foi liberada.\n");
        }
    } else if (strcmp(objeto->nome, "CONSOLE") == 0) {
        /* ENIGMA 3: módulo de memória + decodificação da cifra */
        if (jogo->enigmas[ENIGMA_SONAR].resolvido) {
            printf("O sonar ja esta calibrado.\n");
        } else if (!estado->moduloInstalado) {
            printf("A mensagem HFR pisca na tela. Sem a chave de decodificacao, nao faz sentido.\n");
            printf("O console tem uma entrada para MODULO DE MEMORIA.\n");
        } else if (tentarEnigma(jogo, ENIGMA_SONAR,
                   "Mensagem codificada: HFR\n"
                   "  Pista: a regra da cifra veio no MODULO DE MEMORIA (DIARIO mostra de novo).")) {
            jogo->ambientes[AMB_MANUTENCAO].bloqueado = 0;
            printf("O sonar emite um pulso grave e se estabiliza. Na tela: ECO.\n");
            printf("Ao longe, as travas da PASSAGEM DE MANUTENCAO se soltam (SUL).\n");
        }
    } else if (strcmp(objeto->nome, "NUCLEO") == 0) {
        /* DESAFIO FINAL: amostra (origem) + palavra do sonar + intervalo do diário */
        if (estado->sinalDesativado) {
            printf("O nucleo esta desligado.\n");
        } else if (!estado->origemDescoberta) {
            printf("No escuro voce nao encontra nenhum controle. Algo que brilhe ajudaria.\n");
        } else if (tentarEnigma(jogo, ENIGMA_NUCLEO,
                   "Pistas: a palavra saiu do SONAR e o intervalo esta no diario da Dra. Varga.\n"
                   "  Formato: PALAVRA-NUMERO (exemplo de formato: SOM-3).")) {
            estado->sinalDesativado = 1;
            estado->docaLiberada = 1;
            desbloquearPassagem(jogo->ambientes, AMB_CONTENCAO, SUL);
            printf("O zumbido cessa. Os alto-falantes da estacao ficam mudos.\n");
            printf("[ TRANSMISSAO ENCERRADA ] A quarentena foi suspensa: a DOCA DE ESCAPE (SUL) esta livre.\n");
        }
    } else if (strcmp(objeto->nome, "CAPSULA") == 0) {
        estado->fugaRealizada = 1;
        estado->jogoFinalizado = 1;
    } else if (strcmp(objeto->nome, "LEITOR") == 0) {
        if (jogo->enigmas[ENIGMA_PORTA].resolvido)
            printf("O leitor mostra luz verde. A porta esta liberada.\n");
        else
            printf("ACESSO NEGADO. O leitor espera um cartao (USAR CARTAO).\n");
    } else {
        /* Terminal, diario, manual etc. não têm mecanismo: interagir = examinar. */
        examinarAlvo(jogo, objeto->nome);
    }
}

static void comandoUsar(Jogo *jogo, const char *nome)
{
    Item *item;

    if (nome[0] == '\0') {
        printf("Usar o que? Use: USAR <ITEM>\n");
        return;
    }
    item = buscarItem(jogo->itens, QTD_ITENS, nome, LOCAL_INVENTARIO);
    if (item != NULL) {
        usarItem(jogo, item);
        return;
    }
    if (buscarItem(jogo->itens, QTD_ITENS, nome, jogo->jogador.localAtual) != NULL)
        printf("Voce precisa pegar o item antes de usa-lo.\n");
    else if (buscarObjetoCenario(jogo->jogador.localAtual, nome) != NULL)
        interagirObjeto(jogo, nome);       /* "USAR PAINEL" funciona como INTERAGIR */
    else
        printf("Voce nao possui '%s'.\n", nome);
}

/* =====================================================================
 * 9. BUSCA RECURSIVA DE CAMINHOS (DFS COM BACKTRACKING)
 * ===================================================================== */

/* Verifica se existe caminho LIVRE de 'atual' até 'destino'.
 *
 * visitados[i] == 0  -> ambiente i não está no caminho que está sendo testado
 * visitados[i] == k  -> ambiente i é o k-ésimo passo do caminho atual
 *
 * - CASO-BASE: atual == destino -> caminho encontrado (retorna 1).
 * - PASSO RECURSIVO: para cada saída livre, chama buscarCaminho a partir do vizinho.
 * - CICLOS: vizinhos já marcados em visitados[] são ignorados.
 * - BACKTRACKING: se nenhuma saída leva ao destino, o ambiente é desmarcado
 *   (visitados[atual] = 0) e a função retorna 0, voltando para tentar a
 *   próxima saída do ambiente anterior. */
int buscarCaminho(const Ambiente ambientes[], int quantidade, int atual, int destino, int visitados[])
{
    int direcao, i, profundidade = 0;

    /* 1. Validação de origem e destino */
    if (ambientes == NULL || visitados == NULL)
        return 0;
    if (atual < 0 || atual >= quantidade || destino < 0 || destino >= quantidade)
        return 0;

    /* 3. Marca o ambiente com sua posição no caminho */
    for (i = 0; i < quantidade; i++)
        if (visitados[i] > profundidade)
            profundidade = visitados[i];
    visitados[atual] = profundidade + 1;

    /* 2. CASO-BASE: chegamos ao destino */
    if (atual == destino)
        return 1;

    /* 4. Explora as quatro saídas */
    for (direcao = NORTE; direcao <= OESTE; direcao++) {
        int vizinho = ambientes[atual].saidas[direcao];

        if (vizinho == SEM_SAIDA)
            continue;
        if (!passagemLivre(ambientes, atual, direcao))      /* 6. respeita bloqueios */
            continue;
        if (visitados[vizinho] != 0)                         /* 5. evita ciclos */
            continue;

        printf("  Explorando: %s\n", ambientes[vizinho].nome);
        if (buscarCaminho(ambientes, quantidade, vizinho, destino, visitados))   /* 7. recursão */
            return 1;                                                           /* 8. sucesso  */
        printf("  (sem saida a partir de %s, voltando)\n", ambientes[vizinho].nome);
    }

    /* BACKTRACKING: este ambiente não leva ao destino */
    visitados[atual] = 0;
    return 0;                                                                   /* 9. fracasso */
}

/* Encontra um ambiente pelo nome (exato ou parcial). */
int buscarAmbientePorNome(const Ambiente ambientes[], int quantidade, const char *nome)
{
    int i;

    if (ambientes == NULL || nome == NULL || nome[0] == '\0')
        return -1;
    for (i = 0; i < quantidade; i++)
        if (strcmp(ambientes[i].nome, nome) == 0)
            return i;
    for (i = 0; i < quantidade; i++)
        if (textoCorresponde(ambientes[i].nome, nome))
            return i;
    return -1;
}

/* Comando BUSCAR: chama a DFS e, se houver caminho, mostra a rota usando a
 * ordem guardada em visitados[]. */
void comandoBuscar(const Jogo *jogo, const char *nomeDestino)
{
    int visitados[QTD_AMBIENTES];
    int origem = jogo->jogador.localAtual;
    int destino, i, passo, anterior;

    if (nomeDestino[0] == '\0') {
        printf("Use: BUSCAR <DESTINO>   (ex.: BUSCAR DOCA DE ESCAPE)\n");
        return;
    }
    destino = buscarAmbientePorNome(jogo->ambientes, QTD_AMBIENTES, nomeDestino);
    if (destino < 0) {
        printf("Ambiente '%s' desconhecido. Ambientes da estacao:\n", nomeDestino);
        for (i = 0; i < QTD_AMBIENTES; i++)
            printf("  - %s\n", jogo->ambientes[i].nome);
        return;
    }
    if (destino == origem) {
        printf("Voce ja esta em %s.\n", jogo->ambientes[destino].nome);
        return;
    }

    for (i = 0; i < QTD_AMBIENTES; i++)
        visitados[i] = 0;

    printf("\n> BUSCAR %s\n\nIniciando busca de caminho a partir de %s...\n\n",
           jogo->ambientes[destino].nome, jogo->ambientes[origem].nome);

    if (!buscarCaminho(jogo->ambientes, QTD_AMBIENTES, origem, destino, visitados)) {
        printf("\nNENHUM CAMINHO LIVRE ENCONTRADO.\n");
        printf("Passagens bloqueadas impedem chegar a %s por enquanto.\n", jogo->ambientes[destino].nome);
        return;
    }

    printf("\nCAMINHO ENCONTRADO!\nRota: ");
    anterior = origem;
    for (passo = 2; passo <= QTD_AMBIENTES; passo++) {
        int proximo = -1, d;
        for (i = 0; i < QTD_AMBIENTES; i++)
            if (visitados[i] == passo)
                proximo = i;
        if (proximo < 0)
            break;
        for (d = NORTE; d <= OESTE; d++)
            if (jogo->ambientes[anterior].saidas[d] == proximo)
                printf("%s%s", passo > 2 ? " -> " : "", NOMES_DIRECOES[d]);
        anterior = proximo;
    }
    printf("\n");
}

/* =====================================================================
 * 10. INTERPRETADOR DE COMANDOS
 * ===================================================================== */

int direcaoPorNome(const char *texto)
{
    if (texto == NULL)
        return -1;
    if (strcmp(texto, "NORTE") == 0 || strcmp(texto, "N") == 0) return NORTE;
    if (strcmp(texto, "SUL") == 0   || strcmp(texto, "S") == 0) return SUL;
    if (strcmp(texto, "LESTE") == 0 || strcmp(texto, "L") == 0) return LESTE;
    if (strcmp(texto, "OESTE") == 0 || strcmp(texto, "O") == 0) return OESTE;
    return -1;
}

/* Tira artigos e preposições do começo do nome digitado, porque é natural
 * escrever "usar o cartao" ou "interagir com o painel":
 *   "O CARTAO MAGNETICO" -> "CARTAO MAGNETICO"
 *   "COM O PAINEL"       -> "PAINEL"
 * A última palavra nunca sai, senão "IR O" (oeste) ficaria vazio.
 * Palavras no meio do nome ficam: "DOCA DE ESCAPE" continua igual. */
static void removerPalavrasIniciais(char *argumento)
{
    static const char *IGNORADAS[] = {
        "O", "A", "OS", "AS", "UM", "UMA", "COM", "NO", "NA", "NOS", "NAS",
        "EM", "DE", "DO", "DA", "AO", "PARA", "PRA", "PRO"
    };
    const int quantidade = (int)(sizeof(IGNORADAS) / sizeof(IGNORADAS[0]));
    int removeu = 1;

    while (removeu) {
        char *espaco = strchr(argumento, ' ');
        int i;

        removeu = 0;
        if (espaco == NULL)
            return;                       /* sobrou uma palavra só */
        for (i = 0; i < quantidade; i++) {
            size_t tamanho = strlen(IGNORADAS[i]);
            if ((size_t)(espaco - argumento) == tamanho &&
                strncmp(argumento, IGNORADAS[i], tamanho) == 0) {
                memmove(argumento, espaco + 1, strlen(espaco + 1) + 1);
                removeu = 1;
                break;
            }
        }
    }
}

/* Par "jeito que o jogador escreve" -> "comando oficial". */
typedef struct {
    const char *variacao;
    const char *comando;
} Sinonimo;

/* Variações comuns em português (imperativo, 3ª pessoa, palavras
 * parecidas). Para aceitar uma palavra nova, basta acrescentar uma linha. */
static const Sinonimo SINONIMOS[] = {
    { "ENCERRAR", "SAIR" }, { "ENCERRA", "SAIR" }, { "FECHAR", "SAIR" },
    { "FECHA", "SAIR" }, { "FIM", "SAIR" }, { "SAI", "SAIR" },
    { "QUIT", "SAIR" }, { "EXIT", "SAIR" },
    { "PEGA", "PEGAR" }, { "PEGUE", "PEGAR" }, { "COLETAR", "PEGAR" },
    { "COLETA", "PEGAR" }, { "APANHAR", "PEGAR" },
    { "USA", "USAR" }, { "USE", "USAR" }, { "UTILIZAR", "USAR" }, { "UTILIZA", "USAR" },
    { "OLHA", "OLHAR" }, { "OLHE", "OLHAR" }, { "VER", "OLHAR" },
    { "EXAMINA", "EXAMINAR" }, { "EXAMINE", "EXAMINAR" }, { "INSPECIONAR", "EXAMINAR" },
    { "LER", "EXAMINAR" }, { "LE", "EXAMINAR" }, { "LEIA", "EXAMINAR" },
    { "VAI", "IR" }, { "VA", "IR" }, { "ANDAR", "IR" }, { "ANDA", "IR" },
    { "SEGUIR", "IR" }, { "SEGUE", "IR" },
    { "LARGAR", "DESCARTAR" }, { "LARGA", "DESCARTAR" }, { "SOLTAR", "DESCARTAR" },
    { "SOLTA", "DESCARTAR" }, { "DESCARTA", "DESCARTAR" },
    { "MEXER", "INTERAGIR" }, { "MEXE", "INTERAGIR" }, { "ACIONAR", "INTERAGIR" },
    { "ACIONA", "INTERAGIR" }, { "INTERAGE", "INTERAGIR" }, { "APERTAR", "INTERAGIR" },
    { "ATIVAR", "INTERAGIR" },
    { "INV", "INVENTARIO" }, { "ITENS", "INVENTARIO" }, { "MOCHILA", "INVENTARIO" },
    { "BUSCA", "BUSCAR" }, { "SALVA", "SALVAR" }, { "GRAVAR", "SALVAR" },
    { "CARREGA", "CARREGAR" }, { "HELP", "AJUDA" }, { "COMANDOS", "AJUDA" },
    { "OBJETIVO", "RESUMO" }, { "ONDE", "RESUMO" }
};

/* Troca o verbo digitado pelo comando oficial, se ele for um sinônimo. */
static void traduzirSinonimo(char *verbo, size_t tamanho)
{
    const int quantidade = (int)(sizeof(SINONIMOS) / sizeof(SINONIMOS[0]));
    int i;

    for (i = 0; i < quantidade; i++) {
        if (strcmp(verbo, (SINONIMOS + i)->variacao) == 0) {
            copiarTexto(verbo, (SINONIMOS + i)->comando, tamanho);
            return;
        }
    }
}

/* Separa a linha digitada em verbo e argumento.
 * "  pegar   a chave inglesa " -> verbo "PEGAR", argumento "CHAVE INGLESA".
 * O verbo passa pela tabela de sinônimos ("encerra" -> "SAIR"), e
 * "olhar <algo>" vira "examinar <algo>".
 * Retorna 0 se a linha estiver vazia. */
int interpretarComando(char comando[], Comando *resultado)
{
    char *espaco;

    if (comando == NULL || resultado == NULL)
        return 0;
    resultado->verbo[0] = '\0';
    resultado->argumento[0] = '\0';

    removerQuebraLinha(comando);
    normalizarTexto(comando);
    if (strlen(comando) == 0)
        return 0;

    espaco = strchr(comando, ' ');
    if (espaco != NULL) {
        *espaco = '\0';                                   /* separa as duas partes */
        copiarTexto(resultado->argumento, espaco + 1, sizeof(resultado->argumento));
        removerPalavrasIniciais(resultado->argumento);
    }
    copiarTexto(resultado->verbo, comando, sizeof(resultado->verbo));
    traduzirSinonimo(resultado->verbo, sizeof(resultado->verbo));

    /* "olhar o painel" / "ver o manual": olhar algo específico é examinar. */
    if (strcmp(resultado->verbo, "OLHAR") == 0 && resultado->argumento[0] != '\0')
        copiarTexto(resultado->verbo, "EXAMINAR", sizeof(resultado->verbo));
    return 1;
}

static void comandoSair(Jogo *jogo)
{
    char resposta[16];

    printf("Deseja salvar antes de sair? (S/N): ");
    if (lerLinha(resposta, sizeof(resposta))) {
        normalizarTexto(resposta);
        if (resposta[0] == 'S')
            salvarPartida(jogo);
    }
    printf("\nVoce se encolhe no canto e espera. O sinal continua...\n");
    jogo->estado.jogoFinalizado = 1;
}

/* Chama a função certa para o verbo digitado. */
static void despacharComando(Jogo *jogo, const Comando *comando)
{
    const char *verbo;
    const char *argumento;
    int direcao;

    verbo = comando->verbo;
    argumento = comando->argumento;

    direcao = direcaoPorNome(verbo);
    if (direcao < 0 && strcmp(verbo, "IR") == 0) {
        direcao = direcaoPorNome(argumento);
        if (direcao < 0) {
            printf("Direcao invalida. Use NORTE, SUL, LESTE ou OESTE.\n");
            return;
        }
    }

    if (direcao >= 0)                               comandoMover(jogo, direcao);
    else if (strcmp(verbo, "OLHAR") == 0) {
        limparTela();
        mostrarTela(jogo);
    }
    else if (strcmp(verbo, "RESUMO") == 0)          mostrarResumo(jogo);
    else if (strcmp(verbo, "EXAMINAR") == 0)        examinarAlvo(jogo, argumento);
    else if (strcmp(verbo, "PEGAR") == 0)           comandoPegar(jogo, argumento);
    else if (strcmp(verbo, "USAR") == 0)            comandoUsar(jogo, argumento);
    else if (strcmp(verbo, "DESCARTAR") == 0)       comandoDescartar(jogo, argumento);
    else if (strcmp(verbo, "INTERAGIR") == 0)       interagirObjeto(jogo, argumento);
    else if (strcmp(verbo, "INVENTARIO") == 0 || strcmp(verbo, "I") == 0)
                                                    mostrarInventario(&jogo->jogador, jogo->itens);
    else if (strcmp(verbo, "DIARIO") == 0)          mostrarDiario(jogo);
    else if (strcmp(verbo, "MAPA") == 0)
        mostrarMapa(jogo->ambientes, QTD_AMBIENTES, jogo->jogador.localAtual);
    else if (strcmp(verbo, "BUSCAR") == 0)          comandoBuscar(jogo, argumento);
    else if (strcmp(verbo, "STATUS") == 0)          mostrarStatus(jogo);
    else if (strcmp(verbo, "SALVAR") == 0)          salvarPartida(jogo);
    else if (strcmp(verbo, "CARREGAR") == 0) {
        if (carregarPartida(jogo)) {
            limparTela();
            mostrarTela(jogo);
            printf("\n[ Partida carregada de %s ]\n", ARQ_SAVE);
        }
    }
    else if (strcmp(verbo, "AJUDA") == 0)           mostrarAjuda();
    else if (strcmp(verbo, "SAIR") == 0)            comandoSair(jogo);
    else if (isdigit((unsigned char)verbo[0])) {
        printf("Numeros so valem como resposta de enigma. Primeiro digite INTERAGIR <OBJETO>\n");
        printf("(por exemplo INTERAGIR PAINEL) e responda quando o jogo pedir.\n");
    }
    else {
        printf("Comando desconhecido: '%s'.\n", verbo);
        printf("Comandos principais: NORTE, SUL, LESTE, OESTE, OLHAR, EXAMINAR, PEGAR, USAR,\n");
        printf("INTERAGIR, INVENTARIO, DIARIO, MAPA, RESUMO, SALVAR e SAIR. (AJUDA mostra todos)\n");
    }
}

/* Executa o comando e, se ele fez a aventura avançar, avisa o novo objetivo.
 * (Depois de CARREGAR o objetivo já aparece no resumo da tela.) */
void executarComando(Jogo *jogo, const Comando *comando)
{
    char objetivoAntes[200];

    if (jogo == NULL || comando == NULL)
        return;
    copiarTexto(objetivoAntes, objetivoAtual(jogo), sizeof(objetivoAntes));

    despacharComando(jogo, comando);

    if (!jogo->estado.jogoFinalizado && strcmp(comando->verbo, "CARREGAR") != 0 &&
        strcmp(objetivoAntes, objetivoAtual(jogo)) != 0)
        printf("\n>> NOVO OBJETIVO: %s\n", objetivoAtual(jogo));
}

/* =====================================================================
 * 11. SALVAR E CARREGAR (ARQUIVO TXT)
 * ===================================================================== */

/* Escreve uma lista "1,3,5" (ou "-" se vazia). */
static void gravarLista(FILE *arquivo, const char *chave, const int *valores, int quantidade)
{
    int i;

    fprintf(arquivo, "%s=", chave);
    if (quantidade == 0)
        fputs("-", arquivo);
    for (i = 0; i < quantidade; i++)
        fprintf(arquivo, "%s%d", i > 0 ? "," : "", *(valores + i));
    fputs("\n", arquivo);
}

/* Grava a partida em texto (chave=valor). Primeiro escreve em partida.tmp
 * e só no final troca pelo partida.txt. Se der erro no meio do caminho,
 * o save antigo continua lá. */
int salvarPartida(const Jogo *jogo)
{
    FILE *arquivo;
    const Jogador *j;
    const EstadoJogo *e;
    int i, erro;

    if (jogo == NULL)
        return 0;
    j = &jogo->jogador;
    e = &jogo->estado;

    arquivo = fopen(ARQ_SAVE_TEMP, "w");
    if (arquivo == NULL) {
        printf("[ERRO] Nao foi possivel criar %s. A pasta 'saves' existe?\n", ARQ_SAVE_TEMP);
        return 0;
    }

    fprintf(arquivo, "VERSAO=%d\n", VERSAO_SAVE);
    fputs("# SINAL ZERO - estado da partida. Nao edite manualmente.\n", arquivo);
    fprintf(arquivo, "JOGADOR=%s\n", j->nome);
    fprintf(arquivo, "VIDA=%d\n", j->vida);
    fprintf(arquivo, "LOCAL=%d\n", j->localAtual);
    fprintf(arquivo, "QTD_ITENS=%d\n", j->quantidadeItens);
    gravarLista(arquivo, "INVENTARIO", j->inventario, j->quantidadeItens);
    fprintf(arquivo, "ENERGIA=%d\n", e->energiaRestaurada);
    fprintf(arquivo, "SINAL_DESATIVADO=%d\n", e->sinalDesativado);
    fprintf(arquivo, "ORIGEM_DESCOBERTA=%d\n", e->origemDescoberta);
    fprintf(arquivo, "PAINEL_ABERTO=%d\n", e->painelAberto);
    fprintf(arquivo, "FUSIVEL_INSTALADO=%d\n", e->fusivelInstalado);
    fprintf(arquivo, "MODULO_INSTALADO=%d\n", e->moduloInstalado);
    fprintf(arquivo, "DOCA_LIBERADA=%d\n", e->docaLiberada);

    fputs("# ITEM=id;local;coletado;utilizado  (local -1 = inventario, -2 = consumido)\n", arquivo);
    for (i = 0; i < QTD_ITENS; i++) {
        const Item *item = jogo->itens + i;
        fprintf(arquivo, "ITEM=%d;%d;%d;%d\n", item->id, item->localAtual, item->coletado, item->utilizado);
    }
    fputs("# AMBIENTE=id;visitado;bloqueado;travaN;travaS;travaL;travaO\n", arquivo);
    for (i = 0; i < QTD_AMBIENTES; i++) {
        const Ambiente *a = jogo->ambientes + i;
        fprintf(arquivo, "AMBIENTE=%d;%d;%d;%d;%d;%d;%d\n", a->id, a->visitado, a->bloqueado,
                a->saidasBloqueadas[NORTE], a->saidasBloqueadas[SUL],
                a->saidasBloqueadas[LESTE], a->saidasBloqueadas[OESTE]);
    }
    fputs("# ENIGMA=id;resolvido;tentativas\n", arquivo);
    for (i = 0; i < QTD_ENIGMAS; i++)
        fprintf(arquivo, "ENIGMA=%d;%d;%d\n", jogo->enigmas[i].id,
                jogo->enigmas[i].resolvido, jogo->enigmas[i].tentativas);
    gravarLista(arquivo, "DIARIO", jogo->diario.ids, jogo->diario.quantidade);
    fputs("FIM\n", arquivo);

    /* Confere se a gravação foi concluída. */
    erro = ferror(arquivo);
    if (fclose(arquivo) != 0)
        erro = 1;
    if (erro) {
        remove(ARQ_SAVE_TEMP);
        printf("[ERRO] Falha ao gravar a partida. O save anterior foi mantido.\n");
        return 0;
    }
    remove(ARQ_SAVE);                       /* no Windows, rename falha se o destino existe */
    if (rename(ARQ_SAVE_TEMP, ARQ_SAVE) != 0) {
        printf("[ERRO] Nao foi possivel finalizar %s.\n", ARQ_SAVE);
        return 0;
    }
    printf("\n[ PARTIDA SALVA EM %s ]\n", ARQ_SAVE);
    return 1;
}

/* Lê uma lista "1,3,5" (ou "-"). Para cada número chama a ação desejada:
 * tipo 0 = inventário, tipo 1 = diário. Retorna 0 se algo for inválido. */
static int lerLista(Jogo *temp, char *valor, int tipo)
{
    char *parte;
    int numero;

    if (strcmp(valor, "-") == 0)
        return 1;
    for (parte = strtok(valor, ","); parte != NULL; parte = strtok(NULL, ",")) {
        if (!lerInteiro(parte, &numero))
            return 0;
        if (tipo == 0) {
            Jogador *j = &temp->jogador;
            if (numero < 0 || numero >= QTD_ITENS || possuiItem(j, numero) ||
                j->quantidadeItens >= MAX_INVENTARIO)
                return 0;
            j->inventario[j->quantidadeItens++] = numero;
        } else {
            if (adicionarAoDiario(&temp->diario, numero) != 1)   /* inválido, repetido ou sem memória */
                return 0;
        }
    }
    return 1;
}

/* Lê o save numa partida temporária e confere tudo antes de usar.
 * Se o arquivo não existe, está vazio ou tem algum valor errado, a
 * partida que está rodando fica do jeito que estava. */
int carregarPartida(Jogo *jogo)
{
    FILE *arquivo;
    Jogo temp;
    char linha[512];
    const char *motivo = NULL;
    int versao = 0, lidos, qtdDeclarada = -1, fimEncontrado = 0, temNome = 0;
    int itensLidos[QTD_ITENS] = {0};
    int ambientesLidos[QTD_AMBIENTES] = {0};
    int enigmasLidos[QTD_ENIGMAS] = {0};
    int i;

    if (jogo == NULL)
        return 0;
    arquivo = fopen(ARQ_SAVE, "r");
    if (arquivo == NULL) {
        printf("\nNenhuma partida salva encontrada (%s).\n", ARQ_SAVE);
        return 0;
    }

    /* fscanf lê a primeira linha já no formato esperado. */
    lidos = fscanf(arquivo, "VERSAO=%d", &versao);
    if (lidos == EOF) {
        printf("\nO arquivo de salvamento esta vazio.\n");
        fclose(arquivo);
        return 0;
    }
    if (lidos != 1 || versao != VERSAO_SAVE) {
        printf("\nArquivo de salvamento invalido ou de outra versao.\n");
        fclose(arquivo);
        return 0;
    }

    inicializarMundo(&temp);              /* parte do estado inicial e aplica o que foi salvo */
    temp.jogador.vida = -1;
    temp.jogador.localAtual = -1;

    while (motivo == NULL && fgets(linha, sizeof(linha), arquivo) != NULL) {
        char *igual, *chave, *valor;
        int a, b, c, d, e2, f, g;
        char extra;

        removerQuebraLinha(linha);
        if (linha[0] == '\0' || linha[0] == '#')
            continue;
        igual = strchr(linha, '=');
        if (igual == NULL) {
            if (strcmp(linha, "FIM") == 0) { fimEncontrado = 1; continue; }
            motivo = "linha sem '='";
            break;
        }
        *igual = '\0';
        chave = linha;
        valor = igual + 1;

        if (strcmp(chave, "JOGADOR") == 0) {
            if (strlen(valor) == 0 || strlen(valor) >= sizeof(temp.jogador.nome))
                motivo = "nome do jogador invalido";
            else {
                copiarTexto(temp.jogador.nome, valor, sizeof(temp.jogador.nome));
                temNome = 1;
            }
        } else if (strcmp(chave, "VIDA") == 0) {
            if (!lerInteiro(valor, &temp.jogador.vida)) motivo = "vida invalida";
        } else if (strcmp(chave, "LOCAL") == 0) {
            if (!lerInteiro(valor, &temp.jogador.localAtual)) motivo = "local invalido";
        } else if (strcmp(chave, "QTD_ITENS") == 0) {
            if (!lerInteiro(valor, &qtdDeclarada)) motivo = "quantidade de itens invalida";
        } else if (strcmp(chave, "INVENTARIO") == 0) {
            if (!lerLista(&temp, valor, 0)) motivo = "inventario invalido";
        } else if (strcmp(chave, "DIARIO") == 0) {
            if (!lerLista(&temp, valor, 1)) motivo = "diario invalido";
        } else if (strcmp(chave, "ENERGIA") == 0) {
            if (!lerFlag(valor, &temp.estado.energiaRestaurada)) motivo = "ENERGIA invalida";
        } else if (strcmp(chave, "SINAL_DESATIVADO") == 0) {
            if (!lerFlag(valor, &temp.estado.sinalDesativado)) motivo = "SINAL_DESATIVADO invalido";
        } else if (strcmp(chave, "ORIGEM_DESCOBERTA") == 0) {
            if (!lerFlag(valor, &temp.estado.origemDescoberta)) motivo = "ORIGEM_DESCOBERTA invalida";
        } else if (strcmp(chave, "PAINEL_ABERTO") == 0) {
            if (!lerFlag(valor, &temp.estado.painelAberto)) motivo = "PAINEL_ABERTO invalido";
        } else if (strcmp(chave, "FUSIVEL_INSTALADO") == 0) {
            if (!lerFlag(valor, &temp.estado.fusivelInstalado)) motivo = "FUSIVEL_INSTALADO invalido";
        } else if (strcmp(chave, "MODULO_INSTALADO") == 0) {
            if (!lerFlag(valor, &temp.estado.moduloInstalado)) motivo = "MODULO_INSTALADO invalido";
        } else if (strcmp(chave, "DOCA_LIBERADA") == 0) {
            if (!lerFlag(valor, &temp.estado.docaLiberada)) motivo = "DOCA_LIBERADA invalida";
        } else if (strcmp(chave, "ITEM") == 0) {
            /* %c extra detecta lixo no fim da linha */
            if (sscanf(valor, "%d;%d;%d;%d %c", &a, &b, &c, &d, &extra) != 4 ||
                a < 0 || a >= QTD_ITENS || itensLidos[a] ||
                b < LOCAL_CONSUMIDO || b >= QTD_AMBIENTES ||
                (c != 0 && c != 1) || (d != 0 && d != 1)) {
                motivo = "linha ITEM invalida";
            } else {
                temp.itens[a].localAtual = b;
                temp.itens[a].coletado = c;
                temp.itens[a].utilizado = d;
                itensLidos[a] = 1;
            }
        } else if (strcmp(chave, "AMBIENTE") == 0) {
            if (sscanf(valor, "%d;%d;%d;%d;%d;%d;%d %c", &a, &b, &c, &d, &e2, &f, &g, &extra) != 7 ||
                a < 0 || a >= QTD_AMBIENTES || ambientesLidos[a] ||
                (b != 0 && b != 1) || (c != 0 && c != 1) ||
                d < TRAVA_NENHUMA || d > TRAVA_SINAL || e2 < TRAVA_NENHUMA || e2 > TRAVA_SINAL ||
                f < TRAVA_NENHUMA || f > TRAVA_SINAL || g < TRAVA_NENHUMA || g > TRAVA_SINAL) {
                motivo = "linha AMBIENTE invalida";
            } else {
                Ambiente *amb = temp.ambientes + a;
                amb->visitado = b;
                amb->bloqueado = c;
                amb->saidasBloqueadas[NORTE] = d;
                amb->saidasBloqueadas[SUL] = e2;
                amb->saidasBloqueadas[LESTE] = f;
                amb->saidasBloqueadas[OESTE] = g;
                ambientesLidos[a] = 1;
            }
        } else if (strcmp(chave, "ENIGMA") == 0) {
            if (sscanf(valor, "%d;%d;%d %c", &a, &b, &c, &extra) != 3 ||
                a < 0 || a >= QTD_ENIGMAS || enigmasLidos[a] ||
                (b != 0 && b != 1) || c < 0 || c > 10000) {
                motivo = "linha ENIGMA invalida";
            } else {
                temp.enigmas[a].resolvido = b;
                temp.enigmas[a].tentativas = c;
                enigmasLidos[a] = 1;
            }
        } else {
            motivo = "chave desconhecida";
        }
    }
    fclose(arquivo);

    /* Validações finais de consistência */
    if (motivo == NULL && !fimEncontrado)
        motivo = "arquivo incompleto (falta FIM)";
    if (motivo == NULL && !temNome)
        motivo = "nome do jogador ausente";
    if (motivo == NULL && (temp.jogador.vida < 1 || temp.jogador.vida > VIDA_MAXIMA))
        motivo = "vida fora do limite (1 a 100)";
    if (motivo == NULL && (temp.jogador.localAtual < 0 || temp.jogador.localAtual >= QTD_AMBIENTES))
        motivo = "local inexistente";
    if (motivo == NULL && qtdDeclarada != temp.jogador.quantidadeItens)
        motivo = "QTD_ITENS nao confere com o inventario";
    for (i = 0; motivo == NULL && i < QTD_ITENS; i++) {
        if (!itensLidos[i])
            motivo = "faltam itens";
        else if ((temp.itens[i].localAtual == LOCAL_INVENTARIO) != possuiItem(&temp.jogador, i))
            motivo = "inventario e itens nao conferem";
    }
    for (i = 0; motivo == NULL && i < QTD_AMBIENTES; i++)
        if (!ambientesLidos[i])
            motivo = "faltam ambientes";
    for (i = 0; motivo == NULL && i < QTD_ENIGMAS; i++)
        if (!enigmasLidos[i])
            motivo = "faltam enigmas";
    if (motivo == NULL && temp.ambientes[temp.jogador.localAtual].bloqueado)
        motivo = "jogador dentro de ambiente bloqueado";
    if (motivo == NULL &&
        (temp.estado.energiaRestaurada != temp.enigmas[ENIGMA_ENERGIA].resolvido ||
         temp.estado.sinalDesativado != temp.enigmas[ENIGMA_NUCLEO].resolvido ||
         temp.estado.docaLiberada != temp.estado.sinalDesativado))
        motivo = "estado dos enigmas inconsistente";

    if (motivo != NULL) {
        printf("\n[ERRO] Arquivo de salvamento invalido: %s.\n", motivo);
        printf("A partida atual NAO foi alterada.\n");
        liberarDiario(&temp.diario);      /* libera o que foi alocado durante a leitura */
        return 0;
    }

    /* Tudo válido: troca a partida atual pela carregada. */
    liberarDiario(&jogo->diario);
    *jogo = temp;                          /* cópia da struct inteira (o diário passa junto) */
    printf("\n[ PARTIDA CARREGADA: %s | vida %d | %s ]\n",
           jogo->jogador.nome, jogo->jogador.vida, jogo->ambientes[jogo->jogador.localAtual].nome);
    return 1;
}

/* =====================================================================
 * 12. VITÓRIA, DERROTA E MENU INICIAL
 * ===================================================================== */

int verificarVitoria(const EstadoJogo *estado)
{
    if (estado == NULL)
        return 0;
    return estado->origemDescoberta && estado->sinalDesativado &&
           estado->docaLiberada && estado->fugaRealizada;
}

int verificarDerrota(const Jogador *jogador)
{
    return jogador != NULL && jogador->vida <= 0;
}

static void mostrarVitoria(const Jogo *jogo)
{
    printf("\nVoce entra na capsula e puxa a alavanca de lancamento.\n");
    printf("A NEREIDA-7 fica para tras, escura e silenciosa.\n\n");
    printf("              |\n");
    printf("             /^\\\n");
    printf("            | C |   ~ subindo ~\n");
    printf("            |___|\n");
    printf("        ~~~~~~~~~~~~~~~~~~~~~~~~\n\n");
    printf("Horas depois, a luz do sol atravessa a escotilha.\n");
    printf("No painel da capsula, um aviso pisca: 1 MENSAGEM RECEBIDA. Remetente: %s.\n", jogo->jogador.nome);
    printf("\n==================================================\n");
    printf("        VITORIA - VOCE ESCAPOU DA NEREIDA-7\n");
    printf("==================================================\n");
    printf("Vida final: %d/%d | Registros encontrados: %d de %d\n",
           jogo->jogador.vida, VIDA_MAXIMA, jogo->diario.quantidade, QTD_REGISTROS);
}

static void mostrarDerrota(const Jogo *jogo)
{
    printf("\nSua visao escurece. O chiado dos alto-falantes forma uma unica palavra.\n");
    printf("Em algum lugar da estacao, um terminal acende:\n");
    printf("\"SE VOCE ESTA LENDO ISSO, NAO ATIVE O SONAR.\" -- %s\n", jogo->jogador.nome);
    printf("\n==================================================\n");
    printf("          DERROTA - SINAL PERDIDO\n");
    printf("==================================================\n");
}

/* Mostra o final (se houver) e indica se a partida acabou. */
int verificarFimDeJogo(Jogo *jogo)
{
    if (jogo == NULL)
        return 1;
    if (verificarDerrota(&jogo->jogador)) {
        mostrarDerrota(jogo);
        jogo->estado.jogoFinalizado = 1;
        return 1;
    }
    if (verificarVitoria(&jogo->estado)) {
        mostrarVitoria(jogo);
        jogo->estado.jogoFinalizado = 1;
        return 1;
    }
    return jogo->estado.jogoFinalizado;
}

static void mostrarIntroducao(const Jogador *jogador)
{
    printf("\n%s, tecnico de manutencao, voce foi enviado a NEREIDA-7 apos 72 horas\n", jogador->nome);
    printf("sem contato com a equipe cientifica. A descida terminou com um estrondo.\n");
    printf("A estacao esta parcialmente inundada, sem energia e com portas travadas.\n");
    printf("O radio chia uma mensagem que parece ter sido gravada... por voce.\n");
    printf("\nObjetivo: descobrir a origem do sinal, desativa-lo e escapar.\n");
    printf("Digite AJUDA a qualquer momento para ver os comandos.\n");
}

/* Menu antes da partida: nova aventura ou carregar. Retorna 0 para sair. */
int menuInicial(Jogo *jogo)
{
    char opcao[MAX_COMANDO];
    char nome[50];

    while (1) {
        printf("[1] Nova partida\n[2] Carregar partida salva\n[0] Sair\nEscolha: ");
        if (!lerLinha(opcao, sizeof(opcao)))
            return 0;
        normalizarTexto(opcao);

        if (strcmp(opcao, "1") == 0) {
            printf("\nIdentifique-se, tecnico (nome): ");
            if (!lerLinha(nome, sizeof(nome)))
                return 0;
            aparar(nome);
            iniciarJogo(&jogo->jogador);
            if (strlen(nome) > 0)
                copiarTexto(jogo->jogador.nome, nome, sizeof(jogo->jogador.nome));
            mostrarIntroducao(&jogo->jogador);
            return 1;
        }
        if (strcmp(opcao, "2") == 0) {
            if (carregarPartida(jogo))
                return 1;
            printf("\n");
        } else if (strcmp(opcao, "0") == 0) {
            return 0;
        } else {
            printf("Opcao invalida. Digite 1, 2 ou 0.\n\n");
        }
    }
}
