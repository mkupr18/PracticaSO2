#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "directorios.h"

int main(int argc, char **argv) {
    //si la sintaxis es correcta
    if (argc != 3 && argc != 4) {
        fprintf(stderr, "Sintaxis: ./mi_ls [-l] <nombre_dispositivo> </ruta>\n");
        return -1;
    }

    //
    char *disco;
    char *camino;
    int formato_largo = 0;

    if (argc == 3) {
        disco = argv[1];
        camino = argv[2];
    } else if (argc == 4) {
        if (strcmp(argv[1], "-l") != 0) {
            fprintf(stderr, RED "Error: opción no válida.\n" RESET);
            return -1;
        }
        formato_largo = 1;
        disco = argv[2];
        camino = argv[3];
    }

    if (bmount(disco) == -1) {
        fprintf(stderr, RED "Error al montar el dispositivo.\n" RESET);
        return -1;
    }

    char buffer[TAMBUFFER];
    memset(buffer, 0, sizeof(buffer));

    int entradas;

    if (formato_largo) {
        entradas = mi_dir(camino, buffer, 'l'); // modo largo
    } else {
        entradas = mi_dir(camino, buffer, 's'); // modo simple
    }

    if (entradas < 0) {
        fprintf(stderr, RED "Error: No existe el archivo o el directorio.\n" RESET);
        bumount();
        return -1;
    }

    if (formato_largo) {
        printf("Total: %d\n", entradas);
        printf("Tipo\tModo\tmTime\t\tTamaño\t\tNombre\n");
        printf("--------------------------------------------------------------------------------------------\n");
        printf("%s", buffer);
    } else {
        printf("Total: %d\n", entradas);
        printf("%s", buffer);
    }

    bumount();
    return 0;
}
