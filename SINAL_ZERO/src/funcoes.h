/*
 * funcoes.h
 * SINAL ZERO - Protótipos das funções do jogo.
 */
#ifndef FUNCOES_H
#define FUNCOES_H

#include <stddef.h>
#include "estruturas.h"

/* ---------- Utilitários de terminal e strings ---------- */
void configurarTerminal(void);
int  lerLinha(char *destino, int tamanho);
int  lerOpcao(void);
void removerQuebraLinha(char *texto);
void aparar(char *texto);
void paraMaiusculas(char *texto);
void copiarTexto(char *destino, const char *origem, size_t tamanho);
int  substituirMarcador(char *texto, size_t tamanho, const char *marcador, const char *valor);
int  dividirCampos(char *linha, char separador, char *campos[], int maximo);
void imprimirTexto(const char *texto);
void registrarEvento(const char *formato, ...);

/* ---------- Inicialização, carga de CSV e memória ---------- */
int  inicializarJogo(Jogo *jogo);
void iniciarJogador(Jogador *jogador, const char *nome);
int  carregarCenarios(Jogo *jogo);
int  carregarItens(Jogo *jogo);
int  carregarRegistros(Jogo *jogo);
void reiniciarItens(Jogo *jogo);
void liberarJogo(Jogo *jogo);

/* ---------- Buscas ---------- */
Item     *buscarItemPorId(const Jogo *jogo, int id);
Registro *buscarRegistroPorId(const Jogo *jogo, int id);
int  temItem(const Jogador *jogador, int idItem);
int  registroJaEncontrado(const Jogador *jogador, int idRegistro);
int  registroContem(const Registro *registro, const char *termoMaiusculo);
int  buscarRegistroRecursivo(const Jogo *jogo, int posicao, const char *termoMaiusculo);

/* ---------- Inventário e registros do jogador ---------- */
int  adicionarAoInventario(Jogador *jogador, int idItem);
int  removerDoInventario(Jogador *jogador, int idItem);
int  adicionarRegistroEncontrado(Jogador *jogador, int idRegistro);

/* ---------- Interface ---------- */
void mostrarAbertura(void);
int  menuInicial(Jogo *jogo);
void mostrarStatus(const Jogo *jogo);
void mostrarCenario(const Jogo *jogo);
void mostrarMenu(const Jogo *jogo);
void processarOpcao(Jogo *jogo, int opcao);

/* ---------- Ações do jogador ---------- */
void examinarAmbiente(const Jogo *jogo);
void procurarObjetos(Jogo *jogo);
int  coletarItem(Jogo *jogo, int idItem);
void mostrarInventario(const Jogo *jogo);
void escolherItemParaUsar(Jogo *jogo);
int  usarItem(Jogo *jogo, int idItem);
void acessarTerminal(Jogo *jogo);
void exibirRegistro(const Jogo *jogo, const Registro *registro);
int  condicaoAtendida(const Jogo *jogo, const char *condicao);
int  resolverEnigma(Jogo *jogo);
int  podeAcessar(const Jogo *jogo, int destino);
void explicarBloqueio(const Jogo *jogo, int destino);
void moverJogador(Jogo *jogo);
int  moverPara(Jogo *jogo, int destino);
void menuRegistros(const Jogo *jogo);
void pesquisarRegistros(const Jogo *jogo);

/* ---------- Arquivos de progresso e final ---------- */
int  salvarProgresso(const Jogo *jogo);
int  carregarProgresso(Jogo *jogo);
int  verificarVitoria(const Jogo *jogo);
void mostrarFinal(Jogo *jogo);

#endif /* FUNCOES_H */
