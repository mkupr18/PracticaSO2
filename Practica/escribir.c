#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ficheros.h"

#define NUM_OFFSETS 5

int main(int argc, char **argv) {
    // Validación de la sintaxis
    if (argc != 4) {
        fprintf(stderr, "Sintaxis: escribir <nombre_dispositivo> \"<texto>\" <diferentes_inodos>\n");
        fprintf(stderr, "Offsets: 9000, 209000, 30725000, 409605000, 480000000\n");
        fprintf(stderr, "Si diferentes_inodos=0 se reserva un solo inodo para todos los offsets\n");
        return FALLO;
    }

    // Obtener los argumentos
    char *nombre_dispositivo = argv[1];
    char *texto = argv[2];
    int diferentes_inodos = atoi(argv[3]);

    // Validar los permisos (deben estar entre 0 y 1)
    if (diferentes_inodos < 0 || diferentes_inodos > 1) {
        fprintf(stderr, "Error: diferentes_inodos debe ser 0 o 1\n");
        return FALLO;
    }

    // Montar el dispositivo
    if (bmount(nombre_dispositivo) == -1) {
        fprintf(stderr, "Error al montar el dispositivo\n");
        return FALLO;
    }

    // Offsets definidos
    unsigned int offsets[NUM_OFFSETS] = {9000, 209000, 30725000, 409605000, 480000000};
    int tamTexto = strlen(texto);
    char buf_original[tamTexto + 1]; // +1 para el carácter nulo

    memset(buf_original, 0, tamTexto + 1);
    strcpy(buf_original, texto);

    // Reservar un inodo inicial
    unsigned int ninodo = reservar_inodo('f', 6);
   
    if (ninodo == -1) {
        fprintf(stderr, "Error al reservar inodo\n");
        bumount();
        return FALLO;
    }

    printf("longitud texto: %d\n\n", tamTexto);

    // Escritura en los offsets
    for (int i = 0; i < NUM_OFFSETS; i++) {
        if (diferentes_inodos) {
            // Si diferentes_inodos=1, reservar un nuevo inodo para cada offset
            
            ninodo = reservar_inodo('f', 6);
            
            if (ninodo == -1) {
                fprintf(stderr, "Error al reservar inodo\n");
                bumount();
                return FALLO;
            }
           
        }

        printf("Nº inodo reservado: %d\n", ninodo);
        printf("offset: %u\n", offsets[i]);

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
    }

    // Desmontar el dispositivo
    if (bumount() == -1) {
        fprintf(stderr, "Error al desmontar el dispositivo\n");
        return FALLO;
    }

    return EXITO;
}