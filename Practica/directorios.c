// Autores: Kalyarat Asawapoom, Rupak Guni, Maria Kupriyenko

#include <stdio.h>
#include <string.h>

#include "directorios.h"

#define DEBUGN6 0
#define DEBUGN9 0

static struct UltimaEntrada UltimaEntradaEscritura[CACHESIZE];  //  Tabla global de la caché para escrituras FIFO
static struct UltimaEntrada UltimaEntradaLectura; // Variable global para la caché de lectura
static int ultima_pos_escritura = -1; // Índice de la última posición utilizada en la caché (FIFO circular)

/**
 * @brief Muestra un mensaje de error según el código de error proporcionado.
 *
 * @param error Código de error devuelto por la función buscar_entrada.
 */
void mostrar_error_buscar_entrada(int error)
{

    switch (error)
    {
    case -2:
        fprintf(stderr, RED"Error: Camino incorrecto.\n"RESET);
        break;
    case -3:
        fprintf(stderr, RED"Error: Permiso denegado de lectura.\n"RESET);
        break;
    case -4:
        fprintf(stderr, RED"Error: No existe el archivo o el directorio.\n"RESET);
        break;
    case -5:
        fprintf(stderr, RED"Error: No existe algún directorio intermedio.\n"RESET);
        break;
    case -6:
        fprintf(stderr, RED"Error: Permiso denegado de escritura.\n"RESET);
        break;
    case -7:
        fprintf(stderr, RED"Error: El archivo ya existe.\n"RESET);
        break;
    case -8:
        fprintf(stderr, RED"Error: No es un directorio.\n"RESET);
        break;
    }
}

/**
 * @brief Dada una ruta que comienza con '/', separa su contenido en inicial, final y tipo.
 *
 * @param camino La ruta completa de entrada. Debe comenzar con '/'.
 * @param inicial Puntero al buffer donde se almacenará el primer componente (directorio o fichero).
 * @param final Puntero al buffer donde se almacenará el resto de la ruta.
 * @param tipo Puntero a un char donde se almacenará 'd' (directorio) o 'f' (fichero).
 *
 * @return 0 si éxito, -1 si hay algún error.
 */
int extraer_camino(const char *camino, char *inicial, char *final, char *tipo) {
    // Verifica que el camino comienza con '/'
    if (camino == NULL || camino[0] != '/') {
        return ERROR_CAMINO_INCORRECTO;
    }

    // Busca el siguiente '/' después del inicial
    const char *slash = strchr(camino + 1, '/');

    if (slash != NULL) {
        // Hay más componentes después del primero (directorio)
        int len = slash - (camino + 1);
        strncpy(inicial, camino + 1, len);
        inicial[len] = '\0';
        strcpy(final, slash);
        *tipo = 'd';
    } else {
        // Es el último componente (puede ser directorio o fichero)
        strcpy(inicial, camino + 1);
        final[0] = '\0';
        
        // Determina si es un directorio (termina con '/')
        if (camino[strlen(camino) - 1] == '/') {
            *tipo = 'd';
        } else {
            *tipo = 'f';
        }
    }

    return EXITO;
}


/**
 * @brief Función recursiva que busca una entrada en un directorio y, si es necesario, la crea.
 *
 * @param camino_parcial Ruta parcial de la entrada a buscar.
 * @param p_inodo_dir Puntero al inodo del directorio donde buscar.
 * @param p_inodo Puntero donde se almacenará el inodo encontrado o creado.
 * @param p_entrada Puntero donde se almacenará el número de entrada encontrada o creada.
 * @param reservar Indica si se debe crear la entrada si no existe (1 para sí, 0 para no).
 * @param permisos Permisos que se asignarán en caso de la creación de una nueva entrada.
 *
 * @return 0 si se encuentra o crea la entrada con éxito,
 *         ERROR_CAMINO_INCORRECTO si la ruta es incorrecta,
 *         ERROR_PERMISO_LECTURA si no hay permisos de lectura,
 *         ERROR_NO_EXISTE_ENTRADA_CONSULTA si la entrada no existe y no se debe reservar,
 *         ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO si se intenta crear en un fichero,
 *         ERROR_PERMISO_ESCRITURA si no hay permisos de escritura,
 *         ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO si el directorio intermedio no existe,
 *         -1 en caso de error inesperado.
 */

int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, 
                   unsigned int *p_inodo, unsigned int *p_entrada, 
                   char reservar, unsigned char permisos) {
    struct entrada entradas[BLOCKSIZE / sizeof(struct entrada)];
    struct inodo inodo_dir;
    char inicial[TAMNOMBRE];
    char final[strlen(camino_parcial)];
    char tipo;
    unsigned int cant_entradas_inodo, num_entrada_inodo = 0;
    int error;

    // En el caso de que la ruta sea solo "/"
    if (strcmp(camino_parcial, "/") == 0) {
        // Lee el superbloque para obtener el inodo raíz
        struct superbloque SB;
        if (bread(posSB, &SB) == FALLO) {
            fprintf(stdout, "Error al leer el superbloque");
            return FALLO;
        }
        *p_inodo = SB.posInodoRaiz;
        *p_entrada = 0;
        return EXITO;
    }

    // Extrae el siguiente componente
    if ((error = extraer_camino(camino_parcial, inicial, final, &tipo)) < 0) {
        return error;
    }
    #if DEBUGN6
        fprintf(stderr, GRAY "[buscar_entrada()→ inicial: %s, final: %s, reservar: %d]\n" RESET, inicial, final, reservar);
    #endif

    // Lee el inodo del directorio actual
    if (leer_inodo(*p_inodo_dir, &inodo_dir) < 0) {
        return FALLO;
    }

    // Verifica los permisos
    if (!(inodo_dir.permisos & 4)) { // Permiso de lectura
        #if DEBUGN6
            fprintf(stderr, GRAY "[buscar_entrada()→ El inodo %d no tiene permisos de lectura]\n" RESET, *p_inodo_dir) ; 
        #endif
        return ERROR_PERMISO_LECTURA;
    }

    // Calcula el número de entradas
    cant_entradas_inodo = inodo_dir.tamEnBytesLog / sizeof(struct entrada);

    // Busca la entrada en el directorio
    if (cant_entradas_inodo > 0) {
        unsigned int offset = 0;
        int leidos = mi_read_f(*p_inodo_dir, entradas, offset, sizeof(entradas));
        if (leidos < 0) return leidos;

        // Compara el nombre buscado con las entradas leídas del directorio actual
        while (num_entrada_inodo < cant_entradas_inodo && strcmp(inicial, entradas[num_entrada_inodo].nombre) != 0) {
            num_entrada_inodo++;
            if (num_entrada_inodo % (BLOCKSIZE / sizeof(struct entrada)) == 0) {
                offset += sizeof(entradas);
                leidos = mi_read_f(*p_inodo_dir, entradas, offset, sizeof(entradas));
                if (leidos < 0) return leidos;
            }
        }
    }

    // Si no lo encuentra
    if (num_entrada_inodo == cant_entradas_inodo) {
        if (reservar == 0) { // Error
            return ERROR_NO_EXISTE_ENTRADA_CONSULTA;
        } else { // Hay que crear la entrada
            // Comprueba que no estamos creandola dentro de un fichero
            if (inodo_dir.tipo == 'f') {
                return ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO;
            }
            // Comprueba los permisos de escritura
            if (!(inodo_dir.permisos & 2)) {
                return ERROR_PERMISO_ESCRITURA;
            }

            struct entrada nueva;
            memset(&nueva, 0, sizeof(struct entrada));
            strcpy(nueva.nombre, inicial);
            
            // Reserva el inodo
            if (tipo == 'd') {
                if (strcmp(final, "/") == 0) {
                    nueva.ninodo = reservar_inodo('d', permisos);
                    #if DEBUGN6
                        fprintf(stderr, GRAY"[buscar_entrada()→ reservado inodo %d tipo %c con permisos %u para %s]\n" RESET,
                        nueva.ninodo, tipo, permisos, inicial);
                    #endif
                } else {
                    return ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO;
                }
            } else {
                nueva.ninodo = reservar_inodo('f', permisos);
                #if DEBUGN6
                    fprintf(stderr, GRAY"[buscar_entrada()→ reservado inodo %d tipo %c con permisos %u para %s]\n" RESET, nueva.ninodo, tipo, permisos, inicial);
                #endif 
            }

            // Escribe la nueva entrada
            if (mi_write_f(*p_inodo_dir, &nueva, inodo_dir.tamEnBytesLog, sizeof(struct entrada)) < 0) {
                liberar_inodo(nueva.ninodo);
                return FALLO;
            }
            #if DEBUGN6
                fprintf(stderr, GRAY"[buscar_entrada()→ creada entrada: %s, %d]\n" RESET, inicial, nueva.ninodo);*p_inodo = nueva.ninodo;
            #endif
        }
    } else { // Entrada encontrada
        *p_inodo = entradas[num_entrada_inodo].ninodo;
    }

    // Realización de la llamada recursiva si no se ha terminado
    if (strcmp(final, "/") == 0 || strcmp(final, "") == 0) {
        if (reservar == 1 && num_entrada_inodo != cant_entradas_inodo) {
            return ERROR_ENTRADA_YA_EXISTENTE;
        }
        *p_entrada = num_entrada_inodo;
        return EXITO;
    } else {
        *p_inodo_dir = *p_inodo;
        return buscar_entrada(final, p_inodo_dir, p_inodo, p_entrada, reservar, permisos);
    }
}


/**
 * mi_creat - Crea un fichero o directorio en la ruta especificada.
 *
 * Precondiciones:
 * - camino debe ser una cadena de texto válida y no nula.
 * - permisos debe estar en el rango [0, 7].
 * - Todos los directorios intermedios del camino deben existir.
 * - El sistema de ficheros debe estar montado.
 *
 * Postcondiciones:
 * - Si la entrada no existía, se crea un fichero o directorio con los permisos indicados.
 * - Si la entrada ya existe o hay error (permisos, inexistencia de directorios intermedios...), se devuelve un código de error negativo.
 * - Si se crea correctamente, devuelve 0.
 */
int mi_creat(const char *camino, unsigned char permisos) {

    
    mi_waitSem(); // Entrada sección crítica
    unsigned int p_inodo_dir = 0, p_inodo, p_entrada;

    // Llama a buscar_entrada con reservar=1 para que cree la entrada si no existe.
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 1, permisos);

    // Si hay error, se retorna el código de error.
    if (error < 0){
        mostrar_error_buscar_entrada(error);
        mi_signalSem(); // Salida sección crítica
        return FALLO;
    }

    mi_signalSem(); // Salida sección crítica
    return EXITO; // Si no hay error, devuelve 0 (éxito).
}


/* 
 * mi_chmod - Cambia los permisos de un fichero o directorio especificados
 *
 * Precondiciones: 
 *  - El dispositivo virtual debe estar montado (bmount).
 *  - El parámetro 'camino' debe ser una cadena válida que apunte a un fichero o directorio existente.
 *  - El parámetro 'permisos' debe ser un valor entre 0 y 7 (inclusive).
 *
 * Postcondiciones:
 *  - Si la operación tiene éxito, los permisos del inodo correspondiente al 'camino' habrán sido actualizados.
 *  - El tiempo de cambio (ctime) del inodo se actualizará.
 *  - Se devuelve 0 en caso de éxito, o un código de error negativo si falla (error de buscar_entrada o de mi_chmod_f).
 */
int mi_chmod(const char *camino, unsigned char permisos) {

    unsigned int p_inodo_dir = 0; // Inodo del directorio raíz
    unsigned int p_inodo = 0;     
    unsigned int p_entrada = 0;   

    // Busca la entrada correspondiente al camino
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
    if (error < 0) {
        return error;
    }

    // Cambia los permisos
    return mi_chmod_f(p_inodo, permisos);
}

/*
* mi_stat - Llama a buscar_entrada(), luego a mi_stat_f() para rellenar STAT
*
* Precondición: 
* - El camino debe ser una ruta válida en el sistema de ficheros montado.
*
* Postcondición: 
* - Se rellena la estructura struct STAT con los datos del inodo asociado.
*/

int mi_stat(const char *camino, struct STAT *p_stat) {
    unsigned int p_inodo_dir = 0; // Inodo del directorio raíz
    unsigned int p_inodo;
    unsigned int p_entrada;

    // Buscamos la entrada para obtener el inodo
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
    if (error < 0) {
        return error;
    }

    // Llamamos a mi_stat_f() si la entrada existe
    if (mi_stat_f(p_inodo, p_stat) == -1) {
        fprintf(stderr, RED"Error: mi_stat_f()\n"RESET);
        return FALLO;
    }

    return p_inodo; // Devolvemos el número de inodo
}

/**
 * mi_dir - Lista el contenido de un directorio o muestra información de un fichero.
 *
 * Precondiciones:
 * - camino debe ser una cadena de texto válida y no nula.
 * - buffer debe ser una cadena de texto con espacio suficiente (TAMBUFFER).
 * - El sistema de ficheros debe estar montado.
 *
 * Postcondiciones:
 * - Si se trata de un directorio, llena el buffer con la información de sus entradas.
 * - Si se trata de un fichero, llena el buffer con la información de ese fichero.
 * - Devuelve el número de entradas listadas o un código de error negativo si falla.
 */
 int mi_dir(const char *camino, char *buffer, char flag) {
    unsigned int p_inodo_dir = 0, p_inodo, p_entrada;
    struct inodo inodo;
    int error, nentradas = 0;
    struct entrada entradas[BLOCKSIZE / sizeof(struct entrada)];
    int offset = 0;
    char tmp[TAMFILA];

    // Busca la entrada correspondiente al camino
    error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
    if (error < 0) return error;

    // Lee el inodo correspondiente
    if (leer_inodo(p_inodo, &inodo) < 0) return FALLO;

    // Comprueba los permisos de lectura
    if (!(inodo.permisos & 4)) return FALLO;

    // Si es un directorio, leemos bloques enteros de entradas de directorios
    if (inodo.tipo == 'd') {
        int nbytes;
        do {
            nbytes = mi_read_f(p_inodo, entradas, offset, sizeof(entradas));
            if (nbytes < 0) return nbytes;

            int total = nbytes / sizeof(struct entrada); // Entradas del bloque

            for (int i = 0; i < total; i++) {
                if (entradas[i].nombre[0] == '\0') continue; // Entrada vacía

                if (flag == 'l') { // Formato largo -l
                    struct inodo inodo_aux;

                    // Lee el inodo apuntado por la entrada
                    if (leer_inodo(entradas[i].ninodo, &inodo_aux) < 0) return FALLO;

                    // Información sobre el tiempo
                    struct tm *tm_info = localtime(&inodo_aux.mtime);

                    // Muestra si es directorio o fichero
                    sprintf(tmp, "%c\t", (inodo_aux.tipo == 'd') ? 'd' : 'f');
                    strcat(buffer, tmp);

                    // Información sobre los permisos
                    strcat(buffer, (inodo_aux.permisos & 4) ? "r" : "-");
                    strcat(buffer, (inodo_aux.permisos & 2) ? "w" : "-");
                    strcat(buffer, (inodo_aux.permisos & 1) ? "x\t" : "-\t");

                    strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S\t", tm_info);
                    strcat(buffer, tmp);

                    // Añade el tamaño lógico y el nombre
                    sprintf(tmp, "%d\t\t", inodo_aux.tamEnBytesLog);
                    strcat(buffer, tmp);

                    strcat(buffer, entradas[i].nombre);
                    strcat(buffer, "\n");
                } else { // Formato simple
                    strcat(buffer, entradas[i].nombre);
                    strcat(buffer, "\t");
                } 
                nentradas++;
            }

            offset += nbytes; // Avanza el offset para la próxima lectura
        } while (nbytes > 0);
    }
    // Si es un fichero
    else if (inodo.tipo == 'f') {
        if (flag == 'l') {
            struct tm *tm_info = localtime(&inodo.mtime);

            sprintf(tmp, "f\t");
            strcat(buffer, tmp);

            strcat(buffer, (inodo.permisos & 4) ? "r" : "-");
            strcat(buffer, (inodo.permisos & 2) ? "w" : "-");
            strcat(buffer, (inodo.permisos & 1) ? "x\t" : "-\t");

            strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S\t", tm_info);
            strcat(buffer, tmp);

            sprintf(tmp, "%d\t\t", inodo.tamEnBytesLog);
            strcat(buffer, tmp);

            const char *nombre = strrchr(camino, '/');
            if (nombre != NULL) {
                strcat(buffer, nombre + 1);
            } else {
                strcat(buffer, camino);
            }
            strcat(buffer, "\n");
        } else { // Formato simple
            const char *nombre = strrchr(camino, '/');
            if (nombre != NULL) {
                strcat(buffer, nombre + 1);
            } else {
                strcat(buffer, camino);
            }
            strcat(buffer, "\n");
        } 
        nentradas = 1; // Solo una entrada (el fichero)
    } 
    return nentradas;
}

/**
 * @brief Escribe datos en un fichero especificado por su ruta lógica.
 *        Utiliza una caché FIFO para evitar búsquedas repetidas.
 *
 * @param camino Ruta del fichero.
 * @param buf Puntero al buffer con los datos a escribir.
 * @param offset Desplazamiento desde el inicio del fichero.
 * @param nbytes Número de bytes a escribir.
 *
 * @return Número de bytes escritos si éxito, código de error si falla.
 */
int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes) {
    unsigned int p_inodo_dir, p_inodo;
    unsigned int p_entrada;
    int error;

    int encontrado = 0; // Indica si encontramos el camino en la de caché de escritura

    // Búsqueda en la caché FIFO de escritura
    for (int i = 0; i < CACHESIZE; i++) { 
        // Si encontramos el camino usamos el p_inodo almacenado y lo indicamos
        if (strcmp(UltimaEntradaEscritura[i].camino, camino) == 0)
        {
            p_inodo = UltimaEntradaEscritura[i].p_inodo; 
            encontrado = 1;
            #if DEBUGN9
                fprintf(stderr, ORANGE "[mi_write() → Utilizamos la caché de escritura en posición %d]\n" RESET, i);
            #endif 
        }
    }

    // Si no lo encontramos, llamamos a buscar_entrada para obtener el inodo
    if (!encontrado)
    {
        p_inodo_dir = 0; 
        error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
        if (error < 0)
        {
            return error; 
        }

       // Actualizamos la caché
        ultima_pos_escritura = (ultima_pos_escritura + 1) % CACHESIZE; 
        strcpy(UltimaEntradaEscritura[ultima_pos_escritura].camino, camino);
        UltimaEntradaEscritura[ultima_pos_escritura].p_inodo = p_inodo; 
        #if DEBUGN9
            fprintf(stderr, ORANGE "[mi_write() → Actualizamos la caché de escritura en posición %d]\n" RESET, ultima_pos_escritura);
        #endif
    }

    // Verifica si es un fichero
    struct STAT stat;
    if (mi_stat_f(p_inodo, &stat) < 0)
    {
        return FALLO;
    }

    if (stat.tipo != 'f')
    {
        fprintf(stderr, "Error: el camino se corresponde a un directorio.\n");
        return FALLO;
    }

    // Llamamos a la función de escritura a bajo nivel
    return mi_write_f(p_inodo, buf, offset, nbytes); 
}

/**
 * @brief Lee datos de un fichero a partir de una ruta, usando offset y tamaño.
 *
 * @param camino Ruta del fichero (debe ser un fichero, no un directorio).
 * @param buf Puntero al buffer donde se almacenarán los datos leídos.
 * @param offset Desplazamiento inicial desde el que comenzar a leer.
 * @param nbytes Cantidad de bytes a leer.
 *
 * @return Número de bytes leídos si éxito, o un código de error si fallo.
 */
int mi_read(const char *camino, void *buf, unsigned int offset, unsigned int nbytes) {
    unsigned int p_inodo_dir, p_inodo;
    unsigned int p_entrada;
    int error;

    // Comprueba si la última lectura fue del mismo fichero que el del actual, aprovechando el p_inodo
    if (strcmp(UltimaEntradaLectura.camino, camino) == 0) {
        p_inodo = UltimaEntradaLectura.p_inodo;
        #if DEBUGN9  
            fprintf(stderr, LBLUE"[mi_read() → Utilizamos la caché de lectura]\n"RESET);
        #endif
    } else { // Si no está en la caché, buscamos la entrada para obtener el inodo
        p_inodo_dir = 0;
        error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
        if (error < 0) {
            return error;
        }

        // Actualizamos la caché con la nueva ruta
        strcpy(UltimaEntradaLectura.camino, camino);
        UltimaEntradaLectura.p_inodo = p_inodo;
        #if DEBUGN9  
            fprintf(stderr, LBLUE "[mi_read() → Actualizamos la caché de lectura]\n"RESET);
        #endif
    }

    // Realizamos la lectura
    return mi_read_f(p_inodo, buf, offset, nbytes);
}

/**
 * @brief Crea el enlace de una entrada de directorio camino2 al inodo especificado por otra entrada de directorio camino1.
 *
 * Este enlace hace que ambas rutas apunten al mismo inodo (contenido), incrementando el contador
 * de enlaces (`nlinks`) del fichero original. No se permite realizar enlaces a directorios.
 *
 * @param camino1 Ruta del fichero original (debe existir y ser un fichero).
 * @param camino2 Ruta del nuevo enlace (no debe existir previamente).
 *
 * @return 0 si éxito, o un código de error si fallo (valor negativo).
 */
int mi_link(const char *camino1, const char *camino2) {
    mi_waitSem(); // Entrada sección crítica

    unsigned int p_inodo_dir1, p_inodo1, p_entrada1;
    struct inodo inodo1;

    // Busca el inodo del fichero original
    int error = buscar_entrada(camino1, &p_inodo_dir1, &p_inodo1, &p_entrada1, 0, 0);
    if (error < 0){
        mi_signalSem(); // Salida sección crítica
        return error;
    }

    // Lee el inodo para comprobar el tipo y los permisos
    if (leer_inodo(p_inodo1, &inodo1) < 0){
        mi_signalSem(); // Salida sección crítica
        return FALLO;
    }

    // Verifica los permisos de lectura y que es un fichero regular
    if ((inodo1.permisos & 4) != 4 || inodo1.tipo != 'f'){
        mi_signalSem(); // Salida sección crítica
        return FALLO;
    } 

    // Crea la nueva entrada camino2, se reservará un inodo automáticamente
    unsigned int p_inodo_dir2, p_inodo2, p_entrada2;
    error = buscar_entrada(camino2, &p_inodo_dir2, &p_inodo2, &p_entrada2, 1, 6);
    if (error < 0){
        mi_signalSem(); // Salida sección crítica
        return error;
    }

    //printf("INFO DEBUG: Entrada %s creada con inodo %d\n", camino2, p_inodo2);

    // Lee la entrada creada de camino2
    struct entrada entrada;
    if (mi_read_f(p_inodo_dir2, &entrada, p_entrada2 * sizeof(struct entrada), sizeof(struct entrada)) < 0){
        mi_signalSem(); // Salida sección crítica
        return FALLO;
    }

    if (liberar_inodo(entrada.ninodo) == -1){
        mi_signalSem(); // Salida sección crítica
        return FALLO;
    } 

    // Crea el enlace
    entrada.ninodo = p_inodo1;
   
    // Escribe la entrada modificada
    if (mi_write_f(p_inodo_dir2, &entrada, p_entrada2 * sizeof(struct entrada), sizeof(struct entrada)) < 0){
        mi_signalSem(); // Salida sección crítica
        return FALLO;
    }
    
    // Incrementa la cantidad de enlaces, actualiza el ctime y lo guarda
    inodo1.nlinks++;
    inodo1.ctime = time(NULL);
    if (escribir_inodo(p_inodo1, &inodo1) < 0){
        mi_signalSem(); // Salida sección crítica
        return FALLO;
    }

    mi_signalSem(); // Salida sección crítica
    return EXITO;
}

/**
 * @brief Elimina una entrada de directorio, y si el inodo asociado queda sin enlaces, lo libera.
 *
 * @param camino Ruta del fichero o directorio a eliminar.
 *
 * @return 0 si éxito, o código de error si falla.
 */
int mi_unlink(const char *camino)
{
    mi_waitSem(); // Entrada sección crítica

    // Comprueba que no sea el directorio raíz
    if (strcmp(camino, "/") == 0)
    {
        fprintf(stderr, RED "Error: No se puede eliminar el directorio raíz.\n" RESET);
        mi_signalSem(); // Salida sección crítica
        return FALLO;
    }

    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    //struct entrada entrada;
    struct inodo inodo, inodo_dir;

    // Buscamos la entrada a eliminar en el sistema de ficheros
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
    if (error < 0){
        mi_signalSem(); // Salida sección crítica
        return error;
    }
        

    // Leemos el inodo asociado al camino
    if (leer_inodo(p_inodo, &inodo) < 0){
        mi_signalSem(); // Salida sección crítica
        return FALLO;
    }
        

    // Si es un directorio no vacío, no se puede eliminar
    if (inodo.tipo == 'd' && inodo.tamEnBytesLog > 0)
    {
        fprintf(stderr, RED "Error: No se puede eliminar un directorio no vacío.\n" RESET);
        mi_signalSem(); // Salida sección crítica
        return FALLO;
    }

    // Leemos el inodo del directorio padre
    if (leer_inodo(p_inodo_dir, &inodo_dir) < 0){
        mi_signalSem(); // Salida sección crítica
        return FALLO;
    }
        
    int n_entradas = inodo_dir.tamEnBytesLog / sizeof(struct entrada);

    // Si no es la última entrada, la intercambiamos con la última
    if (p_entrada != n_entradas - 1)
    {
        // Leemos la última entrada
        struct entrada ultima;
        if (mi_read_f(p_inodo_dir, &ultima, (n_entradas - 1) * sizeof(struct entrada), sizeof(struct entrada)) < 0){
            mi_signalSem(); // Salida sección crítica
            return FALLO;
        }
            

        // Sobrescribimos la entrada a eliminar con la última
        if (mi_write_f(p_inodo_dir, &ultima, p_entrada * sizeof(struct entrada), sizeof(struct entrada)) < 0) {
            mi_signalSem(); // Salida sección crítica
            return FALLO;
        }
            
    }

    // Truncamos el inodo del directorio padre
    if (mi_truncar_f(p_inodo_dir, inodo_dir.tamEnBytesLog - sizeof(struct entrada)) < 0) {
        mi_signalSem(); // Salida sección crítica
        return FALLO;
    }
        

    // Leemos inodo del directorio eliminado
    if (leer_inodo(p_inodo, &inodo) < 0) {
        mi_signalSem(); // Salida sección crítica
        return FALLO;
    }
        

    // Disminuimos el número de enlaces
    inodo.nlinks--;

    if (inodo.nlinks == 0)
    {
        // Si ya no quedan enlaces, liberamos el inodo y sus bloques
        if (liberar_inodo(p_inodo) < 0) {
            mi_signalSem(); // Salida sección crítica
            return FALLO;
        }
            
    }
    else
    {
        // Si aún hay enlaces, actualizamos el ctime y escribimos el inodo
        inodo.ctime = time(NULL);
        if (escribir_inodo(p_inodo, &inodo) < 0) {
            mi_signalSem(); // Salida sección crítica
            return FALLO;
        }
            
    }
    mi_signalSem(); // Salida sección crítica
    return EXITO;
}


