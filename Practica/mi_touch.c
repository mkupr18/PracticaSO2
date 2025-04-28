#include "directorios.h"

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Sintaxis: ./mi_touch <disco> <permisos> </ruta_fichero>\n");
        return FALLO;
    }

    char *disco = argv[1];
    unsigned char permisos = atoi(argv[2]);
    char *camino = argv[3];

    if (permisos > 7) {
        fprintf(stderr, "Error: modo inválido: <<%d>>\n", permisos);
        return FALLO;
    }

    if (bmount(disco) < 0) {
        perror("Error montando disco");
        return FALLO;
    }

    // Llamada a mi_creat()
    int error = mi_creat(camino, permisos);
    if (error < 0) {
        mostrar_error_buscar_entrada(error);
    }

    bumount();
    return error;
}
