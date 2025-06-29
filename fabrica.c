/*
Sistemas Operativos. 2025 
Nombres: Sebastian Gomez y Angelika Moya Bolaños
Especificaciones: Solo correr el archivo fabrica.c, los demás archivos (robots) serán ejecutados por el archivo fabrica.c. El archivo fabrica.c recibirá un número par como argumento y se encargará de crear los procesos necesarios para la simulación.
*/

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
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
#include <time.h>

#define SHM_NAME "/cinta_transportadora"
#define TAM_MEM sizeof(char) * 2

int main(int argc, char *argv[]){
  srand(time(NULL));
  // 1. Verificación de que si se pasen 2 argumentos
  if (argc != 2) {
    errno= EINVAL;
    perror("Error: se debe pasar un número par");
    return 1;
  }


  int fd[2];
  char *conjunto[] = {"AB", "AC", "BC"};
  const char *nomsemp = "/SEMP";

  // Definición de los semáforos
  sem_unlink("/SEMP");   // Borra cualquier rastro previo
  sem_t *semp = sem_open(nomsemp, O_CREAT, 0666, 1);
  if (semp == SEM_FAILED) {
    perror("Error al crear semáforo del productor");
    exit(1);
}

  sem_unlink("/SEMR2");   // Borra cualquier rastro previo
  sem_t *semr2 = sem_open("/SEMR2", O_CREAT, 0666, 0);
  if (semr2 == SEM_FAILED) {
    perror("Error al crear semáforo de robot 2");
    exit(1);
}

  sem_unlink("/SEMR3");
  sem_t *semr3 = sem_open("/SEMR3", O_CREAT, 0666, 0);
  if (semr3 == SEM_FAILED) {
    perror("Error al crear semáforo de robot 3");
    exit(1);
}

  sem_unlink("/SEMR1");
  sem_t *semr1 = sem_open("/SEMR1", O_CREAT, 0666, 0);
  if (semr1 == SEM_FAILED ) {
    perror("Error al crear semáforo de robot 1");
    exit(1);
}
  //  Crear memoria compartida
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

  // Crear el pipe de comunicación fábica-productor

  int pipefd[2];
  if (pipe(pipefd) <0) {
    perror("Error al crear la tubería");
    exit(1);
}

 // Creación de los pipe para pasar el numero de parejas desde los robots

  int pipe_r1[2];
  if (pipe(pipe_r1) <0) {
    perror("Error al crear la tubería para conectar al robot1");
    exit(1);
}
  int pipe_r2[2];
  if (pipe(pipe_r2) <0) {
    perror("Error al crear la tubería para conectar al robot2");
    exit(1);
}
  int pipe_r3[2];
  if (pipe(pipe_r3) <0) {
    perror("Error al crear la tubería para conectar al robot1");
    exit(1);
}

    // Creación del proceso productor
  pid_t pid = fork();


  if(pid < 0){
    perror("Error al crear proceso\n");
    return 3;
  }
  else if(pid == 0){
    // Tubería
    close(pipefd[1]);

    int N;

    // Leer N
    if (read(pipefd[0], &N, sizeof(int)) != sizeof(int)) {
      perror("Error leyendo N desde la tubería");
      exit(1);
      }


    close(pipefd[0]);  // Cerrar el extremo de lectura

    printf("Proceso productor: N = %d\n", N);


    for (int i = 0; i < N; i++) {
      const char *producto = conjunto[rand() % 3];
      sem_wait(semp);
      strncpy(cinta, producto, 2);  // Escribe en memoria compartida
      printf("Producido: %s\n", producto);
      fflush(stdout);
   // Verificación de cual robot le corresponde para despertarlo
      if (strcmp(producto, "AB") == 0)
        sem_post(semr1);

      else if (strcmp(producto, "AC") == 0)
        sem_post(semr2);

      else
  sem_post(semr3);
    }
      sem_wait(semp);
      strncpy(cinta, "ZZ", 2); //Finalizado
      printf("Producido: ZZ\n");
      fflush(stdout);


        // Despertar a todos para que se cierren
    sem_post(semr1);
    sem_post(semr2);
    sem_post(semr3);


  }

   else {

      // Cierre del extremo de lectura
    close(pipefd[0]);
    int N = atoi(argv[1]);

      // 2. Verificación que el número sea par
    if (N <=0 || N%2 != 0) {
      errno= EINVAL;
      perror("Error, el número debe ser par");
      return 1;
 }

    // Escribir N
    if (write(pipefd[1], &N, sizeof(int)) != sizeof(int)) {
      perror("Error escribiendo N en la tubería");
      exit(1);
    }

      close(pipefd[1]);
      //Padre
    pid_t r1 = fork();
    if (r1 == 0) {
      close(pipe_r1[0]);
      char fd_str[10];
      sprintf(fd_str, "%d", pipe_r1[1]);
      execl("./robot1", "robot1", fd_str, NULL);
      perror("Error ejecutando robot1");
      exit(1);
    }
    close(pipe_r1[1]);

    pid_t r2 = fork();
    if (r2 == 0) {
      close(pipe_r2[0]);
      char fd_str[10];
      sprintf(fd_str, "%d", pipe_r2[1]);
      execl("./robot2", "robot2", fd_str, NULL);
      perror("Error ejecutando robot2");
      exit(1);
    }
    close(pipe_r2[1]);

    pid_t r3 = fork();
    if (r3 == 0) {
      close(pipe_r3[0]);
      char fd_str[10];
      sprintf(fd_str, "%d", pipe_r3[1]);
      execl("./robot3", "robot3", fd_str, NULL);
      perror("Error ejecutando robot3");
      exit(1);
    }
    close(pipe_r3[1]);

    //  Esperas a que todos terminen
    for (int i = 0; i < 4; i++) wait(NULL); // productor + 3 robots

    //  Luego lees desde las tuberías de los robots

    int cp1, cp2, cp3;
    read(pipe_r1[0], &cp1, sizeof(int));
    read(pipe_r2[0], &cp2, sizeof(int));
    read(pipe_r3[0], &cp3, sizeof(int));

    printf("Robot 1 empaquetó %d productos\n", cp1);
    printf("Robot 2 empaquetó %d productos\n", cp2);
    printf("Robot 3 empaquetó %d productos\n", cp3);
      // Liberar recursos
      sem_close(semp); sem_unlink("/SEMP");
      sem_close(semr1); sem_unlink("/SEMR1");
      sem_close(semr2); sem_unlink("/SEMR2");
      sem_close(semr3); sem_unlink("/SEMR3");
      munmap(cinta, TAM_MEM);
      close(shm_fd);
      shm_unlink(SHM_NAME);
  }

  return 0;
}