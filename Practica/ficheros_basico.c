#include "ficheros_basico.h"
#include "bloques.h"
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


int escribir_bit(unsigned int nbloque, unsigned int bit) {
    unsigned char bufferMB[BLOCKSIZE]; //Buffer auxiliar que hacemos de Mapa de bit
    unsigned char mascara = 128; // 10000000 en binario, servirá de mascara

    // Calcula la posición del byte y bit en el mapa de bits
    unsigned int posbyte = nbloque / 8;
    unsigned int posbit = nbloque % 8;
    unsigned int nbloqueMB = posbyte / BLOCKSIZE;
    unsigned int nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;

    // Leemos el mapa de bit y lo pasamos en nuestro auxiliar, para no modificar directamente en el principal para ahorar espacio
    if (bread(nbloqueabs, bufferMB) == -1) {
        perror(RED "Error leyendo el mapa de bits" RESET);
        return -1;
    }

    // Ajustamos la posición dentro del bloque leído
    posbyte = posbyte % BLOCKSIZE;

    // Desplazaramos la máscara para situarla en el bit correspondiente
    mascara >>= posbit;

    // Modificamos segun el bit correspondiente que queramos poner
    if (bit == 1) {
        bufferMB[posbyte] |= mascara;  // Poner el bit a 1
    } else { 
        bufferMB[posbyte] &= ~mascara; // Poner el bit a 0
    }

    // Escribir el bloque actualizado de vuelta en el dispositivo
    if (bwrite(nbloqueabs, bufferMB) == -1) {
        perror(RED "Error escribiendo en el mapa de bits" RESET);
        return -1;
    }

    return 0;
}

char leer_bit(unsigned int nbloque) {
    unsigned char bufferMB[BLOCKSIZE];  //Buffer auxiliar que hacemos de Mapa de bit
    unsigned char mascara = 128;  // 10000000 en binario, servirá de mascara

    // Calcular la posición del byte y bit en el MB
    unsigned int posbyte = nbloque / 8;
    unsigned int posbit = nbloque % 8;
    unsigned int nbloqueMB = posbyte / BLOCKSIZE;
    unsigned int nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;

    // Leemos el mapa de bit y lo pasamos en nuestro auxiliar, para no modificar directamente en el principal para ahorar espacio
    if (bread(nbloqueabs, bufferMB) == -1) {
        perror("Error leyendo el mapa de bits");
        return -1;
    }

    // Ajustamos la posición dentro del bloque leído
    posbyte = posbyte % BLOCKSIZE;

    // Desplazamos la máscara hasta la posición del bit
    mascara >>= posbit;

    // Aplicamos el operador AND para extraer el bit
    mascara &= bufferMB[posbyte];

    // Desplazamos a la derecha para obtener el bit en la posición menos significativa
    mascara >>= (7 - posbit);

    return mascara;  // Devuelve 0 o 1 según el bit leído
}

