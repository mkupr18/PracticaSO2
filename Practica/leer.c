// Autores: Kalyarat Asawapoom, Rupak Guni, Maria Kupriyenko

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ficheros.h"

#define TAM_BUFFER 1500  // Tamaño configurable del buffer

int main(int argc, char **argv) {
    // Validación de la sintaxis
    if (argc != 3) {
        fprintf(stderr, RED "Sintaxis: ./leer <nombre_dispositivo> <ninodo>\n" RESET);
        return FALLO;
    }

    // Obtiene los argumentos
    char *nombre_dispositivo = argv[1];
    unsigned int ninodo = atoi(argv[2]);

    // Monta el dispositivo
    if (bmount(nombre_dispositivo) == -1) {
        fprintf(stderr, RED "Error al montar el dispositivo\n" RESET);
        return FALLO;
    }

    // Lee el inodo para obtener su tamaño
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == -1) {
        fprintf(stderr, RED "Error al leer el inodo %u\n"RESET, ninodo );
        bumount();
        return FALLO;
    }

    // Buffer de lectura
    char buffer_texto[TAM_BUFFER];
    int leidos, total_leidos = 0;
    unsigned int offset = 0;

    // Bucle de lectura
    memset(buffer_texto, 0, TAM_BUFFER);
    leidos = mi_read_f(ninodo, buffer_texto, offset, TAM_BUFFER);
    
    while (leidos > 0) {
        // Escribe el contenido leído en la salida estándar
        write(1, buffer_texto, leidos);
        total_leidos += leidos;
        offset += leidos;
        
        // Prepara la siguiente lectura
        memset(buffer_texto, 0, TAM_BUFFER);
        leidos = mi_read_f(ninodo, buffer_texto, offset, TAM_BUFFER);
    }

    // Verificación de errores
    if (leidos == -1) {
        fprintf(stderr, RED "Error al leer el fichero\n" RESET);
        bumount();
        return FALLO;
    }

    // Muestra estadísticas por stderr según la profe
    char mensaje[128];
    sprintf(mensaje, "total_leidos %d\ntamEnBytesLog %u\n", 
            total_leidos, inodo.tamEnBytesLog);
    write(2, mensaje, strlen(mensaje));

    // Desmonta el dispositivo
    if (bumount() == -1) {
        fprintf(stderr, RED "Error al desmontar el dispositivo\n" RESET);
        return FALLO;
    }

    return EXITO;
}