
// Autores: Kalyarat Asawapoom, Rupak Guni, Maria Kupriyenko

#include "directorios.h"

#define TAM_BUFFER 4000  // Tamaño configurable del buffer

/**
 * Programa que muestra todo el contenido de un fichero.
 */
int main(int argc, char **argv) {
    // Verifica la sintaxis
    if (argc != 3) {
        fprintf(stderr, RED "Sintaxis: ./mi_cat <nombre_dispositivo> </ruta_fichero>\n" RESET);
        return FALLO;
    }

    const char *nombre_dispositivo = argv[1];
    const char *ruta_fichero = argv[2];

    // Monta el dispositivo
    if (bmount(nombre_dispositivo) == -1) {
        fprintf(stderr, RED "Error al montar el dispositivo\n" RESET);
        return FALLO;
    }

    // Comprueba que la ruta NO es un directorio
    if (ruta_fichero[strlen(ruta_fichero) - 1] == '/') {
        fprintf(stderr, RED "Error: la ruta corresponde a un directorio, no a un fichero\n" RESET);
        bumount();
        return FALLO;
    }

    char buffer[TAM_BUFFER];
    int leidos, total_leidos = 0;
    unsigned int offset = 0;

    // Lee hasta EOF
    memset(buffer, 0, TAM_BUFFER);
    leidos = mi_read(ruta_fichero, buffer, offset, TAM_BUFFER);

    while (leidos > 0) {
        write(1, buffer, leidos);
        total_leidos += leidos;
        offset += leidos;
        memset(buffer, 0, TAM_BUFFER);
        leidos = mi_read(ruta_fichero, buffer, offset, TAM_BUFFER);
    }

    if (leidos < 0) {
        mostrar_error_buscar_entrada(leidos);
        bumount();
        return FALLO;
    }

    // Obtiene el tamaño lógico real del fichero
    struct STAT stat;
    if (mi_stat(ruta_fichero, &stat) == -1) {
        fprintf(stderr, RED "Error al obtener stat del fichero\n" RESET);
        bumount();
        return FALLO;
    }

    char mensaje[256];
    sprintf(mensaje, "\n\nTotal_leidos %d\n", total_leidos);
    write(2, mensaje, strlen(mensaje));

    // Desmonta el dispositivo
    if (bumount() == -1) {
        fprintf(stderr, RED "Error al desmontar el dispositivo\n" RESET);
        return FALLO;
    }

    return EXITO;
}
