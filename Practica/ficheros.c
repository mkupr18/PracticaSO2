#include "ficheros.h"
#include "ficheros_basico.h"
#include <stdio.h>
#include <string.h>

int mi_stat_f(unsigned int ninodo, struct STAT *p_stat) {
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == -1) return -1;

    p_stat->tipo = inodo.tipo;
    p_stat->permisos = inodo.permisos;
    p_stat->nlinks = inodo.nlinks;
    p_stat->tamEnBytesLog = inodo.tamEnBytesLog;
    p_stat->mtime = inodo.mtime;
    p_stat->ctime = inodo.ctime;
    p_stat->numBloquesOcupados = inodo.numBloquesOcupados;
    return 0;
}

int mi_chmod_f(unsigned int ninodo, unsigned char permisos) {
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == -1) return -1;
    inodo.permisos = permisos;
    inodo.ctime = time(NULL);
    return escribir_inodo(ninodo, &inodo);
}