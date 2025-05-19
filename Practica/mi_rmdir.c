// Autores: Kalyarat Asawapoom, Rupak Guni, Maria Kupriyenko
#include <stdio.h>
#include <string.h>
#include "directorios.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, RED"Sintaxis: ./mi_rmdir disco /ruta\n"RESET);
        return FALLO;
    }

    // Monta el dispositivo
    if (bmount(argv[1]) == FALLO) {
        fprintf(stderr, RED"Error al montar el dispositivo\n"RESET);
        return FALLO;
    }

    // Verifica que la ruta termina en '/'
    char *ruta = argv[2];
    if (ruta[strlen(ruta)-1] != '/') {
        fprintf(stderr, "Error: la ruta debe ser un directorio (terminar con /)\n");
        bumount();
        return FALLO;
    }

    // Llama a mi_unlink
    int error = mi_unlink(ruta);
    if (error < 0) {
        mostrar_error_buscar_entrada(error);
        bumount();
        return FALLO;
    }

    // Desmonta el dispositivo
    if (bumount() == FALLO) {
        fprintf(stderr, "Error al desmontar el dispositivo\n");
        return FALLO;
    }

    return EXITO;
}