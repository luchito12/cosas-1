                                              ///////////////////////////PLANIFICADOR LARGO PLAZO//////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
                                               
#define MAX_PROCESOS 100
#define MAX_PRIORIDAD 10

//creo colas 
Hilo colaReady[MAX_HILOS];
Hilo colaNew[MAX_HILOS];
Hilo colaExit[MAX_HILOS];
Hilo colaWaiting[MAX_HILOS];

//inicializo colas 
int indiceReady = 0;
int indiceNew = 0;
int indiceExit = 0;
int indiceWaiting = 0;

//defino estructura hilo y proceso
typedef struct {
    uint32_t tid;
    uint32_t estado; // 0: READY, 1: RUNNING, 2: WAITING, 3: TERMINATED
    uint32_t prioridad;
} Hilo;

typedef struct {
    uint32_t pid;
    uint32_t estado; // 0: NEW, 1: READY, 2: RUNNING, 3: WAITING, 4: TERMINATED
} Proceso;

//inicializo la cola de procesos en new 
Proceso colaNew[MAX_PROCESOS];
int indiceNew = 0;

//inicializo la cola de los hilos en ready 
Hilo colaReady[MAX_HILOS];
int indiceReady = 0;

//antes de inicializar un proceso 
bool verificarMemoriaDisponible() {
    // Simulación de verificación de memoria disponible
    // A ca implementar la lógica real para verificar la memoria
    return true; // Asumimos que siempre hay memoria disponible para simplificar
}

//iniciamos el proceso 
void inicializarProceso(Proceso *proceso) {
    if (verificarMemoriaDisponible()) {
        proceso->estado = 1; // READY
        printf("Proceso %d inicializado y en estado READY\n", proceso->pid);
    } else {
        printf("Memoria insuficiente para inicializar el proceso %d\n", proceso->pid);
    }
}

//liberamos memoria 
void liberarMemoria(Proceso *proceso) {
    // Simulación de liberación de memoria
    // Aca puedes implementar la lógica real para liberar la memoria
    printf("Memoria del proceso %d liberada\n", proceso->pid);
}


//finalizar proceso 
void finalizarProceso(Proceso *proceso) {
    liberarMemoria(proceso);
    proceso->estado = 4; // TERMINATED
    printf("Proceso %d finalizado\n", proceso->pid);

    // Intentar inicializar el siguiente proceso en la cola NEW si existe
    if (indiceNew > 0) {
        inicializarProceso(&colaNew[0]);
        // Mover todos los procesos en la cola NEW hacia adelante
        for (int i = 1; i < indiceNew; i++) {
            colaNew[i - 1] = colaNew[i];
        }
        indiceNew--;
    }
}

//creacion de proceso 
void crearProceso(uint32_t pid) {
    Proceso nuevoProceso = {pid, 0}; // NEW
    colaNew[indiceNew++] = nuevoProceso;
    printf("Proceso %d creado y encolado en NEW\n", pid);

    if (indiceNew == 1) {
        inicializarProceso(&colaNew[0]);
    }
}



void *funcionHilo(void *arg) {
    uint32_t tid = *(uint32_t *)arg;
    printf("Hilo %d ejecutándose\n", tid);
    // Simulación de trabajo del hilo
    sleep(1);
    pthread_exit(NULL);
}

//creacion de hilos
void crearHilo(uint32_t tid, uint32_t prioridad) {
    pthread_t hilo;
    Hilo nuevoHilo = {tid, 0, prioridad}; // READY
    colaReady[indiceReady++] = nuevoHilo;

    if (pthread_create(&hilo, NULL, funcionHilo, &tid) != 0) {
        perror("Error al crear el hilo");
        exit(EXIT_FAILURE);
    }

    printf("Hilo %d creado y en estado READY\n", tid);
}


//finalizar hilo
void finalizarHilo(Hilo *hilo) {
    hilo->estado = 3; // TERMINATED
    colaExit[indiceExit++] = *hilo;
    printf("Hilo %d finalizado y movido a EXIT\n", hilo->tid);

    // Mover al estado READY a los hilos bloqueados por este TID
    for (int i = 0; i < indiceWaiting; i++) {
        if (colaWaiting[i].estado == 2) { // WAITING
            colaWaiting[i].estado = 0; // READY
            colaReady[indiceReady++] = colaWaiting[i];
            printf("Hilo %d desbloqueado y en estado READY\n", colaWaiting[i].tid);
        }
    }
}


//mover hilo a blocked
void moverHiloAWaiting(Hilo *hilo) {
    hilo->estado = 2; // WAITING
    colaWaiting[indiceWaiting++] = *hilo;
    printf("Hilo %d movido a WAITING\n", hilo->tid);
}


////////////////////////////////////////////////////////////////////////////////////////BOSQUEJO DE FUNCIONES PARA PLANIFICACION CORTO PLAZO///////////////////////////////


//DEFINICION DE ESTRUCTURAS 
typedef struct {
    int pid;
    int tid;
    int prioridad;
    int estado; // 0: READY, 1: EXEC, 2: BLOCKED
} Proceso;

typedef struct {
    Proceso procesos[MAX_PROCESOS];
    int count;
} Cola;

Cola listoParaEncolar;
Cola prioridadDeLAcola[MAX_PRIORIDAD];
int quantum[MAX_PRIORIDAD];


//FUNCIONES DE COLAS 
void inicializarColas() {
    listoParaEncolar.count = 0;
    for (int i = 0; i < MAX_PRIORIDAD; i++) {
        prioridadDeLAcola[i].count = 0;
        quantum[i] = 5; // Ejemplo de quantum, puede ser configurado
    }
}

void agregarProceso(Cola *cola, Proceso proceso) {
    cola->procesos[cola->count++] = proceso;
}


//FUNCIONES PARA ALGORITMOS 
Proceso obtenerSiguienteProcesoFIFO() {
    return listoParaEncolar.procesos[0];
}

Proceso obtenerSiguienteProcesoPrioridad() {
    for (int i = 0; i < MAX_PRIORIDAD; i++) {
        if (prioridadDeLAcola[i].count > 0) {
            return prioridadDeLAcola[i].procesos[0];
        }
    }
    // Si no hay procesos, devolver un proceso vacío
    Proceso p = {0, 0, 0, 0};
    return p;
}

Proceso obtenerSiguienteProcesoColasMultinivel() {
    for (int i = 0; i < MAX_PRIORIDAD; i++) {
        if (prioridadDeLAcola[i].count > 0) {
            return prioridadDeLAcola[i].procesos[0];
        }
    }
    // Si no hay procesos, devolver un proceso vacío
    Proceso p = {0, 0, 0, 0};
    return p;
}



//EJECUTAR UN PROCESO 
void ejecutarProceso(Proceso proceso) {
    printf("Ejecutando proceso: PID=%d, TID=%d\n", proceso.pid, proceso.tid);
    // Simulación de ejecución
}

/* Ejemplo de procesos
    Proceso p1 = {1, 101, 2, 0};
    Proceso p2 = {2, 102, 1, 0};
    Proceso p3 = {3, 103, 3, 0};

    agregarProceso(&readyQueue, p1);
    agregarProceso(&priorityQueues[p2.prioridad], p2);
    agregarProceso(&priorityQueues[p3.prioridad], p3);

    Ejemplo de selección y ejecución de procesos
    Proceso siguienteProceso = obtenerSiguienteProcesoPrioridad();
    ejecutarProceso(siguienteProceso); 
   */


