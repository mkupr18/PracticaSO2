#include <stdio.h>
#include <string.h>
#include "directorios.h"

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
        fprintf(stderr, RED "Error: Camino incorrecto.\n" RESET);
        break;
    case -3:
        fprintf(stderr, RED "Error: Permiso denegado de lectura.\n"RESET);
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
int extraer_camino(const char *camino, char *inicial, char *final, char *tipo)
{
    // Comprobamos si el camino empieza por '/'
    if (camino == NULL || camino[0] != '/')
    {
        // fprintf(stderr, "Error en extraer_camino: El camino no empieza por '/'\n");
        return FALLO;
    }

    // Buscamos el primer carácter después del '/' inicial
    const char *start_inicial = camino + 1;

    // Buscamos la siguiente aparición de '/' a partir de start_inicial
    char *segundo_slash = strchr(start_inicial, '/');

    if (segundo_slash != NULL) // Se encontró un segundo '/', indica un directorio en 'inicial'
    {
        // Calculamos la longitud del nombre en 'inicial'
        int longitud_inicial = segundo_slash - start_inicial;

        // Copiamos el nombre del directorio a 'inicial'
        strncpy(inicial, start_inicial, longitud_inicial);
        inicial[longitud_inicial] = '\0'; // Aseguramos la terminación NULL

        // Copiamos el resto del camino (desde el segundo '/') a 'final'
        strcpy(final, segundo_slash);

        // Asignamos el tipo 'd' (directorio)
        *tipo = 'd';
    }
    else // No se encontró un segundo '/', indica un fichero en 'inicial'
    {
        // Copiamos todo desde start_inicial hasta el final de 'camino' a 'inicial'
        strcpy(inicial, start_inicial);

        // 'final' es una cadena vacía
        final[0] = '\0'; // O strcpy(final, "");

        // Asignamos el tipo 'f' (fichero)
        *tipo = 'f';
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
int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada, char reservar, unsigned char permisos) {
    struct entrada entrada;
    struct inodo inodo_dir;
    char inicial[sizeof(entrada.nombre)];
    char final[strlen(camino_parcial)];
    char tipo;
    int cant_entradas_inodo, num_entrada_inodo = 0;
    struct superbloque SB;

    // Leer superbloque
    if (bread(posSB, &SB) == FALLO) {
        fprintf(stderr, "Error al leer el superbloque\n");
        return FALLO;
    }

    // Caso especial: directorio raíz
    if (strcmp(camino_parcial, "/") == 0) {
        *p_inodo = SB.posInodoRaiz;
        *p_entrada = 0;
        return EXITO;
    }

    // Extraer primer componente del camino
    if (extraer_camino(camino_parcial, inicial, final, &tipo) == FALLO) {
        //fprintf(stderr, "[buscar_entrada()→ Error al extraer camino]\n");
        return ERROR_CAMINO_INCORRECTO;
    }
    printf("[buscar_entrada()→ inicial: %s, final: %s, reservar: %d]\n", inicial, final, reservar);

    // Leer inodo del directorio actual
    if (leer_inodo(*p_inodo_dir, &inodo_dir) == FALLO) {
        //fprintf(stderr, "Error al leer el inodo\n");
        return FALLO;
    }

    // Verificar permisos de lectura
    if (!(inodo_dir.permisos & 4)) {
        //fprintf(stderr, "[buscar_entrada()→ Permiso denegado de lectura]\n");
        return ERROR_PERMISO_LECTURA;
    }

    // Calcular número de entradas en el directorio
    cant_entradas_inodo = inodo_dir.tamEnBytesLog / sizeof(struct entrada);

    // Buffer para leer entradas
    //struct entrada entradas[BLOCKSIZE / sizeof(struct entrada)];
    int encontrada = 0;

    // Buscar la entrada en el directorio
    for (num_entrada_inodo = 0; num_entrada_inodo < cant_entradas_inodo && !encontrada; num_entrada_inodo++) {
        // Leer la entrada actual
        if (mi_read_f(*p_inodo_dir, &entrada, num_entrada_inodo * sizeof(struct entrada), sizeof(struct entrada)) != sizeof(struct entrada)) {
            //fprintf(stderr, "Error al leer la entrada\n");
            return FALLO;
        }
        
        if (strcmp(inicial, entrada.nombre) == 0) {
            encontrada = 1;
            break;
        }
    }

    // Si la entrada no existe
    if (!encontrada) {
        // Modo consulta: entrada no existe
        if (!reservar) {
            //fprintf(stderr, "[buscar_entrada()→ No existe entrada]\n");
            return ERROR_NO_EXISTE_ENTRADA_CONSULTA;
        }

        // Modo escritura: crear nueva entrada
        if (inodo_dir.tipo == 'f') {
            //fprintf(stderr, "[buscar_entrada()→ No se puede crear entrada en fichero]\n");
            return ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO;
        }

        if (!(inodo_dir.permisos & 2)) {
            //fprintf(stderr, "[buscar_entrada()→ Permiso denegado de escritura]\n");
            return ERROR_PERMISO_ESCRITURA;
        }

        // Preparar nueva entrada
        strcpy(entrada.nombre, inicial);
        
        if (tipo == 'd') {
            if (strcmp(final, "/") == 0) {
                // Crear nuevo directorio
                entrada.ninodo = reservar_inodo('d', permisos);
                if (entrada.ninodo == FALLO) {
                    fprintf(stderr, "Error al reservar inodo\n");
                    return FALLO;
                }
                printf("[buscar_entrada()→ reservado inodo %d tipo %c con permisos %d para %s]\n", 
                       entrada.ninodo, 'd', permisos, inicial);
            } else {
                // Directorio intermedio no existe
                //fprintf(stderr, "[buscar_entrada()→ No existe directorio intermedio]\n");
                return ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO;
            }
        } else {
            // Crear nuevo fichero
            entrada.ninodo = reservar_inodo('f', permisos);
            if (entrada.ninodo == FALLO) {
                //fprintf(stderr, "Error al reservar inodo\n");
                return FALLO;
            }
            printf("[buscar_entrada()→ reservado inodo %d tipo %c con permisos %d para %s]\n", 
                   entrada.ninodo, 'f', permisos, inicial);
        }

        // Escribir la nueva entrada al final del directorio
        if (mi_write_f(*p_inodo_dir, &entrada, inodo_dir.tamEnBytesLog, sizeof(struct entrada)) != sizeof(struct entrada)) {
            //fprintf(stderr, "Error al escribir entrada\n");
            liberar_inodo(entrada.ninodo);
            return FALLO;
        }
        printf("[buscar_entrada()→ creada entrada: %s, %d]\n", entrada.nombre, entrada.ninodo);

        // Actualizar tamaño del directorio
        inodo_dir.tamEnBytesLog += sizeof(struct entrada);
        if (escribir_inodo(*p_inodo_dir, &inodo_dir) == FALLO) {
            //fprintf(stderr, "Error al actualizar inodo directorio\n");
            return FALLO;
        }

        // El número de entrada es la última posición
        num_entrada_inodo = cant_entradas_inodo;
    } else if (reservar) {
        // Entrada ya existe y estamos en modo reserva
        // fprintf(stderr, "[buscar_entrada()→ Entrada ya existente]\n");
        return ERROR_ENTRADA_YA_EXISTENTE;
    }

    // Comprobar si hemos llegado al final del camino
    if (strlen(final) == 0) {
        *p_inodo = entrada.ninodo;
        *p_entrada = num_entrada_inodo;
        return EXITO;
    }

    // Llamada recursiva para continuar con el resto del camino
    *p_inodo_dir = entrada.ninodo;
    return buscar_entrada(final, p_inodo_dir, p_inodo, p_entrada, reservar, permisos);
}