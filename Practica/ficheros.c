#include "ficheros.h"
#include "ficheros_basico.h"
#include <stdio.h>
#include <string.h>


int mi_write_f(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes) {
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == -1) return -1;

    if ((inodo.permisos & 2) != 2) {
        fprintf(stderr, "No hay permisos de escritura\n");
        return -1;
    }

    unsigned int primerBL = offset / BLOCKSIZE;
    unsigned int ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;
    unsigned int desp1 = offset % BLOCKSIZE;
    unsigned int desp2 = (offset + nbytes - 1) % BLOCKSIZE;

    unsigned int nbfisico;
    char buf_bloque[BLOCKSIZE];
    unsigned int escritos = 0;

    if (primerBL == ultimoBL) {
        nbfisico = traducir_bloque_inodo(ninodo, primerBL, 1);
        if (nbfisico == -1) return -1;

        if (bread(nbfisico, buf_bloque) == -1) return -1;
        memcpy(buf_bloque + desp1, buf_original, nbytes);
        if (bwrite(nbfisico, buf_bloque) == -1) return -1;
        escritos = nbytes;
    } else {
        nbfisico = traducir_bloque_inodo(ninodo, primerBL, 1);
        if (nbfisico == -1) return -1;

        if (bread(nbfisico, buf_bloque) == -1) return -1;
        memcpy(buf_bloque + desp1, buf_original, BLOCKSIZE - desp1);
        if (bwrite(nbfisico, buf_bloque) == -1) return -1;
        escritos += BLOCKSIZE - desp1;

        for (unsigned int bl = primerBL + 1; bl < ultimoBL; bl++) {
            nbfisico = traducir_bloque_inodo(ninodo, bl, 1);
            if (nbfisico == -1) return -1;
            if (bwrite(nbfisico, buf_original + escritos) == -1) return -1;
            escritos += BLOCKSIZE;
        }

        nbfisico = traducir_bloque_inodo(ninodo, ultimoBL, 1);
        if (nbfisico == -1) return -1;
        if (bread(nbfisico, buf_bloque) == -1) return -1;
        memcpy(buf_bloque, buf_original + escritos, desp2 + 1);
        if (bwrite(nbfisico, buf_bloque) == -1) return -1;
        escritos += desp2 + 1;
    }

    if (inodo.tamEnBytesLog < offset + nbytes) inodo.tamEnBytesLog = offset + nbytes;
    inodo.mtime = time(NULL);
    inodo.ctime = time(NULL);
    if (escribir_inodo(ninodo, &inodo) == -1) return -1;

    return escritos;
}

int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes) {
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == -1) return -1;

    if ((inodo.permisos & 4) != 4) {
        fprintf(stderr, "No hay permisos de lectura\n");
        return -1;
    }

    if (offset >= inodo.tamEnBytesLog) return 0;
    if ((offset + nbytes) >= inodo.tamEnBytesLog) nbytes = inodo.tamEnBytesLog - offset;

    unsigned int primerBL = offset / BLOCKSIZE;
    unsigned int ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;
    unsigned int desp1 = offset % BLOCKSIZE;
    unsigned int desp2 = (offset + nbytes - 1) % BLOCKSIZE;

    unsigned int nbfisico;
    char buf_bloque[BLOCKSIZE];
    unsigned int leidos = 0;

    if (primerBL == ultimoBL) {
        nbfisico = traducir_bloque_inodo(ninodo, primerBL, 0);
        if (nbfisico == -1) return leidos;
        if (bread(nbfisico, buf_bloque) == -1) return -1;
        memcpy(buf_original, buf_bloque + desp1, nbytes);
        leidos = nbytes;
    } else {
        nbfisico = traducir_bloque_inodo(ninodo, primerBL, 0);
        if (nbfisico == -1) return leidos;
        if (bread(nbfisico, buf_bloque) == -1) return -1;
        memcpy(buf_original, buf_bloque + desp1, BLOCKSIZE - desp1);
        leidos += BLOCKSIZE - desp1;

        for (unsigned int bl = primerBL + 1; bl < ultimoBL; bl++) {
            nbfisico = traducir_bloque_inodo(ninodo, bl, 0);
            if (nbfisico == -1) return leidos;
            if (bread(nbfisico, buf_original + leidos) == -1) return -1;
            leidos += BLOCKSIZE;
        }

        nbfisico = traducir_bloque_inodo(ninodo, ultimoBL, 0);
        if (nbfisico == -1) return leidos;
        if (bread(nbfisico, buf_bloque) == -1) return -1;
        memcpy(buf_original + leidos, buf_bloque, desp2 + 1);
        leidos += desp2 + 1;
    }

    inodo.atime = time(NULL);
    if (escribir_inodo(ninodo, &inodo) == -1) return -1;
    return leidos;
}

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

int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes) {
    struct inodo inodo;
    // Leer el inodo correspondiente al número de inodo proporcionado
    if (leer_inodo(ninodo, &inodo) == -1) {
        fprintf(stderr, RED "Error al leer el inodo\n" RESET);
        return FALLO;
    }

    // Verificar permisos de lectura
    if ((inodo.permisos & 4) != 4) {
        fprintf(stderr, RED "No hay permisos de lectura\n" RESET);
        return FALLO;
    }

    // Verificar si el offset está más allá del tamaño del archivo
    if (offset >= inodo.tamEnBytesLog) {
        return 0; // No hay nada que leer
    }

    // Ajustar nbytes si se intenta leer más allá del EOF
    if (offset + nbytes >= inodo.tamEnBytesLog) {
        nbytes = inodo.tamEnBytesLog - offset;
    }

    unsigned int bytes_leidos = 0;
    unsigned int bloque_logico_inicial = offset / BLOCKSIZE;
    unsigned int bloque_logico_final = (offset + nbytes - 1) / BLOCKSIZE;
    unsigned int desplazamiento_inicial = offset % BLOCKSIZE;

    char buf_bloque[BLOCKSIZE];

    // Recorrer los bloques lógicos necesarios para la lectura
    for (unsigned int bloque_logico = bloque_logico_inicial; bloque_logico <= bloque_logico_final; bloque_logico++) {
        // Traducir el bloque lógico a físico
        int bloque_fisico = traducir_bloque_inodo(ninodo, bloque_logico, 0);

        if (bloque_fisico == -1) {
            // No hay bloque físico asignado, saltar pero acumular bytes
            bytes_leidos += BLOCKSIZE;
            continue;
        }

        // Leer el bloque físico
        if (bread(bloque_fisico, buf_bloque) == -1) {
            fprintf(stderr, RED "Error al leer el bloque físico\n" RESET);
            return FALLO;
        }

        // Calcular cuántos bytes copiar en esta iteración
        unsigned int bytes_a_copiar;
        if (bloque_logico == bloque_logico_inicial && bloque_logico == bloque_logico_final) {
            bytes_a_copiar = nbytes;
        } else if (bloque_logico == bloque_logico_inicial) {
            bytes_a_copiar = BLOCKSIZE - desplazamiento_inicial;
        } else if (bloque_logico == bloque_logico_final) {
            bytes_a_copiar = (offset + nbytes) % BLOCKSIZE;
        } else {
            bytes_a_copiar = BLOCKSIZE;
        }

        // Copiar los datos al buffer original
        memcpy(buf_original + bytes_leidos, buf_bloque + (bloque_logico == bloque_logico_inicial ? desplazamiento_inicial : 0), bytes_a_copiar);
        bytes_leidos += bytes_a_copiar;
    }

    // Actualizar el atime del inodo
    inodo.atime = time(NULL);
    if (escribir_inodo(ninodo, &inodo) == -1) {
        fprintf(stderr, RED "Error al actualizar el inodo\n" RESET);
        return FALLO;
    }
    
    // Devolver la cantidad de bytes leídos
    return bytes_leidos; 
}

