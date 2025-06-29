#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <errno.h>
#include <semaphore.h>
#define SHM_NAME "/cinta_transportadora"
#define TAM_MEM sizeof(char) * 2
#define SEM_P "/SEMP"
#define SEM_R1 "/SEMR1"
#define PRODUCTO "AB"
#define ROBOT "Robot 1"
int main(int argc, char *argv[]) {
    // Apertura y verificación de semáforos
  sem_t *semp = sem_open(SEM_P, 0);
  if (semp == SEM_FAILED) {
      perror("Error abriendo semp");
      exit(1);
    }

    sem_t *semr1 = sem_open(SEM_R1, 0);
    if (semr1 == SEM_FAILED) {
        perror("Error abriendo semr1");
        exit(1);
    }


    // Apertura de memoria compartida
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Error abriendo memoria compartida");
        exit(1);
    }

    char *cinta = mmap(NULL, TAM_MEM, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (cinta == MAP_FAILED) {
        perror("Error mapeando memoria compartida");
        exit(1);
    }
    int fd_pipe = atoi(argv[1]);
    int cp = 0;
    while (1) {
      sem_wait(semr1);  // Espera su turno

      if (strncmp(cinta, "ZZ", 2) == 0){
        write(fd_pipe, &cp, sizeof(int));
        close(fd_pipe);
        break;
}

      if (strncmp(cinta, PRODUCTO, 2) == 0) {
        printf(ROBOT " empaqueta productos " PRODUCTO "\n");
        fflush(stdout);
        strncpy(cinta, "--", 2);
        cp++;
	sem_post(semp);  // Libera al productor
        }
    }


   munmap(cinta, TAM_MEM);
   close(shm_fd);
   sem_close(semp);
   sem_close(semr1);
   return 0;
}