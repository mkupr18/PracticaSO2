#include <stdio.h>
#include <string.h>
#include "directorios.h"
#define DEBUGN6 0

static struct UltimaEntrada UltimaEntradaEscritura;  //  Variable global que guarde la última entrada para escritura
static struct UltimaEntrada UltimaEntradaLectura; // Variable global para la caché de lectura

/**
 * @brief Muestra un mensaje de error según el código de error proporcionado.
 *
 * @param error Código de error devuelto por la función buscar_entrada.
 */
void mostrar_error_buscar_entrada(int error)
{
    // fprintf(stderr, "Error: %d\n", error);
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
    // Verificar que el camino comienza con '/'
    if (camino == NULL || camino[0] != '/') {
        return ERROR_CAMINO_INCORRECTO;
    }

    // Buscar el siguiente '/' después del inicial
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
        
        // Determinar si es directorio (termina con '/')
        if (camino[strlen(camino)-1] == '/') {
            *tipo = 'd';
        } else {
            *tipo = 'f';
        }
    }

    return EXITO;
}


/**
 * @brief Busca una entrada en un directorio y, si es necesario, la crea.
 *
 * @param camino_parcial Ruta parcial de la entrada a buscar.
 * @param p_inodo_dir Puntero al inodo del directorio donde buscar.
 * @param p_inodo Puntero donde se almacenará el inodo encontrado o creado.
 * @param p_entrada Puntero donde se almacenará el número de entrada encontrada o creada.
 * @param reservar Indica si se debe crear la entrada si no existe (1 para sí, 0 para no).
 * @param permisos Permisos que se asignarán en caso de lacreación de una nueva entrada.
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

    if (strcmp(camino_parcial, "/") == 0) {
        struct superbloque SB;
        if (bread(posSB, &SB) == FALLO) {
            fprintf(stdout, "Error al leer el superbloque");
            return FALLO;
        }
        *p_inodo = SB.posInodoRaiz;
        *p_entrada = 0;
        return 0;
    }

    if ((error = extraer_camino(camino_parcial, inicial, final, &tipo)) < 0) {
        return error;
    }
    #if DEBUGN6
        fprintf(stderr, GRAY "[buscar_entrada()→ inicial: %s, final: %s, reservar: %d]\n" RESET, inicial, final, reservar);
    #endif

    if (leer_inodo(*p_inodo_dir, &inodo_dir) < 0) {
        return FALLO;
    }

    if (!(inodo_dir.permisos & 4)) { // Permiso lectura
        #if DEBUGN6
            fprintf(stderr, GRAY "[buscar_entrada()→ El inodo %d no tiene permisos de lectura]\n" RESET, *p_inodo_dir) ; 
        #endif
        return ERROR_PERMISO_LECTURA;
    }

    cant_entradas_inodo = inodo_dir.tamEnBytesLog / sizeof(struct entrada);

    if (cant_entradas_inodo > 0) {
        unsigned int offset = 0;
        int leidos = mi_read_f(*p_inodo_dir, entradas, offset, sizeof(entradas));
        if (leidos < 0) return leidos;

        while (num_entrada_inodo < cant_entradas_inodo && strcmp(inicial, entradas[num_entrada_inodo].nombre) != 0) {
            num_entrada_inodo++;
            if (num_entrada_inodo % (BLOCKSIZE / sizeof(struct entrada)) == 0) {
                offset += sizeof(entradas);
                leidos = mi_read_f(*p_inodo_dir, entradas, offset, sizeof(entradas));
                if (leidos < 0) return leidos;
            }
        }
    }

    if (num_entrada_inodo == cant_entradas_inodo) { // No encontrada
        if (reservar == 0) {
            return ERROR_NO_EXISTE_ENTRADA_CONSULTA;
        } else {
            if (inodo_dir.tipo == 'f') {
                return ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO;
            }
            if (!(inodo_dir.permisos & 2)) { // Permiso escritura
                return ERROR_PERMISO_ESCRITURA;
            }

            struct entrada nueva;
            memset(&nueva, 0, sizeof(struct entrada));
            strcpy(nueva.nombre, inicial);

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

            if (mi_write_f(*p_inodo_dir, &nueva, inodo_dir.tamEnBytesLog, sizeof(struct entrada)) < 0) {
                liberar_inodo(nueva.ninodo);
                return FALLO;
            }
            #if DEBUGN6
                fprintf(stderr, GRAY"[buscar_entrada()→ creada entrada: %s, %d]\n" RESET, inicial, nueva.ninodo);*p_inodo = nueva.ninodo;
            #endif
        }
    } else { // Encontrada
        *p_inodo = entradas[num_entrada_inodo].ninodo;
    }

    if (strcmp(final, "/") == 0 || strcmp(final, "") == 0) {
        if (reservar == 1 && num_entrada_inodo != cant_entradas_inodo) {
            return ERROR_ENTRADA_YA_EXISTENTE;
        }
        *p_entrada = num_entrada_inodo;
        return 0;
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
    unsigned int p_inodo_dir = 0, p_inodo, p_entrada;

    // Llama a buscar_entrada con reservar=1 para que cree la entrada si no existe.
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 1, permisos);

    // Si hay error, se retorna el código de error.
    if (error < 0) return error;

    // Si no hay error, devuelve 0 (éxito).
    return 0;
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

    unsigned int p_inodo_dir = 0; // inodo del directorio raíz
    unsigned int p_inodo = 0;     
    unsigned int p_entrada = 0;   

    // Buscar la entrada correspondiente al camino
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
    if (error < 0) {
        return error;
    }

    // Cambiar los permisos
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
    unsigned int p_inodo_dir = 0; // inodo del directorio raíz
    unsigned int p_inodo;
    unsigned int p_entrada;

    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
    if (error < 0) {
        return error;
    }

    if (mi_stat_f(p_inodo, p_stat) == -1) {
        fprintf(stderr, RED"Error: mi_stat_f()\n"RESET);
        return FALLO;
    }

    return p_inodo; // Devolvemos también el número de inodo como pide el enunciado
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

    // Buscar la entrada correspondiente al camino
    error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
    if (error < 0) return error;

    // Leer el inodo correspondiente
    if (leer_inodo(p_inodo, &inodo) < 0) return -1;

    // Comprobar permisos de lectura
    if (!(inodo.permisos & 4)) return -1; // Permiso de lectura (r)

    // Si es un directorio
    if (inodo.tipo == 'd') {
        int nbytes;
        do {
            nbytes = mi_read_f(p_inodo, entradas, offset, sizeof(entradas));
            if (nbytes < 0) return nbytes;

            int total = nbytes / sizeof(struct entrada);

            for (int i = 0; i < total; i++) {
                if (entradas[i].nombre[0] == '\0') continue; // entrada vacía

                if (flag == 'l') { // Formato largo
                    struct inodo inodo_aux;
                    if (leer_inodo(entradas[i].ninodo, &inodo_aux) < 0) return -1;

                    struct tm *tm_info = localtime(&inodo_aux.mtime);

                    sprintf(tmp, "%c\t", (inodo_aux.tipo == 'd') ? 'd' : 'f');
                    strcat(buffer, tmp);

                    strcat(buffer, (inodo_aux.permisos & 4) ? "r" : "-");
                    strcat(buffer, (inodo_aux.permisos & 2) ? "w" : "-");
                    strcat(buffer, (inodo_aux.permisos & 1) ? "x\t" : "-\t");

                    strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S\t", tm_info);
                    strcat(buffer, tmp);

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

            offset += nbytes; // Avanzar el offset para la próxima lectura
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

int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes) {
    unsigned int p_inodo_dir, p_inodo;
    unsigned int p_entrada;
    int error;

    if (strcmp(UltimaEntradaEscritura.camino, camino) == 0) {
        p_inodo = UltimaEntradaEscritura.p_inodo;
        fprintf(stderr, ORANGE"[mi_write() → Utilizamos la caché de escritura]\n"RESET);
    } else {
        p_inodo_dir = 0;
        error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
        if (error < 0) {
            return error;
        }
        strcpy(UltimaEntradaEscritura.camino, camino);
        UltimaEntradaEscritura.p_inodo = p_inodo;
        fprintf(stderr, ORANGE"[mi_write() → Actualizamos la caché de escritura]\n"RESET);
    }

    struct STAT stat;
    if (mi_stat_f(p_inodo, &stat) < 0) {
        return -1;
    }
    if (stat.tipo != 'f') {
        fprintf(stderr, "Error: el camino se corresponde a un directorio.\n");
        return -1;
    }

    return mi_write_f(p_inodo, buf, offset, nbytes);
}


int mi_read(const char *camino, void *buf, unsigned int offset, unsigned int nbytes) {
    unsigned int p_inodo_dir, p_inodo;
    unsigned int p_entrada;
    int error;

    if (strcmp(UltimaEntradaLectura.camino, camino) == 0) {
        p_inodo = UltimaEntradaLectura.p_inodo;
        fprintf(stderr, LBLUE"[mi_read() → Utilizamos la caché de lectura]\n"RESET);
    } else {
        p_inodo_dir = 0;
        error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
        if (error < 0) {
            return error;
        }
        strcpy(UltimaEntradaLectura.camino, camino);
        UltimaEntradaLectura.p_inodo = p_inodo;
        fprintf(stderr, LBLUE "[mi_read() → Actualizamos la caché de lectura]\n"RESET);
    }

    return mi_read_f(p_inodo, buf, offset, nbytes);
}