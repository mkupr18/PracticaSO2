#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ficheros.h"
#include "bloques.h"

#define NUM_OFFSETS 5

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis: escribir <nombre_dispositivo> \"<texto>\" <diferentes_inodos>\n" RESET);
        fprintf(stderr, RED "Offsets: 9000, 209000, 30725000, 409605000, 480000000\n" RESET);
        fprintf(stderr, RED "Si diferentes_inodos=0 se reserva un solo inodo para todos los offsets\n" RESET);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    char *texto = argv[2];
    int diferentes_inodos = atoi(argv[3]);
    unsigned int ninodos[NUM_OFFSETS];
    unsigned int offsets[NUM_OFFSETS] = {9000, 209000, 30725000, 409605000, 480000000};
    int tamTexto = strlen(texto);

    if (bmount(nombre_dispositivo) == -1) {
        fprintf(stderr, RED "Error al montar el dispositivo\n" RESET);
        return FALLO;
    }

    printf("longitud texto: %d\n\n", tamTexto);
    
    // Reserva de inodos
    if (diferentes_inodos == 0) {
        ninodos[0] = reservar_inodo('f', 6);
        if (ninodos[0] == -1) {
            fprintf(stderr, RED "Error al reservar inodo\n" RESET);
            bumount();
            return FALLO;
        }
        for (int i = 1; i < NUM_OFFSETS; i++) {
            ninodos[i] = ninodos[0];
        }
    } else {
        for (int i = 0; i < NUM_OFFSETS; i++) {
            //printf("Estamos dentro de dif 1");
            ninodos[i] = reservar_inodo('f', 6);
            if (ninodos[i] == -1) {
                fprintf(stderr, RED "Error al reservar inodo %d\n" RESET, i);
                bumount();
                return FALLO;
            }
        }
    }

    // Procesamiento de cada offset
    for (int i = 0; i < NUM_OFFSETS; i++) {
        printf("\nNº inodo reservado: %d\n", ninodos[i]);
        printf("offset: %d\n", offsets[i]);

        int bytes_escritos = mi_write_f(ninodos[i], texto, offsets[i], tamTexto);
        if (bytes_escritos == -1) {
            fprintf(stderr, RED "Error al escribir en el inodo %d\n" RESET, ninodos[i]);
            bumount();
            return FALLO;
        }
        printf("Bytes escritos: %d\n", bytes_escritos);

        struct STAT stat;
        if (mi_stat_f(ninodos[i], &stat) == -1) {
            fprintf(stderr, RED "Error al obtener stat del inodo %d\n" RESET, ninodos[i]);
            bumount();
            return FALLO;
        }
        printf("stat.tamEnBytesLog=%u\n", stat.tamEnBytesLog);
        printf("stat.numBloquesOcupados=%u\n", stat.numBloquesOcupados);
    }
    if (bumount() == -1) {
        fprintf(stderr, RED "Error al desmontar el dispositivo\n" RESET);
        return FALLO;
    }

    return EXITO;
}