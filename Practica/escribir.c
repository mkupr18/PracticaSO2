#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ficheros.h"

#define NUM_OFFSETS 5

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Sintaxis: escribir <nombre_dispositivo> \"<texto>\" <diferentes_inodos>\n");
        fprintf(stderr, "Offsets: 9000, 209000, 30725000, 409605000, 480000000\n");
        fprintf(stderr, "Si diferentes_inodos=0 se reserva un solo inodo para todos los offsets\n");
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    char *texto = argv[2];
    int diferentes_inodos = atoi(argv[3]);

    if (diferentes_inodos < 0 || diferentes_inodos > 1) {
        fprintf(stderr, "Error: diferentes_inodos debe ser 0 o 1\n");
        return FALLO;
    }

    if (strlen(texto) == 0) {
        fprintf(stderr, "Error: El texto no puede estar vacío\n");
        return FALLO;
    }

    if (bmount(nombre_dispositivo) == -1) {
        fprintf(stderr, "Error al montar el dispositivo\n");
        return FALLO;
    }

    unsigned int offsets[NUM_OFFSETS] = {9000, 209000, 30725000, 409605000, 480000000};
    int tamTexto = strlen(texto);
    char buf_original[tamTexto + 1];

    memset(buf_original, 0, tamTexto + 1);
    strcpy(buf_original, texto);

    unsigned int ninodo = reservar_inodo('f', 6);
    if (ninodo == -1) {
        fprintf(stderr, "Error al reservar inodo\n");
        bumount();
        return FALLO;
    }

    printf("Longitud texto: %d\n\n", tamTexto);

    for (int i = 0; i < NUM_OFFSETS; i++) {
        if (diferentes_inodos) {
            printf("Estamos dentro del bucle dif = 1\n");

            unsigned int nuevo_inodo = reservar_inodo('f', 6);
            if (nuevo_inodo == -1) {
                fprintf(stderr, "Error al reservar inodo\n");
                bumount();
                return FALLO;
            }
            printf("Estamos dentro del bucle dif. Tamaño nuevo inodo: %d\n", nuevo_inodo);
            ninodo = nuevo_inodo;
        }

        printf("Nº inodo reservado: %d\n", ninodo);
        printf("Offset: %u\n", offsets[i]);

        int bytes_escritos = mi_write_f(ninodo, buf_original, offsets[i], tamTexto);
        if (bytes_escritos == -1) {
            fprintf(stderr, "Error al escribir en el inodo %d\n", ninodo);
            bumount();
            return FALLO;
        }

        struct STAT stat;
        if (mi_stat_f(ninodo, &stat) == -1) {
            fprintf(stderr, "Error al obtener stat del inodo %d\n", ninodo);
            bumount();
            return FALLO;
        }

        printf("Bytes escritos: %d\n", bytes_escritos);
        printf("stat.tamEnBytesLog=%u\n", stat.tamEnBytesLog);
        printf("stat.numBloquesOcupados=%u\n\n", stat.numBloquesOcupados);

        char buf_leido[tamTexto + 1];
        memset(buf_leido, 0, tamTexto + 1);
        int bytes_leidos = mi_read_f(ninodo, buf_leido, offsets[i], tamTexto);
        if (bytes_leidos == -1) {
            fprintf(stderr, "Error al leer del inodo %d\n", ninodo);
            bumount();
            return FALLO;
        }
    }

    if (bumount() == -1) {
        fprintf(stderr, "Error al desmontar el dispositivo\n");
        return FALLO;
    }

    return EXITO;
}
