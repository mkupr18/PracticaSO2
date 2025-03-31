#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ficheros.h"

void mostrar_inodo(unsigned int ninodo) {
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == -1) {
        fprintf(stderr, "Error al leer el inodo\n");
        return;
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
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Sintaxis: truncar <nombre_dispositivo> <ninodo> <nbytes>\n");
        return -1;
    }
    
    char *nombre_dispositivo = argv[1];
    unsigned int ninodo = atoi(argv[2]);
    unsigned int nbytes = atoi(argv[3]);
    
    // Montar el dispositivo
    if (bmount(nombre_dispositivo) == -1) {
        fprintf(stderr, "Error al montar el dispositivo\n");
        return -1;
    }
    
    int resultado;
    if (nbytes == 0) {
        // Liberar inodo completo
        printf("$ time ./truncar %s %d %d\n", nombre_dispositivo, ninodo, nbytes);
        resultado = liberar_inodo(ninodo);
    } else {
        // Truncar a nbytes
        printf("$ time ./truncar %s %d %d\n", nombre_dispositivo, ninodo, nbytes);
        resultado = mi_truncar_f(ninodo, nbytes);
    }
    
    if (resultado == -1) {
        fprintf(stderr, "Error al truncar/liberar\n");
    } else {
        mostrar_inodo(ninodo);
    }
    
    // Desmontar el dispositivo
    if (bumount() == -1) {
        fprintf(stderr, "Error al desmontar el dispositivo\n");
        return -1;
    }
    
    return 0;
}
