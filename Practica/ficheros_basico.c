// Autores: Kalyarat Asawapoom, Rupak Guni, Maria Kupriyenko

#include "ficheros_basico.h"
#include <limits.h> // Para UINT_MAX
#include <string.h> // Para memset
#include <time.h>   // Para manejar timestamps

#define DEBUGN0 0
int breads = 0;
int bwrites = 0;

/**
 * @brief Calcula el tamaño en bloques necesario para el mapa de bits.
 * 
 * @param nbloques Número total de bloques del sistema de archivos.
 * 
 * @pre nbloques debe ser un número positivo.
 * @post Retorna el número de bloques necesarios para almacenar el mapa de bits.
 * 
 * @return Número de bloques necesarios para el mapa de bits.
 */

int tamMB(unsigned int nbloques)
{
    int tam = (nbloques / 8) / BLOCKSIZE;
    if ((nbloques % (8 * BLOCKSIZE)) != 0)
    {
        tam++; // Añadimos un bloque si hay resto
    }
    return tam;
}

/**
 * @brief Calcula el tamaño en bloques del array de inodos.
 * 
 * @param ninodos Número total de inodos del sistema de archivos.
 * 
 * @pre `ninodos` debe ser un número positivo.
 * @post Retorna el número de bloques necesarios para almacenar la estructura de inodos.
 * 
 * @return Número de bloques necesarios para el array de inodos.
 */
int tamAI(unsigned int ninodos)
{
    int tam = (ninodos * INODOSIZE) / BLOCKSIZE;
    if ((ninodos % (BLOCKSIZE / INODOSIZE)) != 0)
    {
        tam++; // Añadimos un bloque si hay resto
    }
    return tam;
}

/**
 * @brief Inicializa el superbloque con la información del sistema de archivos.
 * 
 * @param nbloques Número total de bloques del sistema de archivos.
 * @param ninodos Número total de inodos del sistema de archivos.
 * 
 * @pre `nbloques` y `ninodos` deben ser valores positivos.
 *      Debe ser posible leer el superbloque desde el almacenamiento (`bread(posSB, &SB) != -1`).
 *      Las constantes como `posSB`, `tamSB`, `BLOCKSIZE`, `INODOSIZE` deben estar definidas.
 *      La estructura `superbloque` debe estar correctamente declarada.
 * @post Se inicializa el superbloque con la información esencial del sistema de archivos.
 *       Se definen las posiciones de los bloques del mapa de bits, inodos y datos.
 *       Se establecen los contadores de bloques e inodos libres.
 *       Se escribe la estructura del superbloque en almacenamiento (`bwrite(posSB, &SB)`).
 *       Retorna `EXITO` si la operación es exitosa, o `FALLO` si ocurre un error.
 * 
 * @return `EXITO` si la inicialización fue correcta, `FALLO` en caso de error.
 */
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

/**
 * @brief Inicializa el mapa de bits, marcando como ocupados los bloques de metadatos.
 * 
 * @pre El superbloque debe estar inicializado y accesible.
 *      `bread(posSB, &SB) != -1` debe ser verdadero.
 *      `bwrite()` debe ser capaz de escribir en el almacenamiento.
 * @post Los bloques de metadatos estarán marcados como ocupados en el mapa de bits.
 *       Se actualizará el superbloque y se guardará en almacenamiento.
 *       Retorna `EXITO` si la operación es exitosa, `FALLO` en caso de error.
 * 
 * @return `EXITO` si la inicialización fue correcta, `FALLO` en caso de error.
 */
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

/**
 * @brief Inicializa el array de inodos, marcándolos como libres y enlazándolos en lista.
 * 
 * @pre El superbloque debe estar inicializado y accesible.
 *      `bread(posSB, &SB) != -1` debe ser verdadero.
 *      `bwrite()` debe ser capaz de escribir en el almacenamiento.
 *      La estructura `struct inodo` debe estar definida.
 * @post Todos los inodos se marcarán como libres ('l').
 *       Se enlazarán los inodos libres en una lista enlazada.
 *       Se escribirán en el almacenamiento los bloques de inodos inicializados.
 *       Retorna `EXITO` si la operación es exitosa, `FALLO` en caso de error.
 * 
 * @return `EXITO` si la inicialización fue correcta, `FALLO` en caso de error.
 */
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
/**
 * @brief Escribe el valor indicado por el parámetro bit en un determinado bit del mapa de bits.
 *
 * @param nbloque Número del bloque cuyo bit se modificará.
 * @param bit Valor a escribir en el bit (0 para libre, 1 para ocupado).
 *
 * @pre `nbloque` debe ser un índice válido dentro del sistema de archivos.
 *      `bit` debe ser 0 o 1.
 *      El superbloque debe estar correctamente inicializado y accesible.
 *
 * @post Se actualiza el bit correspondiente en el mapa de bits.
 *       Si el proceso es exitoso, el bit del bloque en el MB reflejará el nuevo estado.
 *       En caso de error, el mapa de bits no se modificará y se retornará `FALLO`.
 *
 * @return `EXITO` si la escritura fue exitosa, `FALLO` en caso de error.
 */
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

    // Escribimos el bloque actualizado de vuelta en el dispositivo
    if (bwrite(nbloqueabs, bufferMB) == -1)
    {
        fprintf(stderr, RED "Error escribiendo en el mapa de bits" RESET);
        return FALLO;
    }

    return EXITO;
}

/**
 * @brief Lee el valor de un determinado bit en el mapa de bits.
 *
 * @param nbloque Número del bloque cuyo bit se leerá.
 *
 * @pre `nbloque` debe ser un índice válido dentro del sistema de archivos.
 *      El superbloque debe estar correctamente inicializado y accesible.
 *
 * @post Se obtiene el valor actual del bit correspondiente en el mapa de bits.
 *       En caso de error, se retornará `-1`.
 *
 * @return `0` si el bit indica un bloque libre, `1` si está ocupado, `-1` en caso de error.
 */
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

    // Calcula la posición del byte y bit en el MB
    unsigned int posbyte = nbloque / 8;
    unsigned int posbit = nbloque % 8;
    unsigned int nbloqueMB = posbyte / BLOCKSIZE;
    unsigned int nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;

    // Leemos el mapa de bit y lo pasamos al nuestro auxiliar, para no modificar directamente el principal y ahorar espacio
    if (bread(nbloqueabs, bufferMB) == -1)
    {
        fprintf(stdout, RED "Error leyendo el mapa de bits" RESET);
        return FALLO;
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

/**
 * @brief Encuentra el primer bloque libre, lo ocupa y devuelve su posición.
 *
 * @pre El sistema de archivos debe estar montado y accesible.
 *      Debe haber bloques libres disponibles en el sistema de archivos.
 *      El superbloque debe estar correctamente inicializado y accesible.
 *
 * @post Se reserva un bloque libre en el mapa de bits, marcándolo como ocupado.
 *       Se decrementa el contador de bloques libres en el superbloque.
 *       El bloque reservado se inicializa con ceros en la zona de datos.
 *       Se retorna el número del bloque reservado o `FALLO` si no hay bloques disponibles.
 *
 * @return Número del bloque reservado si tiene éxito, `FALLO` en caso de error.
 */
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

/**
 * @brief Libera un bloque previamente reservado en el sistema de archivos.
 *
 * @param nbloque Número del bloque que se desea liberar.
 *
 * @pre `nbloque` debe ser un bloque previamente reservado dentro del sistema de archivos.
 *      El superbloque debe estar correctamente inicializado y accesible.
 *
 * @post El bloque `nbloque` se marca como libre en el mapa de bits.
 *       Se incrementa el contador de bloques libres en el superbloque.
 *       Se actualiza el superbloque en el sistema de archivos.
 *
 * @return El número del bloque liberado si tiene éxito, `FALLO` en caso de error.
 */
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

/**
 * @brief Escribe el contenido de un inodo en la posición correspondiente del array de inodos.
 *
 * @param ninodo Número del inodo a escribir.
 * @param inodo Puntero a la estructura `inodo` con la información a guardar.
 *
 * @pre `ninodo` debe estar dentro del rango permitido en el sistema de archivos.
 *      El superbloque debe estar correctamente inicializado y accesible.
 *      La estructura `inodo` debe estar correctamente inicializada con datos válidos.
 *
 * @post La información del inodo se almacena en la posición correspondiente en el array de inodos.
 *       Se actualiza el bloque de inodos en el sistema de archivos.
 *
 * @return `EXITO` si la operación fue exitosa, `FALLO` en caso de error.
 */
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

/**
 * @brief Lee un inodo del array de inodos y lo almacena en la estructura proporcionada.
 *
 * @param ninodo Número del inodo a leer.
 * @param inodo Puntero a la estructura `inodo` donde se almacenará el inodo leído.
 *
 * @pre `ninodo` debe ser un índice válido dentro del sistema de archivos.
 *      El superbloque debe estar correctamente inicializado y accesible.
 *      La estructura `inodo` debe ser una referencia válida.
 *
 * @post Se carga en `inodo` la información correspondiente al inodo solicitado.
 *       En caso de error, no se modifica `inodo` y se retorna `FALLO`.
 *
 * @return `EXITO` si la operación fue exitosa, `FALLO` en caso de error.
 */
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

/**
 * @brief Reserva un inodo y devuelve su número.
 *
 * @param tipo Tipo del inodo a reservar (archivo, directorio, etc.).
 * @param permisos Permisos del inodo en el sistema de archivos.
 *
 * @pre Debe haber inodos libres disponibles en el sistema de archivos.
 *      El superbloque debe estar correctamente inicializado y accesible.
 *
 * @post Se reserva un inodo y se inicializa con los valores proporcionados.
 *       Se actualiza la cantidad de inodos libres en el superbloque.
 *       Se guarda el inodo en el array de inodos y se actualiza el superbloque.
 *       En caso de error, no se modifica el estado del sistema.
 *
 * @return Número del inodo reservado si la operación fue exitosa, `FALLO` en caso de error.
 */
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

/**
 * @brief Determina el nivel de punteros en el que se encuentra el bloque lógico y devuelve el puntero correspondiente.
 *
 * @param inodo Puntero a la estructura `inodo` en la que se buscará el bloque lógico.
 * @param nblogico Número del bloque lógico cuyo nivel de punteros se desea obtener.
 * @param ptr Puntero donde se almacenará el puntero correspondiente al nivel encontrado.
 *
 * @pre `inodo` debe ser un puntero válido y `nblogico` debe estar dentro del rango permitido en el sistema de archivos.
 *
 * @post Se almacena en `ptr` el puntero correspondiente al nivel de punteros del bloque lógico `nblogico`.
 *       En caso de éxito, se retorna el nivel de punteros (`0` para directo, `1` para indirecto simple, `2` para doble, `3` para triple).
 *       Si el `nblogico` está fuera de rango, se almacena `0` en `ptr` y se retorna `FALLO`.
 *
 * @return Nivel de punteros (`0` a `3`) si la operación fue exitosa, `FALLO` en caso de error.
 */
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

/**
 * @brief Calcula el índice dentro del bloque de punteros dependiendo del nivel de punteros.
 *
 * @param nblogico Número del bloque lógico cuyo índice se desea calcular.
 * @param nivel_punteros Nivel de punteros en el que se encuentra el bloque lógico.
 *
 * @pre `nblogico` debe estar dentro del rango permitido en el sistema de archivos.
 *      `nivel_punteros` debe ser un valor entre `0` y `3` correspondiente a los niveles de punteros (directo, simple, doble, triple).
 *
 * @post Se obtiene el índice dentro del bloque de punteros correspondiente al `nblogico` y `nivel_punteros`.
 *       Si el `nblogico` está fuera de rango, se retorna `FALLO`.
 *
 * @return Índice dentro del bloque de punteros si la operación fue exitosa, `FALLO` en caso de error.
 */
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

/**
 * @brief Traduce un bloque lógico a su correspondiente bloque físico, reservándolo si es necesario.
 *
 * @param ninodo Número del inodo en el que se busca el bloque lógico.
 * @param nblogico Número del bloque lógico a traducir.
 * @param reservar Indica si se debe reservar un nuevo bloque en caso de que el `nblogico` no tenga uno asignado (`1` para reservar, `0` para solo consultar).
 *
 * @pre `ninodo` debe ser un inodo válido en el sistema de archivos.
 *      `nblogico` debe estar dentro del rango permitido en el sistema de archivos.
 *      Si `reservar` es `1`, debe haber bloques libres disponibles en el sistema.
 *
 * @post Se obtiene el número del bloque físico correspondiente al `nblogico` en el inodo `ninodo`.
 *       Si `reservar` es `1` y el bloque lógico no tenía un bloque físico asignado, se reserva uno nuevo.
 *       Se actualizan los punteros en el inodo y, si es necesario, se reservan y actualizan bloques de punteros intermedios.
 *
 * @return Número del bloque físico correspondiente si la operación fue exitosa, `FALLO` en caso de error.
 */
int traducir_bloque_inodo(unsigned int ninodo, unsigned int nblogico, unsigned char reservar)
{
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == -1)
        return FALLO;

    unsigned int ptr, ptr_ant;
    int nRangoBL = obtener_nRangoBL(&inodo, nblogico, &ptr); // Obtiene el nivel de punteros
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
/**
 * @brief Libera un inodo, eliminando todos sus bloques asociados y marcándolo como libre.
 *
 * @param ninodo Número del inodo que se desea liberar.
 *
 * @pre `ninodo` debe ser un inodo válido dentro del sistema de archivos.
 *      El superbloque y el inodo deben estar correctamente inicializados y accesibles.
 *
 * @post Se eliminan todos los bloques asociados al inodo.
 *       Se actualizan los metadatos del inodo marcándolo como libre.
 *       Se enlaza a la lista de inodos libres en el superbloque.
 *       Se incrementa el contador de inodos libres en el superbloque.
 *
 * @return `ninodo` si la operación fue exitosa, `FALLO` en caso de error.
 */
int liberar_inodo(unsigned int ninodo){
    struct superbloque sb;
    struct inodo inodo;

    // Lee el superbloque
    if (bread(posSB, &sb) == -1)
    {
        return FALLO;
    }

    // Lee el inodo a liberar
    if (leer_inodo(ninodo, &inodo) == -1)
    {
        return FALLO;
    }

    // Libera todos los bloques del inodo
    int bloques_liberados = liberar_bloques_inodo(0, &inodo);
    if (bloques_liberados == -1)
    {
        return FALLO;
    }

    // Actualiza el inodo
    inodo.numBloquesOcupados -= bloques_liberados;
    inodo.tipo = 'l'; // Tipo libre
    inodo.tamEnBytesLog = 0;
    
    // Actualiza la lista de inodos libres
    inodo.punterosDirectos[0] = sb.posPrimerInodoLibre;
    sb.posPrimerInodoLibre = ninodo;
    sb.cantInodosLibres++;

    // Actualiza tiempos
    inodo.ctime = time(NULL);

    // Escribe los cambios
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



int liberar_directos(unsigned int *nBL, unsigned int ultimoBL, struct inodo *inodo, int *eof) {
    int liberados = 0;
    for (unsigned int d = *nBL; d < DIRECTOS && !(*eof); d++) {
        if (inodo->punterosDirectos[d] != 0) {
            liberar_bloque(inodo->punterosDirectos[d]);
            fprintf(stderr, GRAY "[liberar_bloques_inodo()→[ liberado BF %u de datos para BL %u]\n" RESET, inodo->punterosDirectos[d], d);
            inodo->punterosDirectos[d] = 0;
            liberados++;
        }
        (*nBL)++;
        if (*nBL > ultimoBL) *eof = 1;
    }
    return liberados;
}

int liberar_indirectos_recursivo(unsigned int *nBL, unsigned int primerBL, unsigned int ultimoBL, struct inodo *inodo, int nRangoBL, unsigned int nivel_punteros, unsigned int *ptr, int *eof) {
    int liberados = 0, indice_inicial;
    unsigned int bloquePunteros[NPUNTEROS], bloquePunteros_Aux[NPUNTEROS], bufferCeros[NPUNTEROS] = {0};
    
    if (*ptr) {
        indice_inicial = obtener_indice(*nBL, nivel_punteros);
        if (indice_inicial == 0 || *nBL == primerBL) {
            if (bread(*ptr, bloquePunteros) == -1) return FALLO;
            breads++;
            memcpy(bloquePunteros_Aux, bloquePunteros, BLOCKSIZE);
        }
        for (int i = indice_inicial; i < NPUNTEROS && !(*eof); i++) {
            if (bloquePunteros[i] != 0) {
                if (nivel_punteros == 1) {
                    fprintf(stdout, GRAY "[liberar_bloques_inodo()→ liberado BF %d de datos para BL %d]\n" RESET, bloquePunteros[i], *nBL);
                    liberar_bloque(bloquePunteros[i]);
                    bloquePunteros[i] = 0;
                    liberados++;
                    (*nBL)++;
                } else {
                    liberados += liberar_indirectos_recursivo(nBL, primerBL, ultimoBL, inodo, nRangoBL, nivel_punteros - 1, &bloquePunteros[i], eof);
                }
            } else {
                unsigned int salto_inicio = *nBL;
                switch (nivel_punteros) {
                    case 1: *nBL += 1; 
                    break;
                    case 2: *nBL += NPUNTEROS;
                    //fprintf(stdout, LBLUE "[liberar_bloques_inodo()→ Estamos en el BL %d y saltamos hasta el BL %d]\n" RESET, salto_inicio, *nBL); 
                    break;
                    case 3: *nBL += NPUNTEROS * NPUNTEROS; 
                    fprintf(stdout, LBLUE "[liberar_bloques_inodo()→ Estamos en el BL %d y saltamos hasta el BL %d]\n" RESET, salto_inicio, *nBL);
                    break;
                }
                
            }
            if (*nBL > ultimoBL) *eof = 1;
        }
        if (memcmp(bloquePunteros, bloquePunteros_Aux, BLOCKSIZE) != 0) {
            if (memcmp(bloquePunteros, bufferCeros, BLOCKSIZE) != 0) {
                bwrite(*ptr, bloquePunteros);
                bwrites++;
                fprintf(stdout, ORANGE "[liberar_bloques_inodo()→ salvado BF %d de punteros_nivel%d correspondiente al BL %d]\n" RESET, *ptr, nivel_punteros, *nBL-1);
            } else {
                fprintf(stdout, GRAY "[liberar_bloques_inodo()→ liberado BF %d de punteros_nivel%d correspondiente al BL %d]\n" RESET, *ptr, nivel_punteros, *nBL-1);
                liberar_bloque(*ptr);
                *ptr = 0;
                if (nRangoBL == nivel_punteros) {
                    inodo->punterosIndirectos[nRangoBL - 1] = 0;
                }
                liberados++;
            }
        }
    } else {
        switch (nRangoBL) {
            case 1: *nBL = INDIRECTOS0; break;
            case 2: *nBL = INDIRECTOS1; break;
            case 3: *nBL = INDIRECTOS2; break;
        }
        
    }
    return liberados;
}

/**
 * @brief Libera los bloques ocupados por un inodo a partir de un bloque lógico determinado.
 *
 * @param primerBL Número del primer bloque lógico desde donde se empezará a liberar.
 * @param inodo Puntero al inodo cuyos bloques se van a liberar.
 *
 * @pre `inodo` debe ser un puntero válido a una estructura `inodo` inicializada.
 *      `primerBL` debe estar dentro del rango de bloques del inodo.
 *
 * @post Se liberan todos los bloques a partir de `primerBL`.
 *       Se actualiza el inodo con el nuevo número de bloques ocupados.
 *
 * @return `EXITO` si la operación se realiza correctamente, `FALLO` en caso de error.
 */
int liberar_bloques_inodo(unsigned int primerBL, struct inodo *inodo) {
    if (inodo->tamEnBytesLog == 0) return 0;
    
    unsigned int nivel_punteros = 0, nBL = primerBL, ultimoBL;
    unsigned int ptr = 0;
    int nRangoBL = 0, liberados = 0, eof = 0;
    
    ultimoBL = (inodo->tamEnBytesLog % BLOCKSIZE == 0) ? (inodo->tamEnBytesLog / BLOCKSIZE - 1) : (inodo->tamEnBytesLog / BLOCKSIZE);
    
    nRangoBL = obtener_nRangoBL(inodo, nBL, &ptr);
    fprintf(stdout, LBLUE "[liberar_bloques_inodo()→ primer BL: %d, último BL: %d]\n" RESET, primerBL, ultimoBL);

    if (nRangoBL == 0) {
        liberados += liberar_directos(&nBL, ultimoBL, inodo, &eof);
    }
    while (!eof) {
        nRangoBL = obtener_nRangoBL(inodo, nBL, &ptr);
        nivel_punteros = nRangoBL;
        liberados += liberar_indirectos_recursivo(&nBL, primerBL, ultimoBL, inodo, nRangoBL, nivel_punteros, &ptr, &eof);
    }
    fprintf(stdout, LBLUE "[liberar_bloques_inodo()→ total bloques liberados: %d , total_breads: %d, total_bwrites: %d ]\n" RESET, liberados, breads, bwrites);

    return liberados;
}
