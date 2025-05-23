// Autores: Kalyarat Asawapoom, Rupak Guni, Maria Kupriyenko

#include "directorios.h"

/**
 * Programa que muestra la información acerca del inodo de un fichero o directorio, llamando a la función mi_stat().
 */
int main(int argc, char **argv) {
    // Valida los argumentos
    if (argc != 3) {
        fprintf(stderr, RED"Sintaxis: ./mi_stat <disco> </ruta>\n"RESET);
        return FALLO;
    }

    // Monta el dispositivo
    if (bmount(argv[1]) == -1) {
        fprintf(stderr, RED"Error al montar el dispositivo.\n"RESET);
        return FALLO;
    }

    // Muestra la información del inodo correspondiente
    struct STAT stat;
    int err = mi_stat(argv[2], &stat);

    if (err < 0) {
        mostrar_error_buscar_entrada(err);
        bumount();
        return FALLO;
    }

    fprintf(stderr, "Nº de inodo: %d\n", err);
    fprintf(stderr, "tipo: %c\n", stat.tipo);
    fprintf(stderr, "permisos: %d\n", stat.permisos);

    struct tm *tm;

    tm = localtime(&stat.atime);
    fprintf(stderr, "atime: %02d-%02d-%02d %02d:%02d:%02d\n",
        tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
        tm->tm_hour, tm->tm_min, tm->tm_sec);

    tm = localtime(&stat.mtime);
    fprintf(stderr, "mtime: %02d-%02d-%02d %02d:%02d:%02d\n",
        tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
        tm->tm_hour, tm->tm_min, tm->tm_sec);

    tm = localtime(&stat.ctime);
    fprintf(stderr, "ctime: %02d-%02d-%02d %02d:%02d:%02d\n",
        tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
        tm->tm_hour, tm->tm_min, tm->tm_sec);

    tm = localtime(&stat.btime);
    fprintf(stderr, "btime: %02d-%02d-%02d %02d:%02d:%02d\n",
        tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
        tm->tm_hour, tm->tm_min, tm->tm_sec);

    fprintf(stderr, "nlinks: %d\n", stat.nlinks);
    fprintf(stderr, "tamEnBytesLog: %d\n", stat.tamEnBytesLog);
    fprintf(stderr, "numBloquesOcupados: %d\n", stat.numBloquesOcupados);

    // Desmonta el dispositivo
    bumount();

    return EXITO;
}
