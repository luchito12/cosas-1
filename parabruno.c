Para desarrollar la función de **finalización de hilos**, debemos cumplir con los siguientes pasos clave:

1. **Informar a la memoria** que el hilo ha finalizado.
2. **Liberar el TCB asociado** al hilo que ha terminado.
3. **Desbloquear hilos** que estaban esperando (por `THREAD_JOIN`) o bloqueados por un **mutex** que estaba siendo controlado por el hilo finalizado.
4. **Mover al estado `READY`** a los hilos que se desbloquearon debido a la finalización del hilo.

### Suposiciones:
- Existe una función para comunicar con la memoria, informando sobre la finalización del hilo.
- Cada hilo (TCB) puede tener un campo de dependencias, es decir, otros hilos que están esperando a que este termine (por `THREAD_JOIN`).
- Los mutex tomados por el hilo finalizado deben ser liberados, y los hilos bloqueados por esos mutex se deben mover al estado `READY`.

### Función: `finalizar_hilo`

```c
void finalizar_hilo(t_tcb* hiloFinalizado) {
    log_info(logger, "Finalizando hilo TID: %d del proceso PID: %d", hiloFinalizado->tid, hiloFinalizado->procesoPadre);

    // 1. Informar a la memoria sobre la finalización del hilo.
    notificar_a_memoria_finalizacion(hiloFinalizado->tid, hiloFinalizado->procesoPadre);

    // 2. Liberar el TCB asociado al hilo.
    liberar_tcb(hiloFinalizado);

    // 3. Desbloquear los hilos bloqueados por THREAD_JOIN.
    desbloquear_hilos_join(hiloFinalizado->tid);

    // 4. Desbloquear los hilos bloqueados por mutex controlados por el hilo.
    liberar_mutex_controlados(hiloFinalizado);

    // 5. Liberar otros recursos que pudieran estar asociados al hilo (si es necesario).
    log_info(logger, "Recursos del hilo TID: %d liberados", hiloFinalizado->tid);

    // Finalmente, puedes marcar el hilo como terminado.
    hiloFinalizado->estadoDelHilo = TERMINADO;

    // Aquí podrías liberar la memoria asociada al TCB si no lo haces en `liberar_tcb`.
}
```

### 1. Informar a la memoria sobre la finalización del hilo
Para este paso, necesitamos una función que se comunique con la memoria y le notifique que el hilo ha terminado.

```c
void notificar_a_memoria_finalizacion(int tid, int pid) {
    // Lógica para enviar el mensaje a memoria indicando la finalización del hilo.
    // Por ejemplo, podrías tener un socket abierto con el proceso de memoria y enviarle un mensaje:
    // send(socket_memoria, mensaje_finalizacion, sizeof(mensaje_finalizacion), 0);

    log_info(logger, "Notificando a memoria que el hilo TID: %d del proceso PID: %d ha finalizado", tid, pid);
}
```

### 2. Liberar el TCB asociado
Para liberar el TCB (Control Block de Thread), puedes definir una función que libere los recursos asociados al hilo (como su memoria y estructuras internas).

```c
void liberar_tcb(t_tcb* tcb) {
    log_info(logger, "Liberando TCB del hilo TID: %d", tcb->tid);
    
    // Aquí liberarías la memoria asociada a la estructura TCB.
    free(tcb);
}
```

### 3. Desbloquear hilos que esperaban con THREAD_JOIN
En este paso, identificamos los hilos que estaban esperando a que el hilo finalizado termine, y los movemos a la cola `READY`.

```c
void desbloquear_hilos_join(int tid_finalizado) {
    log_info(logger, "Desbloqueando hilos que esperaban por THREAD_JOIN del TID: %d", tid_finalizado);

    pthread_mutex_lock(&mutex_Block);
    // Supongamos que existe una lista de hilos bloqueados por THREAD_JOIN.
    for (int i = 0; i < list_size(hilosEnBlock); i++) {
        t_tcb* hiloBloqueado = (t_tcb*) list_get(hilosEnBlock, i);

        // Si este hilo está bloqueado esperando el `tid_finalizado`, lo movemos a READY.
        if (hiloBloqueado->tid_bloqueante == tid_finalizado) {
            log_info(logger, "Moviendo hilo TID: %d a READY", hiloBloqueado->tid);
            
            // Mover el hilo a la cola de READY.
            pthread_mutex_lock(&mutex_Ready);
            list_add(hilosEnReady, hiloBloqueado);
            pthread_mutex_unlock(&mutex_Ready);

            sem_post(&semHiloEnReady); // Indicar que hay un nuevo hilo en READY.

            // Remover el hilo de la cola BLOCK.
            list_remove(hilosEnBlock, i);
            i--; // Ajustar el índice tras remover un elemento.
        }
    }
    pthread_mutex_unlock(&mutex_Block);
}
```

### 4. Liberar los mutex controlados por el hilo finalizado
Cuando un hilo termina y tiene mutex tomados, esos mutex deben ser liberados, y los hilos que estaban bloqueados esperando ese mutex deben moverse al estado `READY`.

```c
void liberar_mutex_controlados(t_tcb* hiloFinalizado) {
    log_info(logger, "Liberando mutex tomados por el hilo TID: %d", hiloFinalizado->tid);

    // Suponemos que hay una lista de mutex en el TCB del hilo.
    for (int i = 0; i < list_size(hiloFinalizado->mutex_tomados); i++) {
        t_mutex* mutex = (t_mutex*) list_get(hiloFinalizado->mutex_tomados, i);
        
        log_info(logger, "Liberando mutex ID: %d", mutex->id);

        // Desbloquear hilos que estaban esperando este mutex.
        desbloquear_hilos_mutex(mutex);
    }
}

void desbloquear_hilos_mutex(t_mutex* mutex) {
    log_info(logger, "Desbloqueando hilos que esperaban el mutex ID: %d", mutex->id);

    pthread_mutex_lock(&mutex_Block);
    // Suponemos que existe una lista de hilos bloqueados esperando por mutex.
    for (int i = 0; i < list_size(hilosEnBlock); i++) {
        t_tcb* hiloBloqueado = (t_tcb*) list_get(hilosEnBlock, i);

        // Si este hilo estaba esperando el mutex, lo movemos a READY.
        if (hiloBloqueado->mutex_bloqueante == mutex->id) {
            log_info(logger, "Moviendo hilo TID: %d a READY", hiloBloqueado->tid);

            // Mover el hilo a la cola de READY.
            pthread_mutex_lock(&mutex_Ready);
            list_add(hilosEnReady, hiloBloqueado);
            pthread_mutex_unlock(&mutex_Ready);

            sem_post(&semHiloEnReady); // Indicar que hay un nuevo hilo en READY.

            // Remover el hilo de la cola BLOCK.
            list_remove(hilosEnBlock, i);
            i--; // Ajustar el índice tras remover un elemento.
        }
    }
    pthread_mutex_unlock(&mutex_Block);
}
```

### Resumen del flujo de la función:
1. **Finalización**: Se informa a la memoria que el hilo ha terminado.
2. **Liberación**: Se liberan los recursos del hilo y se eliminan referencias como el TCB.
3. **Desbloqueo**: Se mueven a `READY` los hilos que estaban esperando a que el hilo finalizara o que estaban bloqueados por mutex controlados por el hilo finalizado.
4. **Estado**: Se marca el hilo como **TERMINADO** y se libera toda la memoria que aún estuviera asociada a él.

Este flujo asegura que los hilos dependientes del hilo finalizado puedan continuar su ejecución de forma adecuada, y que los recursos del hilo sean gestionados de manera eficiente.