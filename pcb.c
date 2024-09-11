#include <utils/pcb.h>


///////////////////////////////////////////////////////////////CREACION DE LAS SYSCALLS 
//syscalls 

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////PROCESS
void PROCESS_CREATE(char* nombreArchivo, uint32_t tamanoMemoria, uint32_t prioridad) {
    static int pidCounter = 1;
    PCB *pcb = crearPCB(pidCounter++, nombreArchivo, tamanoMemoria, prioridad);
    printf("Proceso creado: PID=%d, Archivo=%s, Tamaño=%d, Prioridad=%d\n", pcb->pid, nombreArchivo, tamanoMemoria, prioridad);
}

/*Se define una estructura TCB y PCB para representar los bloques de control de hilos y procesos.
La función crearPCB crea un nuevo PCB y un TCB asociado con TID 0, dejándolo en estado NEW.
La función PROCESS_CREATE recibe los parámetros de la CPU, crea un nuevo PCB y TCB, y los añade al sistema.
*/


void PROCESS_EXIT(uint32_t pid) {
    printf("Proceso finalizado: PID=%d\n", pid);
    
    PCB *pcb = buscarPCB(pid);
    if (pcb != NULL) {
        enviarATodosLosTCBsAExit(pcb);
        notificarMemoria(pid);
    } else {
        printf("PCB no encontrado para PID=%d\n", pid);
    }
}
/*Se define una estructura TCB y PCB para representar los bloques de control de hilos y procesos.
Se inicializa un sistema con una lista de PCBs.
La función PROCESS_EXIT busca el PCB correspondiente al PID dado, cambia el estado de todos sus TCBs a EXIT, y notifica a la memoria la finalización del proceso.
*/


//////////////////////////////////////////////////////////////////////////////////////HILOS
void THREAD_CREATE(char* nombreArchivo, uint32_t prioridad) {
    static int tidCounter = 1;
    uint32_t pid = 1; // Asumiendo que el PID del proceso actual es 1
    TCB *tcb = crearTCB(tidCounter++, pid, nombreArchivo, prioridad);
    printf("Hilo creado: TID=%d, Archivo=%s, Prioridad=%d\n", tcb->tid, nombreArchivo, prioridad);
}

/*Se define una estructura TCB y PCB para representar los bloques de control de hilos y procesos.
La función crearTCB crea un nuevo TCB con un TID autoincremental, lo asocia al PCB correspondiente y lo pone en estado READY.
La función THREAD_CREATE recibe los parámetros de la CPU, crea un nuevo TCB y lo añade al sistema.*/

void THREAD_JOIN(uint32_t tid) {
    printf("Esperando a que finalice el hilo: TID=%d\n", tid);
    
    TCB *tcb = buscarTCB(tid);
    if (tcb == NULL || tcb->estado == 4) {
        printf("El hilo TID=%d no existe o ya ha finalizado. Continuando ejecución.\n", tid);
        return;
    }
    
    // Bloquear el hilo que invoca esta syscall
    // Suponiendo que el TID del hilo que invoca es 0 
    TCB *invocador = buscarTCB(0);
    if (invocador != NULL) {
        invocador->estado = 3; // Cambiar estado a BLOCKED
        printf("Hilo invocador TID=%d bloqueado esperando a TID=%d\n", invocador->tid, tid);
        
        // Simulación de espera hasta que el TID pasado por parámetro finalice
        while (tcb->estado != 4) {
            // Aquí iría la lógica de espera real en un sistema operativo
        }
        
        // Desbloquear el hilo invocado
        invocador->estado = 1; // Cambia el estado a READY
        printf("Hilo invocador TID=%d desbloqueado\n", invocador->tid);
    }
}

/*Se define una estructura TCB y PCB para representar los bloques de control de hilos y procesos.
La función THREAD_JOIN busca el TCB correspondiente al TID dado y bloquea el hilo que invoca la syscall si el TID existe y no ha finalizado.
Se simula la espera hasta que el TID pasado por parámetro finalice, y luego se desbloquea el hilo invocador*/

void THREAD_CANCEL(uint32_t tid) {
    printf("Hilo cancelado: TID=%d\n", tid);
    
    TCB *tcb = buscarTCB(tid);
    if (tcb == NULL || tcb->estado == 4) {
        printf("El hilo TID=%d no existe o ya ha finalizado. Continuando ejecución.\n", tid);
        return;
    }
    
    // Finalizar el TCB correspondiente
    tcb->estado = 4; // Cambiar estado a EXIT
    notificarMemoria(tid);
    printf("Hilo TID=%d finalizado y notificado a la memoria.\n", tid);
}

/*La función THREAD_CANCEL busca el TCB correspondiente al TID dado.
Si el TID no existe o ya ha finalizado, la función no hace nada y el hilo que la invocó continúa su ejecución.
Si el TID existe y no ha finalizado, se cambia su estado a EXIT y se notifica a la memoria la finalización del hilo.*/

void THREAD_EXIT(uint32_t tid) {
    printf("Hilo finalizado: TID=%d\n", tid);
    
    TCB *tcb = buscarTCB(tid);
    if (tcb != NULL) {
        tcb->estado = 4; // Cambiar estado a EXIT
        notificarMemoria(tid);
        printf("Hilo TID=%d finalizado y notificado a la memoria.\n", tid);
    } else {
        printf("El hilo TID=%d no existe.\n", tid);
    }
}

/*La función THREAD_EXIT busca el TCB correspondiente al TID del hilo que la invoca.
Si el TID existe, cambia su estado a EXIT y notifica a la memoria la finalización del hilo.
Si el TID no existe, imprime un mensaje indicando que el hilo no existe.*/




/////////////////////////////////////////////////////////////////////////////////////////////////////////////MUTEX//////////////////////////////////////////////////////
void MUTEX_CREATE() {
    if (sistemaMutex.count < MAX_MUTEX) {
        Mutex nuevoMutex = {sistemaMutex.count + 1, false, -1, {NULL}, 0};
        sistemaMutex.mutexes[sistemaMutex.count++] = nuevoMutex;
        printf("Mutex creado: ID=%d\n", nuevoMutex.id);
    } else {
        printf("No se pueden crear más mutexes.\n");
    }
}


void MUTEX_LOCK(char* mutexId, uint32_t tid) {
    printf("Intentando bloquear mutex: ID=%d por TID=%d\n", mutexId, tid);
    
    Mutex* mutex = buscarMutex(mutexId);
    if (mutex == NULL) {
        printf("Mutex ID=%d no existe. Enviando hilo TID=%d a EXIT.\n", mutexId, tid);
        THREAD_EXIT(tid);
        return;
    }
    
    if (!mutex->tomado) {
        mutex->tomado = true;
        mutex->tid = tid;
        printf("Mutex ID=%d bloqueado por TID=%d\n", mutexId, tid); 
        printf("Mutex ID=%d ya está tomado. Bloqueando TID=%d\n", mutexId, tid);
        // Bloquear el hilo en la cola de bloqueados del mutex
        TCB* tcb = buscarTCB(tid);
        if (tcb != NULL) {
            tcb->estado = 3; // Cambiar estado a BLOCKED
            mutex->colaBloqueados[mutex->bloqueadosCount++] = tcb;
        }
        else{
        mutex->tomado = false;
        mutex->tid = tid;
        printf("Mutex ID=%d no está tomado. asignado mutex a TID=%d\n", mutexId, tid);
        //implementar logica 
        }
    
}
}

void MUTEX_UNLOCK(char* mutexId, uint32_t tid) {
    printf("Intentando desbloquear mutex: ID=%d por TID=%d\n", mutexId, tid);
    
    Mutex* mutex = buscarMutex(mutexId);
    if (mutex == NULL) {
        printf("Mutex ID=%d no existe. Enviando hilo TID=%d a EXIT.\n", mutexId, tid);
        THREAD_EXIT(tid);
        return;
    }
    
    if (mutex->tomado && mutex->tid == tid) {
        printf("Mutex ID=%d desbloqueado por TID=%d\n", mutexId, tid);
        if (mutex->bloqueadosCount > 0) {
            // Desbloquear el primer hilo en la cola de bloqueados
            TCB* tcb = mutex->colaBloqueados[0];
            for (int i = 1; i < mutex->bloqueadosCount; i++) {
                mutex->colaBloqueados[i - 1] = mutex->colaBloqueados[i];
            }
            mutex->bloqueadosCount--;
            tcb->estado = 1; // Cambiar estado a READY
            mutex->tid = tcb->tid;
            printf("Mutex ID=%d asignado a TID=%d\n", mutexId, tcb->tid);
        } else {
            mutex->tomado = false;
            mutex->tid = -1;
        }
    } else {
        printf("El hilo TID=%d no tiene el mutex ID=%d. No se realiza ningún desbloqueo.\n", tid, mutexId);
    }
}


/*// Ejemplo de uso
    MUTEX_CREATE();
    MUTEX_LOCK(1, 0);
    MUTEX_UNLOCK(1, 0);*/

