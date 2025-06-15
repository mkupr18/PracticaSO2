// Autores: Kalyarat Asawapoom, Rupak Guni, Maria Kupriyenko

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "directorios.h"

int main(int argc, char **argv) {
    // Si la sintaxis es correcta
    if (argc != 3 && argc != 4) {
        fprintf(stderr, RED "Sintaxis: ./mi_ls [-l] <nombre_dispositivo> </ruta>\n" RESET);
        return FALLO;
    }
 
    char *disco;
    char *camino;
    int formato_largo = 0;

    if (argc == 3) { // Modo simple
        disco = argv[1];
        camino = argv[2];
    } else if (argc == 4) { // Modo largo
        if (strcmp(argv[1], "-l") != 0) {
            fprintf(stderr, RED "Error: opción no válida.\n" RESET);
            return FALLO;
        }
        formato_largo = 1;
        disco = argv[2];
        camino = argv[3];
    }

    // Montamos el dispositivo
    if (bmount(disco) == -1) {
        fprintf(stderr, RED "Error al montar el dispositivo.\n" RESET);
        return FALLO;
    }

    // Buffer para almacenar resultados
    char buffer[TAMBUFFER];
    memset(buffer, 0, sizeof(buffer));


    // Obtenemos entradas del directorio
    int entradas;
    if (formato_largo) {
        entradas = mi_dir(camino, buffer, 'l'); // Modo largo
    } else {
        entradas = mi_dir(camino, buffer, 's'); // Modo simple
    }

    if (entradas < 0) {
        fprintf(stderr, RED "Error: No existe el archivo o el directorio.\n" RESET);
        bumount();
        return FALLO;
    }

    // Imprime los resultados
    if (formato_largo) {
        printf("Total: %d\n", entradas);
        printf("Tipo\tModo\tmTime\t\tTamaño\t\tNombre\n");
        printf("--------------------------------------------------------------------------------------------\n");
        printf("%s", buffer);
    } else {
        printf("Total: %d\n", entradas);
        if(entradas != 0){
            printf("%s", buffer);}
        
    }

    bumount();
    return EXITO;
}
