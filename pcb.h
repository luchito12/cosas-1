#ifndef PCB_H_
#define PCB_H_

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <stdint.h>
#include <semaphore.h>
#include <stdbool.h>


#define MAX_PROCESOS 100
#define MAX_PRIORIDAD 10
#define MAX_MUTEX 100
#define MAX_TCB 100
#define MAX_PCB 100
#define MAX_TID 100
typedef enum {
    NEW,
    READY,
    EXECUTE,
    BLOCKED,
    EXIT,
    BLOQUEADO_POR_MUTEX
}t_estado;

typedef enum{
    PTHREAD_JOIN,
    MUTEX,
    IO
}t_motivo_de_bloqueo;

// - Busca un pcb mediante el pid en una lista y lo remueve
// - Retorna NULL en caso de no encontrar el pid en la lista
//t_PCB* buscar_pcb_por_pid_y_remover(int pid, t_list* lista);
// - Busca un pcb mediante el pid en una lista y lo obtiene
// - Retorna NULL en caso de no encontrar el pid en la lista
//t_PCB* buscar_pcb_por_pid_y_obtener(int pid, t_list* lista);
// - Busca un pcb mediante el pid en una lista y devuelve su posicion en la lista
// - Retorna (-1) en caso de no encontrar el pid en la lista
//int posicion_de_pcb_por_pid(int pid, t_list* lista);


/////////////////////////////// Adminitracion de Estructuras ///////////////////////////////////
//PCB* crearPCB(int pid, char *nombreArchivo, int tamanoMemoria, int prioridad);
//void finalizarPCB(PCB *pcb);
//TCB* crearTCB(int tid, int pid, char *nombreArchivo, int prioridad);
//void finalizarTCB(TCB *tcb) ;
//////////////////////////////////////////////////////////////////////////////




/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Definición de estructuras
//DECLARO LA ESTRUCTURA DEL PBC
typedef struct{
    uint32_t pid;
    char* archivoDePseudocodigo;
    t_list* listaDeHilosDelProceso;
    t_list* listaDeMutexDelProceso;
    t_estado estadoDelProceso;
    uint32_t tamanioMemoria;
} PCB;

//DECLARO LA ESTRUCTURA DEL TCB 
typedef struct{
    uint32_t tid;
    uint32_t procesoPadre;
    t_estado estadoDelHilo;  // 0: NEW, 1: READY, 2: EXEC, 3: BLOCKED, 4: EXIT
    uint32_t prioridad;
    char nombreArchivo[256];

} TCB;





// DECLARO LA ESTRUCTURA DEL MUTEX 
typedef struct{
    char* mutexId;
    t_list* hilosBloqueados;
    t_tcb* tomadoPor; //antes lo haciamos con un bool en pcb.c quedo con bool
} Mutex;


//FUNCIONES PARA MUTEX  
typedef struct {
    Mutex mutexes[MAX_MUTEX];
    int count;
} SistemaMutex;

SistemaMutex sistemaMutex;

void inicializarSistemaMutex() {
    sistemaMutex.count = 0;
}



//FUNCIONES PCB 
typedef struct {
    PCB pcbs[MAX_PCB];
    int pcb_count;
} Sistema;

Sistema sistema;

void inicializarSistema() {
    sistema.pcb_count = 0;
}

void agregarPCB(PCB pcb) {
    sistema.pcbs[sistema.pcb_count++] = pcb;
}


//funciones de BUSQUEDA 
PCB* buscarPCB(uint32_t pid) {
    for (int i = 0; i < sistema.pcb_count; i++) {
        if (sistema.pcbs[i].pid == pid) {
            return &sistema.pcbs[i];
        }
    }
    return NULL;

}

TCB* buscarTCB(uint32_t tid) {
    for (int i = 0; i < sistema.pcb_count; i++) {
        for (int j = 0; j < sistema.pcbs[i].tcb_count; j++) {
            if (sistema.pcbs[i].tcbs[j].tid == tid) {
                return &sistema.pcbs[i].tcbs[j];
            }
        }
    }
    return NULL;
}

Mutex* buscarMutex(char* mutexId) {
    for (int i = 0; i < sistemaMutex.count; i++) {
        if (sistemaMutex.mutexes[i].id == mutexId) {
            return &sistemaMutex.mutexes[i];
        }
    }
    return NULL;
}


//otras funciones 
void enviarATodosLosTCBsAExit(PCB *pcb) {
    for (int i = 0; i < pcb->tcb_count; i++) {
        pcb->tcbs[i].estado = 4; // Cambiar estado a EXIT
        printf("TCB finalizado: TID=%d\n", pcb->tcbs[i].tid);
    }
}

void notificarMemoria(uint32_t pid) {
    // Lógica para notificar a la memoria la finalización del proceso
    printf("Notificando a la memoria la finalización del proceso PID=%d\n", pid);
}


// DECLARACION DE Funciones para syscalls
void PROCESS_CREATE(char* nombreArchivo, uint32_t tamanioMemoria, uint32_t prioridad);
void PROCESS_EXIT(uint32_t pid);
void THREAD_CREATE(char* nombreArchivo, uint32_t prioridad);
void THREAD_JOIN(uint32_t tid);
void THREAD_CANCEL(uint32_t tid);
void THREAD_EXIT(uint32_t tid);
void MUTEX_CREATE();
void MUTEX_LOCK(uint32_t mutexId, uint32_t tid);
void MUTEX_UNLOCK(uint32_t mutexId, uint32_t tid);

t

///////////////////////////////////////////////////////////////////////////CREACION Y FINALIZACION ////////////////////////////////////////////////////////
// Implementación de funciones para crear y finalizar tcb y pcb 
PCB* crearPCB(uint32_t pid, char* archivoDePseudocodigo, uint32_t tamanioMemoria, uint32_t prioridad) {
    PCB *pcb = &sistema.pcbs[sistema.pcb_count++];
    pcb->pid = pid;
    strcpy(pcb->archivoDePseudocodigo, archivoDePseudocodigo);
    pcb->tamanioMemoria = tamanioMemoria;
    pcb->tcb_count = 1;
    pcb->tcbs[0].tid = 0;
    pcb->tcbs[0].pid = pid;
    pcb->tcbs[0].prioridad = prioridad;
    pcb->tcbs[0].estado = 0; // Estado NEW
    return pcb;
}

void finalizarPCB(PCB *pcb) {
    //free(pcb->tids);
    //free(pcb->Mutex);
    free(pcb);
}

TCB* crearTCB(uint32_t tid, uint32_t pid, char *nombreArchivo, uint32_t prioridad) {
    PCB *pcb = &sistema.pcbs[pid - 1]; // Asumiendo que el PID es el índice + 1
    TCB *tcb = &pcb->tcbs[pcb->tcb_count++];
    tcb->tid = tid;
    tcb->procesoPadre = procesoPadre;
    tcb->prioridad = prioridad;
    tcb->estadoDelHilo = 1; // Estado READY
    strcpy(tcb->nombreArchivo, nombreArchivo);
    return tcb;
}



void finalizarTCB(TCB *tcb) {
    free(tcb);
}



#endif