#include "ficheros_basico.h"

int tamAI(unsigned int ninodos){


}

int tamMB(unsigned int nbloques){


}

int initSB(unsigned int nbloques, unsigned int ninodos){
    SB.posPrimerBloqueMB = posSB+tamSB 
    SB.posUltimoBloqueMB = SB. posPrimerBloqueMB + tamMB(nbloques) -1
    SB.posPrimerBloqueAI = SB.posUltimoBloqueMB + 1    
    SB.posUltimoBloqueAI = SB. posPrimerBloqueAI + tamAI(ninodos) -1
    SB.posPrimerBloqueDatos = SB.posUltimoBloqueAI + 1    
    SB.posUltimoBloqueDatos = nbloques -1 
    SB.posInodoRaiz = 0 
    SB.posPrimerInodoLibre = 0 
    SB.cantBloquesLibres = nbloques  
    SB.cantInodosLibres = ninodos    
    SB.totBloques =  nbloques  
    SB.totInodos =  ninodos 
}

// Poner 3139 bits a 1 -> 392 bytes. NO llega a completar 1 bloque que es de 1024 bytes
// bufferMB sera un array de [0...391] = 255 en binario, es decir todos 1s
// bufferMB[392] = 1110 0000 o 224 en binario
int initMB(){


}

int initAI(){


}