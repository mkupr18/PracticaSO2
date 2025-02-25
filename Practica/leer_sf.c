#include <stdio.h>
#include "ficheros_basico.h"

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

    // Mostrar información del superbloque
    printf("DATOS DEL SUPERBLOQUE\n");
    printf("posPrimerBloqueMB = %d\n", SB.posPrimerBloqueMB);
    printf("posUltimoBloqueMB = %d\n", SB.posUltimoBloqueMB);
    printf("posPrimerBloqueAI = %d\n", SB.posPrimerBloqueAI);
    printf("posUltimoBloqueAI = %d\n", SB.posUltimoBloqueAI);
    printf("posPrimerBloqueDatos = %d\n", SB.posPrimerBloqueDatos);
    printf("posUltimoBloqueDatos = %d\n", SB.posUltimoBloqueDatos);
    printf("posInodoRaiz = %d\n", SB.posInodoRaiz);
    printf("posPrimerInodoLibre = %d\n", SB.posPrimerInodoLibre);
    printf("cantBloquesLibres = %d\n", SB.cantBloquesLibres);
    printf("cantInodosLibres = %d\n", SB.cantInodosLibres);
    printf("totBloques = %d\n", SB.totBloques);
    // Mostrar tamaños de estructuras
    printf("\nsizeof struct superbloque: %lu\n", sizeof(struct superbloque));
    printf("sizeof struct inodo: %lu\n", sizeof(struct inodo));

    // Recorrido de la lista de inodos libres
    printf("\nRECORRIDO LISTA ENLAZADA DE INODOS LIBRES\n");
    struct inodo inodos[BLOCKSIZE / INODOSIZE];
    unsigned int inodoLibre = SB.posPrimerInodoLibre;

    for (unsigned int i = SB.posPrimerBloqueAI; i <= 10; i++) {
        if (bread(i, inodos) == FALLO) {
            perror("Error al leer el bloque de inodos");
            bumount();
            return FALLO;
        }

        for (unsigned int j = 0; j < 5; j++) {
            printf("%d ", inodoLibre);
            if (inodoLibre == UINT_MAX) {
                printf("\n");
                bumount();
                return EXITO;
            }
            inodoLibre = inodos[j].punterosDirectos[0];
        }
    }

    printf("\n");
    bumount();
    return EXITO;
}
