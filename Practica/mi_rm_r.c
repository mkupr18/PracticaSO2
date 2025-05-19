// Autores: Kalyarat Asawapoom, Rupak Guni, Maria Kupriyenko
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "directorios.h"

// Función auxiliar recursiva para eliminar contenido
static int eliminar_recursivo(const char *camino) {
    unsigned int p_inodo_dir, p_inodo, p_entrada;
    struct inodo inodo;
    char buffer[TAMBUFFER];
    char ruta_absoluta[strlen(camino) + TAMNOMBRE + 2];
    
    // Buscar la entrada
    p_inodo_dir = 0;
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
    if (error < 0) return error;
    
    // Leer el inodo
    if (leer_inodo(p_inodo, &inodo) < 0) return FALLO;
    
    // Si es un directorio y no está vacío, eliminar su contenido primero
    if (inodo.tipo == 'd' && inodo.tamEnBytesLog > 0) {
        // Listar contenido del directorio
        int n = mi_dir(camino, buffer, 's');
        if (n < 0) return n;
        
        // Procesar cada entrada
        char *token = strtok(buffer, "\t");
        while (token != NULL) {
            // Construir ruta absoluta
            sprintf(ruta_absoluta, "%s%s", camino, token);
            
            // Si es directorio, añadir '/'
            struct STAT stat;
            if (mi_stat(ruta_absoluta, &stat) < 0) return FALLO;
            if (stat.tipo == 'd') {
                strcat(ruta_absoluta, "/");
            }
            
            // Eliminar recursivamente
            if (eliminar_recursivo(ruta_absoluta) < 0) return FALLO;
            
            token = strtok(NULL, "\t");
        }
    }
    
    // Eliminar la entrada actual
    return mi_unlink(camino);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Sintaxis: ./mi_rm_r disco /ruta\n");
        return FALLO;
    }

    // Montar el dispositivo
    if (bmount(argv[1]) == FALLO) {
        fprintf(stderr, "Error al montar el dispositivo\n");
        return FALLO;
    }

    // Verificar que no se intenta borrar el directorio raíz
    if (strcmp(argv[2], "/") == 0) {
        fprintf(stderr, "Error: no se puede borrar el directorio raíz\n");
        bumount();
        return FALLO;
    }

    // Eliminar recursivamente
    int error = eliminar_recursivo(argv[2]);
    if (error < 0) {
        mostrar_error_buscar_entrada(error);
        bumount();
        return FALLO;
    }

    // Desmontar el dispositivo
    if (bumount() == FALLO) {
        fprintf(stderr, "Error al desmontar el dispositivo\n");
        return FALLO;
    }

    return EXITO;
}