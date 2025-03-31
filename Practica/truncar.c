#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ficheros.h"

int main(int argc, char **argv) {
    // Validación de sintaxis
    if (argc != 4) {
        fprintf(stderr, "Sintaxis: truncar <nombre_dispositivo> <ninodo> <nbytes>\n");
        return FALLO;
    }

    // Montar dispositivo
    if (bmount(argv[1]) == -1) {
        fprintf(stderr, "Error al montar el dispositivo\n");
        return FALLO;
    }

    unsigned int ninodo = atoi(argv[2]);
    unsigned int nbytes = atoi(argv[3]);
    int resultado;

    // Operación de truncado/liberación
    if (nbytes == 0) {
        resultado = liberar_inodo(ninodo);
    } else {
        resultado = mi_truncar_f(ninodo, nbytes);
    }

    // Obtener información del inodo
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == -1) {
        fprintf(stderr, "Error al leer el inodo\n");
        bumount();
        return FALLO;
    }

    // Obtener información STAT
    struct STAT stat;
    if (mi_stat_f(ninodo, &stat) == -1) {
        fprintf(stderr, "Error al obtener stat del inodo\n");
        bumount();
        return FALLO;
    }

    struct tm *ts;
    char atime[80], mtime[80], ctime[80], btime[80];

    ts = localtime(&inodo.atime);
    strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&inodo.mtime);
    strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&inodo.ctime);
    strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&inodo.btime);
    strftime(btime, sizeof(btime), "%a %Y-%m-%d %H:%M:%S", ts);

    // Mostrar información del inodo
        fprintf(stdout,"\nDATOS INODO %d:\n", ninodo);
        fprintf(stdout,"tipo=%c\n", inodo.tipo);
        fprintf(stdout,"permisos=%d\n", inodo.permisos);
        fprintf(stdout,"atime: %s\n", atime);
        fprintf(stdout,"mtime: %s\n", mtime);
        fprintf(stdout,"ctime: %s\n", ctime);
        fprintf(stdout,"btime: %s\n", btime);
        fprintf(stdout,"nlinks=%d\n", inodo.nlinks);
        fprintf(stdout,"tamEnBytesLog: %u\n", stat.tamEnBytesLog);
        fprintf(stdout,"numBloquesOcupados: %u\n", stat.numBloquesOcupados);

    // Desmontar dispositivo
    if (bumount() == -1) {
        fprintf(stderr, "Error al desmontar el dispositivo\n");
        return FALLO;
    }

    return resultado;
}