// Autores: Kalyarat Asawapoom, Rupak Guni, Maria Kupriyenko

//verificacion.c
#include "verificacion.h"


#define CANT_REGISTROS_BUFFER 256

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <nombre_dispositivo> <directorio_simulacion>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *nombre_dispositivo = argv[1];
    const char *directorio_simulacion = argv[2];
    struct INFORMACION info;
    struct entrada entradas[NUMPROCESOS];
    int n_entradas = 0;
    char camino_fichero[200];
    unsigned int offset = 0;
    struct REGISTRO buffer[CANT_REGISTROS_BUFFER];

    // Montar dispositivo
    if (bmount(nombre_dispositivo) == -1) {
        fprintf(stderr, RED "Error montando el dispositivo" RESET);
        return FALLO;
    }

    // Calcular número de entradas del directorio de simulación (revisar)
    int numentradas = mi_dir((char *)directorio_simulacion, entradas);

    if (numentradas != NUMPROCESOS) {
        fprintf(stderr, "Error: número de entradas (%d) != NUMPROCESOS (%d)\n", numentradas, NUMPROCESOS);
        bumount();
        return FALLO;
    }

    // Crear informe.txt
    char informe_path[200];
    sprintf(informe_path, "%s/informe.txt", directorio_simulacion);
    if (mi_creat(informe_path, 6) < 0) {
        fprintf(stderr, RED "Error creando informe.txt" RESET);
        bumount();
        return FALLO;
    }

    // Recorrer cada directorio de proceso
    for (int i = 0; i < numentradas; i++) {
        // Extraer PID
        char *ptr = strchr(entradas[i].nombre, '_');
        int pid = atoi(ptr + 1);
        info.pid = pid;
        info.nEscrituras = 0;

        // Inicializar struct
        memset(&info.PrimeraEscritura, 0, sizeof(struct REGISTRO));
        memset(&info.UltimaEscritura, 0, sizeof(struct REGISTRO));
        memset(&info.MenorPosicion, 0, sizeof(struct REGISTRO));
        memset(&info.MayorPosicion, 0, sizeof(struct REGISTRO));

        // Abrir prueba.dat
        sprintf(camino_fichero, "%s/%s/prueba.dat", directorio_simulacion, entradas[i].nombre);
          offset = 0;
        int bytes_leidos;

        while (1) {
            memset(buffer, 0, sizeof(buffer));  // 🔧 Limpieza del buffer antes de cada lectura

            bytes_leidos = mi_read(camino_fichero, buffer, offset, sizeof(buffer));
            if (bytes_leidos <= 0) break;

            int registros_leidos = bytes_leidos / sizeof(struct REGISTRO);
            for (int j = 0; j < registros_leidos; j++) {
                if (buffer[j].pid == pid) {
                    if (info.nEscrituras == 0) {
                        info.PrimeraEscritura = buffer[j];
                        info.UltimaEscritura = buffer[j];
                        info.MenorPosicion = buffer[j];
                        info.MayorPosicion = buffer[j];
                    } else {
                        if (buffer[j].nEscritura < info.PrimeraEscritura.nEscritura)
                            info.PrimeraEscritura = buffer[j];
                        if (buffer[j].nEscritura > info.UltimaEscritura.nEscritura)
                            info.UltimaEscritura = buffer[j];
                        if (buffer[j].nRegistro < info.MenorPosicion.nRegistro)
                            info.MenorPosicion = buffer[j];
                        if (buffer[j].nRegistro > info.MayorPosicion.nRegistro)
                            info.MayorPosicion = buffer[j];
                    }
                    info.nEscrituras++;
                }
            }
            offset += bytes_leidos;
        }
        // Escribir en informe.txt
        char linea[300];
        sprintf(linea, "PID: %d\nNumero de escrituras: %d\n", info.pid, info.nEscrituras);
        mi_write(informe_path, linea, -1, strlen(linea));

        struct REGISTRO registros[4] = {info.PrimeraEscritura, info.UltimaEscritura, info.MenorPosicion, info.MayorPosicion};
        const char *etiquetas[4] = {"Primera Escritura", "Ultima Escritura", "Menor Posicion", "Mayor Posicion"};
        char fecha[80];

        for (int k = 0; k < 4; k++) {
            struct tm *tm = localtime(&registros[k].fecha);
            strftime(fecha, sizeof(fecha), "%Y-%m-%d %H:%M:%S", tm);
            sprintf(linea, "%s\t%d\t%d\t%s\n", etiquetas[k], registros[k].nEscritura, registros[k].nRegistro, fecha);
            mi_write(informe_path, linea, -1, strlen(linea));
        }

        mi_write(informe_path, "\n", -1, 1);

        printf("[%d) %d escrituras validadas en %s]\n", i + 1, info.nEscrituras, camino_fichero);
    }

    // Desmontar dispositivo
    bumount();
    return EXIT_SUCCESS;
}