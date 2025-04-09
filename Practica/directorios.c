// #include <stdio.h>
// #include <string.h>
// #include "directorios.h"

// /**
//  * @brief Muestra un mensaje de error según el código de error proporcionado.
//  *
//  * @param error Código de error devuelto por la función buscar_entrada.
//  */
// void mostrar_error_buscar_entrada(int error)
// {
//     // fprintf(stderr, "Error: %d\n", error);
//     switch (error)
//     {
//     case -2:
//         fprintf(stderr, "Error: Camino incorrecto.\n");
//         break;
//     case -3:
//         fprintf(stderr, "Error: Permiso denegado de lectura.\n");
//         break;
//     case -4:
//         fprintf(stderr, "Error: No existe el archivo o el directorio.\n");
//         break;
//     case -5:
//         fprintf(stderr, "Error: No existe algún directorio intermedio.\n");
//         break;
//     case -6:
//         fprintf(stderr, "Error: Permiso denegado de escritura.\n");
//         break;
//     case -7:
//         fprintf(stderr, "Error: El archivo ya existe.\n");
//         break;
//     case -8:
//         fprintf(stderr, "Error: No es un directorio.\n");
//         break;
//     }
// }

// /**
//  * @brief Dada una ruta que comienza con '/', separa su contenido en inicial, final y tipo.
//  *
//  * @param camino La ruta completa de entrada. Debe comenzar con '/'.
//  * @param inicial Puntero al buffer donde se almacenará el primer componente (directorio o fichero).
//  * @param final Puntero al buffer donde se almacenará el resto de la ruta.
//  * @param tipo Puntero a un char donde se almacenará 'd' (directorio) o 'f' (fichero).
//  *
//  * @return 0 si éxito, -1 si hay algún error.
//  */
// int extraer_camino(const char *camino, char *inicial, char *final, char *tipo)
// {
//     // Comprobamos si el camino empieza por '/'
//     if (camino == NULL || camino[0] != '/')
//     {
//         // fprintf(stderr, "Error en extraer_camino: El camino no empieza por '/'\n");
//         return FALLO;
//     }

//     // Buscamos el primer carácter después del '/' inicial
//     const char *start_inicial = camino + 1;

//     // Buscamos la siguiente aparición de '/' a partir de start_inicial
//     char *segundo_slash = strchr(start_inicial, '/');

//     if (segundo_slash != NULL) // Se encontró un segundo '/', indica un directorio en 'inicial'
//     {
//         // Calculamos la longitud del nombre en 'inicial'
//         int longitud_inicial = segundo_slash - start_inicial;

//         // Copiamos el nombre del directorio a 'inicial'
//         strncpy(inicial, start_inicial, longitud_inicial);
//         inicial[longitud_inicial] = '\0'; // Aseguramos la terminación NULL

//         // Copiamos el resto del camino (desde el segundo '/') a 'final'
//         strcpy(final, segundo_slash);

//         // Asignamos el tipo 'd' (directorio)
//         *tipo = 'd';
//     }
//     else // No se encontró un segundo '/', indica un fichero en 'inicial'
//     {
//         // Copiamos todo desde start_inicial hasta el final de 'camino' a 'inicial'
//         strcpy(inicial, start_inicial);

//         // 'final' es una cadena vacía
//         final[0] = '\0'; // O strcpy(final, "");

//         // Asignamos el tipo 'f' (fichero)
//         *tipo = 'f';
//     }

//     return EXITO;
// }

// /**
//  * @brief Busca una entrada en un directorio y, si es necesario, la crea.
//  *
//  * @param camino_parcial Ruta parcial de la entrada a buscar.
//  * @param p_inodo_dir Puntero al inodo del directorio donde buscar.
//  * @param p_inodo Puntero donde se almacenará el inodo encontrado o creado.
//  * @param p_entrada Puntero donde se almacenará el número de entrada encontrada o creada.
//  * @param reservar Indica si se debe crear la entrada si no existe (1 para sí, 0 para no).
//  * @param permisos Permisos que se asignarán en caso de lacreación de una nueva entrada.
//  *
//  * @return 0 si se encuentra o crea la entrada con éxito,
//  *         ERROR_CAMINO_INCORRECTO si la ruta es incorrecta,
//  *         ERROR_PERMISO_LECTURA si no hay permisos de lectura,
//  *         ERROR_NO_EXISTE_ENTRADA_CONSULTA si la entrada no existe y no se debe reservar,
//  *         ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO si se intenta crear en un fichero,
//  *         ERROR_PERMISO_ESCRITURA si no hay permisos de escritura,
//  *         ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO si el directorio intermedio no existe,
//  *         -1 en caso de error inesperado.
//  */
// int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada, char reservar, unsigned char permisos)
// {
//     struct entrada entrada;
//     struct inodo inodo_dir;
//     char inicial[sizeof(entrada.nombre)]; // Nombre de la entrada actual
//     char final[strlen(camino_parcial)];   // Parte restante del camino
//     char tipo;
//     int cant_entradas_inodo, num_entrada_inodo = 0;
//     struct superbloque SB;
//     if (bread(posSB, &SB) == FALLO) {
//         fprintf(stdout,"Error al leer el superbloque");
//         bumount();
//         return FALLO;
//     }

//     // Si el camino es "/", devolvemos la raíz
//     if (strcmp(camino_parcial, "/") == 0)
//     {
//         *p_inodo = SB.posInodoRaiz; //nuestra raiz siempre estará asociada al inodo 0

//         *p_entrada = 0;
//         return 0;
//     }

//     // Extraemos la primera parte del camino
//     if (extraer_camino(camino_parcial, inicial, final, &tipo) < 0)
//     {
//         return ERROR_CAMINO_INCORRECTO;
//     }

//     // Leemos el inodo del directorio actual
//     if (leer_inodo(*p_inodo_dir, &inodo_dir) < 0)
//     {
//         return ERROR_PERMISO_LECTURA;
//     }

//     // Verificamos permisos de lectura
//     if (!(inodo_dir.permisos & 4))
//     {
//         return ERROR_PERMISO_LECTURA;
//     }

//     // Calculamos cuántas entradas hay en el directorio
//     cant_entradas_inodo = inodo_dir.tamEnBytesLog / sizeof(struct entrada);

//     // Buscamos la entrada en el directorio
//     while (num_entrada_inodo < cant_entradas_inodo)
//     {
//         if (leer_entrada(*p_inodo_dir, num_entrada_inodo, &entrada) < 0)
//         {
//             return ERROR_NO_EXISTE_ENTRADA_CONSULTA;
//         }
//         if (strcmp(inicial, entrada.nombre) == 0)
//         {
//             break; // Encontramos la entrada, salimos
//         }
//         num_entrada_inodo++;
//     }

//     // Si la entrada no existe
//     if (num_entrada_inodo == cant_entradas_inodo)
//     {
//         //modo consulta. Como no existe retornamos error
//         if (!reservar) 
//         {
//             return ERROR_NO_EXISTE_ENTRADA_CONSULTA;
//         }

//         //modo escritura 
//         //Creamos la entrada en el directorio referenciado por *p_inodo_dir
//         //si es fichero no permitir escritura
//         if (inodo_dir.tipo == 'f')
//         {
//             return ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO;
//         }

//         //si es directorio comprobar que tiene permiso de escritura
//         if (!(inodo_dir.permisos & 2))
//         {
//             return ERROR_PERMISO_ESCRITURA;
//         }

//         // Creamos una nueva entrada
//         strcpy(entrada.nombre, inicial);
//         entrada.ninodo = (tipo == 'd' && strcmp(final, "/") == 0) ? reservar_inodo('d', permisos) : reservar_inodo('f', permisos);
//         if (entrada.ninodo < 0)
//         {
//             return ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO;
//         }

//         // Guardamos la nueva entrada
//         if (escribir_entrada(*p_inodo_dir, &entrada, num_entrada_inodo) < 0)
//         {
//             liberar_inodo(entrada.ninodo);
//             return FALLO;
//         }
//     }

//     // Si hemos llegado al final del camino, asignamos el inodo encontrado
//     if (strlen(final) == 0)
//     {
//         *p_inodo = entrada.ninodo;
//         *p_entrada = num_entrada_inodo;
//         return EXITO;
//     }

//     // Llamada recursiva con la parte restante del camino
//     *p_inodo_dir = entrada.ninodo;
//     return buscar_entrada(final, p_inodo_dir, p_inodo, p_entrada, reservar, permisos);
// }

