// Autores: Kalyarat Asawapoom, Rupak Guni, Maria Kupriyenko

#include <stdio.h>
#include <time.h>
#include "directorios.h"
#define DEBUGN1 0
#define DEBUGN2 0
#define DEBUGN7 0

void mostrar_buscar_entrada(char *camino, char reservar){

    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;
    printf("\ncamino: %s, reservar: %d\n", camino, reservar);
    if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, reservar, 6)) < 0) {
      mostrar_error_buscar_entrada(error);
    }
    printf("**********************************************************************\n");
    return;
}

int main(int argc, char **argv) {
    if (argc != 2)
    {
        fprintf(stderr, RED "Uso: %s <nombre_dispositivo>\n" RESET, argv[0]);
        return FALLO;
    }

    // Monta el dispositivo virtual
    if (bmount(argv[1]) == FALLO)
    {
        fprintf(stdout, "Error al montar el dispositivo virtual");
        return FALLO;
    }

    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO)
    {
        fprintf(stdout, "Error al leer el superbloque");
        bumount();
        return FALLO;
    }

    // Mostramos la información del superbloque
    printf("\nDATOS DEL SUPERBLOQUE\n");
    printf("posPrimerBloqueMB = %d\n", SB.posPrimerBloqueMB);
    printf("posUltimoBloqueMB = %d\n", SB.posUltimoBloqueMB);
    printf("posPrimerBloqueAI = %d\n", SB.posPrimerBloqueAI);
    printf("posUltimoBloqueAI = %d\n", SB.posUltimoBloqueAI);
    printf("posPrimerBloqueDatos = %d\n", SB.posPrimerBloqueDatos);
    printf("posUltimoBloqueDatos = %d\n", SB.posUltimoBloqueDatos);
    printf("posInodoRaiz = %d\n", SB.posInodoRaiz);
    printf("posPrimerInodoLibre = %d\n", SB.posPrimerInodoLibre);
    printf(LBLUE "cantBloquesLibres = %d\n" RESET, SB.cantBloquesLibres);
    printf("cantInodosLibres = %d\n", SB.cantInodosLibres);
    printf("totBloques = %d\n", SB.totBloques);
    printf("totInodos = %d\n", SB.totInodos);

    #if DEBUGN1 // Muestra el mapa de bits (primer y último bit de cada zona)
        fprintf(stdout, "\nMAPA DE BITS CON BLOQUES DE METADATOS OCUPADOS\n");
        fprintf(stdout, "SB.posSB: %d → leer_bit(0) = %d\n", 0, leer_bit(0));
        fprintf(stdout, "SB.posPrimerBloqueMB: %d → leer_bit(%d) = %d\n", SB.posPrimerBloqueMB, SB.posPrimerBloqueMB, leer_bit(SB.posPrimerBloqueMB));
        fprintf(stdout, "SB.posUltimoBloqueMB: %d → leer_bit(%d) = %d\n", SB.posUltimoBloqueMB, SB.posUltimoBloqueMB, leer_bit(SB.posUltimoBloqueMB));
        fprintf(stdout, "SB.posPrimerBloqueAI: %d → leer_bit(%d) = %d\n", SB.posPrimerBloqueAI, SB.posPrimerBloqueAI, leer_bit(SB.posPrimerBloqueAI));
        fprintf(stdout, "SB.posUltimoBloqueAI: %d → leer_bit(%d) = %d\n", SB.posUltimoBloqueAI, SB.posUltimoBloqueAI, leer_bit(SB.posUltimoBloqueAI));
        fprintf(stdout, "SB.posPrimerBloqueDatos: %d → leer_bit(%d) = %d\n", SB.posPrimerBloqueDatos, SB.posPrimerBloqueDatos, leer_bit(SB.posPrimerBloqueDatos));
        fprintf(stdout, "SB.posUltimoBloqueDatos: %d → leer_bit(%d) = %d\n", SB.posUltimoBloqueDatos, SB.posUltimoBloqueDatos, leer_bit(SB.posUltimoBloqueDatos));

        // Reserva y libera un bloque
        fprintf(stdout, "\nRESERVAMOS UN BLOQUE Y LUEGO LO LIBERAMOS\n");
        int bloque_reservado = reservar_bloque();
        if (bloque_reservado == -1)
        {
            perror("Error al reservar un bloque");
        }
        else
        {
            fprintf(stdout, "Se ha reservado el bloque físico nº %d que era el 1º libre indicado por el MB\n", bloque_reservado);
            bread(posSB, &SB); // Vuelve a leer el superbloque para actualizar datos
            fprintf(stdout, "SB.cantBloquesLibres = %d\n", SB.cantBloquesLibres);

            // Libera el bloque reservado
            if (liberar_bloque(bloque_reservado) == -1)
            {
                fprintf(stderr, "Error al liberar el bloque");
            }
            else
            {
                bread(posSB, &SB);
                fprintf(stdout, "Liberamos ese bloque y después SB.cantBloquesLibres = %d\n", SB.cantBloquesLibres);
            }
        }

        // Muestra datos del inodo del directorio raíz
        struct inodo inodo;
        if (leer_inodo(SB.posInodoRaiz, &inodo) == FALLO)
        {
            fprintf(stderr, "Error al leer el inodo raíz");
            bumount();
            return FALLO;
        }

        // Obtiene los tiempos
        char atime[80], mtime[80], ctime[80], btime[80];
        struct tm *ts;

        ts = localtime(&inodo.atime);
        strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);

        ts = localtime(&inodo.mtime);
        strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);

        ts = localtime(&inodo.ctime);
        strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);

        ts = localtime(&inodo.btime);
        strftime(btime, sizeof(btime), "%a %Y-%m-%d %H:%M:%S", ts);

        // Mostramos datos
        fprintf(stdout, "\nDATOS DEL DIRECTORIO RAIZ\n");
        fprintf(stdout, "tipo: %c\n", (inodo.tipo == 'd') ? 'd' : '-');
        fprintf(stdout, "permisos: %d\n", inodo.permisos);
        fprintf(stdout, "atime: %s\n", atime);
        fprintf(stdout, "mtime: %s\n", mtime);
        fprintf(stdout, "ctime: %s\n", ctime);
        fprintf(stdout, "btime: %s\n", btime);
        fprintf(stdout, "nlinks: %d\n", inodo.nlinks);
        fprintf(stdout, "tamEnBytesLog: %d\n", inodo.tamEnBytesLog);
        fprintf(stdout, "numBloquesOcupados: %d\n", inodo.numBloquesOcupados);

        // Desmonta el sistema de ficheros
        bumount();
    #endif

    // char *disco = argv[1];
    // if (bmount(disco) == -1) {
    //     fprintf(stderr, "Error al montar el sistema de archivos\n");
    //     return FALLO;
    // }
    #if DEBUGN2

        // Reserva un inodo para la prueba
        int ninodo = reservar_inodo('f', 6);
        if (ninodo == -1)
        {
            fprintf(stderr, "Error al reservar inodo\n");
            return 1;
        }

        // Creamos un inodo
        struct inodo inodo;
        if (leer_inodo(ninodo, &inodo) == -1)
        {
            fprintf(stderr, "Error al leer inodo %d\n", ninodo);
            bumount();
            return FALLO;
        }

        char atime[80], mtime[80], ctime[80], btime[80];
        struct tm *ts;

        ts = localtime(&inodo.atime);
        strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);

        ts = localtime(&inodo.mtime);
        strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);

        ts = localtime(&inodo.ctime);
        strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);

        ts = localtime(&inodo.btime);
        strftime(btime, sizeof(btime), "%a %Y-%m-%d %H:%M:%S", ts);

        // Traducimos el inodo en diferentes offsets y mostramos su información para validar que traducir_boque_inodo funciona bien
        fprintf(stdout, "INODO %d. TRADUCCIÓN DE BLOQUES LÓGICOS 8, 204, 30004, 400004 y 468750\n\n", ninodo);

        int bloques[] = {8, 204, 30004, 400004, 468750};
        for (int i = 0; i < 5; i++)
        {
            int ptr = traducir_bloque_inodo(ninodo, bloques[i], 1);
            printf("\n");
            if (ptr == -1)
            {
                fprintf(stderr, "Error al traducir bloque lógico %d\n", bloques[i]);
            }
        }

        // Mostramos información por consola
        fprintf(stdout, "\nDATOS DEL INODO RESERVADO %d\n", ninodo);
        fprintf(stdout, "Tipo: %c\n", inodo.tipo);
        fprintf(stdout, "Permisos: %d\n", inodo.permisos);
        fprintf(stdout, "atime: %s\n", atime);
        fprintf(stdout, "mtime: %s\n", mtime);
        fprintf(stdout, "ctime: %s\n", ctime);
        fprintf(stdout, "btime: %s\n", btime);
        fprintf(stdout, "nlinks: %d\n", inodo.nlinks);
        fprintf(stdout, "tamEnBytesLog: %d\n", inodo.tamEnBytesLog);
        fprintf(stdout, BLUE "numBloquesOcupados: %d\n" RESET, inodo.numBloquesOcupados);
        fprintf(stdout, BLUE "posPrimerInodoLibre = %d\n" RESET, SB.posPrimerInodoLibre);
    #endif

    #if DEBUGN7
        //Mostrar creación directorios y errores
        mostrar_buscar_entrada("pruebas/", 1); //ERROR_CAMINO_INCORRECTO
        mostrar_buscar_entrada("/pruebas/", 0); //ERROR_NO_EXISTE_ENTRADA_CONSULTA
        mostrar_buscar_entrada("/pruebas/docs/", 1); //ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO
        mostrar_buscar_entrada("/pruebas/", 1); // creamos /pruebas/
        mostrar_buscar_entrada("/pruebas/docs/", 1); //creamos /pruebas/docs/
        mostrar_buscar_entrada("/pruebas/docs/doc1", 1); //creamos /pruebas/docs/doc1
        mostrar_buscar_entrada("/pruebas/docs/doc1/doc11", 1);  
        //ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO
        mostrar_buscar_entrada("/pruebas/", 1); //ERROR_ENTRADA_YA_EXISTENTE
        mostrar_buscar_entrada("/pruebas/docs/doc1", 0); //consultamos /pruebas/docs/doc1
        mostrar_buscar_entrada("/pruebas/docs/doc1", 1); //ERROR_ENTRADA_YA_EXISTENTE
        mostrar_buscar_entrada("/pruebas/casos/", 1); //creamos /pruebas/casos/
        mostrar_buscar_entrada("/pruebas/docs/doc2", 1); //creamos /pruebas/docs/doc2
    #endif
    // Desmontamos el disco
    bumount();

    return EXITO;
}
