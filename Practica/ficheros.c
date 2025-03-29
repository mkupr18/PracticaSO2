#include "ficheros.h"
#include "bloques.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Escribe datos en un inodo en la posición y tamaño especificados.
 *
 * @param ninodo Número del inodo donde se escribirá.
 * @param buf_original Puntero al buffer con los datos a escribir.
 * @param offset Posición dentro del archivo donde comenzará la escritura.
 * @param nbytes Cantidad de bytes a escribir.
 *
 * @pre `ninodo` debe ser un inodo válido con permisos de escritura.
 *      `buf_original` debe ser un puntero válido con al menos `nbytes` datos disponibles.
 *      `offset` debe estar dentro del rango permitido para el inodo.
 *
 * @post Se escriben `nbytes` desde `buf_original` en el inodo, comenzando desde `offset`.
 *       Se actualiza el tamaño lógico del inodo si la escritura expande el archivo.
 *       Se actualizan los tiempos de modificación (`mtime`) y cambio (`ctime`).
 *
 * @return Número de bytes escritos si la operación fue exitosa, `FALLO` en caso de error.
 */
int mi_write_f(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes) {
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        return FALLO;
    }

    // Verificar permisos de escritura
    if ((inodo.permisos & 2) != 2) {
        fprintf(stderr, RED "No hay permisos de escritura\n" RESET);
        return FALLO;
    }

    unsigned int primerBL = offset / BLOCKSIZE;
    unsigned int ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;
    unsigned int desp1 = offset % BLOCKSIZE;
    unsigned int desp2 = (offset + nbytes - 1) % BLOCKSIZE;

    int nbfisico;
    char buf_bloque[BLOCKSIZE];
    unsigned int bytes_escritos = 0;

    if (primerBL == ultimoBL) {
        // Caso 1: Todo cabe en un solo bloque
        nbfisico = traducir_bloque_inodo(ninodo, primerBL, 1);
        if (nbfisico == FALLO) {
            return FALLO;
        }
        if (bread(nbfisico, buf_bloque) == FALLO) {
            return FALLO;
        }
        memcpy(buf_bloque + desp1, buf_original, nbytes);
        if (bwrite(nbfisico, buf_bloque) == FALLO) {
            return FALLO;
        }
        bytes_escritos = nbytes;
    } else {
        // Caso 2: Escritura en múltiples bloques
        // Primer bloque
        nbfisico = traducir_bloque_inodo(ninodo, primerBL, 1);
        if (nbfisico == FALLO) {
            return FALLO;
        }
        if (bread(nbfisico, buf_bloque) == FALLO) {
            return FALLO;
        }
        memcpy(buf_bloque + desp1, buf_original, BLOCKSIZE - desp1);
        if (bwrite(nbfisico, buf_bloque) == FALLO) {
            return FALLO;
        }
        bytes_escritos += BLOCKSIZE - desp1;

        // Bloques intermedios
        for (unsigned int bl = primerBL + 1; bl < ultimoBL; bl++) {
            nbfisico = traducir_bloque_inodo(ninodo, bl, 1);
            if (nbfisico == FALLO) {
                return FALLO;
            }
            if (bwrite(nbfisico, buf_original + (BLOCKSIZE - desp1) + (bl - primerBL - 1) * BLOCKSIZE) == FALLO) {
                return FALLO;
            }
            bytes_escritos += BLOCKSIZE;
        }

        // Último bloque
        nbfisico = traducir_bloque_inodo(ninodo, ultimoBL, 1);
        if (nbfisico == FALLO) {
            return FALLO;
        }
        if (bread(nbfisico, buf_bloque) == FALLO) {
            return FALLO;
        }
        memcpy(buf_bloque, buf_original + (nbytes - (desp2 + 1)), desp2 + 1);
        if (bwrite(nbfisico, buf_bloque) == FALLO) {
            return FALLO;
        }
        bytes_escritos += desp2 + 1;
    }

    // Actualizar metainformación del inodo
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        return FALLO;
    }
    if (offset + nbytes > inodo.tamEnBytesLog) {
        inodo.tamEnBytesLog = offset + nbytes;
    }
    inodo.mtime = time(NULL);
    inodo.ctime = time(NULL);
    if (escribir_inodo(ninodo, &inodo) == FALLO) {
        return FALLO;
    }

    return bytes_escritos;
}

/**
 * @brief Lee datos de un inodo desde una posición y tamaño especificados.
 *
 * @param ninodo Número del inodo del que se leerá.
 * @param buf_original Puntero al buffer donde se almacenarán los datos leídos.
 * @param offset Posición dentro del archivo desde donde comenzará la lectura.
 * @param nbytes Cantidad de bytes a leer.
 *
 * @pre `ninodo` debe ser un inodo válido con permisos de lectura.
 *      `buf_original` debe ser un puntero válido con espacio suficiente para `nbytes`.
 *      `offset` debe estar dentro del rango permitido del inodo.
 *
 * @post Se copian hasta `nbytes` datos desde el inodo al `buf_original`, comenzando desde `offset`.
 *       Si `offset + nbytes` excede el tamaño lógico del inodo, se ajusta la cantidad de datos leídos.
 *       Se actualiza el tiempo de acceso (`atime`).
 *
 * @return Número de bytes leídos si la operación fue exitosa, `FALLO` en caso de error.
 */
int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes) {
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == -1) {
        fprintf(stderr, RED "Error al leer el inodo %u\n" RESET, ninodo);
        return FALLO;
    }

    // Verificar permisos de lectura
    if ((inodo.permisos & 4) != 4) {
        fprintf(stderr, RED "No hay permisos de lectura en el inodo %u\n" RESET, ninodo);
        return FALLO;
    }
    unsigned int bytes_leidos = 0;
    // Verificar si el offset está más allá del tamaño del archivo
    if (offset >= inodo.tamEnBytesLog) {
        bytes_leidos = 0;
        return bytes_leidos; // No hay nada que leer
    }

    // Ajustar nbytes si se intenta leer más allá del EOF
    if (offset + nbytes >= inodo.tamEnBytesLog) {
        nbytes = inodo.tamEnBytesLog - offset;
    }

   
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
            fprintf(stderr, RED "Error al leer el bloque físico %d\n" RESET, bloque_fisico);
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
        fprintf(stderr, RED "Error al actualizar el inodo %u\n" RESET, ninodo);
        return FALLO;
    }

    return bytes_leidos; // Devolver la cantidad de bytes leídos
}

/**
 * @brief Obtiene la información del estado de un inodo.
 *
 * @param ninodo Número del inodo del que se obtendrá la información.
 * @param p_stat Puntero a una estructura `STAT` donde se almacenará la información.
 *
 * @pre `ninodo` debe ser un inodo válido.
 *      `p_stat` debe ser un puntero válido a una estructura `STAT`.
 *
 * @post Se llena la estructura `STAT` con los datos del inodo, incluyendo tipo, permisos,
 *       tamaño en bytes, número de bloques ocupados y tiempos de modificación (`mtime`) y creación (`ctime`).
 *
 * @return `EXITO` si la operación fue exitosa, `FALLO` en caso de error.
 */
int mi_stat_f(unsigned int ninodo, struct STAT *p_stat) {
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == -1) return FALLO;

    p_stat->tipo = inodo.tipo;
    p_stat->permisos = inodo.permisos;
    p_stat->nlinks = inodo.nlinks;
    p_stat->tamEnBytesLog = inodo.tamEnBytesLog;
    p_stat->mtime = inodo.mtime;
    p_stat->ctime = inodo.ctime;
    p_stat->numBloquesOcupados = inodo.numBloquesOcupados;
    return EXITO;
}

/**
 * @brief Cambia los permisos de un inodo.
 *
 * @param ninodo Número del inodo cuyos permisos se modificarán.
 * @param permisos Nuevo valor de permisos (bits de lectura, escritura y ejecución).
 *
 * @pre `ninodo` debe ser un inodo válido.
 *      `permisos` debe ser un valor válido dentro de la representación de permisos (ej. `0-7` en octal).
 *
 * @post Se actualiza el campo de permisos del inodo.
 *       Se actualiza el tiempo de cambio (`ctime`).
 *
 * @return `EXITO` si la operación fue exitosa, `FALLO` en caso de error.
 */
int mi_chmod_f(unsigned int ninodo, unsigned char permisos) {
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == -1) return FALLO;
    inodo.permisos = permisos;
    inodo.ctime = time(NULL);

    return escribir_inodo(ninodo, &inodo);
}

int mi_truncar_f(unsigned int ninodo, unsigned int nbytes){
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        return FALLO;
    }

    // Verificar permisos de escritura
    if ((inodo.permisos & 2) != 2) {
        fprintf(stderr, "No hay permisos de escritura\n");
        return FALLO;
    }
        // No se puede truncar más allá del tamaño actual
        if (nbytes >= inodo.tamEnBytesLog) {
            return 0;
        }
    

    unsigned int primerBL = 0;
     
    //Primer bloque a liberar
    if(nbytes%BLOCKSIZE == 0){
        primerBL = nbytes/BLOCKSIZE;
    } else{
        primerBL = nbytes/BLOCKSIZE + 1;
    }
    int bloques_liberados = liberar_bloques_inodo(primerBL, &inodo);
    if (bloques_liberados == -1) {
        fprintf(stderr, "Error al liberar el bloque inodo %u\n", ninodo);
        return FALLO;
    }
    inodo.mtime = time(NULL);
    inodo.ctime = time(NULL);

    inodo.tamEnBytesLog = nbytes;
    inodo.numBloquesOcupados -= bloques_liberados;

    if (escribir_inodo(ninodo, &inodo) == FALLO) {
        return FALLO;
    }

    return bloques_liberados;
}
