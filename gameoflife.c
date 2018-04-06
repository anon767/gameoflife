
#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <mpi.h>
#include <assert.h>

#define calcIndex(width, x, y)  ((y)*(width) + (x))
#define MESSAGE_INIT 0
#define MESSAGE_SEGMENTSEND 1

long TimeSteps = 500;

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
            float value = data[calcIndex(h, x, y)];
            fwrite((unsigned char *) &value, sizeof(float), 1, fp);
        }
    }

    fprintf(fp, "\n</AppendedData>\n");
    fprintf(fp, "</VTKFile>\n");
    fclose(fp);
}


void show(const double *currentfield, int w, int h) {
    printf("\033[H");
    for (unsigned y = 0; y < h; y++) {
        for (unsigned x = 0; x < w; x++) printf(currentfield[calcIndex(w, x, y)] ? "\033[07m  \033[m" : "  ");
        printf("\n");
    }
    fflush(stdout);
}

void
evolve(const double *currentfield, double *newfield, const unsigned startw, const unsigned starth, const unsigned w,
       const unsigned h, int procID, int procNum) {
    for (unsigned y = starth; y < h; y++) {
        for (unsigned x = startw; x < w; x++) {
            if (!((x > 0) && (x < w - 1)))
                continue; //skip edges
            double sum = currentfield[calcIndex(w, x - 1, y - 1)]
                         + currentfield[calcIndex(w, x, y - 1)]
                         + currentfield[calcIndex(w, x - 1, y)]
                         + currentfield[calcIndex(w, x + 1, y + 1)]
                         + currentfield[calcIndex(w, x + 1, y)]
                         + currentfield[calcIndex(w, x, y + 1)]
                         + currentfield[calcIndex(w, x - 1, y + 1)]
                         + currentfield[calcIndex(w, x + 1, y - 1)];
            double currentcell = currentfield[calcIndex(w, x, y)];

            if (currentcell == 0 && sum == 3)
                newfield[calcIndex(w, x, y)] = 1;
            else if (currentcell == 1 && sum < 2)
                newfield[calcIndex(w, x, y)] = 0;
            else if (currentcell == 1 && sum > 3)
                newfield[calcIndex(w, x, y)] = 0;
            else if (currentcell == 1 && (sum == 2 || sum == 3))
                newfield[calcIndex(w, x, y)] = currentcell;

        }
    }
}

void filling(double *currentfield, const unsigned w, const unsigned h) {
    for (size_t i = 0; i < h * w; i++) {
        currentfield[i] = (rand() < RAND_MAX / 5) ? 1 : 0; ///< init domain randomly
    }
    for (size_t y = 0; y < h; y++) {
        for (size_t x = 0; x < w; x++) {
            if (!((x > 0) && (x < w - 1) && (y > 0) && (y < h - 1)))
                currentfield[calcIndex(w, x, y)] = 0;
        }
    }
}

void game(const unsigned w, const unsigned h, const unsigned int procID, const unsigned int procNum) {
    double *currentfield = calloc(w * h, sizeof(double));
    double *newfield = calloc(w * h, sizeof(double));
    filling(currentfield, w, h);
    MPI_Status status;

    MPI_Comm comm = setUpCommunicator();

    int segmentSize = (w * h) / (procNum - 1);


    for (unsigned int t = 0; t < TimeSteps; t++) {

        if (procID == 0) {


            for (unsigned int i = 1; i < procNum; i++) {
                int modifierSize = (i > 1) ? 2 * w : 0; //send n+1 if procNum not the last
                double *segment = malloc((modifierSize + segmentSize) * sizeof(double));
                memcpy(segment, currentfield + (segmentSize) * (i - 1) - modifierSize, (modifierSize + segmentSize) * sizeof(double));
                MPI_Send(segment, modifierSize + segmentSize, MPI_DOUBLE, i, MESSAGE_INIT, comm);
            }


            for (int i = 1; i < procNum; i++) {
                int modifierSize = (i > 1) ? 1 * w : 0; //send n+1 if procNum not the last

                MPI_Recv(newfield + segmentSize * (i - 1) - modifierSize, (segmentSize + modifierSize) * sizeof(double),
                         MPI_DOUBLE, i,
                         MESSAGE_SEGMENTSEND,
                         comm, &status);

            }

            memcpy(currentfield, newfield, w * h * sizeof(double));
            show(currentfield, w, h);
            usleep(100000);
        } else {
            int modifierSize = (procID > 1) ? 2 * w : 0;
            int modifierSendSize = (procID > 1 ) ? w : 0;
            int modifierOffset = (procID > 1 ) ? 1 : 0;
            double *myField = calloc(modifierSize + segmentSize, sizeof(double));
            double *tempmynewfield = calloc(modifierSize + segmentSize, sizeof(double));
            MPI_Recv(myField, modifierSize + segmentSize, MPI_DOUBLE, 0, MESSAGE_INIT, comm, &status);

            evolve(myField, tempmynewfield, 0, 1, w, modifierOffset+h / (procNum - 1), procID, procNum);

            MPI_Send(tempmynewfield + modifierSendSize,  modifierSendSize + segmentSize, MPI_DOUBLE, 0, MESSAGE_SEGMENTSEND, comm);
        }


    }





//       writeVTK2(t, currentfield, "gol", w, h);

    free(currentfield);
    free(newfield);


}

int main(int c, char **v) {
    int procID, size;
    MPI_Init(&c, &v);

    MPI_Comm_rank(MPI_COMM_WORLD, &procID);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (procID == 0)
        printf("initialized with %d procs\n", size);
    if (size <= 1) {
        printf("you should start with more than one process\n");
        exit(0);
    }

    unsigned int w = 0, h = 0;
    if (c > 1) w = atoi(v[1]);
    if (c > 2) h = atoi(v[2]);
    if (w <= 0) w = 30;
    if (h <= 0) h = 30;

    game(w, h, procID, size);
    MPI_Finalize();
    return 0;
}


