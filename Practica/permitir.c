#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ficheros_basico.h"
#include "ficheros.h"

int main(int argc, char **argv) {
    // Validación de la sintaxis
    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis incorrecta: ./permitir <nombre_dispositivo> <ninodo> <permisos>\n" RESET);
        return FALLO;
    }

    // Obtener los argumentos
    char *nombre_dispositivo = argv[1];
    unsigned int ninodo = atoi(argv[2]);
    unsigned int permisos = atoi(argv[3]);

    // Validar los permisos (deben estar entre 0 y 7)
    if (permisos < 0 || permisos > 7) {
        fprintf(stderr, RED "Error: Los permisos deben ser un valor entre 0 y 7\n" RESET);
        return FALLO;
    }

    // Montar el dispositivo
    if (bmount(nombre_dispositivo) == -1) {
        fprintf(stderr, RED "Error al montar el dispositivo\n" RESET);
        return FALLO;
    }

    // Cambiar los permisos del inodo
    if (mi_chmod_f(ninodo, permisos) == -1) {
        fprintf(stderr, RED "Error al cambiar los permisos del inodo\n" RESET);
        bumount(); // Desmontar el dispositivo antes de salir
        return FALLO;
    }

    // Desmontar el dispositivo
    if (bumount() == -1) {
        fprintf(stderr, RED "Error al desmontar el dispositivo\n" RESET);
        return FALLO;
    }

    printf("Permisos del inodo %d cambiados a %d correctamente.\n", ninodo, permisos);
    return EXITO;
}