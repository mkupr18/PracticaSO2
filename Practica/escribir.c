#include “ficheros.h”
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <nombre_dispositivo>\n", argv[0]);
        return FALLO;
    }
    
    // Montar el dispositivo virtual
    if (bmount(argv[1]) == FALLO) {
        perror("Error al montar el dispositivo virtual");
        return FALLO;
    }

    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        perror("Error al leer el superbloque");
        bumount();
        return FALLO;
    }
}