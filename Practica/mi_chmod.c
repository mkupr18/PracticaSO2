// Autores: Kalyarat Asawapoom, Rupak Guni, Maria Kupriyenko
#include "directorios.h"

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, RED"Sintaxis: ./mi_chmod <nombre_dispositivo> <permisos> </ruta>\n"RESET);
        return FALLO;
    }

    char *disco = argv[1];
    int permisos = atoi(argv[2]);
    char *camino = argv[3];

    if (permisos < 0 || permisos > 7) {
        fprintf(stderr, RED"Error: permisos debe ser un valor entre 0 y 7.\n"RESET);
        return FALLO;
    }

    if (bmount(disco) == -1) {
        fprintf(stderr, RED"Error en bmount");
        return FALLO;
    }

    int resultado = mi_chmod(camino, (unsigned char)permisos);

    if (resultado < 0) {
        mostrar_error_buscar_entrada(resultado);
    }

    bumount();
    return resultado;
}
