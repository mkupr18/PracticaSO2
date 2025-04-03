#include "directorios.h"

void mostrar_error_buscar_entrada(int error) {
    // fprintf(stderr, "Error: %d\n", error);
    switch (error) {
    case -2: fprintf(stderr, "Error: Camino incorrecto.\n"); break;
    case -3: fprintf(stderr, "Error: Permiso denegado de lectura.\n"); break;
    case -4: fprintf(stderr, "Error: No existe el archivo o el directorio.\n"); break;
    case -5: fprintf(stderr, "Error: No existe algún directorio intermedio.\n"); break;
    case -6: fprintf(stderr, "Error: Permiso denegado de escritura.\n"); break;
    case -7: fprintf(stderr, "Error: El archivo ya existe.\n"); break;
    case -8: fprintf(stderr, "Error: No es un directorio.\n"); break;
    }
 }
 