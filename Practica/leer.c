#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ficheros.h"

int main(int argc, char **argv) {
    // Validación de la sintaxis
    if (argc != 3) {
        fprintf(stderr, "Sintaxis: leer <nombre_dispositivo> <ninodo>\n");
        return FALLO;
    }

    // Obtener los argumentos
    char *nombre_dispositivo = argv[1];
    unsigned int ninodo = atoi(argv[2]);

    // Montar el dispositivo
    if (bmount(nombre_dispositivo) == -1) {
        fprintf(stderr, "Error al montar el dispositivo\n");
        return FALLO;
    }

    // Leer el inodo para obtener su tamaño
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == -1) {
        fprintf(stderr, "Error al leer el inodo %u\n", ninodo);
        bumount();
        return FALLO;
    }

    // Buffer para almacenar el contenido del fichero
    unsigned int tamano_fichero = inodo.tamEnBytesLog;
    char *buffer = (char *)malloc(tamano_fichero);
    if (!buffer) {
        fprintf(stderr, "Error al reservar memoria para el buffer\n");
        bumount();
        return FALLO;
    }

    // Leer el contenido del fichero
    int bytes_leidos = mi_read_f(ninodo, buffer, 0, tamano_fichero);
    if (bytes_leidos == -1) {
        fprintf(stderr, "Error al leer el fichero\n");
        free(buffer);
        bumount();
        return FALLO;
    }

    // Escribir el contenido en la salida estándar (redireccionado a ext1.txt)
    fwrite(buffer, 1, bytes_leidos, stdout);

    // Mostrar el número de bytes leídos y el tamaño en bytes lógicos
    fprintf(stderr, "total_leidos %d\n", bytes_leidos);
    fprintf(stderr, "tamEnBytesLog %u\n", inodo.tamEnBytesLog);

    // Liberar memoria y desmontar el dispositivo
    free(buffer);
    if (bumount() == -1) {
        fprintf(stderr, "Error al desmontar el dispositivo\n");
        return FALLO;
    }

    return EXITO;
}