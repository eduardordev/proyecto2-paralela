#include <mpi.h>
#include <openssl/des.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

void encrypt(unsigned char *key, unsigned char *text, int len) {
    DES_key_schedule schedule;
    DES_key_sched((DES_cblock *)&key, &schedule);

    for (int i = 0; i < len; i += 8) {
        DES_ecb_encrypt((DES_cblock *)(text + i), (DES_cblock *)(text + i), &schedule, DES_ENCRYPT);
    }
}

void decrypt(unsigned char *key, unsigned char *text, int len) {
    DES_key_schedule schedule;
    DES_key_sched((DES_cblock *)&key, &schedule);

    for (int i = 0; i < len; i += 8) {
        DES_ecb_encrypt((DES_cblock *)(text + i), (DES_cblock *)(text + i), &schedule, DES_DECRYPT);
    }
}

int main(int argc, char *argv[]) {
    int rank, numprocs;
    unsigned char key[8], text[64], decrypted_text[64];
    FILE *file;
    double start_time, end_time;
    MPI_Status status;
    int flag = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

    if (rank == 0) {
        file = fopen(argv[1], "r");
        if (file == NULL) {
            printf("Error opening file.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        fread(text, 1, 64, file);
        fclose(file);
        for (int i = 1; i < numprocs; i++) {
            MPI_Send(text, 64, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD);
        }
    } else {
        MPI_Recv(text, 64, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, &status);
    }

    start_time = MPI_Wtime();

    for (unsigned long long i = (0xFFFFFFFFFFFFFFUL / numprocs) * rank; i < (0xFFFFFFFFFFFFFFUL / numprocs) * (rank + 1); i++) {
        for (int j = 0; j < 8; j++) {
            key[j] = (i >> (8 * j)) & 0xFF;
        }

        memcpy(decrypted_text, text, 64);
        encrypt(key, decrypted_text, 64);
        decrypt(key, decrypted_text, 64);

        if (memcmp(text, decrypted_text, 64) == 0) {
            end_time = MPI_Wtime();
            printf("%d : Encrypted phrase: %s\n", rank, text);
            printf("================================================\n");
            printf("%d : Decrypted phrase: %s\n", rank, decrypted_text);
            printf("================================================\n");
            printf("%d : Phrase found successfully\n", rank);
            printf("================================================\n");
            printf("%d : Runtime: %f secs\n", rank, end_time - start_time);
            printf("================================================\n");

            for (int i = 0; i < numprocs; i++) {
                if (i != rank) {
                    MPI_Send(&flag, 1, MPI_INT, i, 1, MPI_COMM_WORLD);
                }
            }
            break;
        }

        MPI_Iprobe(MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &flag, &status);
        if (flag) {
            break;
        }
    }

    MPI_Finalize();
    return 0;
}
