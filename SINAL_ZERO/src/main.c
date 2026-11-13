/*
 * main.c
 * SINAL ZERO - Jogo de aventura, mistério e terror psicológico em C (C99).
 *
 * Ponto de entrada: inicializa o jogo, mostra o menu inicial e executa o
 * laço principal. Toda a lógica fica em funcoes.c.
 *
 * Compilar (a partir da pasta SINAL_ZERO):
 *   gcc -std=c99 -Wall -Wextra -I src src/main.c src/funcoes.c -o sinal_zero
 */
#include <stdio.h>
#include "funcoes.h"

int main(void)
{
    Jogo jogo;          /* todo o estado do jogo; passado às funções por ponteiro */
    int opcao;

    configurarTerminal();
    mostrarAbertura();

    /* Lê os CSV e aloca a memória. Se falhar, libera o que já foi alocado. */
    if (!inicializarJogo(&jogo)) {
        printf("\nNão foi possível iniciar o SINAL ZERO.\n");
        liberarJogo(&jogo);
        return 1;
    }

    if (menuInicial(&jogo)) {
        mostrarCenario(&jogo);

        /* Laço principal: menu -> ação -> verifica vitória. */
        while (jogo.ativo) {
            mostrarMenu(&jogo);
            opcao = lerOpcao();
            processarOpcao(&jogo, opcao);

            if (jogo.ativo && verificarVitoria(&jogo)) {
                mostrarFinal(&jogo);
                jogo.ativo = 0;
            }
        }
    }

    liberarJogo(&jogo);
    printf("\n[SISTEMA] Memória liberada. Conexão encerrada.\n");
    return 0;
}
