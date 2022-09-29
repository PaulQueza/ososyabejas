#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// Recipiente de Miel
int *recipienteMiel, *interruptor;
int MAXABEJAS, MAXOSOS, MAXBUFFER, rangoPorciones, porcentajeRecipiente,
    porcionComida;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t *sem_abeja, *sem_oso;
void *crear_memoria_compartida(size_t tamano) {
  return mmap(NULL, tamano, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS,
              -1, 0);
}
void *oso() {
  if (recipienteMiel[0] > 0 && recipienteMiel[0] <= MAXBUFFER) {
    porcentajeRecipiente = (recipienteMiel[0] * 100) / MAXBUFFER;
    if (porcentajeRecipiente <= 20) {
      porcionComida = (recipienteMiel[0] - 1);
      recipienteMiel[0] = 0;
    } else {
      porcionComida = rand() % rangoPorciones;
      recipienteMiel[0] -= 1 + porcionComida;
    }
    printf("El oso [pid:%d] comio una porcion de %d[Porcion de miel], cantidad "
           "de miel en el recipiente [%d] \n",
           getpid(), (porcionComida + 1), recipienteMiel[0]);
    if (recipienteMiel[0] <= 0) {
      printf("Oso [pid:%d], avisa a las abejas que se acabo la miel \n",
             getpid());
    }
  }
  return 0;
}

void *abeja() {
  // Si el recipiente esta vacio, entonces la abeja pondra una porcion en el
  // buffer
  if (recipienteMiel[0] < MAXBUFFER) {
    recipienteMiel[0] += 1;
    printf("La abeja [pid:%d] produjo una porcion de 1(Porcion de miel), "
           "cantidad de miel en el recipiente [%d] \n",
           getpid(), recipienteMiel[0]);
    if (recipienteMiel[0] >= MAXBUFFER) {
      printf("Abeja [pid:%d], desperto a los OSOS \n", getpid());
    }
  }
  return 0;
}

void funcErr(char *err) {
  // Funcion que indica que hubo un error con la creacion de algun hilo
  perror(err);
  exit(1);
}

int main(int argc, char *argv[]) {
  // No se reciven argumentos por argv[]
  printf("Ingrese la cantidad de abejas: \n");
  scanf("%d", &MAXABEJAS);
  printf("Ingrese la cantidad de osos: \n");
  scanf("%d", &MAXOSOS);
  printf("Ingrese la cantidad del recipiente: \n");
  scanf("%d", &MAXBUFFER);
  if (MAXABEJAS > 0 && MAXOSOS > 0 && MAXBUFFER > 0) {
    rangoPorciones = MAXBUFFER / MAXOSOS;
    if (MAXBUFFER < MAXOSOS) {
      printf("No pueden haber más osos que la cantidad de porciones, debido a "
             "que ningun oso puede quedar con hambre \n");
      exit(1);
    }
    printf("---------------------------------------------------\n");
    printf("|                   INFROMACION                   |\n");
    printf("|                   N° Abejas: %d                 |\n", MAXABEJAS);
    printf("|                   N° Osos: %d                   |\n", MAXOSOS);
    printf("|                   Recipiente: %d                |\n", MAXBUFFER);
    printf("|              Rango de porciones: %d              |\n",
           rangoPorciones);
    printf("---------------------------------------------------\n");
    sleep(1);
  } else {
    printf("Los valores no pueden ser nulos o negativos");
    exit(0);
  }
  int pid1, pid2;
  printf("Creando el buffer ... \n");
  recipienteMiel = crear_memoria_compartida(1);
  recipienteMiel[0] = 0;
  printf("¡Listo! \n");
  sleep(1);
  printf("Creando los semaforos ... \n");
  sem_abeja = crear_memoria_compartida(sizeof(sem_t));
  sem_oso = crear_memoria_compartida(sizeof(sem_t));
  int *bandera = crear_memoria_compartida(1);
  bandera[0] = 0;
  sem_init(sem_abeja, 1, 1);
  sem_init(sem_oso, 1, 0);
  printf("Iniciando los semaforos(MUTEX) ... \n");
  sleep(1);
  printf("¡Listo! \n");
  sleep(1);
  // Creacion de las abejas
  printf("Creando las abejas ... \n");
  for (int i = 0; i < MAXABEJAS; i++) {
    pid1 = fork();
    if (pid1 < 0) {
      funcErr("Se produjo un error al crear las abejas (PROBLEM PROCESS)");
    } else if (pid1 == 0) {
      printf("Abeja creada [pid: %d],¡Listo! \n", getpid());
      break;
    }
  }
  if (pid1 > 0) {
    sleep(1);
    printf("¡Listo! \n");
    printf("Creando los osos ... \n");
    for (int i = 0; i < MAXOSOS; i++) {
      pid2 = fork();
      if (pid2 < 0) {
        funcErr("Se produjo un error al crear las abejas (PROBLEM PROCESS)");
      } else if (pid2 == 0) {
        printf("Oso creado [pid: %d],¡Listo! \n", getpid());
        break;
      }
    }
    if (pid2 > 0) {
      sleep(1);
      printf("¡Listo! \n");
    }
  }
  // Espera a que se creen todos los procesos
  sleep(2);
  while (1) {
    sleep(1);
    usleep(rand() % 20000);
    if (pid1 == 0) {
      if (bandera[0] == 0 && recipienteMiel[0] <= MAXBUFFER) {
        // Bloqueamos la abeja hasta que se ejecuten las funciones(PRODUCIR)
        sem_wait(sem_abeja);
        abeja();
        bandera[0] = 0;
        if (recipienteMiel[0] >= MAXBUFFER) {
          // Si el recipiente se lleno, entonces liberamos a los osos, y no
          // liberamos a las abejas(para que no produscan miel)
          bandera[0] = 1;
          sem_post(sem_oso);
        } else {
          // Si el recipiente no esta vacio, liberamos a la abeja para que otra
          // abeja pueda producir
          sem_post(sem_abeja);
        }
      }
    }
    if (pid2 == 0) {
      if (bandera[0] == 1 && recipienteMiel[0] >= 0) {
        // Bloqueamos el oso hasta que se ejecuten las funciones(COMER)
        sem_wait(sem_oso);
        oso();
        bandera[0] = 1;
        if (recipienteMiel[0] <= 0) {
          // Si el recipiente esta vacio, entonces liberamos a las abejas para
          // que puedan producir y no liberamos a los osos para que no puedan
          // consumir
          bandera[0] = 0;
          sem_post(sem_abeja);
        } else {
          // Si el recipiente todabia no esta vacio, liberamos a el oso para que
          // otro pueda consumir
          sem_post(sem_oso);
        }
      }
    }
    sleep(1);
  }
  return 0;
}
