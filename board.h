//
// Created by tom on 18.02.18.
//

#ifndef GAMEOFLIFE_BOARD_H
#define GAMEOFLIFE_BOARD_H
#define calcIndex(width, x, y)  ((y)*(width) + (x))

typedef struct {
    unsigned int w;
    unsigned int h;
    double *currentField;
    double *newField;

    void (*show)(const void *self);

    void
    (*evolve)(void *self, const unsigned int startw, const unsigned int starth, const unsigned int endw,
              const unsigned int endh);
} Board;

Board *newBoard(unsigned int w, unsigned int h);

#endif //GAMEOFLIFE_BOARD_H
