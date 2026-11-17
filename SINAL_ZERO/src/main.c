/*
 * main.c
 * SINAL ZERO - Aventura textual de mistério em C (C99).
 *
 * A main só inicializa o jogo e controla o ciclo principal:
 *   ler comando -> interpretar -> executar -> verificar vitória/derrota.
 * Toda a lógica está em src/jogo.c.
 *
 * Compilar (dentro da pasta SINAL_ZERO):
 *   gcc -std=c99 -Wall -Wextra -pedantic -I include src/main.c src/jogo.c -o SINAL_ZERO.exe
 */
#include <stdio.h>
#include "jogo.h"

int main(void)
{
    Jogo jogo;                       /* estado completo da partida */
    char linha[MAX_COMANDO];         /* texto digitado pelo jogador */
    Comando comando;                 /* verbo + argumento */

    inicializarMundo(&jogo);         /* passagem por endereço: preenche 'jogo' */
    mostrarTitulo();

    if (menuInicial(&jogo)) {
        mostrarTela(&jogo);

        while (!jogo.estado.jogoFinalizado) {
            mostrarPrompt(&jogo);        /* "[SALA] O que deseja fazer?" */
            if (!lerLinha(linha, sizeof(linha))) {
                printf("\n[Entrada encerrada]\n");
                break;
            }
            if (!interpretarComando(linha, &comando)) {
                printf("Digite um comando (AJUDA mostra a lista).\n");
                continue;
            }
            executarComando(&jogo, &comando);
            verificarFimDeJogo(&jogo);   /* após vitória/derrota o laço termina */
        }
    }

    liberarJogo(&jogo);              /* free() do diário dinâmico */
    printf("\n[SISTEMA] Memoria liberada. Fim da transmissao.\n");
    return 0;
}
