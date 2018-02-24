
#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <mpi.h>
#include "board.h"
#include "vtkwrite.h"

const unsigned int TimeSteps = 200;


void game(Board *board, unsigned int procNum) {
    unsigned numthreads = 0;
    printf("gimme thread num nomnomnomnom:");
    fscanf(stdin, "%d", &numthreads);
    for (unsigned int t = 0; t < TimeSteps; t++) {
        board->show(board);
        memcpy(board->newField, board->currentField, board->w * board->h * sizeof(double));

        board->evolve(board, 0, (board->h / numthreads) * procNum, board->w,
                      (board->h / numthreads) * (procNum + 1));

        writeVTK2(t, board->currentField, procNum, board->w, board->h);
        printf("%d timestep\n", t);
        usleep(100000);
        memcpy(board->currentField, board->newField, board->w * board->h * sizeof(double)); //swap
    }
    free(board);

}

MPI_Comm setUpCommunicator() {
    int size, dim[2], period[2];
    MPI_Comm comm;

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    dim[0] = size;
    dim[1] = 1;
    period[0] = 1;
    period[1] = 1;
    MPI_Cart_create(MPI_COMM_WORLD, 2, dim, period, 0, &comm);
    return comm;
}

int main(int c, char **v) {
    int procId;
    MPI_Init(&c, &v);
    MPI_Comm communicator = setUpCommunicator();
    MPI_Comm_rank(MPI_COMM_WORLD, &procId);


    MPI_Finalize();
    return 0;
/**
    unsigned int w = 0, h = 0;
    if (c > 1) w = atoi(v[1]);
    if (c > 2) h = atoi(v[2]);
    if (w <= 0) w = 40;
    if (h <= 0) h = 40;
    game(newBoard(w, h));
**/
}


