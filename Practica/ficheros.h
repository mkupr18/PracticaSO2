#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bloques.h"
#include "ficheros_basico.h"

// Estructura Stat para guardar información de inodo
struct STAT {
    char tipo;
    unsigned char permisos;
    unsigned int nlinks;
    unsigned int tamEnBytesLog;
    time_t atime;
    time_t mtime;
    time_t ctime;
    unsigned int numBloquesOcupados;
};

// Funciones de la capa de ficheros
int mi_write_f(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes);
int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes);
int mi_stat_f(unsigned int ninodo, struct STAT *p_stat);
int mi_chmod_f(unsigned int ninodo, unsigned char permisos);
