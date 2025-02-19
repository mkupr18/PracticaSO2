#include "bloques.h"
//#include "ficheros_basico.h"
#include <string.h>

int main(int argc, char **argv){
    // Queremos tres argumentos al iniciar el programa, nuestra memoria virtual
    if (argc != 3) {
        // Mostramos como queremos que se ejecute
        fprintf(stderr, "Uso: %s <nombre_dispositivo> <nbloques>\n", argv[0]);
        return FALLO;
    }

    // Obtenemos los parámetros, el nombre y el numero de bloques
    char *nombre_dispositivo = argv[1];
    int nbloques = atoi(argv[2]);

    // Caso de que el numero de bloques no pueda existir
    if (nbloques <= 0) {
        fprintf(stderr, "Error: El número de bloques debe ser mayor que 0.\n");
        return FALLO;
    }

    // Montamos el dispositivo virtual
    if (bmount(nombre_dispositivo) == FALLO) {
        perror("Error al montar el dispositivo virtual");
        return FALLO;
    }

    // Inicializamos el buffer con ceros
    unsigned char buffer[BLOCKSIZE];
    // Guardamos espacio
    memset(buffer, 0, BLOCKSIZE);

    // Escribir nbloques de ceros en el dispositivo virtual
    for (int i = 0; i < nbloques; i++) {
        if (bwrite(i, buffer) == FALLO) {
            perror("Error al escribir en el dispositivo virtual");
            bumount();
            return FALLO;
        }
    }

    printf("Dispositivo '%s' formateado con %d bloques de %d bytes.\n", nombre_dispositivo, nbloques, BLOCKSIZE);

    // Desmontar el dispositivo virtual
    if (bumount() == FALLO) {
        perror("Error al desmontar el dispositivo virtual");
        return FALLO;
    }

    return EXITO;
}