//
// Created by tom on 18.02.18.
//

#ifndef GAMEOFLIFE_VTKWRITE_H
#define GAMEOFLIFE_VTKWRITE_H


#include <glob.h>

void writeVTK2(size_t timestep, const double *data, const char prefix[1024], unsigned w, unsigned h);

#endif //GAMEOFLIFE_VTKWRITE_H
