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
    int tam = (ninodos * INODOSIZE) / BLOCKSIZE;
    if ((ninodos % (BLOCKSIZE / INODOSIZE)) != 0)
    {
        tam++; // Añadimos un bloque si hay resto
    }
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
int initMB(){


}

int initAI(){


}