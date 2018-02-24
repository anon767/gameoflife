//
// Created by tom on 18.02.18.
//

#ifndef GAMEOFLIFE_VTKWRITE_H
#define GAMEOFLIFE_VTKWRITE_H


#include <glob.h>

void writeVTK2(long timestep,const double *data, char prefix[1024], long w, long h);

#endif //GAMEOFLIFE_VTKWRITE_H
