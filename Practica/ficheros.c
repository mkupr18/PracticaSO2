// Autores: Kalyarat Asawapoom, Rupak Guni, Maria Kupriyenko

#include <stdio.h>
#include <string.h>

#include "ficheros.h"

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
int mi_write_f(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes)
{
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == FALLO)
    {
        return FALLO;
    }

    // Verifica los permisos de escritura
    if ((inodo.permisos & 2) != 2)
    {
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

    if (primerBL == ultimoBL)
    {
        mi_waitSem(); // Entrada sección crítica
        // Caso 1: Todo cabe en un solo bloque
        nbfisico = traducir_bloque_inodo(ninodo, primerBL, 1);
        mi_signalSem(); // Salida sección crítica

        if (nbfisico == FALLO)
        {
            return FALLO;
        }

        if (bread(nbfisico, buf_bloque) == FALLO)
        {
            return FALLO;
        }

        memcpy(buf_bloque + desp1, buf_original, nbytes);

        if (bwrite(nbfisico, buf_bloque) == FALLO)
        {
            return FALLO;
        }
        bytes_escritos = nbytes;
    }
    else
    {
        mi_waitSem(); // Entrada sección crítica
        // Caso 2: Escritura en múltiples bloques
        // Primer bloque
        nbfisico = traducir_bloque_inodo(ninodo, primerBL, 1);
        mi_signalSem(); // Salida sección crítica

        if (nbfisico == FALLO)
        {
            return FALLO;
        }

        if (bread(nbfisico, buf_bloque) == FALLO)
        {
            return FALLO;
        }

        memcpy(buf_bloque + desp1, buf_original, BLOCKSIZE - desp1);

        if (bwrite(nbfisico, buf_bloque) == FALLO)
        {
            return FALLO;
        }
        bytes_escritos += BLOCKSIZE - desp1;

        // Bloques intermedios
        for (unsigned int bl = primerBL + 1; bl < ultimoBL; bl++)
        {
            mi_waitSem(); // Entrada sección crítica
            nbfisico = traducir_bloque_inodo(ninodo, bl, 1);
            mi_signalSem(); // Salida sección crítica

            if (nbfisico == FALLO)
            {
                return FALLO;
            }
            if (bwrite(nbfisico, buf_original + (BLOCKSIZE - desp1) + (bl - primerBL - 1) * BLOCKSIZE) == FALLO)
            {
                return FALLO;
            }
            bytes_escritos += BLOCKSIZE;
        }

        mi_waitSem(); // Entrada sección crítica
        // Último bloque
        nbfisico = traducir_bloque_inodo(ninodo, ultimoBL, 1);
        mi_signalSem(); // Salida sección crítica

        if (nbfisico == FALLO)
        {
            return FALLO;
        }
        if (bread(nbfisico, buf_bloque) == FALLO)
        {
            return FALLO;
        }

        memcpy(buf_bloque, buf_original + (nbytes - (desp2 + 1)), desp2 + 1);

        if (bwrite(nbfisico, buf_bloque) == FALLO)
        {
            return FALLO;
        }
        bytes_escritos += desp2 + 1;
    }

    mi_waitSem(); // Entrada sección crítica
    // Actualiza la metainformación del inodo
    if (leer_inodo(ninodo, &inodo) == FALLO)
    {
        //mi_signalSem(); // Salida sección crítica
        return FALLO;
    }

    if (offset + nbytes > inodo.tamEnBytesLog)
    {
        inodo.tamEnBytesLog = offset + nbytes;
    }

    inodo.mtime = time(NULL);
    inodo.ctime = time(NULL);

    if (escribir_inodo(ninodo, &inodo) == FALLO)
    {
        //mi_signalSem(); // Salida sección crítica
        return FALLO;
    }

    mi_signalSem(); // Salida sección crítica
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
int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes)
{
    mi_waitSem(); // Entrada sección crítica

    struct inodo inodo;

    // Lee el inodo
    if (leer_inodo(ninodo, &inodo) == -1)
    {
        // fprintf(stderr, RED "Error al leer el inodo %u\n" RESET, ninodo);
        return FALLO;
    }

    // Verifica permisos de lectura
    if ((inodo.permisos & 4) != 4)
    {
        fprintf(stderr, RED "No hay permisos de lectura\n" RESET);
        return FALLO;
    }

    // Verifica si el offset está más allá del tamaño del archivo
    if (offset >= inodo.tamEnBytesLog)
    { 
        return 0; // No hay nada que leer
    }

    // Ajusta nbytes si se intenta leer más allá del EOF
    if (offset + nbytes >= inodo.tamEnBytesLog)
    {
        nbytes = inodo.tamEnBytesLog - offset;
    }

    unsigned int primerBL = offset / BLOCKSIZE;
    unsigned int ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;
    unsigned int desp1 = offset % BLOCKSIZE;
    unsigned int desp2 = (offset + nbytes - 1) % BLOCKSIZE;

    char buf_bloque[BLOCKSIZE];
    unsigned int bytes_leidos = 0;

    // Caso 1: Lectura dentro de un solo bloque
    if (primerBL == ultimoBL)
    {
        int nbfisico = traducir_bloque_inodo(ninodo, primerBL, 0);
        if (nbfisico == FALLO)
        {
            // Bloque no asignado, devolver 0 bytes leídos para esta parte
            return 0;
        }

        if (bread(nbfisico, buf_bloque) == FALLO)
        {
            return FALLO;
        }

        memcpy(buf_original, buf_bloque + desp1, nbytes);
        bytes_leidos = nbytes;
    }
    // Caso 2: Lectura en múltiples bloques
    else
    {
        // Primer bloque parcial
        int nbfisico = traducir_bloque_inodo(ninodo, primerBL, 0);
        if (nbfisico != FALLO)
        {
            if (bread(nbfisico, buf_bloque) == FALLO)
            {
                return FALLO;
            }
            unsigned int bytes_a_leer = BLOCKSIZE - desp1;
            memcpy(buf_original, buf_bloque + desp1, bytes_a_leer);
            bytes_leidos += bytes_a_leer;
        }
        else
        {
            // Bloque no asignado, saltamos pero contamos los bytes
            bytes_leidos += BLOCKSIZE - desp1;
        }

        // Bloques intermedios completos
        for (unsigned int bl = primerBL + 1; bl < ultimoBL; bl++)
        {
            nbfisico = traducir_bloque_inodo(ninodo, bl, 0);
            if (nbfisico != FALLO)
            {
                if (bread(nbfisico, buf_bloque) == FALLO)
                {
                    return FALLO;
                }
                memcpy(buf_original + bytes_leidos, buf_bloque, BLOCKSIZE);
                bytes_leidos += BLOCKSIZE;
            }
            else
            {
                // Bloque no asignado, saltamos pero contamos los bytes
                bytes_leidos += BLOCKSIZE;
            }
        }

        // Último bloque parcial
        nbfisico = traducir_bloque_inodo(ninodo, ultimoBL, 0);
        if (nbfisico != FALLO)
        {
            if (bread(nbfisico, buf_bloque) == FALLO)
            {
                return FALLO;
            }
            unsigned int bytes_a_leer = desp2 + 1;
            memcpy(buf_original + bytes_leidos, buf_bloque, bytes_a_leer);
            bytes_leidos += bytes_a_leer;
        }
        else
        {
            // Bloque no asignado, saltamos pero contamos los bytes
            bytes_leidos += desp2 + 1;
        }
    }

    // Actualizar atime del inodo
    inodo.atime = time(NULL);

    if (escribir_inodo(ninodo, &inodo) == FALLO)
    {
        //mi_signalSem(); // Salida sección crítica PREGUNTAR
        return FALLO;
    }
    mi_signalSem(); // Salida sección crítica
    return bytes_leidos;
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
int mi_stat_f(unsigned int ninodo, struct STAT *p_stat)
{
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == -1)
        return FALLO;

    p_stat->tipo = inodo.tipo;
    p_stat->permisos = inodo.permisos;
    p_stat->nlinks = inodo.nlinks;
    p_stat->tamEnBytesLog = inodo.tamEnBytesLog;
    p_stat->atime = inodo.atime;
    p_stat->mtime = inodo.mtime;
    p_stat->ctime = inodo.ctime;
    p_stat->btime = inodo.btime;
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
int mi_chmod_f(unsigned int ninodo, unsigned char permisos)
{
    mi_waitSem(); // Entrada sección crítica

    struct inodo inodo;

    if (leer_inodo(ninodo, &inodo) == -1){
        mi_signalSem(); // Salida sección crítica
        return FALLO;
    }
        
    inodo.permisos = permisos;
    inodo.ctime = time(NULL);

    mi_signalSem(); // Salida sección crítica
    return escribir_inodo(ninodo, &inodo);
}

/**
 * @brief Trunca un fichero/directorio.
 *
 * @param ninodo Número del inodo que se truncará.
 * @param nbytes Número de bytes.
 *
 * @pre `ninodo` debe ser un inodo válido y tener permisos de escritura.
 *      No se puede truncar más allá del tamaño en bytes lógicos (EOF) del fichero/directorio.
 *
 * @post Se actualiza mtime, ctime, tamEnBytesLog y numBloquesOcupados.
 *       Se liberan los bloques necesarios y se devuelve la cantidad de estos.
 *
 * @return `EXITO` si la operación fue exitosa, `FALLO` en caso de error.
 */
int mi_truncar_f(unsigned int ninodo, unsigned int nbytes)
{

    struct inodo inodo;

    // Leemos el inodo
    if (leer_inodo(ninodo, &inodo) == FALLO)
    {
        return FALLO;
    }

    // Verifica los permisos de escritura
    if ((inodo.permisos & 2) != 2)
    {
        fprintf(stderr, "No hay permisos de escritura\n");
        return FALLO;
    }
    // No se puede truncar más allá del tamaño actual
    if (nbytes >= inodo.tamEnBytesLog)
    {
        return 0;
    }

    unsigned int primerBL = 0;

    // Primer bloque a liberar
    if (nbytes % BLOCKSIZE == 0)
    {
        primerBL = nbytes / BLOCKSIZE;
    }
    else
    {
        primerBL = nbytes / BLOCKSIZE + 1;
    }

    int bloques_liberados = liberar_bloques_inodo(primerBL, &inodo);

    if (bloques_liberados == -1)
    {
        fprintf(stderr, "Error al liberar el bloque inodo %u\n", ninodo);
        return FALLO;
    }

    inodo.mtime = time(NULL);
    inodo.ctime = time(NULL);

    inodo.tamEnBytesLog = nbytes;
    inodo.numBloquesOcupados -= bloques_liberados;

    if (escribir_inodo(ninodo, &inodo) == FALLO)
    {
        return FALLO;
    }

    return bloques_liberados;
}
