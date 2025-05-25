// Autores: Kalyarat Asawapoom, Rupak Guni, Maria Kupriyenko

#include "directorios.h"

/**
 * Crea un fichero, separando la funcionalidad de mi_mkdir
 */
int main(int argc, char **argv) {
    // Verifica la sintaxis
    if (argc != 4) {
        fprintf(stderr, "Sintaxis: ./mi_touch <disco> <permisos> </ruta_fichero>\n");
        return FALLO;
    }

    char *disco = argv[1];
    unsigned char permisos = atoi(argv[2]);
    char *camino = argv[3];

    // Verifica los permisos
    if (permisos > 7) {
        fprintf(stderr, "Error: modo inválido: <<%d>>\n", permisos);
        return FALLO;
    }

    // Monta el dipositivo
    if (bmount(disco) < 0) {
        perror("Error montando disco");
        return FALLO;
    }

    // Llama a mi_creat()
    int error = mi_creat(camino, permisos);
    if (error < 0) {
        mostrar_error_buscar_entrada(error);
    }

    // Desmonta el dispositivo
    bumount();
    return error;
}
