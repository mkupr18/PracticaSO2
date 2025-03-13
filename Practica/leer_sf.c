#include <stdio.h>
#include <time.h>
#include "ficheros_basico.h"
#define DEBUGN1 1

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <nombre_dispositivo>\n", argv[0]);
        return FALLO;
    }
    #if DEBUGN1
        // Montar el dispositivo virtual
        if (bmount(argv[1]) == FALLO) {
            perror("Error al montar el dispositivo virtual");
            return FALLO;
        }

        struct superbloque SB;
        if (bread(posSB, &SB) == FALLO) {
            perror("Error al leer el superbloque");
            bumount();
            return FALLO;
        }

        // Mostrar información del superbloque
        printf("DATOS DEL SUPERBLOQUE\n");
        printf("posPrimerBloqueMB = %d\n", SB.posPrimerBloqueMB);
        printf("posUltimoBloqueMB = %d\n", SB.posUltimoBloqueMB);
        printf("posPrimerBloqueAI = %d\n", SB.posPrimerBloqueAI);
        printf("posUltimoBloqueAI = %d\n", SB.posUltimoBloqueAI);
        printf("posPrimerBloqueDatos = %d\n", SB.posPrimerBloqueDatos);
        printf("posUltimoBloqueDatos = %d\n", SB.posUltimoBloqueDatos);
        printf("posInodoRaiz = %d\n", SB.posInodoRaiz);
        printf("posPrimerInodoLibre = %d\n", SB.posPrimerInodoLibre);
        printf("%d\n", SB.cantBloquesLibres);
        printf("cantBloquesLibres = %d\n", SB.cantBloquesLibres);
        printf("cantInodosLibres = %d\n", SB.cantInodosLibres);
        printf("totBloques = %d\n", SB.totBloques);
        printf("totInodos = %d\n", SB.totInodos);

        // Mostrar el mapa de bits (primer y último bit de cada zona)
        printf("\nMAPA DE BITS CON BLOQUES DE METADATOS OCUPADOS\n");
        printf("SB.posSB: %d → leer_bit(0) = %d\n", 0, leer_bit(0));
        printf("SB.posPrimerBloqueMB: %d → leer_bit(%d) = %d\n", SB.posPrimerBloqueMB, SB.posPrimerBloqueMB, leer_bit(SB.posPrimerBloqueMB));
        printf("SB.posUltimoBloqueMB: %d → leer_bit(%d) = %d\n", SB.posUltimoBloqueMB, SB.posUltimoBloqueMB, leer_bit(SB.posUltimoBloqueMB));
        printf("SB.posPrimerBloqueAI: %d → leer_bit(%d) = %d\n", SB.posPrimerBloqueAI, SB.posPrimerBloqueAI, leer_bit(SB.posPrimerBloqueAI));
        printf("SB.posUltimoBloqueAI: %d → leer_bit(%d) = %d\n", SB.posUltimoBloqueAI, SB.posUltimoBloqueAI, leer_bit(SB.posUltimoBloqueAI));
        printf("SB.posPrimerBloqueDatos: %d → leer_bit(%d) = %d\n", SB.posPrimerBloqueDatos, SB.posPrimerBloqueDatos, leer_bit(SB.posPrimerBloqueDatos));
        printf("SB.posUltimoBloqueDatos: %d → leer_bit(%d) = %d\n", SB.posUltimoBloqueDatos, SB.posUltimoBloqueDatos, leer_bit(SB.posUltimoBloqueDatos));

        // Reservar y liberar un bloque
        printf("\nRESERVAMOS UN BLOQUE Y LUEGO LO LIBERAMOS\n");
        int bloque_reservado = reservar_bloque();
        if (bloque_reservado == -1) {
            perror("Error al reservar un bloque");
        } else {
            printf("Se ha reservado el bloque físico nº %d que era el 1º libre indicado por el MB\n", bloque_reservado);
            bread(posSB, &SB);  // Volver a leer el superbloque para actualizar datos
            printf("SB.cantBloquesLibres = %d\n", SB.cantBloquesLibres);

            // Liberar el bloque reservado
            if (liberar_bloque(bloque_reservado) == -1) {
                perror("Error al liberar el bloque");
            } else {
                bread(posSB, &SB);
                printf("Liberamos ese bloque y después SB.cantBloquesLibres = %d\n", SB.cantBloquesLibres);
            }
        }

        // Mostrar datos del inodo del directorio raíz
        struct inodo inodo;
        if (leer_inodo(SB.posInodoRaiz, &inodo) == FALLO) {
            perror("Error al leer el inodo raíz");
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

        printf("\nDATOS DEL DIRECTORIO RAIZ\n");
        printf("tipo: %c\n", (inodo.tipo == 'd') ? 'd' : '-');
        printf("permisos: %d\n", inodo.permisos);
        printf("atime: %s\n", atime);
        printf("mtime: %s\n", mtime);
        printf("ctime: %s\n", ctime);
        printf("btime: %s\n", btime);
        printf("nlinks: %d\n", inodo.nlinks);
        printf("tamEnBytesLog: %d\n", inodo.tamEnBytesLog);
        printf("numBloquesOcupados: %d\n", inodo.numBloquesOcupados);

        // Desmontar el sistema de ficheros
        bumount();
    #endif

    char *disco = argv[1];
    if (bmount(disco) == -1) {
        fprintf(stderr, "Error al montar el sistema de archivos\n");
        return 1;
    }

    // Reservar un inodo para la prueba
    int ninodo = reservar_inodo('f', 6);
    if (ninodo == -1) {
        fprintf(stderr, "Error al reservar inodo\n");
        return 1;
    }

    printf("INODO %d. TRADUCCIÓN DE BLOQUES LÓGICOS 8, 204, 30004, 400004 y 468750\n", ninodo);

    int bloques[] = {8, 204, 30004, 400004, 468750};
    for (int i = 0; i < 5; i++) {
        int ptr = traducir_bloque_inodo(ninodo, bloques[i], 1);
        if (ptr == -1) {
            printf("Error al traducir bloque lógico %d\n", bloques[i]);
        }
    }

    //struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == -1) {
        fprintf(stderr, "Error al leer inodo %d\n", ninodo);
        return 1;
    }

    printf("\nDATOS DEL INODO RESERVADO %d\n", ninodo);
    printf("Tipo: %c\n", inodo.tipo);
    printf("Permisos: %d\n", inodo.permisos);
    printf("Número de bloques ocupados: %d\n", inodo.numBloquesOcupados);

    bumount();
    return EXITO;
}
