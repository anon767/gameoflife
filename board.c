//
// Created by tom on 18.02.18.
//
#include <stdlib.h>
#include <stdio.h>
#include "board.h"



void show(const void *self) {
    Board *b = (Board *) self;
    printf("\033[H");
    for (unsigned y = 0; y < b->h; y++) {
        for (unsigned x = 0; x < b->w; x++) printf(b->currentField[calcIndex(b->w, x, y)] ? "\033[07m  \033[m" : "  ");
        printf("\n");
    }
    fflush(stdout);
}

void evolve(void *self, const unsigned int startw, const unsigned int starth, const unsigned int endw,
            const unsigned int endh) {
    Board *b = (Board *) self;
    for (unsigned y = starth; y < endh; y++) {
        for (unsigned x = startw; x < endw; x++) {
            if (!((x > 0) && (x < b->w - 1) && (y > 0) && (y < b->h - 1)))
                continue; //skip edges
            double sum = b->currentField[calcIndex(b->w, x - 1, y - 1)]
                         + b->currentField[calcIndex(b->w, x, y - 1)]
                         + b->currentField[calcIndex(b->w, x - 1, y)]
                         + b->currentField[calcIndex(b->w, x + 1, y + 1)]
                         + b->currentField[calcIndex(b->w, x + 1, y)]
                         + b->currentField[calcIndex(b->w, x, y + 1)]
                         + b->currentField[calcIndex(b->w, x - 1, y + 1)]
                         + b->currentField[calcIndex(b->w, x + 1, y - 1)];
            double currentCell = b->currentField[calcIndex(b->w, x, y)];

            if (currentCell == 0 && sum == 3)
                b->newField[calcIndex(b->w, x, y)] = 1;
            else if (currentCell == 1 && sum < 2)
                b->newField[calcIndex(b->w, x, y)] = 0;
            else if (currentCell == 1 && sum > 3)
                b->newField[calcIndex(b->w, x, y)] = 0;
            else if (currentCell == 1 && (sum == 2 || sum == 3))
                b->newField[calcIndex(b->w, x, y)] = currentCell;
        }
    }
}

Board *newBoard(const unsigned int w, const unsigned int h) {
    Board *b = (Board *) malloc(sizeof(Board));
    b->w = w;
    b->h = h;
    double *newField = calloc(w * h, sizeof(double));
    double *currentField = calloc(w * h, sizeof(double));
    for (size_t i = 0; i < h * w; i++) {
        currentField[i] = (rand() < RAND_MAX / 5) ? 1 : 0; ///< init domain randomly
    }
    for (size_t y = 0; y < h; y++) {
        for (size_t x = 0; x < w; x++) {
            if (!((x > 0) && (x < w - 1) && (y > 0) && (y < h - 1)))
                currentField[calcIndex(w, x, y)] = 0;
        }
    }
    b->currentField = currentField;
    b->newField = newField;
    b->show = show;
    b->evolve = evolve;
    return b;
}



