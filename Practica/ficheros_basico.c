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

// Inicializa el mapa de bits, marcando como ocupados con 1 los bloques de metadatos
int initMB() {
    struct superbloque SB;
    // Leemos el superbloque para obtener información necesaria
    if (bread(posSB, &SB) == -1) {
        return FALLO;
    }
    // Obtenemos el tamaño de mapa de bits en bloques
    unsigned int tamMB_bloques = tamMB(SB.totBloques);
    // Obtenemos el tamaño de bloques ocupados por meta datos
    unsigned int bloquesOcupados = SB.posPrimerBloqueDatos;
    // Obtenemos el tamaño en bytes los bloques ocupados por meta datos
    unsigned int bytesOcupados = bloquesOcupados / 8;
    // Obtenemos cuántos bits restantes están ocupados
    unsigned int bitsRestantes = bloquesOcupados % 8;
    // Buffer para el mapa de bits
    unsigned char bufferMB[BLOCKSIZE];

    // Inicializamos el buffer con todos los bits a 1 (255 en decimal = 11111111 en binario)
    memset(bufferMB, 255, BLOCKSIZE);
    // Recorremos cada bloque del mapa de bits e escribir el bloque completo con bits a 1
    for (unsigned int i = 0; i < tamMB_bloques; i++) {
        if (i < (bytesOcupados / BLOCKSIZE)) {
            if (bwrite(SB.posPrimerBloqueMB + i, bufferMB) == -1) {
                return FALLO;
            }
        } else {
            // Si estamos en un bloque parcialmente ocupado
            memset(bufferMB, 0, BLOCKSIZE);  // Inicializamos el buffer a 0
            memcpy(bufferMB, bufferMB, bytesOcupados % BLOCKSIZE); // Copiamos bytes ocupados

            // Marcar los bits restantes con 1
            if (bitsRestantes > 0) {
                bufferMB[bytesOcupados % BLOCKSIZE] = (unsigned char)(~(0xFF >> bitsRestantes));
            }
            if (bwrite(SB.posPrimerBloqueMB + i, bufferMB) == -1) {
                return FALLO;
            }
            break;
        }
    }
    // Actualizamos la cantidad de bloques libres en el superbloque
    SB.cantBloquesLibres -= bloquesOcupados;
    return bwrite(posSB, &SB) == -1 ? FALLO : EXITO;
}

// Inicialización de array de inodos
int initAI() {
    struct superbloque SB;
    // Leemos el superbloque para obtener información necesaria
    if (bread(posSB, &SB) == -1) {
        return FALLO;
    }
    // Array de inodos para almacenar los inodos
    struct inodo inodos[BLOCKSIZE / INODOSIZE];
    // Contador de inodos libres
    unsigned int contInodos = SB.posPrimerInodoLibre + 1;

    // Recorremos cada bloque del array de inodos
    for (unsigned int i = SB.posPrimerBloqueAI; i <= SB.posUltimoBloqueAI; i++) {
        // Inicializamos el bloque de inodos a 0
        memset(inodos, 0, BLOCKSIZE);
        // Recorremos cada inodo dentro del bloque
        for (unsigned int j = 0; j < BLOCKSIZE / INODOSIZE; j++) {
            // Marcamos el inodo como libre ('l')
            inodos[j].tipo = 'l';
            // Si aún hay inodos libres, enlazarlos
            if (contInodos < SB.totInodos) {
                inodos[j].punterosDirectos[0] = contInodos;
                contInodos++;
            } else {
                // Si no hay más inodos libres, marcamos el final con UINT_MAX
                inodos[j].punterosDirectos[0] = UINT_MAX;
                break;
            }
        }
        // Escribimos el bloque de inodos en el dispositivo
        if (bwrite(i, inodos) == -1) {
            return FALLO;
        }
    }

    return EXITO;
}
