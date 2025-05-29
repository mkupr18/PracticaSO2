// Autores: Kalyarat Asawapoom, Rupak Guni, Maria Kupriyenko

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "simulacion.h"
#include "directorios.h"


int acabados = 0;

// Función enterrador
void reaper() {
    pid_t ended;
    signal(SIGCHLD, reaper);
    while ((ended = waitpid(-1, NULL, WNOHANG)) > 0) {
        acabados++;
    }
}

// Genera la ruta del directorio de simulación con timestamp
// Formato: simul_aaaammddhhmmss 
// donde aaaa es el año, mm es el mes, dd es el día, 
// hh es la hora, mm es el minuto y ss es el segundo de creación.
void generar_nombre_directorio(char *nombre) {
    time_t ahora = time(NULL);
    struct tm *tm_info = localtime(&ahora);
    strftime(nombre, 100, "/simul_%Y%m%d%H%M%S/", tm_info);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, RED"Uso: ./simulacion <disco>\n"RESET);
        return FALLO;
    }

    signal(SIGCHLD, reaper);

    // Montar dispositivo padre
    if (bmount(argv[1]) < 0) {
        fprintf(stderr, "Error al montar el dispositivo\n");
        return FALLO;
    }

    char simul_dir[100];
    // Crear directorio de simulacion
    generar_nombre_directorio(simul_dir);
    if (mi_creat(simul_dir, 6) < 0) {
        fprintf(stderr, "Error creando directorio simulación\n");
        bumount();
       return FALLO;
    }

    struct STAT stat;
    if (mi_stat(simul_dir, &stat) < 0) {
        fprintf(stderr, "No se pudo verificar creación del directorio\n");
        bumount();
        return FALLO;
    }
    

    printf("*** SIMULACIÓN DE %d PROCESOS REALIZANDO CADA UNO %d ESCRITURAS ***\n", NUMPROCESOS, NUMESCRITURAS);
    printf("Directorio de simulación: %s\n", simul_dir);

     // Crear procesos hijos
    for (int i = 0; i < NUMPROCESOS; i++) {
        pid_t pid = fork();
        
        if (pid == 0) { // Proceso hijo
            // Montar dispositivo en el hijo
            if (bmount(argv[1]) < 0) {
                fprintf(stderr, RED"Hijo %d: Error al montar dispositivo hijo\n"RESET,getpid());
                return FALLO;
            }

            // Crear directorio para este proceso
            char proceso_dir[128];
            sprintf(proceso_dir,"%sproceso_%d/", simul_dir, getpid());
            //printf("DEBUG: proceso_dir = %s\n", proceso_dir);
            // Usar mi_creat para crear una entrada en el directorio
            if (mi_creat(proceso_dir, 6) < 0) {
                fprintf(stderr, RED"Hijo %d: Error creando directorio de proceso\n"RESET, getpid());
                bumount();
                exit(1);
            }

             // Crear fichero prueba.dat
            char fichero[256];
            sprintf(fichero,"%sprueba.dat", proceso_dir);
            //printf("DEBUG: fichero = %s\n", fichero);

            //printf("%s", fichero);

            if (mi_creat(fichero, 6) < 0) {
                fprintf(stderr, RED"Hijo %d: Error creando fichero prueba.dat\n" RESET, getpid());
                bumount();
                exit(1);
            }

            // Inicializar semilla aleatoria
            srand(time(NULL) + getpid());

            // Realizar escrituras
            for (int j = 1; j <= NUMESCRITURAS; j++) {
                struct REGISTRO reg;
                reg.fecha = time(NULL);
                reg.pid = getpid();
                reg.nEscritura = j;
                reg.nRegistro = rand() % REGMAX;
                //reg.nRegistro = j;

                // Escribir el registro en el fichero
                if (mi_write(fichero, &reg, reg.nRegistro * sizeof(struct REGISTRO), sizeof(struct REGISTRO)) < 0) {
                    fprintf(stderr, RED"Error escribiendo registro\n"RESET);
                    return FALLO;
                }

                //fprintf(stderr, "[Proceso %d: completadas %d escrituras en %s]\n", getpid(), NUMESCRITURAS, fichero);
                // Esperar entre escrituras
                usleep(50000);  // 0.05 seg
            }

            fprintf(stderr,"[Proceso %d: Completadas %d escrituras en %s]\n", getpid(), NUMESCRITURAS, fichero);
            // Desmontar el dispositivo hijo y salir
            bumount(); 
            return EXITO;
        }
        // Esperar entre creación de procesos
        usleep(150000);  // 0.15 seg
    }

    // Esperar a que terminen todos los hijos
    while (acabados < NUMPROCESOS) {
        pause();
    }

    // Desmontar el dispositivo padre
    bumount();
    return EXITO;
}
