#include "ficheros_basico.h"
#include "bloques.h"
#include <limits.h> // Para UINT_MAX
#include <string.h> // Para memset
#include <time.h>   // Para manejar timestamps

#define DEBUGN0 1

// Calcula el tamaño en bloque necesario para el mapa de bits
int tamMB(unsigned int nbloques)
{
    int tam = (nbloques / 8) / BLOCKSIZE;
    if ((nbloques % (8 * BLOCKSIZE)) != 0)
    {
        tam++; // Añadimos un bloque si hay resto
    }
    return tam;
}

// Calcula el tamaño en bloques del array de inodos
int tamAI(unsigned int ninodos)
{
    int tam = (ninodos * INODOSIZE) / BLOCKSIZE;
    if ((ninodos % (BLOCKSIZE / INODOSIZE)) != 0)
    {
        tam++; // Añadimos un bloque si hay resto
    }
    return tam;
}

// Inicializa el superbloque con la información del sistema de ficheros
int initSB(unsigned int nbloques, unsigned int ninodos)
{
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
    SB.cantBloquesLibres = nbloques - SB.posPrimerBloqueDatos; // Restamos los metadatos
    SB.cantInodosLibres = ninodos;
    SB.totBloques = nbloques;
    SB.totInodos = ninodos;

    return bwrite(posSB, &SB) == -1 ? FALLO : EXITO;
}

// Inicializa el mapa de bits, marcando como ocupados con 1 los bloques de metadatos
int initMB()
{
    struct superbloque SB;
    if (bread(posSB, &SB) == -1)
    {
        return FALLO;
    }

    unsigned char bufferMB[BLOCKSIZE];
    memset(bufferMB, 0, BLOCKSIZE);

    unsigned int tamMB_bloques = tamMB(SB.totBloques);
    unsigned int bloquesOcupados = SB.posPrimerBloqueDatos;
    unsigned int bytesOcupados = bloquesOcupados / 8;
    unsigned int bitsRestantes = bloquesOcupados % 8;

    // Inicializamos con 1 todos los bloques ocupados
    memset(bufferMB, 255, BLOCKSIZE);

    for (unsigned int i = 0; i < tamMB_bloques; i++)
    {
        if (i < bytesOcupados / BLOCKSIZE)
        {
            if (bwrite(SB.posPrimerBloqueMB + i, bufferMB) == -1)
            {
                return FALLO;
            }
        }
        else
        {
            // Manejo del último bloque con bits restantes
            unsigned char bufferAux[BLOCKSIZE];
            memset(bufferAux, 0, BLOCKSIZE);
            memcpy(bufferAux, bufferMB, bytesOcupados % BLOCKSIZE);
            memcpy(bufferMB, bufferAux, BLOCKSIZE);

            if (bitsRestantes > 0)
            {
                bufferMB[bytesOcupados % BLOCKSIZE] = (unsigned char)(~(0xFF >> bitsRestantes));
            }

            if (bwrite(SB.posPrimerBloqueMB + i, bufferMB) == -1)
            {
                return FALLO;
            }
            break;
        }
    }

    // SB.cantBloquesLibres -= bloquesOcupados;
    return bwrite(posSB, &SB) == -1 ? FALLO : EXITO;
}

// Inicialización de array de inodos
int initAI()
{
    struct superbloque SB;
    if (bread(posSB, &SB) == -1)
    {
        return FALLO;
    }
    // Array de inodos para almacenar los inodos
    struct inodo inodos[BLOCKSIZE / INODOSIZE];
    // Contador de inodos libres
    unsigned int contInodos = SB.posPrimerInodoLibre + 1;

    // Recorremos cada bloque del array de inodos
    for (unsigned int i = SB.posPrimerBloqueAI; i <= SB.posUltimoBloqueAI; i++)
    {
        // Inicializamos el bloque de inodos a 0
        memset(inodos, 0, BLOCKSIZE);
        // Recorremos cada inodo dentro del bloque
        for (unsigned int j = 0; j < BLOCKSIZE / INODOSIZE; j++)
        {
            // Marcamos el inodo como libre ('l')
            inodos[j].tipo = 'l';
            // Si aún hay inodos libres, enlazarlos
            if (contInodos < SB.totInodos)
            {
                inodos[j].punterosDirectos[0] = contInodos;
                contInodos++;
            }
            else
            {
                // Si no hay más inodos libres, marcamos el final con UINT_MAX
                inodos[j].punterosDirectos[0] = UINT_MAX;
                break;
            }
        }
        // Escribimos el bloque de inodos en el dispositivo
        if (bwrite(i, inodos) == -1)
        {
            return FALLO;
        }
    }

    return EXITO;
}

// Escribe el valor indicado por el parámetro bit: 0 (libre) o 1 (ocupado) en un determinado bit del MB que representa un bloque
int escribir_bit(unsigned int nbloque, unsigned int bit)
{
    struct superbloque SB;
    // Leemos el superbloque para obtener información necesaria
    if (bread(posSB, &SB) == -1)
    {
        return FALLO;
    }
    unsigned char bufferMB[BLOCKSIZE]; // Buffer auxiliar que hacemos del Mapa de bit
    unsigned char mascara = 128;       // 10000000 en binario, servirá de mascara

    // Calcula la posición del byte y bit en el mapa de bits
    unsigned int posbyte = nbloque / 8;
    unsigned int posbit = nbloque % 8;
    unsigned int nbloqueMB = posbyte / BLOCKSIZE;
    unsigned int nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;

    // Leemos el mapa de bit y lo pasamos al nuestro auxiliar, para no modificar directamente el principal y ahorar espacio
    if (bread(nbloqueabs, bufferMB) == -1)
    {
        fprintf(stderr, RED "Error leyendo el mapa de bits" RESET);
        return FALLO;
    }

    // Ajustamos la posición dentro del bloque leído
    posbyte = posbyte % BLOCKSIZE;

    // Desplazaramos la máscara para situarla en el bit correspondiente
    mascara >>= posbit;

    // Modificamos segun el bit correspondiente que queramos poner
    if (bit == 1)
    {
        bufferMB[posbyte] |= mascara; // Poner el bit a 1
    }
    else
    {
        bufferMB[posbyte] &= ~mascara; // Poner el bit a 0
    }

    // Escribir el bloque actualizado de vuelta en el dispositivo
    if (bwrite(nbloqueabs, bufferMB) == -1)
    {
        fprintf(stderr, RED "Error escribiendo en el mapa de bits" RESET);
        return FALLO;
    }

    return EXITO;
}

// Lee un determinado bit del MB y devuelve el valor del bit leído
char leer_bit(unsigned int nbloque)
{
    struct superbloque SB;
    // Leemos el superbloque para obtener información necesaria
    if (bread(posSB, &SB) == -1)
    {
        return FALLO;
    }

    unsigned char bufferMB[BLOCKSIZE]; // Buffer auxiliar que hacemos de Mapa de bit
    unsigned char mascara = 128;       // 10000000 en binario, servirá de mascara

    // Calcular la posición del byte y bit en el MB
    unsigned int posbyte = nbloque / 8;
    unsigned int posbit = nbloque % 8;
    unsigned int nbloqueMB = posbyte / BLOCKSIZE;
    unsigned int nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;

    // Leemos el mapa de bit y lo pasamos al nuestro auxiliar, para no modificar directamente el principal y ahorar espacio
    if (bread(nbloqueabs, bufferMB) == -1)
    {
        fprintf(stdout, RED "Error leyendo el mapa de bits" RESET);
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

    return mascara; // Devuelve 0 o 1 según el bit leído
}

// Encuentra el primer bloque libre, lo ocupa y devuelve su posición
int reservar_bloque()
{

    struct superbloque SB;
    // Leemos el superbloque para obtener información necesaria
    if (bread(posSB, &SB) == -1)
    {
        return FALLO;
    }

    // Buffer para almacenar un bloque del mapa de bits
    unsigned char bufferMB[BLOCKSIZE];
    // Buffer auxiliar lleno de 1s para comparación
    unsigned char bufferAux[BLOCKSIZE];
    // reservamos espacio en la memoria dinamica
    memset(bufferAux, 255, BLOCKSIZE);

    int nbloqueMB, posbyte, posbit, nbloque;

    // Recorremos los bloques del mapa de bits en busca de un bloque libre
    for (nbloqueMB = 0; nbloqueMB < SB.totBloques; nbloqueMB++)
    {
        // Leemos el bloque del mapa de bits correspondiente
        if (bread(SB.posPrimerBloqueMB + nbloqueMB, bufferMB) == -1)
        {
            return FALLO; // Error en la lectura
        }

        // Comparamos con el buffer auxiliar para encontrar un bit en 0
        if (memcmp(bufferMB, bufferAux, BLOCKSIZE) != 0)
        {
            // Recorremos los bytes dentro del bloque del mapa de bits
            for (posbyte = 0; posbyte < BLOCKSIZE; posbyte++)
            {
                if (bufferMB[posbyte] != 255)
                { // Si el byte no es 255, hay al menos un bit en 0
                    // Inicializamos la máscara para encontrar el bit libre
                    unsigned char mascara = 128; // 10000000 en binario
                    posbit = 0;

                    // Recorremos los bits del byte hasta encontrar el primer 0
                    while (bufferMB[posbyte] & mascara)
                    {
                        bufferMB[posbyte] <<= 1; // Desplazamos los bits a la izquierda
                        posbit++;
                    }

                    // Calculamos el número de bloque físico correspondiente
                    nbloque = (nbloqueMB * BLOCKSIZE * 8) + (posbyte * 8) + posbit;

                    // Marcamos el bloque como ocupado en el mapa de bits
                    escribir_bit(nbloque, 1);

                    // Decrementamos el contador de bloques libres en el superbloque
                    SB.cantBloquesLibres--;

                    // Guardamos el superbloque actualizado
                    bwrite(posSB, &SB);

                    // Limpiamos el contenido del nuevo bloque en la zona de datos
                    memset(bufferMB, 0, BLOCKSIZE);
                    bwrite(nbloque, bufferMB);

                    // Devolvemos el número del bloque reservado
                    return nbloque;
                }
            }
        }
    }
    return FALLO; // No se encontraron bloques libres
}

// Libera un bloque determinado
int liberar_bloque(unsigned int nbloque)
{
    struct superbloque SB;
    // Leemos el superbloque para obtener información necesaria
    if (bread(posSB, &SB) == -1)
    {
        return FALLO;
    }

    // Marcamos el bloque como libre en el mapa de bits
    escribir_bit(nbloque, 0);

    // Incrementamos la cantidad de bloques libres en el superbloque
    SB.cantBloquesLibres++;

    // Guardamos el superbloque actualizado
    bwrite(posSB, &SB);

    // Devolvemos el número del bloque liberado
    return nbloque;
}

// Escribe el contenido de un inodo en la posición correspondiente del array de inodos
int escribir_inodo(unsigned int ninodo, struct inodo *inodo)
{
    struct superbloque SB;
    if (bread(posSB, &SB) == -1)
    {
        return FALLO;
    }

    // Calcula el bloque del array de inodos donde se encuentra el inodo solicitado
    unsigned int nbloqueAI = SB.posPrimerBloqueAI + (ninodo / (BLOCKSIZE / INODOSIZE));
    struct inodo inodos[BLOCKSIZE / INODOSIZE];

    // Inicializa el array de inodos
    memset(inodos, 0, BLOCKSIZE);

    // Lee el bloque de inodos
    if (bread(nbloqueAI, inodos) == -1)
    {
        return FALLO;
    }
    // Escribe el inodo en la posición correspondiente dentro del bloque
    unsigned int posInodo = ninodo % (BLOCKSIZE / INODOSIZE);
    inodos[posInodo] = *inodo;

    // printf("escribir_inodo: Guardando inodo %u con numBloquesOcupados=%u, tamEnBytesLog=%u\n",
    //   ninodo, inodo->numBloquesOcupados, inodo->tamEnBytesLog);

    // Escribe el bloque de inodos actualizado en el dispositivo
    if (bwrite(nbloqueAI, inodos) == -1)
    {
        return FALLO; // Error al escribir el bloque de inodos
    }

    return EXITO; // Éxito
}

// Lee un inodo del array de inodos y lo almacena en la estructura proporcionada
int leer_inodo(unsigned int ninodo, struct inodo *inodo)
{
    struct superbloque SB;
    if (bread(posSB, &SB) == -1)
    {
        return FALLO;
    }

    // Calcula el bloque del array de inodos donde se encuentra el inodo solicitado
    unsigned int nbloqueAI = SB.posPrimerBloqueAI + (ninodo / (BLOCKSIZE / INODOSIZE));
    struct inodo inodos[BLOCKSIZE / INODOSIZE];

    // Lee el bloque de inodos
    if (bread(nbloqueAI, inodos) == -1)
    {
        return FALLO;
    }

    // Guardamos el inodo leído en la estructura proporcionada
    *inodo = inodos[ninodo % (BLOCKSIZE / INODOSIZE)];
    return EXITO;
}

// Reserva un inodo y devuelve su número
int reservar_inodo(unsigned char tipo, unsigned char permisos)
{
    struct superbloque SB;
    if (bread(posSB, &SB) == -1 || SB.cantInodosLibres == 0)
    {
        return FALLO; // Error si no hay inodos libres
    }

    // Guarda la posición del primer inodo libre
    unsigned int posInodoReservado = SB.posPrimerInodoLibre;
    struct inodo inodo;
    memset(&inodo, 0, sizeof(struct inodo));
    // Inicializamos los campos del inodo
    inodo.tipo = tipo;
    inodo.permisos = permisos;
    inodo.nlinks = 1;
    inodo.tamEnBytesLog = 0;
    inodo.numBloquesOcupados = 0;
    inodo.atime = inodo.mtime = inodo.ctime = inodo.btime = time(NULL);

    // Inicializamos los punteros directos e indirectos
    for (int i = 0; i < 12; i++)
        inodo.punterosDirectos[i] = 0;
    for (int i = 0; i < 3; i++)
        inodo.punterosIndirectos[i] = 0;

    // Leemos el inodo libre para obtener el siguiente inodo disponible
    struct inodo inodoLibre;
    if (leer_inodo(posInodoReservado, &inodoLibre) == -1)
    {
        return FALLO;
    }

    // Actualizamos la posición del primer inodo libre en el superbloque
    SB.posPrimerInodoLibre = inodoLibre.punterosDirectos[0];
    SB.cantInodosLibres--;

    // Escribimos el inodo reservado en el array de inodos y actualizamos el superbloque
    if (escribir_inodo(posInodoReservado, &inodo) == -1 || bwrite(posSB, &SB) == -1)
    {
        return FALLO;
    }

    return posInodoReservado; // Devolvemos la posición del inodo reservado
}

// Determina el nivel de punteros en el que se encuentra el bloque lógico y devuelve el puntero correspondiente
int obtener_nRangoBL(struct inodo *inodo, unsigned int nblogico, unsigned int *ptr)
{
    if (nblogico < DIRECTOS)
    { // Bloques directos
        *ptr = inodo->punterosDirectos[nblogico];
        return 0;
    }
    else if (nblogico < INDIRECTOS0)
    { // Indirecto simple
        *ptr = inodo->punterosIndirectos[0];
        return 1;
    }
    else if (nblogico < INDIRECTOS1)
    { // Indirecto doble
        *ptr = inodo->punterosIndirectos[1];
        return 2;
    }
    else if (nblogico < INDIRECTOS2)
    { // Indirecto triple
        *ptr = inodo->punterosIndirectos[2];
        return 3;
    }
    *ptr = 0;
    return FALLO; // Error: bloque lógico fuera de rango
}

// Calcula el índice dentro del bloque de punteros dependiendo del nivel de punteros
int obtener_indice(unsigned int nblogico, int nivel_punteros)
{
    if (nblogico < DIRECTOS)
        return nblogico; // Bloques directos
    if (nblogico < INDIRECTOS0)
        return nblogico - DIRECTOS; // Indirecto simple
    if (nblogico < INDIRECTOS1)
    {
        if (nivel_punteros == 2)
            return (nblogico - INDIRECTOS0) / NPUNTEROS; // Índice en indirecto doble
        if (nivel_punteros == 1)
            return (nblogico - INDIRECTOS0) % NPUNTEROS; // Índice en bloque de datos
    }
    if (nblogico < INDIRECTOS2)
    {
        if (nivel_punteros == 3)
            return (nblogico - INDIRECTOS1) / (NPUNTEROS * NPUNTEROS); // Índice en indirecto triple
        if (nivel_punteros == 2)
            return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) / NPUNTEROS;
        if (nivel_punteros == 1)
            return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) % NPUNTEROS;
    }
    return FALLO; // Error: fuera de rango
}

// Traduce un bloque lógico a su correspondiente bloque físico, reservándolo si es necesario
int traducir_bloque_inodo(unsigned int ninodo, unsigned int nblogico, unsigned char reservar)
{
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == -1)
        return FALLO;

    unsigned int ptr, ptr_ant;
    int nRangoBL = obtener_nRangoBL(&inodo, nblogico, &ptr); // Obtener el nivel de punteros
    int nivel_punteros = nRangoBL;

    unsigned int buffer[NPUNTEROS]; // Buffer para bloques de punteros
    int indice;

    // Descendemos por los niveles de punteros
    while (nivel_punteros > 0)
    {
        if (ptr == 0)
        { // Si el puntero no está asignado
            if (reservar == 0)
            { // No reserva si no se solicita
                return FALLO;
            }
            ptr = reservar_bloque(); // Reserva un nuevo bloque
            inodo.numBloquesOcupados++;
            inodo.ctime = time(NULL);
            if (nivel_punteros == nRangoBL)
            {
                inodo.punterosIndirectos[nRangoBL - 1] = ptr;
                #if DEBUGN0
                    fprintf(stdout, GRAY "[traducir_bloque_inodo()\u2192 inodo.punterosIndirectos[%d] = %d (reservado BF %d para punteros_nivel%d)]\n" RESET, nRangoBL - 1, ptr, ptr, nRangoBL);
                #endif
            }
            else
            {
                buffer[indice] = ptr;
                bwrite(ptr_ant, buffer);
                #if DEBUGN0
                    fprintf(stdout, GRAY "[traducir_bloque_inodo()\u2192 punteros_nivel%d [%d] = %d (reservado BF %d para punteros_nivel%d)]\n" RESET, nivel_punteros + 1, indice, ptr, ptr, nivel_punteros);
                #endif
            }
            memset(buffer, 0, BLOCKSIZE); // Limpia el buffer
        }
        else
        {
            bread(ptr, buffer); // Leemos el bloque de punteros
        }

        indice = obtener_indice(nblogico, nivel_punteros); // Obtenemos el índice dentro del bloque
        ptr_ant = ptr;
        ptr = buffer[indice];
        nivel_punteros--;
    }

    // Último nivel
    if (ptr == 0)
    {
        if (reservar == 0)
        {
            return FALLO;
        }
        ptr = reservar_bloque(); // Reserva el bloque de datos
        inodo.numBloquesOcupados++;
        inodo.ctime = time(NULL);
        if (nRangoBL == 0)
        {
            inodo.punterosDirectos[nblogico] = ptr;
            #if DEBUGN0
                fprintf(stdout,GRAY "[traducir_bloque_inodo()\u2192 inodo.punterosDirectos[%d] = %d (reservado BF %d para BL %d)]\n" RESET, nblogico, ptr, ptr, nblogico);
            #endif
        }
        else
        {
            buffer[indice] = ptr;
            bwrite(ptr_ant, buffer);
            #if DEBUGN0
                fprintf(stdout,GRAY "[traducir_bloque_inodo()\u2192 punteros_nivel1 [%d] = %d (reservado BF %d para BL %d)]\n" RESET, indice, ptr, ptr, nblogico);
            #endif
        }
    }

    if (escribir_inodo(ninodo, &inodo) == -1)
    {
        return FALLO;
    }
    return ptr;
}
int liberar_inodo(unsigned int ninodo){
    struct superbloque sb;
    struct inodo inodo;

    // Leer el superbloque
    if (bread(posSB, &sb) == -1)
    {
        return FALLO;
    }

    // Leer el inodo a liberar
    if (leer_inodo(ninodo, &inodo) == -1)
    {
        return FALLO;
    }

    // Liberar todos los bloques del inodo
    int bloques_liberados = liberar_bloques_inodo(0, &inodo);
    if (bloques_liberados == -1)
    {
        return FALLO;
    }

    // Actualizar el inodo
    inodo.numBloquesOcupados -= bloques_liberados;
    inodo.tipo = 'l'; // Tipo libre
    inodo.tamEnBytesLog = 0;
    // Actualizar la lista de inodos libres
    inodo.punterosDirectos[0] = sb.posPrimerInodoLibre;
    sb.posPrimerInodoLibre = ninodo;
    sb.cantInodosLibres++;

    // Actualizar tiempos
    inodo.ctime = time(NULL);

    // Escribir los cambios
    if (bwrite(posSB, &sb) == -1)
    {
        return FALLO;
    }

    if (escribir_inodo(ninodo, &inodo) == -1)
    {
        return FALLO;
    }

    return ninodo;
}

int liberar_bloques_inodo(unsigned int primerBL, struct inodo *inodo)
{
    return EXITO;
}
