#include "bloques.h"

int bmount(const char *camino) {
    if (descriptor > 0) {
        close(descriptor); // Cierra si ya está abierto
    }

    // Abrimos descriptor
    descriptor = open(camino, O_RDWR | O_CREAT, 0666);

    // Si ha dado error
    if (descriptor == -1) {
        fprintf(stderr, RED "Error en bmount: %s\n" RESET, strerror(errno));
        return FALLO;
    }

    // Devolvemos el descriptor en caso de que no haya fallo
    return descriptor;
}

int bumount() {
    // Devuelve un -1 si hay fallo
    if (close(descriptor) == -1) {
        // Mostramos por la pantalla que hay fallo
        fprintf(stderr, RED "Error en bumount: %s\n" RESET, strerror(errno));
        // Devovemos -1
        return FALLO;
    }

    descriptor = 0; // Reiniciar descriptor

    // Devolvemos 0
    return EXITO;
}

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
int bread(unsigned int nbloque, void *buf) {
    // Calculamos el desplazamiento en Bytes
    off_t desplazamiento = nbloque * BLOCKSIZE;

    // Movemos el puntero del fichero al desplazamiento calculado
    if (lseek(descriptor, desplazamiento, SEEK_SET) == (off_t)-1){
        fprintf(stderr, RED "Error en bread: %s\n" RESET, strerror(errno));
        return FALLO;
    }

    // Leemos el bloque del fichero
    ssize_t bytes_leidos = read(descriptor, buf, BLOCKSIZE);

    // Verificamos si la lectura fue exitosa
    if (bytes_leidos == -1) {
        fprintf(stderr, RED "Error en bread: %s\n" RESET, strerror(errno));
        return FALLO; 
    }

    // Devolvemos el número de bytes leídos o -1
    return (int)bytes_leidos;
}

