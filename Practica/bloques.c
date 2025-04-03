// Autores: Kalyarat Asawapoom, Rupak Guni, Maria Kupriyenko

#include "bloques.h"
static int descriptor = 0;

/**
 * @brief Abre un archivo que actúa como disco lógico.
 * 
 * @param camino Ruta del archivo a montar.
 * @pre `camino` debe ser una ruta válida y el usuario debe tener permisos de lectura/escritura.
 * @post Si el archivo existe, se abre en modo lectura/escritura. Si no, se crea con permisos `0666`.
 * 
 * @return Descriptor de archivo si tiene éxito, `FALLO` en caso de error.
 */
int bmount(const char *camino) {
    if (descriptor > 0) {
        close(descriptor); // Lo cierra si ya está abierto
    }

    // Abrimos el descriptor
    descriptor = open(camino, O_RDWR | O_CREAT, 0666);

    // Si ha dado error
    if (descriptor == -1) {
        fprintf(stderr, RED "Error en bmount: %s\n" RESET, strerror(errno));
        return FALLO;
    }

    // Devolvemos el descriptor en caso de que no haya fallo
    return descriptor;
}

/**
 * @brief Desmonta el disco lógico cerrando su descriptor.
 * 
 * @pre `descriptor` debe ser un descriptor de archivo válido (≥ 0).
 * @post Se cierra el archivo y `descriptor` se reinicia a 0.
 * 
 * @return `EXITO` si el cierre fue exitoso, `FALLO` si hubo un error.
 */
int bumount() {
    // Devuelve un -1 si hay fallo
    if (close(descriptor) == -1) {
        // Mostramos por la pantalla que hay fallo
        fprintf(stderr, RED "Error en bumount: %s\n" RESET, strerror(errno));
        // Devovemos -1
        return FALLO;
    }

    descriptor = 0; // Reiniciamos el descriptor

    // Devolvemos 0
    return EXITO;
}

/**
 * @brief Escribe un bloque de datos en el disco lógico.
 * 
 * @param nbloque Número de bloque donde se escribirá.
 * @param buf Puntero al buffer con los datos a escribir.
 * 
 * @pre `descriptor` debe ser un archivo válido.  
 *      `nbloque` debe ser un índice válido dentro del tamaño del disco.
 *      `buf` debe apuntar a un bloque de memoria de al menos `BLOCKSIZE` bytes.
 * @post Se escriben `BLOCKSIZE` bytes en el bloque especificado. 
 *       Si hay un error, no se modifica el archivo.
 * 
 * @return Número de bytes escritos si tiene éxito, `FALLO` en caso de error.
 */
int bwrite(unsigned int nbloque, const void *buf) {
    // Calculamos el desplazamiento en Bytes
    off_t desplazamiento = nbloque * BLOCKSIZE;

    // Movemos el puntero del fichero al desplazamiento calculado
    if (lseek(descriptor, desplazamiento, SEEK_SET) == (off_t)-1) {
        fprintf(stderr, RED "Error en bwrite: %s\n" RESET, strerror(errno));
        return FALLO;
    }

    // Escribimos el bloque
    ssize_t bytes_escritos = write(descriptor, buf, BLOCKSIZE);

    // Verificamos si la escritura fue exitosa
    if (bytes_escritos == -1) {
        fprintf(stderr, RED "Error en bwrite: %s\n" RESET, strerror(errno));
        return FALLO;
    }

    // Devolvemos el número de bytes escritos o -1
    return (int)bytes_escritos;
}

/**
 * @brief Lee un bloque de datos desde el disco lógico.
 * 
 * @param nbloque Número de bloque a leer.
 * @param buf Puntero al buffer donde se almacenarán los datos leídos.
 * 
 * @pre `descriptor` debe ser un archivo abierto en modo lectura.
 *      `nbloque` debe ser un índice válido dentro del tamaño del disco.
 *      `buf` debe apuntar a un bloque de memoria de al menos `BLOCKSIZE` bytes.
 * @post Se leen `BLOCKSIZE` bytes desde el bloque especificado al buffer.
 *       Si hay un error, el buffer podría contener datos no válidos.
 * 
 * @return Número de bytes leídos si tiene éxito, `FALLO` en caso de error.
 */
int bread(unsigned int nbloque, void *buf) {
    // Calculamos el desplazamiento en Bytes
    off_t desplazamiento = nbloque * BLOCKSIZE;

    // Movemos el puntero del fichero al desplazamiento calculado
    if (lseek(descriptor, desplazamiento, SEEK_SET) == (off_t)-1){
        fprintf(stderr, RED "Error en bread: %s\n" RESET, strerror(errno));
        return FALLO;
    }

    // Leemos el bloque del fichero
    size_t bytes_leidos = read(descriptor, buf, BLOCKSIZE);

    // Verificamos si la lectura fue exitosa
    if (bytes_leidos == -1) {
        fprintf(stderr, RED "Error en bread: %s\n" RESET, strerror(errno));
        return FALLO; 
    }

    // Devolvemos el número de bytes leídos o -1
    return bytes_leidos;
}

