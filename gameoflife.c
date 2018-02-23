
#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <omp.h>
#include "board.h"
#include "vtkwrite.h"

const unsigned int TimeSteps = 500;


void game(Board *board) {
    unsigned numthreads = 0;
    printf("gimme thread num nomnomnomnom:");
    fscanf(stdin, "%d", &numthreads);
    for (size_t t = 0; t < TimeSteps; t++) {
        board->show(board);
        memcpy(board->newField, board->currentField, board->w * board->h * sizeof(double));
        writeVTK2(t, board->currentField, "gol", board->w, board->h);
#pragma omp parallel num_threads(numthreads)
        {
            int i = omp_get_thread_num();
            board->evolve(board, 0, (board->h / numthreads) * i, board->w,
                          (board->h / numthreads) * (i + 1));
            printf("%d thread, starth %d ,endh %d \n", omp_get_thread_num(),
                   (board->h / numthreads) * i,
                   (board->h / numthreads) * (i + 1));
        }

#pragma omp barrier
        printf("%ld timestep\n", t);
        usleep(100000);
        memcpy(board->currentField, board->newField, board->w * board->h * sizeof(double)); //swap
    }
    free(board);

}

int main(int c, char **v) {
    unsigned int w = 0, h = 0;
    if (c > 1) w = atoi(v[1]);
    if (c > 2) h = atoi(v[2]);
    if (w <= 0) w = 40;
    if (h <= 0) h = 40;
    game(newBoard(w, h));
}


