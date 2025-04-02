// Autores: Kalyarat Asawapoom, Rupak Guni, Maria Kupriyenko

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ficheros.h"

int main(int argc, char **argv) {
    // Validación de la sintaxis
    if (argc != 3) {
        fprintf(stderr, RED "Sintaxis: leer <nombre_dispositivo> <ninodo>\n" RESET);
        return FALLO;
    }

    // Obtiene los argumentos
    char *nombre_dispositivo = argv[1];
    unsigned int ninodo = atoi(argv[2]);

    // Monta el dispositivo
    if (bmount(nombre_dispositivo) == -1) {
        fprintf(stderr,RED "Error al montar el dispositivo\n" RESET);
        return FALLO;
    }

    // Lee el inodo para obtener su tamaño
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == -1) {
        fprintf(stderr, RED "Error al leer el inodo %u\n" RESET, ninodo);
        bumount();
        return FALLO;
    }

    // Buffer para almacenar el contenido del fichero
    unsigned int tamano_fichero = inodo.tamEnBytesLog;
    char *buffer = (char *)malloc(tamano_fichero);
    if (!buffer) {
        fprintf(stderr, RED "Error al reservar memoria para el buffer\n" RESET);
        bumount();
        return FALLO;
    }

    // Lee el contenido del fichero
    int bytes_leidos = mi_read_f(ninodo, buffer, 0, tamano_fichero);
    if (bytes_leidos == -1) {
        //fprintf(stderr, RED "Error al leer el fichero\n" RESET);
        free(buffer);
        bumount();
        return FALLO;
    }

    // Escribe el contenido en la salida estándar (redireccionado a ext1.txt)
    fwrite(buffer, 1, bytes_leidos, stdout);

    // Muestra el número de bytes leídos y el tamaño en bytes lógicos
    //fprintf(stdout, "total_leidos %d\n", bytes_leidos);
    //fprintf(stdout, "tamEnBytesLog %u\n", inodo.tamEnBytesLog);

    // Libera memoria y desmonta el dispositivo
    free(buffer);
    if (bumount() == -1) {
        fprintf(stderr, RED "Error al desmontar el dispositivo\n" RESET);
        return FALLO;
    }

    return EXITO;
}