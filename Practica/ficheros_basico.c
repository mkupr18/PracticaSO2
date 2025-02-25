#include "ficheros_basico.h"
#include <limits.h> // Para UINT_MAX
#include <string.h> // Para memset

// Calcula el tamaño en bloque necesario para el mapa de bits
int tamMB(unsigned int nbloques){
    int tam = (nbloques / 8) / BLOCKSIZE;
    if ((nbloques % (8 * BLOCKSIZE)) != 0) {
        tam++; // Añadimos un bloque si hay resto
    }
    return tam;
}

// Calcula el tamaño en bloques del array de inodos
int tamAI(unsigned int ninodos){
    // ninodos =nbloques/4;
    int tam = (ninodos * INODOSIZE)/BLOCKSIZE;
    printf("%d\n",ninodos);
    if ((ninodos % (BLOCKSIZE / INODOSIZE)) != 0)
    {
        tam++; // Añadimos un bloque si hay resto
    }
    printf("%d\n",tam);
    return tam;
}

// Inicializa el superbloque con la información del sistema de ficheros
int initSB(unsigned int nbloques, unsigned int ninodos){
    struct superbloque SB;
    if (bread(posSB, &SB) == -1)
        return FALLO;

    SB.posPrimerBloqueMB = posSB + tamSB;
    SB.posUltimoBloqueMB = SB.posPrimerBloqueMB + tamMB(nbloques) - 1;
    SB.posPrimerBloqueAI = SB.posUltimoBloqueMB + 1;
    SB.posUltimoBloqueAI = SB.posPrimerBloqueAI + tamAI(ninodos) - 1;
    SB.posPrimerBloqueDatos = SB.posUltimoBloqueAI + 1;
    SB.posUltimoBloqueDatos = nbloques - 1;
    SB.posInodoRaiz = 0;
    SB.posPrimerInodoLibre = 0;
    SB.cantBloquesLibres = nbloques - (SB.posPrimerBloqueDatos); // Restamos los metadatos
    SB.cantInodosLibres = ninodos;
    SB.totBloques =  nbloques;
    SB.totInodos =  ninodos;

    return bwrite(posSB, &SB) == -1 ? FALLO : EXITO;
}

// Poner 3139 bits a 1 -> 392 bytes. NO llega a completar 1 bloque que es de 1024 bytes
// bufferMB sera un array de [0...391] = 255 en binario, es decir todos 1s
// bufferMB[392] = 1110 0000 o 224 en binario
// Inicializa el mapa de bits, marcando como ocupados los bloques de metadatos
int initMB() {
    struct superbloque SB;
    if (bread(posSB, &SB) == -1) {
        return FALLO;
    }

    unsigned int tamMB_bloques = tamMB(SB.totBloques);
    unsigned int bloquesOcupados = SB.posPrimerBloqueDatos;
    unsigned int bytesOcupados = bloquesOcupados / 8;
    unsigned int bitsRestantes = bloquesOcupados % 8;

    unsigned char bufferMB[BLOCKSIZE];
    memset(bufferMB, 255, BLOCKSIZE);

    for (unsigned int i = 0; i < tamMB_bloques; i++) {
        if (i < (bytesOcupados / BLOCKSIZE)) {
            if (bwrite(SB.posPrimerBloqueMB + i, bufferMB) == -1) {
                return FALLO;
            }
        } else {
            memset(bufferMB, 0, BLOCKSIZE);
            memcpy(bufferMB, bufferMB, bytesOcupados % BLOCKSIZE);
            if (bitsRestantes > 0) {
                bufferMB[bytesOcupados % BLOCKSIZE] = (unsigned char)(~(0xFF >> bitsRestantes));
            }
            if (bwrite(SB.posPrimerBloqueMB + i, bufferMB) == -1) {
                return FALLO;
            }
            break;
        }
    }

    SB.cantBloquesLibres -= bloquesOcupados;
    return bwrite(posSB, &SB) == -1 ? FALLO : EXITO;
}


int initAI() {
    struct superbloque SB;
    if (bread(posSB, &SB) == -1) {
        return FALLO;
    }

    struct inodo inodos[BLOCKSIZE / INODOSIZE];
    unsigned int contInodos = SB.posPrimerInodoLibre + 1;

    for (unsigned int i = SB.posPrimerBloqueAI; i <= SB.posUltimoBloqueAI; i++) {
        memset(inodos, 0, BLOCKSIZE);

        for (unsigned int j = 0; j < BLOCKSIZE / INODOSIZE; j++) {
            inodos[j].tipo = 'l';
            if (contInodos < SB.totInodos) {
                inodos[j].punterosDirectos[0] = contInodos;
                contInodos++;
            } else {
                inodos[j].punterosDirectos[0] = UINT_MAX;
                break;
            }
        }

        if (bwrite(i, inodos) == -1) {
            return FALLO;
        }
    }

    return EXITO;
}
