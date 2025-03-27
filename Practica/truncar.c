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
        resultado = liberar_inodo(ninodo)
    }else{
        resultado = mi_truncar_f(ninodo,nbytes);
    }
    
    struct STAT stat;
    if (mi_stat_f(ninodo, &stat) == -1) {
        fprintf(stderr, RED "Error al obtener stat del inodo %d\n" RESET, ninodos[i]);
        bumount();
        return FALLO;
    }
    fprintf(stderr, "total_leidos %d\n", bytes_leidos);
    fprintf(stderr, "tamEnBytesLog %u\n", inodo.tamEnBytesLog);

    if (bumount() == -1) {
        fprintf(stderr, RED "Error al desmontar el dispositivo\n" RESET);
        return FALLO;
    }
    return resultado;
}