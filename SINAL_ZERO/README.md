# SINAL ZERO

Jogo de aventura, mistério e terror psicológico em **C (C99)**, jogado inteiramente pelo terminal.
Projeto acadêmico da disciplina de programação.

---

## 1. Proposta e história

**NEREIDA-7** é uma estação científica submarina a 2.140 metros de profundidade. A equipe desapareceu.
Você, um técnico de manutenção, é enviado para investigar. Ao acordar no dormitório, a estação está sem
energia, as portas estão travadas e um terminal exibe um aviso assinado **com o seu próprio nome**:

> "SE VOCÊ ESTÁ LENDO ISSO, NÃO ATIVE O SONAR."

Explorando os setores, você descobre que a equipe ampliou o sonar e ele passou a captar
"respostas antes das perguntas". As transmissões estranhas não vêm do oceano: são um **eco** —
mensagens gravadas por você mesmo, no futuro, na câmara de escape, e devolvidas sete minutos ao passado.

**Objetivo:** restabelecer a energia, ativar o sonar com o módulo de memória, decifrar a origem do sinal
(enigma) e liberar a câmara de escape. No final, você grava a mesma mensagem que leu no começo,
fechando o ciclo.

### Caminho da solução (spoiler)

| Passo | Onde | Ação |
|---|---|---|
| 1 | Dormitório | Ler o terminal [4] e coletar o **Cartão de acesso** [2] |
| 2 | Corredor | Usar o cartão [6] → libera o Laboratório |
| 3 | Laboratório | Ler registros [4] e coletar o **Fusível industrial** [2] |
| 4 | Sala de máquinas | Coletar o **Módulo de memória** [2] e usar o fusível [6] → energia ONLINE |
| 5 | Central de sonar | (só abre com energia) Usar o módulo [6], acessar o terminal [4] |
| 6 | Central de sonar | Enigma: as iniciais das transmissões (**E**scute, **C**ada, **O** sinal) → `ECO` |
| 7 | Corredor | Usar o cartão novamente [6] → quarentena encerrada |
| 8 | Câmara de escape | Entrar → **vitória** |

---

## 2. Árvore de diretórios

```
SINAL_ZERO/
├── src/
│   ├── main.c            ponto de entrada e laço principal
│   ├── estruturas.h      structs, constantes e caminhos
│   ├── funcoes.h         protótipos
│   └── funcoes.c         implementação de todas as funções
├── dados/
│   ├── cenarios.csv      6 cenários + conexões (matriz) + requisitos
│   ├── itens.csv         3 objetos
│   └── registros.csv     10 registros/mensagens
├── saves/
│   ├── progresso.txt     progresso salvo (criado/substituído pela opção [8])
│   └── diario_eventos.txt  (gerado durante o jogo: log com data e hora)
├── README.md
└── abrir_projeto.bat     abre a pasta no VS Code
```

---

## 3. Como compilar e executar

Requisito: **GCC** (MinGW/MSYS2 no Windows, ou gcc no Linux). Nenhuma biblioteca externa.

**Sempre execute a partir da pasta `SINAL_ZERO`** — os caminhos `dados/...` e `saves/...` são relativos a ela.

```bash
gcc -std=c99 -Wall -Wextra -I src src/main.c src/funcoes.c -o sinal_zero
```

Windows (PowerShell ou CMD):

```bash
sinal_zero.exe
```

Linux / Git Bash:

```bash
./sinal_zero
```

### Pelo Visual Studio Code

1. Dê dois cliques em `abrir_projeto.bat` (ou, num terminal dentro da pasta, digite `code .`).
2. No VS Code, abra o terminal integrado: menu **Terminal → Novo Terminal** (atalho `` Ctrl+` ``).
   Ele já abre na pasta `SINAL_ZERO`.
3. Cole o comando de compilação acima e depois `.\sinal_zero.exe`.

> Acentos: no Windows o programa chama `SetConsoleOutputCP(CP_UTF8)` para exibir os acentos corretamente.
> Ao digitar respostas, prefira letras sem acento (ex.: `eco`, `sinal`).

---

## 4. Funções (explicação individual)

### Utilitários (strings e entrada)
| Função | O que faz |
|---|---|
| `configurarTerminal()` | No Windows, coloca o terminal em UTF-8 para exibir acentos. |
| `lerLinha()` | Lê uma linha com `fgets` (seguro), descarta excesso e remove `\n`. Retorna 0 em fim de entrada. |
| `lerOpcao()` | Lê um número de menu com `strtol`; letras, vazio ou negativos viram `OPCAO_INVALIDA`. |
| `removerQuebraLinha()` | Usa `strcspn` para cortar `\r\n` (arquivos do Windows). |
| `aparar()` | Remove espaços do início e do fim (`isspace`, `memmove`). |
| `paraMaiusculas()` | Converte o texto percorrendo-o com aritmética de ponteiros (`texto++`). |
| `copiarTexto()` | Usa `strlen` para checar se cabe; copia com `strcpy` ou trunca com `strncpy`. |
| `substituirMarcador()` | Usa `strstr` para achar `[NOME]` no registro e troca pelo nome do jogador (`strcat`). |
| `dividirCampos()` | Divide uma linha CSV no `;` com `strchr`, guardando ponteiros para cada campo. |
| `imprimirTexto()` | Imprime textos do CSV convertendo `|` em quebra de linha. |
| `registrarEvento()` | Acrescenta uma linha com data/hora em `saves/diario_eventos.txt` (modo `"a"`, `stdarg.h`, `strftime`). |

### Inicialização e memória
| Função | O que faz |
|---|---|
| `inicializarJogo()` | Zera a matriz, inicia o jogador e chama as três funções de carga. |
| `iniciarJogador()` | Recebe `Jogador*` e zera inventário, registros e objetivos. |
| `carregarCenarios()` | Lê `cenarios.csv` para o vetor estático e monta a **matriz de adjacência** a partir da coluna `conexoes` (`strtok`). |
| `carregarItens()` | Lê `itens.csv` para um vetor **dinâmico** (`malloc` inicial, `realloc` dobrando a capacidade). |
| `carregarRegistros()` | Igual ao anterior, para `registros.csv`. |
| `reiniciarItens()` | Devolve os itens aos cenários originais (nova aventura). |
| `liberarJogo()` | `free` dos dois vetores dinâmicos; seguro mesmo se a carga falhou. |

### Buscas
| Função | O que faz |
|---|---|
| `buscarItemPorId()` / `buscarRegistroPorId()` | Retornam um **ponteiro** para o elemento no vetor (permite alterar o item original). |
| `temItem()` / `registroJaEncontrado()` | Percorrem os vetores do jogador com `*(vetor + i)`. |
| `registroContem()` | Compara título e texto com o termo, sem diferenciar maiúsculas (`strstr`). |
| `buscarRegistroRecursivo()` | **Busca recursiva** de um termo nos registros encontrados (ver seção 5-K). |

### Inventário
| Função | O que faz |
|---|---|
| `adicionarAoInventario()` | Insere o id no vetor `inventario` se houver espaço. |
| `removerDoInventario()` | Remove deslocando os elementos seguintes para a esquerda. |
| `adicionarRegistroEncontrado()` | Guarda o id do registro lido (sem duplicar). |

### Interface e ações
| Função | O que faz |
|---|---|
| `mostrarAbertura()` | Tela de título. |
| `menuInicial()` | Nova aventura (pede o nome) ou carregar progresso. |
| `mostrarStatus()` | Barra `[ SISTEMA DE ENERGIA: OFFLINE ] [ SONAR ] [ ESCAPE ]` + localização. |
| `mostrarCenario()` | Status + descrição (muda quando a energia volta). |
| `mostrarMenu()` / `processarOpcao()` | Exibe o menu e chama a função da opção (`switch`). |
| `examinarAmbiente()` | Descrição, saídas lidas da **matriz** (com indicação de bloqueio) e dicas. |
| `procurarObjetos()` / `coletarItem()` | Lista os itens do cenário e move o escolhido para o inventário. |
| `mostrarInventario()` | Lista os itens carregados com descrição. |
| `escolherItemParaUsar()` / `usarItem()` | Item só funciona no cenário da coluna `cenario_uso`; o efeito é decidido por `strcmp` na chave. |
| `acessarTerminal()` | Mostra os registros do cenário cuja condição foi atendida e os guarda; no sonar abre o enigma. |
| `exibirRegistro()` | Mostra um registro trocando `[NOME]` pelo nome do jogador. |
| `condicaoAtendida()` | Interpreta `NENHUM`, `ENERGIA`, `MODULO`. |
| `resolverEnigma()` | Lê a resposta, converte para maiúsculas e compara com `ECO`. |
| `podeAcessar()` / `explicarBloqueio()` | Verificam o requisito do cenário (`NENHUM`, `CARTAO`, `ENERGIA`, `QUARENTENA`). |
| `moverJogador()` / `moverPara()` | Listam só destinos com `1` na matriz e efetuam o movimento conferindo matriz + requisito. |
| `menuRegistros()` / `pesquisarRegistros()` | Ler registros encontrados ou pesquisar uma palavra (usa a busca recursiva). |
| `salvarProgresso()` / `carregarProgresso()` | Gravação e leitura do progresso (ver seção 5-G/H). |
| `verificarVitoria()` / `mostrarFinal()` | Checa os objetivos na câmara de escape e mostra o final. |

---

## 5. Como cada conteúdo da disciplina foi aplicado

**A) Structs** — `estruturas.h`: `Item`, `Cenario`, `Registro`, `Jogador` e `Jogo` (que agrupa todo o estado).

**B) Vetores** — `Cenario cenarios[6]`; `int inventario[10]` e `int registros[32]` no `Jogador`;
vetores dinâmicos `Item *itens` e `Registro *registros`; vetores locais de opções em `moverJogador()` e `procurarObjetos()`.

**C) Matriz** — `int conexoes[6][6]` em `Jogo`. É preenchida a partir da coluna `conexoes` de `cenarios.csv`
(`1|2|3` → `conexoes[id][1] = 1` ...). `moverJogador()` só lista os destinos com `1`, e `moverPara()` confere a matriz de novo.

```
            DOR COR LAB MAQ SON ESC
Dormitório   0   1   0   0   0   0
Corredor     1   0   1   1   1   1
Laboratório  0   1   0   1   0   0
Máquinas     0   1   1   0   0   0
Sonar        0   1   0   0   0   0
Escape       0   1   0   0   0   0
```

**D) Strings (`string.h`)** — `strcmp` (requisitos, chaves dos itens, enigma, chaves do save), `strcpy` e `strlen`
(`copiarTexto`), `strstr` (pesquisa de registros e `substituirMarcador`), `strcat`, `strncpy`, `strchr`, `strcspn`, `strtok`, `memmove`.

**E) Ponteiros e referências** — todas as funções recebem `Jogo *` ou `Jogador *`; por exemplo `usarItem()` altera
`j->energiaRestaurada` e `item->localizacao` diretamente, sem retornar structs. `buscarItemPorId()` retorna
`Item *` para que o chamador altere o item real dentro do vetor.

**F) Ponteiros como vetores** — `*(jogador->inventario + i)` em `temItem()`, `(jogo->itens + i)->localizacao` em
`reiniciarItens()`, `cenario = jogo->cenarios + id` em `carregarCenarios()`, `*(locais + i)` em `carregarProgresso()`,
e `for (; *texto; texto++)` em `paraMaiusculas()`.

**G) Arquivos** — leitura dos 3 CSV (`"r"`), gravação do progresso em `saves/progresso.txt` (cenário atual, inventário,
registros, objetivos, localização de cada item) e log de eventos em `saves/diario_eventos.txt` (`"a"`).
Todo `fopen` é verificado; arquivos ausentes geram mensagem e o jogo encerra ou continua sem travar.

**H) Manipulação avançada (somente o que o projeto precisava)**
- **Salvamento seguro**: grava em `progresso.tmp`, confere `ferror`/`fclose`, e só então usa `remove` + `rename` para
  substituir o save. Assim uma falha no meio não destrói o progresso anterior. (`remove` antes porque no Windows
  `rename` falha se o destino existe.)
- **`fseek`/`ftell`/`rewind`** em `carregarProgresso()` para detectar arquivo de save vazio.
- **Validação antes de aplicar**: o save é lido para variáveis temporárias e só substitui o jogo se o cabeçalho
  `SINAL_ZERO_SAVE 1` e o cenário forem válidos.
- **Parsing estruturado**: formato `chave=valor`, listas com `strtok` e pares `id:local` com `sscanf`.
- **Log com data/hora**: `time`, `localtime`, `strftime`, e função com argumentos variáveis (`stdarg.h`, `vfprintf`).

**I) CSV** — `cenarios.csv`, `itens.csv`, `registros.csv`, separados por `;` (padrão do Excel em português,
e permite usar vírgulas dentro dos textos). A primeira linha é cabeçalho; linhas mal formadas são ignoradas com aviso.
`|` dentro dos textos representa quebra de linha.

**J) Alocação de memória** — itens e registros são lidos sem saber a quantidade: começa com `malloc` para 2 elementos
e usa `realloc` dobrando a capacidade (2→4→8→16). Se `realloc` falhar, o ponteiro antigo continua válido e é liberado
em `liberarJogo()`. Em `carregarProgresso()` um `malloc` temporário do tamanho exato do vetor de itens é liberado
logo depois. Cenários **não** usam alocação dinâmica, pois são sempre 6 (tamanho da matriz).
Verificado com um contador de alocações: ao final de cada execução, `blocos vivos = 0`.

**K) Recursividade** — `buscarRegistroRecursivo(jogo, posicao, termo)`:
- **Parada 1**: `posicao >= qtdRegistros` → retorna `-1` (não encontrou).
- **Parada 2**: o registro da posição contém o termo → retorna `posicao`.
- **Chamada recursiva**: `return buscarRegistroRecursivo(jogo, posicao + 1, termo);`

`pesquisarRegistros()` chama a função a partir de `0` e, a cada resultado `p`, chama de novo a partir de `p + 1`,
listando todos os registros que contêm a palavra (menu [7] → [2]).

---

## 6. Roteiro de testes

| # | Teste | Entrada | Resultado esperado |
|---|---|---|---|
| 1 | Início | `sinal_zero.exe` | Mensagem "6 cenários, 3 objetos e 10 registros carregados" |
| 2 | CSV ausente | Executar de outra pasta (ex.: `cd src` e `..\sinal_zero.exe`) | "[ERRO] Arquivo dados/cenarios.csv não encontrado" e saída sem travar |
| 3 | Matriz | No Dormitório, [5] | Só aparece CORREDOR CENTRAL |
| 4 | Bloqueio | Corredor → [5] → Laboratório sem usar cartão | "[BLOQUEADO] ... exige um cartão" |
| 5 | Coleta/inventário | Dormitório [2] [1], depois [3] | Cartão aparece no inventário |
| 6 | Usar item no lugar errado | Usar cartão no Dormitório | "nada aqui reage" |
| 7 | Usar item | Corredor [6] cartão | Laboratório liberado |
| 8 | Energia | Sala de máquinas [6] fusível | `SISTEMA DE ENERGIA: ONLINE`; sonar desbloqueado |
| 9 | Enigma errado | Sonar [4] → `SOS` | "RESPOSTA INCORRETA" + dica |
| 10 | Enigma certo | Sonar [4] → `eco` | "PALAVRA ACEITA: ECO" |
| 11 | Recursão | [7] → [2] → `sinal` | Lista 4 registros que contêm SINAL (após ler todos) |
| 12 | Salvar | [8] | `saves/progresso.txt` contém `SINAL_ZERO_SAVE 1` e os dados |
| 13 | Carregar | Reabrir → [2] Carregar | Volta ao mesmo cenário, inventário e objetivos |
| 14 | Save inválido | Apagar conteúdo de `progresso.txt` e carregar | "arquivo de progresso está vazio" |
| 15 | Entradas inválidas | `abc`, vazio, `-3`, `42` | "[!] Opção inválida", jogo continua |
| 16 | Fim de entrada | Ctrl+Z/Ctrl+D no menu | "[Entrada encerrada]", memória liberada |
| 17 | Vitória | Corredor [6] cartão → [5] Câmara de escape | "FIM DA TRANSMISSÃO - VOCÊ ESCAPOU" |
| 18 | Log | Abrir `saves/diario_eventos.txt` | Eventos com data e hora |

Teste automático da partida completa (Git Bash), da pasta `SINAL_ZERO`:

```bash
printf '1\nAna\n2\n1\n5\n1\n6\n1\n5\n2\n2\n1\n5\n2\n2\n1\n6\n2\n5\n1\n5\n4\n6\n2\n4\neco\n5\n1\n6\n1\n5\n5\n' | ./sinal_zero.exe
```

---

## 7. Roteiro curto para apresentar ao professor (~5 min)

1. **Abrir** o projeto com `abrir_projeto.bat`, mostrar a árvore de pastas e os 3 CSV.
2. **Compilar** no terminal do VS Code com `-Wall -Wextra` (sem avisos).
3. **Jogar** o início: nome, terminal do dormitório (mostra `[NOME]` substituído via `strstr`), coletar o cartão.
4. No corredor, **tentar entrar no laboratório** → bloqueado; mostrar no código `podeAcessar()` e a matriz `conexoes`.
5. Usar o cartão, pegar o fusível, restaurar a energia → mostrar `usarItem()` alterando o jogador por ponteiro.
6. **Salvar** [8], abrir `saves/progresso.txt` no editor; fechar o jogo e **carregar** [2].
7. Sonar: usar o módulo, errar e acertar o **enigma**.
8. [7] → [2] `sinal` → explicar `buscarRegistroRecursivo()` (paradas e chamada recursiva).
9. Liberar a câmara e **vencer**. Mostrar `saves/diario_eventos.txt`.
10. Mostrar `carregarItens()` (`malloc`/`realloc`) e `liberarJogo()` (`free`) fechando a explicação de memória.
