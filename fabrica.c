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


#define SHM_NAME "/cinta_transportadora"
#define TAM_MEM sizeof(char) * 2

int main(int argc, char *argv[]){
    // 1. Verificación de que si se pasen 2 argumentos
    if (argc != 2) {
        errno= EINVAL;
        perror("Error: se debe pasar un número par");
        return 1;
  }


    int N = atoi(argv[1]);

    // 2. Verificación que el número sea par
    if (N <=0 || N%2 != 0) {
        errno= EINVAL;
        perror("Error, el número debe ser par");
        return 1;
}
    printf("N recibido correctamente: %d\n", N);

  int fd[2];
  char *conjunto[] = {"AB", "AC", "BC"};

    // 3. Crear memoria compartida
  int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
  if (shm_fd == -1) {
      perror("Error al crear memoria compartida");
      return 2;
  }

  // Ajustar tamaño de la memoria
  if (ftruncate(shm_fd, TAM_MEM) == -1) {
      perror("Error al configurar tamaño de memoria");
      return 3;
  }

  char *cinta = mmap(NULL, TAM_MEM, PROT_READ | PROT_WRITE,   MAP_SHARED, shm_fd, 0);
  if (cinta == MAP_FAILED) {
      perror("Error al mapear memoria");
      return 4;
  }

  //3. Crear nuevo proceso
  if(pipe(fd) < 0){
    printf("Error al crear pipe\n");
    return 1;
  }

  pid_t pid = fork();

  if(pid < 0){
    perror("Error al crear proceso\n");
    return 3;
  } else if(pid == 0){
    //Hijo - Proceso productor
    for (int i = 0; i < N; i++) {
      const char *producto = conjunto[rand() % 3];
      strncpy(cinta, producto, 2);  // Escribe en memoria compartida
      printf("Producido: %s\n", producto);
      sleep(1);
    }
    strncpy(cinta, "ZZ", 2); //Finalizado
    printf("Producido: ZZ\n");
  } else {
    //Padre
    wait(NULL);
    // Liberar recursos
    munmap(cinta, TAM_MEM);
    close(shm_fd);
    shm_unlink(SHM_NAME);
  }

  return 0;
}
