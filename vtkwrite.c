//
// Created by tom on 18.02.18.
//

#include "vtkwrite.h"
#include "board.h"
#include <stdio.h>

void writeVTK2(const size_t timestep, const double *data, const char prefix[1024], const unsigned w, const unsigned h) {
    char filename[2048];
    unsigned x, y;

    unsigned offsetX = 0;
    unsigned offsetY = 0;
    float deltax = 1.0;
    float deltay = 1.0;
    unsigned nxy = w * h * sizeof(float);

    snprintf(filename, sizeof(filename), "%s-%05ld%s", prefix, timestep, ".vti");
    FILE *fp = fopen(filename, "w");

    fprintf(fp, "<?xml version=\"1.0\"?>\n");
    fprintf(fp, "<VTKFile type=\"ImageData\" version=\"0.1\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n");
    fprintf(fp, "<ImageData WholeExtent=\"%d %d %d %d %d %d\" Origin=\"0 0 0\" Spacing=\"%le %le %le\">\n", offsetX,
            offsetX + w - 1, offsetY, offsetY + h - 1, 0, 0, deltax, deltay, 0.0);
    fprintf(fp, "<CellData Scalars=\"%s\">\n", prefix);
    fprintf(fp, "<DataArray type=\"Float32\" Name=\"%s\" format=\"appended\" offset=\"0\"/>\n", prefix);
    fprintf(fp, "</CellData>\n");
    fprintf(fp, "</ImageData>\n");
    fprintf(fp, "<AppendedData encoding=\"raw\">\n");
    fprintf(fp, "_");
    fwrite((unsigned char *) &nxy, sizeof(long), 1, fp);

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            double value = data[calcIndex(h, x, y)];
            fwrite((unsigned char *) &value, sizeof(double), 1, fp);
        }
    }

    fprintf(fp, "\n</AppendedData>\n");
    fprintf(fp, "</VTKFile>\n");
    fclose(fp);
}
