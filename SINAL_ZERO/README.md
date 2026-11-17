# SINAL ZERO

Jogo de aventura em texto, com mistério e um pouco de terror psicológico, escrito em C99 e compilado com GCC.
Roda no console e só usa a biblioteca padrão do C.

---

## 1. O que é o jogo

Você anda por uma estação submarina digitando comandos como `NORTE`, `PEGAR CHAVE` e `USAR FUSIVEL`.
Lê pistas, junta itens, resolve enigmas e destrava portas até entender de onde vem um sinal estranho.

O projeto tem:

- 9 ambientes ligados entre si, cada um com uma arte ASCII;
- 6 itens, todos com algum efeito no jogo;
- inventário com espaço para 5 itens;
- 3 enigmas e 1 desafio final;
- vida, dano, vitória e derrota;
- save e load em arquivo `.txt`;
- uma busca recursiva de caminhos (DFS com backtracking), usada pelo comando `BUSCAR`;
- um diário de pistas guardado em memória dinâmica.

## 2. História

A NEREIDA-7 é uma estação científica a 2.140 metros de profundidade. Faz 72 horas que ninguém responde de lá.
Você é o técnico de manutenção mandado para descobrir o que houve com a equipe. Chega e encontra tudo meio alagado, sem energia e com as portas travadas.

No dormitório, um terminal mostra um aviso assinado com o seu nome:

> "SE VOCÊ ESTÁ LENDO ISSO, NÃO ATIVE O SONAR."

As pistas vão aparecendo aos poucos. O diário da Dra. Ilse Varga diz que o sinal se repete a cada 7 minutos e vem de dentro da estação. O sonar pega uma mensagem cifrada, `HFR`. O módulo de memória traz a regra da cifra, e a mensagem vira `ECO`.
Na Câmara de Contenção, a luz de uma amostra bioluminescente mostra o resto: o sinal é a sua própria voz, gravada no futuro e mandada de volta pelo sonar.

Para vencer, você precisa descobrir a origem do sinal, desligar a transmissão, liberar a Doca de Escape e fugir na cápsula.
Você perde se a vida chegar a 0. Isso acontece errando enigmas (choques) ou passando muitas vezes pelo vapor da Passagem de Manutenção.

## 3. Regras

### Mapa

```
              [LABO]----------[SONR]
                |               |
  [DORM]------[CORR]----[ARQV]  |
                |               |
              [MAQS]----------[MANT]------[CONT]
                                            |
                                          [DOCA]
```

| ID | Ambiente | Saídas | Bloqueio |
|---|---|---|---|
| 0 | Dormitório | L: Corredor | nenhum |
| 1 | Corredor Central | N: Laboratório, S: Máquinas, L: Arquivo, O: Dormitório | porta norte precisa do cartão |
| 2 | Laboratório Abissal | S: Corredor, L: Sonar | porta leste precisa de energia |
| 3 | Arquivo Técnico | O: Corredor | nenhum |
| 4 | Sala de Máquinas | N: Corredor, L: Manutenção | nenhum |
| 5 | Central de Sonar | O: Laboratório, S: Manutenção | nenhum |
| 6 | Passagem de Manutenção | N: Sonar, O: Máquinas, L: Contenção | travada até calibrar o sonar; tira 10 de vida a cada entrada |
| 7 | Câmara de Contenção | O: Manutenção, S: Doca | porta sul só abre com o sinal desligado |
| 8 | Doca de Escape | N: Contenção | nenhum |

Mudei duas conexões da sugestão original: tirei a ligação entre o Arquivo e o Sonar e coloquei o Sonar ao norte da Manutenção. Do jeito sugerido, o desenho do mapa não fechava (o Sonar ficaria a leste do Laboratório e ao sul do Arquivo ao mesmo tempo).

### Itens

| Item | Onde está | Para que serve |
|---|---|---|
| Cartão magnético | Dormitório | Usado no Corredor, abre a porta do Laboratório (enigma 1) |
| Cilindro de oxigênio | Dormitório | Recupera 40 de vida, sem passar de 100. Some depois de usado |
| Fusível industrial | Laboratório | Instalado no painel da Sala de Máquinas, com a tampa aberta |
| Amostra bioluminescente | Laboratório | Usada na Contenção, ilumina o núcleo e mostra a origem do sinal |
| Módulo de memória | Arquivo Técnico | Encaixado no Sonar, traz a chave da cifra |
| Chave inglesa isolada | Sala de Máquinas | Abre a tampa do painel |

São 6 itens e só 5 espaços, então em algum momento é preciso largar alguma coisa com `DESCARTAR`. O cartão, por exemplo, não serve para mais nada depois que o laboratório abre.

### Desafios

| # | Desafio | Como resolver | Se errar | O que libera |
|---|---|---|---|---|
| 1 | Porta do laboratório | `USAR CARTAO` no Corredor | nada | Corredor ↔ Laboratório |
| 2 | Energia | `USAR CHAVE`, `USAR FUSIVEL`, depois `INTERAGIR PAINEL`. O painel tem `1-TURBINA 2-REDE 3-REFRIGERACAO 4-BOMBA` e o manual do Arquivo diz a ordem: refrigeração, bomba, turbina, rede. Resposta: `3 4 1 2` | −15 de vida | energia ligada e porta Laboratório ↔ Sonar |
| 3 | Sonar | `USAR MODULO` (a cifra é voltar 3 letras), depois `INTERAGIR CONSOLE`: `HFR` vira `ECO` | −10 de vida | Passagem de Manutenção |
| 4 | Núcleo (final) | `USAR AMOSTRA` para enxergar, depois `INTERAGIR NUCLEO` com a palavra do sonar e o intervalo do diário: `ECO-7` | −25 de vida | sinal desligado e porta da Doca |

Para responder um enigma, primeiro digite `INTERAGIR` com o objeto (`INTERAGIR PAINEL`, por exemplo). O jogo mostra a pergunta e uma pista de onde está a resposta, e aí você digita. Cada `INTERAGIR` vale uma tentativa: se errar, o jogo avisa para digitar `INTERAGIR` de novo. Números digitados direto em "O que deseja fazer?" não são lidos como resposta, e o jogo explica isso.

Com tudo isso feito, `INTERAGIR CAPSULA` na Doca termina o jogo com vitória.
Depois de ganhar ou perder, o laço principal acaba e o jogo não aceita mais comando nenhum.

### Comandos

```
NORTE  SUL  LESTE  OESTE   (ou N, S, L, O, ou IR <DIRECAO>)
OLHAR                  EXAMINAR <OBJETO>      PEGAR <ITEM>
USAR <ITEM>            DESCARTAR <ITEM>       INTERAGIR <OBJETO>
INVENTARIO             DIARIO                 MAPA
BUSCAR <DESTINO>       STATUS                 RESUMO
SALVAR                 CARREGAR               AJUDA
SAIR
```

### Tela e resumo

Para a tela não virar uma parede de texto, o jogo limpa o console só quando a cena muda: ao entrar numa sala, no `OLHAR` e ao carregar uma partida. Depois de `EXAMINAR`, `DIARIO`, de um erro ou de um enigma, o texto continua na tela, porque o jogador pode precisar ler aquilo de novo (a ordem das alavancas, por exemplo).

Toda tela de sala termina com um resumo:

```
------------------------------- RESUMO -------------------------------
Voce esta em: SALA DE MAQUINAS | Vida: 85/100
Levando: CILINDRO DE OXIGENIO, AMOSTRA BIOLUMINESCENTE, MODULO DE MEMORIA
Ja fez: leu o aviso no terminal do dormitorio, abriu a porta do laboratorio, religou o gerador.
Pistas no diario: 4 de 6 (DIARIO para reler)
OBJETIVO: Levar o modulo de memoria ate a Central de Sonar e usa-lo no console.
----------------------------------------------------------------------
```

O objetivo é calculado pelo progresso e diz o próximo passo sem entregar a resposta dos enigmas. Quando um comando faz a história andar, aparece `>> NOVO OBJETIVO: ...`. O comando `RESUMO` mostra esse bloco a qualquer momento, e o prompt sempre lembra a sala atual: `[SALA DE MAQUINAS] O que deseja fazer?`.

Tanto faz maiúscula ou minúscula, e espaço sobrando é ignorado. Dá para escrever só parte do nome: `PEGAR CHAVE` acha a chave inglesa isolada.
Artigos e preposições no começo do nome também são ignorados, então `usar o cartao magnetico`, `pegar a chave` e `interagir com o painel` funcionam. Só as palavras do começo saem: `BUSCAR DOCA DE ESCAPE` continua igual.

Os comandos também aceitam variações do jeito que se fala. Uma tabela de sinônimos (`SINONIMOS[]` em `jogo.c`) traduz a palavra digitada para o comando oficial antes de executar:

| Você pode digitar | Vira |
|---|---|
| `encerrar`, `encerra`, `fechar`, `fim`, `sai` | `SAIR` |
| `pega`, `pegue`, `coletar` | `PEGAR` |
| `usa`, `use`, `utilizar` | `USAR` |
| `olha`, `ver` | `OLHAR` (com um objeto depois, vira `EXAMINAR`: `olha o painel`) |
| `examina`, `ler`, `leia` | `EXAMINAR` |
| `vai`, `anda`, `seguir` | `IR` (`vai pro norte`) |
| `larga`, `solta` | `DESCARTAR` |
| `mexer`, `acionar`, `apertar` | `INTERAGIR` |
| `inv`, `itens`, `mochila` | `INVENTARIO` |
| `objetivo`, `onde` | `RESUMO` |

Para aceitar uma palavra nova, basta acrescentar uma linha na tabela. Se mesmo assim o comando não existir, o jogo mostra a lista dos comandos principais.
Digite sem acento. O jogo também mostra tudo sem acento, porque o terminal do Windows costuma bagunçar os caracteres acentuados.

### Solução completa

Útil para a apresentação:

```
PEGAR CARTAO, PEGAR OXIGENIO, EXAMINAR TERMINAL, LESTE, USAR CARTAO, NORTE,
EXAMINAR DIARIO, PEGAR FUSIVEL, PEGAR AMOSTRA, SUL, LESTE, EXAMINAR MANUAL,
EXAMINAR RELATORIO, PEGAR MODULO, OESTE, SUL, DESCARTAR CARTAO, PEGAR CHAVE,
USAR CHAVE, USAR FUSIVEL, INTERAGIR PAINEL -> 3 4 1 2, NORTE, NORTE, LESTE,
EXAMINAR CONSOLE, USAR MODULO, INTERAGIR CONSOLE -> ECO, SUL, LESTE,
USAR AMOSTRA, INTERAGIR NUCLEO -> ECO-7, SUL, INTERAGIR CAPSULA
```

## 4. Structs (`include/estruturas.h`)

| Struct | O que guarda |
|---|---|
| `Jogador` | `nome[50]`, `vida`, `localAtual`, `inventario[5]` com os ids dos itens e `quantidadeItens` |
| `Item` | id, nome, descrição, `coletavel`, `coletado`, `utilizado`, `consumivel`, `localAtual` (id do ambiente, −1 se está com o jogador, −2 se já foi gasto) e `ambienteUso` |
| `Ambiente` | id, nome, descrição, `arteASCII[1000]`, `saidas[4]`, `visitado`, `bloqueado` e `saidasBloqueadas[4]` |
| `Enigma` | descrição, resposta, `resolvido`, `tentativas`, `ambienteID` e `danoErro` |
| `EstadoJogo` | `energiaRestaurada`, `sinalDesativado`, `jogoFinalizado` e outros marcadores de progresso |
| `ObjetoCenario` | coisas fixas da sala (terminal, painel, núcleo...) que dá para examinar |
| `Registro` | os textos de pista que vão para o diário |
| `Diario` | vetor dinâmico: `int *ids`, `quantidade` e `capacidade` |
| `Comando` | a linha digitada separada em `verbo` e `argumento` |
| `Jogo` | junta tudo isso, para as funções receberem um ponteiro só |

As constantes (`QTD_AMBIENTES 9`, `QTD_ITENS 6`, `QTD_ENIGMAS 4`, `MAX_INVENTARIO 5`, `VIDA_MAXIMA 100` e outras) estão no mesmo arquivo.

## 5. Arquivos

```
SINAL_ZERO/
|-- src/
|   |-- main.c          inicialização e laço principal (umas 50 linhas)
|   |-- jogo.c          o resto: mundo, comandos, enigmas, DFS, arquivos, memória
|-- include/
|   |-- estruturas.h    structs e constantes
|   |-- jogo.h          protótipos
|-- saves/
|   |-- LEIA-ME.txt     explica o formato do save (o partida.txt é criado pelo jogo)
|-- README.md
|-- abrir_projeto.bat
```

Os `.h` têm só declarações. Toda a implementação fica em `jogo.c`.

## 6. Funções principais (`src/jogo.c`)

| Função | O que faz |
|---|---|
| `inicializarMundo(Jogo*)` | Cria os 9 ambientes, os 6 itens, os 4 enigmas, as travas do começo e o diário vazio |
| `iniciarJogo(Jogador*)` | Vida 100, Dormitório, inventário vazio |
| `menuInicial(Jogo*)` | Nova partida (pede o nome) ou carregar |
| `interpretarComando(char[], Comando*)` | Normaliza a linha e separa verbo de argumento |
| `executarComando(Jogo*, const Comando*)` | Chama a função certa para cada verbo e avisa quando o objetivo muda |
| `moverJogador(Jogador*, Ambiente[], int, int)` | Confere direção, saída, destino, trava e bloqueio, e só então move |
| `mostrarTela` / `mostrarAmbiente` | Cabeçalho curto (energia, comunicação, local, vida), arte, descrição, saídas, objetos e resumo |
| `limparTela()` | Limpa o console com `system("cls")` no Windows ou `system("clear")` nos outros sistemas (escolhido com `#ifdef _WIN32`) |
| `objetivoAtual(const Jogo*)` | Devolve o texto do próximo passo, de acordo com o que já foi feito |
| `mostrarResumo(const Jogo*)` | Onde o personagem está, o que leva, o que já fez, pistas no diário e objetivo |
| `mostrarPrompt(const Jogo*)` | Mostra `[SALA] O que deseja fazer?` antes de cada comando |
| `mostrarMapa(const Ambiente[], int, int)` | Mapa em ASCII com onde você está, o que ainda não visitou e as passagens travadas (`X`) |
| `coletarItem(Jogador*, Item*)` | Checa se o item já está com você, se está na sala e se cabe; depois altera jogador e item |
| `descartarItem`, `removerDoInventario` | Larga o item na sala e tira o id do vetor, puxando os outros para trás |
| `usarItem(Jogo*, Item*)` | O efeito de cada item no lugar certo |
| `examinarObjeto(const Item*)`, `examinarAlvo` | Descrições; alguns objetos põem uma pista no diário |
| `interagirObjeto` | Painel, console, núcleo, cápsula e leitor |
| `resolverEnigma(Enigma*, const char*)` | Compara a resposta normalizada; marca `resolvido` ou soma uma tentativa |
| `tentarEnigma` | Pergunta a resposta e aplica o dano se estiver errada |
| `aplicarDano` / `restaurarVida` | Mudam a vida pelo ponteiro, sem sair de 0 a 100 |
| `buscarCaminho` | A DFS recursiva |
| `comandoBuscar` | Roda a DFS e mostra a rota |
| `adicionarAoDiario` / `liberarDiario` | `malloc`, `realloc` e `free` |
| `salvarPartida` / `carregarPartida` | Save em TXT, com validação na hora de carregar |
| `verificarVitoria` / `verificarDerrota` / `verificarFimDeJogo` | Fim da partida |

## 7. Ponteiros

A variável `jogo` é criada uma vez só, na `main`. O que circula pelas funções é o endereço dela:

```c
inicializarMundo(&jogo);           // & pega o endereço
aplicarDano(&jogo->jogador, 15);   // dentro de uma função que recebeu Jogo *jogo
```

```c
void aplicarDano(Jogador *jogador, int dano) {   // * declara o ponteiro
    if (jogador == NULL || dano <= 0) return;    // proteção contra NULL
    jogador->vida -= dano;                        // -> mexe no campo da struct original
    if (jogador->vida < 0) jogador->vida = 0;
}
```

Se o parâmetro fosse `Jogador jogador`, sem o `*`, a função receberia uma cópia e a vida do jogador de verdade continuaria igual.

`coletarItem(Jogador *jogador, Item *item)` é outro exemplo: com dois ponteiros ela muda o inventário do jogador e o `localAtual`/`coletado` do item na mesma chamada.

`buscarItem()` devolve um `Item *`, o endereço do item dentro do vetor. Por isso `usarItem(jogo, item)` consegue alterar o item que está no jogo, e não uma cópia.

Funções que só leem recebem `const`: `mostrarAmbiente(const Ambiente *)`, `mostrarInventario(const Jogador *, const Item[])`, `verificarVitoria(const EstadoJogo *)`, `buscarCaminho(const Ambiente[] ...)`. Se eu tentar alterar alguma coisa nelas por engano, o compilador reclama.

Também uso aritmética de ponteiros para andar nos vetores:

```c
*(jogador->inventario + i)                 // possuiItem, mostrarInventario
(jogo->itens + i)->localAtual              // mostrarObjetosDoLocal
atual = ambientes + jogador->localAtual;   // moverJogador
for (; *origem != '\0'; origem++)          // normalizarResposta
```

## 8. Busca recursiva de caminhos (DFS)

```c
int buscarCaminho(const Ambiente ambientes[], int quantidade,
                  int atual, int destino, int visitados[]);
```

`visitados[i]` vale 0 quando o ambiente *i* não está no caminho que a busca está testando. Quando vale `k`, ele é o k-ésimo passo desse caminho.

Passo a passo:

1. Confere se os ponteiros não são `NULL` e se `atual` e `destino` existem.
2. Marca o ambiente atual: `visitados[atual] = profundidade + 1`.
3. Caso-base: `if (atual == destino) return 1;`.
4. Para cada uma das 4 saídas, pula o vizinho se:
   - não existe saída para lá (`SEM_SAIDA`);
   - a passagem está travada (`passagemLivre` olha a trava da porta e o `bloqueado` do ambiente);
   - o vizinho já está marcado em `visitados[]`. É isso que impede a busca de rodar em círculo, por exemplo Corredor → Laboratório → Sonar → Manutenção → Máquinas → Corredor.

   Para os outros, mostra `Explorando: <nome>` e chama `buscarCaminho(..., vizinho, destino, visitados)`.
5. Se alguma chamada devolver 1, esse 1 volta por todas as chamadas até a primeira.
6. Backtracking: se nenhuma saída chegou ao destino, a função apaga a própria marca (`visitados[atual] = 0`) e devolve 0. Com isso a busca desiste daquele beco e o ambiente pode ser usado por outro caminho depois. Na tela aparece `(sem saida a partir de X, voltando)`.

Quando acha o caminho, só os ambientes da rota ficam marcados, numerados na ordem. `comandoBuscar` usa esses números para imprimir a rota (`LESTE -> NORTE -> LESTE ...`).

Saída real no começo do jogo, com a porta do laboratório ainda trancada:

```
> BUSCAR DOCA DE ESCAPE
Iniciando busca de caminho a partir de DORMITORIO...
  Explorando: CORREDOR CENTRAL
  Explorando: SALA DE MAQUINAS
  (sem saida a partir de SALA DE MAQUINAS, voltando)
  Explorando: ARQUIVO TECNICO
  (sem saida a partir de ARQUIVO TECNICO, voltando)
  (sem saida a partir de CORREDOR CENTRAL, voltando)
NENHUM CAMINHO LIVRE ENCONTRADO.
```

## 9. Arquivos (save)

O save fica em `saves/partida.txt`, em texto. Não gravo struct em binário.

Para salvar (`salvarPartida`), o jogo abre o arquivo com `fopen("w")` e escreve com `fprintf` e `fputs`:

- nome, vida, local e quantidade de itens;
- o inventário;
- os marcadores de estado (energia, sinal etc.);
- uma linha `ITEM` por item (local, coletado, utilizado);
- uma linha `AMBIENTE` por ambiente (visitado, bloqueado e as 4 travas);
- uma linha `ENIGMA` por enigma (resolvido e tentativas);
- o diário;
- `FIM` na última linha.

Tudo isso vai primeiro para `partida.tmp`. Depois de conferir `ferror()` e `fclose()`, o jogo troca o arquivo com `remove` e `rename`. Se a gravação falhar no meio, o save antigo não se perde.

Para carregar (`carregarPartida`), `fscanf(arquivo, "VERSAO=%d", ...)` lê a primeira linha. Se ele devolver `EOF`, o arquivo está vazio; se devolver outra coisa que não 1, o arquivo não é um save. Daí em diante, `fgets` lê linha por linha e `strchr`, `strtok`, `sscanf` e `strtol` separam os valores.

Os dados vão para uma variável `Jogo temp`. Ela só substitui a partida atual se tudo isto estiver certo:

- versão certa e linha `FIM` presente;
- vida entre 1 e 100;
- local que existe;
- ids de itens válidos e sem repetição;
- `QTD_ITENS` igual ao tamanho do inventário;
- inventário batendo com o local dos itens;
- travas entre 0 e 3;
- todos os itens, ambientes e enigmas presentes;
- jogador fora de ambiente bloqueado;
- marcadores de estado batendo com os enigmas.

Se alguma coisa estiver errada, o jogo diz qual foi o problema e a partida que estava rodando continua igual.

## 10. Alocação dinâmica

Usei no diário de investigação (`Diario`), que guarda os ids das pistas encontradas. O jogador acha essas pistas aos poucos, numa ordem e numa quantidade que dependem de como ele joga, então o vetor vai crescendo conforme precisa.

```c
if (diario->ids == NULL)
    temporario = malloc(2 * sizeof(int));                            // primeira pista
else
    temporario = realloc(diario->ids, novaCapacidade * sizeof(int)); // dobra: 2 -> 4 -> 8
if (temporario == NULL) { /* avisa e fica com o vetor antigo */ return -1; }
diario->ids = temporario;
```

O `realloc` vai para um ponteiro temporário. Se ele falhar, `diario->ids` continua apontando para o vetor antigo e nada vaza.

No carregamento, o diário lido do arquivo é alocado dentro da partida temporária. Se o arquivo estiver errado, `liberarDiario(&temp.diario)` devolve essa memória. Se estiver certo, o diário antigo é liberado antes da troca. No fim do programa, `liberarJogo()` chama o `free()`.

Ambientes, itens e enigmas não usam alocação dinâmica, porque a quantidade deles é fixa.

## 11. Como compilar e executar

Precisa do GCC para Windows (MinGW-w64 ou MSYS2) no PATH.

```bash
gcc -std=c99 -Wall -Wextra -pedantic -I include src/main.c src/jogo.c -o SINAL_ZERO.exe
```

```powershell
.\SINAL_ZERO.exe
```

Rode de dentro da pasta `SINAL_ZERO`, porque o save usa o caminho relativo `saves/partida.txt`.

### No Visual Studio Code

1. Dê dois cliques em `abrir_projeto.bat`. Ele entra na pasta e roda `code .` (o comando `code` precisa estar no PATH). Também dá para clicar com o botão direito na pasta e escolher *Abrir com Code*.
2. Abra o terminal em **Terminal → Novo Terminal** ou com `` Ctrl+` ``. Ele já abre na pasta `SINAL_ZERO`.
3. Cole o comando do `gcc` e aperte Enter. Se não aparecer nada, compilou sem aviso.
4. Rode com `.\SINAL_ZERO.exe`.

---

## 12. Para explicar na apresentação

### Como a struct `Jogador` guarda os dados

Ela junta o que é do protagonista: `nome`, `vida`, `localAtual` (o id da sala onde ele está) e `inventario[5]`.
O inventário guarda só os ids dos itens, e `quantidadeItens` diz quantas posições estão ocupadas. Por exemplo, `{4, 5, 2}` com quantidade 3 quer dizer oxigênio, amostra e módulo.

### Como as salas se ligam

Cada `Ambiente` tem `saidas[4]` (N, S, L, O) com o id da sala vizinha, ou −1 se não tem saída. O Corredor tem `saidas = {2, 4, 3, 0}`.
As travas ficam em `saidasBloqueadas[4]` (0 livre, 1 cartão, 2 energia, 3 sinal). Quando a sala inteira está fechada, quem marca isso é o campo `bloqueado`.

### Como o movimento funciona

`NORTE` vira a direção 0 e `moverJogador` lê `ambientes[local].saidas[0]`. Não tem um `if` para cada sala; é o vetor que diz para onde ir.
Antes de mover, a função confere, nesta ordem:

1. se a direção é válida;
2. se existe saída;
3. se o destino existe;
4. se a porta está trancada;
5. se a sala de destino está bloqueada.

Só então muda `jogador->localAtual`.

### Ponteiros e passagem por endereço

Está na seção 7. Resumindo: `aplicarDano(&jogador, 20)` manda o endereço do jogador, e lá dentro `jogador->vida -= 20` mexe na variável original, não numa cópia.

### DFS, caso-base e backtracking

Está na seção 8.
- O caso-base é `if (atual == destino) return 1;`.
- Enquanto uma sala está no caminho, ela fica marcada em `visitados[]`, e é por isso que a busca não anda em círculo.
- Se todas as saídas falham, a marca é apagada (`visitados[atual] = 0`) e a busca volta para a sala anterior para tentar a próxima saída.

### Como o TXT guarda a partida

Cada informação vira uma linha `CHAVE=VALOR`. Na hora de carregar, as linhas vão para uma partida temporária. Só depois de conferir tudo o jogo copia essa partida para a atual (`*jogo = temp`).

### Como os enigmas abrem caminho

Quando `resolverEnigma` devolve 1, o código muda os dados das salas:
- `desbloquearPassagem(ambientes, AMB_LABORATORIO, LESTE)` zera a trava dos dois lados da porta;
- `ambientes[AMB_MANUTENCAO].bloqueado = 0` libera a sala inteira.

No próximo movimento (ou no próximo `BUSCAR`), a passagem já aparece livre.

### Onde e por que usei alocação dinâmica

No diário, porque a quantidade de pistas cresce durante a partida. Detalhes na seção 10.

## 13. Testes que eu rodei

| # | Teste | Como | Resultado |
|---|---|---|---|
| 1 | Início | Nova partida com o nome "Ana" | Vida 100/100, no Dormitório |
| 2 | Movimento | `NORTE` no Dormitório, `IR VOAR`, `VOAR`, `NORTE` no Corredor sem cartão | "Nao ha saida", "Direcao invalida", "Comando desconhecido", "[BLOQUEADO] ... CARTAO" |
| 3 | Itens | Pegar 5 itens e tentar o 6º; `PEGAR CARTAO` duas vezes; `PEGAR TERMINAL` | "Inventario cheio (5/5)", "Voce ja carrega", "Nao da para carregar"; `DESCARTAR` abre espaço |
| 4 | Enigmas | Painel com `1 2 3 4` e depois `3-4-1-2`; sonar `ECO`; núcleo `ECO-7` | Erro tira 15 de vida; acerto liga a energia e abre as passagens |
| 5 | Recursão | `BUSCAR DOCA` no começo, `BUSCAR ARQUIVO`, `BUSCAR XYZ`, `BUSCAR DOCA` depois do núcleo | sem caminho / `LESTE -> LESTE` / sala desconhecida / `SUL` |
| 6 | Ponteiros | Programa de teste separado, com 33 verificações em `aplicarDano`, `restaurarVida`, `coletarItem`, `resolverEnigma` e `buscarCaminho`, com `NULL` e com dados reais | 33 certas, 0 falhas |
| 7 | Memória | Contador de `malloc`/`realloc`/`free` e uma falha de alocação forçada no `realloc` | Nenhum bloco sobrando no fim; com a falha, o jogo avisa e segue |
| 8 | Save/Load | Salvar, fechar, abrir de novo e carregar. Também tentei carregar arquivo ausente, vazio, com lixo, vida 500, local 42, item 99, item repetido, `QTD_ITENS` errada, sem `FIM`, trava 9 e estado incoerente | Tudo volta igual (vida 85, local, 4 itens, enigmas, travas, diário). Cada arquivo ruim mostra o motivo e a partida atual não muda |
| 9 | Fim | 7 respostas erradas no painel; partida completa até a cápsula | Derrota com vida 0 e nenhum comando depois; vitória e programa encerrado |
| 10 | Comandos em português natural | `usar o cartao magnetico`, `interagir com o terminal`, `pegar a chave`, `ir para o norte`, `ir o`, `4 1 3 2` no prompt | Os quatro primeiros funcionam; `ir o` ainda vai para oeste; o número no prompt explica que é preciso usar `INTERAGIR` antes |
| 11 | Tela e resumo | Andar entre salas, `OLHAR`, `EXAMINAR MANUAL`, `PEGAR CARTAO`, `RESUMO`, `CARREGAR` | Tela limpa ao trocar de sala, no `OLHAR` e no `CARREGAR`; o texto do manual continua na tela; aparece `>> NOVO OBJETIVO`; o resumo lista local, itens, o que foi feito e o objetivo |
| 12 | Sinônimos | `encerra o jogo`, `pega o cartao`, `olha o terminal`, `vai pro leste`, `usa o cartao`, `ler o diario`, `mochila`, `onde`, `voar` | Todos executam o comando certo; `encerra o jogo` pergunta se quer salvar e sai; `voar` mostra os comandos principais |

## 14. Roteiro da apresentação (5 a 8 minutos)

1. **(30 s)** Abrir `abrir_projeto.bat` e mostrar as pastas `src/`, `include/` e `saves/`.
2. **(1 min)** Em `estruturas.h`, mostrar `Jogador`, `Ambiente` (`saidas[4]` e `saidasBloqueadas[4]`) e `Diario`.
3. **(30 s)** Compilar no terminal do VS Code com `-Wall -Wextra -pedantic` e mostrar que não sai nenhum aviso.
4. **(1 min)** Jogar um pouco: digitar o nome, `EXAMINAR TERMINAL` (a mensagem vem com o seu nome), `PEGAR CARTAO`, `LESTE`, `NORTE`. A porta trancada mostra o motivo.
5. **(1 min)** `BUSCAR DOCA` não acha caminho. Abrir `buscarCaminho` em `jogo.c` e mostrar o caso-base, a chamada recursiva, o `visitados[]` e a linha do backtracking.
6. **(1 min)** `USAR CARTAO`, `NORTE`, `MAPA`. Mostrar `aplicarDano(Jogador *)` e explicar `&`, `*`, `->` e `NULL`.
7. **(1 min)** No painel, errar uma vez (perde 15 de vida) e depois acertar `3 4 1 2`.
8. **(1 min)** `SALVAR`, abrir `saves/partida.txt` no editor, `SAIR`, rodar de novo, `CARREGAR` e `STATUS`. Mostrar as validações em `carregarPartida`.
9. **(30 s)** Mostrar `adicionarAoDiario` (`malloc` e `realloc` com ponteiro temporário) e `liberarJogo` (`free`).
10. **(30 s, se der tempo)** Mostrar o final. Para não perder tempo, vale levar um save feito perto da Doca e só rodar `INTERAGIR CAPSULA`.
