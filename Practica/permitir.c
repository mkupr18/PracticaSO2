// Autores: Kalyarat Asawapoom, Rupak Guni, Maria Kupriyenko

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ficheros.h"


int main(int argc, char **argv) {

    // Validación de la sintaxis
    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis incorrecta: ./permitir <nombre_dispositivo> <ninodo> <permisos>\n" RESET);
        fprintf(stderr, "Ejemplo: ./permitir disco.dat 5 6\n");
        return FALLO;
    }

    // Obtiene los argumentos
    char *nombre_dispositivo = argv[1];
    char *endptr;
    unsigned int ninodo = strtol(argv[2], &endptr, 10);
    if (*endptr != '\0') {
        fprintf(stderr, RED "Error: ninodo no es un número válido\n" RESET);
        return FALLO;
    }

    unsigned int permisos = strtol(argv[3], &endptr, 10);
    if (*endptr != '\0') {
        fprintf(stderr, RED "Error: permisos no es un número válido\n" RESET);
        return FALLO;
    }

    // Valida los permisos (deben estar entre 0 y 7)
    if (permisos < 0 || permisos > 7) {
        fprintf(stderr, RED "Error: Los permisos deben ser un valor entre 0 y 7\n" RESET);
        return FALLO;
    }

    // Monta el dispositivo
    if (bmount(nombre_dispositivo) == -1) {
        fprintf(stderr, RED "Error al montar el dispositivo\n" RESET);
        return FALLO;
    }

    // Cambia los permisos del inodo
    if (mi_chmod_f(ninodo, permisos) == -1) {
        fprintf(stderr, RED "Error al cambiar los permisos del inodo %u\n" RESET, ninodo);
        bumount(); // Desmonta el dispositivo antes de salir
        return FALLO;
    }

    // Desmonta el dispositivo
    if (bumount() == -1) {
        fprintf(stderr, RED "Error al desmontar el dispositivo\n" RESET);
        return FALLO;
    }
    return EXITO;
}