// Autores: Kalyarat Asawapoom, Rupak Guni, Maria Kupriyenko
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "directorios.h"

/**
 * Escribe texto en una posición de un fichero
 */
int main(int argc, char **argv) {
    // Verifica la sintaxis
    if (argc != 5) {
        fprintf(stderr, RED "Sintaxis: ./mi_escribir <disco> </ruta_fichero> <texto> <offset>\n" RESET);
        return FALLO;
    }

    const char *nombre_dispositivo = argv[1];
    const char *ruta_fichero = argv[2];
    const char *texto = argv[3];
    int offset = atoi(argv[4]);
    int tamTexto = strlen(texto);

    if (tamTexto == 0) {
        fprintf(stderr, RED "Error: el texto a escribir está vacío\n" RESET);
        return FALLO;
    }

    // Monta el dispositivo
    if (bmount(nombre_dispositivo) == -1) {
        fprintf(stderr, RED "Error al montar el dispositivo\n" RESET);
        return FALLO;
    }

    // Comprueba que el camino es de un fichero, no de un directorio
    if (ruta_fichero[strlen(ruta_fichero) - 1] == '/') {
        fprintf(stderr, RED "Error: la ruta corresponde a un directorio, no a un fichero\n" RESET);
        bumount();
        return FALLO;
    }

    fprintf(stdout, "longitud texto: %d\n", tamTexto);

    // Realiza la escritura
    int bytes_escritos = mi_write(ruta_fichero, texto, offset, tamTexto);
    int bytes_zero = 0;

    if (bytes_escritos < 0) {
        mostrar_error_buscar_entrada(bytes_escritos);
        fprintf(stdout, "Bytes escritos: %d\n", bytes_zero);
        bumount();
        return FALLO;
    }

    fprintf(stdout, "Bytes escritos: %d\n", bytes_escritos);

    // Desmonta el dispositivo
    if (bumount() == -1) {
        fprintf(stderr, RED "Error al desmontar el dispositivo\n" RESET);
        return FALLO;
    }

    return EXITO;
}
