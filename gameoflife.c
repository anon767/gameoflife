
#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>
#include <mpi.h>

#define calcIndex(width, x, y)  ((y)*(width) + (x))
#define MESSAGE_INIT 0

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
            offsetX + w - 1, offsetY, offsetY + h - 1, 0, 0, deltax, deltax, 0.0);
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
       const unsigned h) {
    for (unsigned y = starth; y < h; y++) {
        for (unsigned x = startw; x < w; x++) {
            if (!((x > 0) && (x < w - 1) && (y > 0) && (y < h - 1)))
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
    MPI_Comm comm = setUpCommunicator();

    int segmentSize = ((h) / procNum) * w;
    for (unsigned int t = 0; t < TimeSteps; t++) {


        if (procID == 0) {

            void *sendbuf;
            MPI_Status status;
            int *rcounts = malloc(procNum * sizeof(int));
            for (unsigned int i = 0; i < procNum; i++) {
                rcounts[i] = w;
            }

            for (unsigned int i = 1; i < procNum; i++) {
                double *segment = malloc(segmentSize * sizeof(double));
                memcpy(segment, currentfield + segmentSize * i, segmentSize);
                MPI_Send(&(segment[calcIndex(w, 0, 0)]), segmentSize, MPI_DOUBLE, i, MESSAGE_INIT, comm);
            }
            double *myField = calloc(segmentSize, sizeof(double));
            double *tempmynewfield = calloc(segmentSize, sizeof(double));
            //MPI_Recv(myField, segmentSize, MPI_DOUBLE, 0, MESSAGE_INIT, comm, &status);

            evolve(myField, tempmynewfield, 0, 0, w, h / procNum);

            MPI_Gather(tempmynewfield, w, MPI_DOUBLE, newfield, w,
                       MPI_DOUBLE, 0,
                       comm);
            memcpy(newfield, currentfield, w * h * sizeof(double));
           // show(currentfield, w, h);
            usleep(200000);
        } else {
            double *myField = calloc(segmentSize, sizeof(double));
            double *newfield = calloc(segmentSize, sizeof(double));
            void *recvbuf;
            MPI_Status status;
            MPI_Recv(myField, segmentSize, MPI_DOUBLE, 0, MESSAGE_INIT, comm, &status);
            printf("received");
            evolve(myField, newfield, 0, 0, w, h / procNum);
            MPI_Gather(newfield, w, MPI_DOUBLE, recvbuf, procNum, MPI_DOUBLE,
                       0, comm);
        }


    }





/**        writeVTK2(t, currentfield, "gol", w, h);



    usleep(100000);
    //SWAP
    double *temp = currentfield;
    currentfield = newfield;
    newfield = temp;


    free(currentfield);
    free(newfield);
    **/

}

int main(int c, char **v) {
    int procID, size;
    MPI_Init(&c, &v);

    MPI_Comm_rank(MPI_COMM_WORLD, &procID);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (procID == 0)
        printf("initialized with %d procs\n", size);
    unsigned int w = 0, h = 0;
    if (c > 1) w = atoi(v[1]);
    if (c > 2) h = atoi(v[2]);
    if (w <= 0) w = 40;
    if (h <= 0) h = 40;
    game(w, h, procID, size);
    MPI_Finalize();
    return 0;
}


