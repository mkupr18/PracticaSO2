#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ficheros.h"

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis: truncar <nombre_dispositivo> <ninodo> <nbytes>\n" RESET);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    unsigned int ninodo = atoi(argv[2]);
    unsigned int nbytes = atoi(argv[3]);

    if (bmount(nombre_dispositivo) == -1) {
        fprintf(stderr, RED "Error al montar el dispositivo\n" RESET);
        return FALLO;
    }

    int resultado;
    if (nbytes == 0){
        resultado = liberar_inodo(ninodo);
    }else{
        resultado = mi_truncar_f(ninodo,nbytes);
    }
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == -1) {
        fprintf(stderr, "Error al leer el inodo\n");
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

    printf("\nDATOS INODO %d:\n", ninodo);
    printf("tipo=%c\n", inodo.tipo);
    printf("permisos=%d\n", inodo.permisos);
    printf("atime: %s\n", atime);
    printf("mtime: %s\n", mtime);
    printf("ctime: %s\n", ctime);
    printf("btime: %s\n", btime);
    printf("nlinks=%d\n", inodo.nlinks);
    printf("tamEnBytesLog=%u\n", inodo.tamEnBytesLog);
    printf("numBloquesOcupados=%u\n", inodo.numBloquesOcupados);
    
    struct STAT stat;
    if (mi_stat_f(ninodo, &stat) == -1) {
        fprintf(stderr, RED "Error al obtener stat del inodo %d\n" RESET, ninodo);
        bumount();
        return FALLO;
    }
    fprintf(stderr, "tamEnBytesLog: %d\n", stat.tamEnBytesLog);
    fprintf(stderr, "numBloquesOcupados: %d\n", stat.numBloquesOcupados);

    if (bumount() == -1) {
        fprintf(stderr, RED "Error al desmontar el dispositivo\n" RESET);
        return FALLO;
    }
    return resultado;
}