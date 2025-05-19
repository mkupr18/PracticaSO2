// Autores: Kalyarat Asawapoom, Rupak Guni, Maria Kupriyenko
#include <stdio.h>
#include <string.h>
#include "directorios.h" 
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, RED "Sintaxis: ./mi_rm <disco> </ruta>\n" RESET);
        return FALLO;
    }
    // Monta el dispositivo
    if (bmount(argv[1]) < 0) return FALLO;
    // Llama a mi_unlink
    if (mi_unlink(argv[2]) < 0) {
        fprintf(stderr, RED "Error: No existe el archivo o el directorio.\n" RESET);
        bumount();
        return FALLO;
    }
    // Desmonta el dispositivo
    bumount();
    return EXITO;
}
