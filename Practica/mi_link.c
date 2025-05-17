#include <stdio.h>
#include <string.h>
#include "directorios.h" 

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Sintaxis: %s <disco> </ruta_fichero_original> </ruta_enlace>\n", argv[0]);
        return -1;
    }

    if (bmount(argv[1]) < 0) {
        fprintf(stderr, "Error al montar el disco.\n");
        return -1;
    }

    // Llamamos a la función de la capa de directorios que mira si las rutas existen
    int resultado = mi_link(argv[2], argv[3]);

    if (resultado < 0) {
        mostrar_error_buscar_entrada(resultado); // función de apoyo habitual
        bumount();
        return -1;
    }

    bumount();
    return 0;
}
