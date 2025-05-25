// Autores: Kalyarat Asawapoom, Rupak Guni, Maria Kupriyenko
#include "directorios.h"

/**
 * Cambia los permisos de un fichero o directorio, llamando a la función mi_chmod().
 *  Los permisos se indican en octal, será 4 para sólo lectura (r--), 2 para sólo escritura (-w-), 1 para sólo ejecución (--x)...
 */
int main(int argc, char **argv) {
    // Verifica la sintaxis
    if (argc != 4) {
        fprintf(stderr, RED"Sintaxis: ./mi_chmod <nombre_dispositivo> <permisos> </ruta>\n"RESET);
        return FALLO;
    }

    char *disco = argv[1];
    int permisos = atoi(argv[2]);
    char *camino = argv[3];

    // Verifica los permisos
    if (permisos < 0 || permisos > 7) {
        fprintf(stderr, RED"Error: permisos debe ser un valor entre 0 y 7.\n"RESET);
        return FALLO;
    }

    // Monta el disco
    if (bmount(disco) == -1) {
        fprintf(stderr, RED"Error en bmount");
        return FALLO;
    }

    // Cambia los permisos
    int resultado = mi_chmod(camino, (unsigned char)permisos);

    if (resultado < 0) {
        mostrar_error_buscar_entrada(resultado);
    }

    // Desmonta el dispositivo
    bumount();

    return resultado;
}
