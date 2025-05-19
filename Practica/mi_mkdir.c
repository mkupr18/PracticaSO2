// Autores: Kalyarat Asawapoom, Rupak Guni, Maria Kupriyenko
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "directorios.h" 

int main(int argc, char **argv) {
    // Verificación de número de argumentos
    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis: %s <disco> <permisos> </ruta_directorio/>\n" RESET, argv[0]);
        return EXIT_FAILURE;
    }

    // Montar el sistema de ficheros
    if (bmount(argv[1]) < 0) {
        perror("Error montando el disco");
        return EXIT_FAILURE;
    }

    // Validar que los permisos están en el rango [0-7]
    int permisos = atoi(argv[2]);
    if (permisos < 0 || permisos > 7) {
        fprintf(stderr, RED "Error: modo inválido: <<%d>>\n" RESET, permisos);
        bumount();
        return EXIT_FAILURE;
    }

    // Validar que la ruta termina en '/' para asegurarnos que es un directorio
    if (argv[3][strlen(argv[3]) - 1] != '/') {
        fprintf(stderr, RED "Error: la ruta no termina en '/'. Use mi_touch para crear ficheros.\n" RESET);
        bumount();
        return EXIT_FAILURE;
    }

    // Crear el directorio
    int error = mi_creat(argv[3], (unsigned char) permisos);
    if (error < 0) {
        mostrar_error_buscar_entrada(error); // función de ayuda que imprime errores amigables
        bumount();
        return EXIT_FAILURE;
    }

    // Desmontar el sistema de ficheros
    bumount();
    return EXIT_SUCCESS;
}