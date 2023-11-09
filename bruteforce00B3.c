#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <openssl/des.h>

void encrypt(long key, char *text, int len) {
    DES_cblock des_key;
    DES_key_schedule schedule;
    DES_key_sched((DES_cblock *)&key, &schedule);

    for (int i = 0; i < len; i += 8) {
        DES_ecb_encrypt((DES_cblock *)(text + i), (DES_cblock *)(text + i), &schedule, DES_ENCRYPT);
    }
}

void decrypt(long key, char *text, int len) {
    DES_cblock des_key;
    DES_key_schedule schedule;
    DES_key_sched((DES_cblock *)&key, &schedule);

    for (int i = 0; i < len; i += 8) {
        DES_ecb_encrypt((DES_cblock *)(text + i), (DES_cblock *)(text + i), &schedule, DES_DECRYPT);
    }
}

int main(int argc, char *argv[]) {
    int N, id;
    double start_time, end_time;
    double serial_runtime, parallel_runtime;

    MPI_Init(NULL, NULL);
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Comm_size(comm, &N);
    MPI_Comm_rank(comm, &id);

    char text[1024];
    char filename[256];
    long encryption_key;

    if (argc != 3) {
        if (id == 0) {
            printf("Usage: %s <filename> <encryption_key>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    strcpy(filename, argv[1]);
    encryption_key = atol(argv[2]);

    start_time = MPI_Wtime();

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Failed to open the file");
        MPI_Abort(comm, 1);
    }
    int text_len = fread(text, 1, sizeof(text), file);
    fclose(file);

    encrypt(encryption_key, text, text_len);

    printf("================================================\n");
    printf("%d : Encrypted phrase: %s\n", id, text);

    decrypt(encryption_key, text, text_len);

    printf("================================================\n");
    printf("%d : Decrypted phrase: %s\n", id, text);
    printf("================================================\n");

    if (strstr(text, "probando") != NULL) {
        printf("%d : frase encontrada con exito\n", id);
    } else {
        printf("%d : No se encontro la frase en el texto\n", id);
    }

    end_time = MPI_Wtime();
    printf("%d : Run time: %f segs\n", id, end_time - start_time);

    MPI_Barrier(comm);

    if (id == 0) {
        serial_runtime = end_time - start_time;
        parallel_runtime = MPI_Wtime() - start_time;

        MPI_Bcast(&serial_runtime, 1, MPI_DOUBLE, 0, comm);

        double speedup = serial_runtime / parallel_runtime;
        printf("================================================\n");
        printf("Speedup time: %f\n", speedup);
    }

    MPI_Finalize();
}
