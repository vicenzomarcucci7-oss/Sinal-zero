/*
 * jogo.h
 * SINAL ZERO - Protótipos das funções implementadas em src/jogo.c.
 */
#ifndef JOGO_H
#define JOGO_H

#include <stddef.h>
#include "estruturas.h"

/* Resultados de moverJogador() */
#define MOV_OK          1
#define MOV_SEM_SAIDA   0
#define MOV_BLOQUEADO (-1)
#define MOV_INVALIDO  (-2)

/* ---------- Utilitários de entrada e texto ---------- */
int  lerLinha(char *destino, int tamanho);
void removerQuebraLinha(char *texto);
void normalizarTexto(char *texto);
void normalizarResposta(const char *origem, char *destino, size_t tamanho);
void copiarTexto(char *destino, const char *origem, size_t tamanho);
int  substituirMarcador(char *texto, size_t tamanho, const char *marcador, const char *valor);
int  textoCorresponde(const char *nome, const char *busca);

/* ---------- Inicialização e memória ---------- */
void iniciarJogo(Jogador *jogador);
void inicializarMundo(Jogo *jogo);
int  menuInicial(Jogo *jogo);
void liberarJogo(Jogo *jogo);
int  adicionarAoDiario(Diario *diario, int idRegistro);
int  diarioContem(const Diario *diario, int idRegistro);
void liberarDiario(Diario *diario);

/* ---------- Telas ---------- */
void mostrarTitulo(void);
void mostrarAmbiente(const Ambiente *ambiente);
void mostrarTela(const Jogo *jogo);
void mostrarInventario(const Jogador *jogador, const Item itens[]);
void mostrarStatus(const Jogo *jogo);
void mostrarMapa(const Ambiente ambientes[], int quantidade, int localAtual);
void mostrarDiario(const Jogo *jogo);
void mostrarRegistro(const Jogo *jogo, int idRegistro);
void mostrarAjuda(void);

/* ---------- Comandos ---------- */
int  interpretarComando(char comando[], Comando *resultado);
void executarComando(Jogo *jogo, const Comando *comando);
int  direcaoPorNome(const char *texto);

/* ---------- Movimentação e passagens ---------- */
int  moverJogador(Jogador *jogador, Ambiente ambientes[], int quantidade, int direcao);
void explicarTrava(int tipoTrava);
void desbloquearPassagem(Ambiente ambientes[], int origem, int direcao);
int  passagemLivre(const Ambiente ambientes[], int origem, int direcao);

/* ---------- Itens ---------- */
Item *buscarItem(Item itens[], int quantidade, const char *nome, int local);
int  possuiItem(const Jogador *jogador, int idItem);
int  coletarItem(Jogador *jogador, Item *item);
int  removerDoInventario(Jogador *jogador, int idItem);
int  descartarItem(Jogador *jogador, Item *item);
void examinarObjeto(const Item *item);
int  usarItem(Jogo *jogo, Item *item);

/* ---------- Interações e enigmas ---------- */
void interagirObjeto(Jogo *jogo, const char *nome);
void examinarAlvo(Jogo *jogo, const char *nome);
int  resolverEnigma(Enigma *enigma, const char *resposta);
int  tentarEnigma(Jogo *jogo, int idEnigma, const char *instrucao);

/* ---------- Vida ---------- */
void aplicarDano(Jogador *jogador, int dano);
void restaurarVida(Jogador *jogador, int quantidade);

/* ---------- Busca recursiva (DFS) ---------- */
int  buscarCaminho(const Ambiente ambientes[], int quantidade, int atual, int destino, int visitados[]);
int  buscarAmbientePorNome(const Ambiente ambientes[], int quantidade, const char *nome);
void comandoBuscar(const Jogo *jogo, const char *nomeDestino);

/* ---------- Arquivos ---------- */
int  salvarPartida(const Jogo *jogo);
int  carregarPartida(Jogo *jogo);

/* ---------- Encerramento ---------- */
int  verificarVitoria(const EstadoJogo *estado);
int  verificarDerrota(const Jogador *jogador);
int  verificarFimDeJogo(Jogo *jogo);

#endif /* JOGO_H */
