// Autores: Kalyarat Asawapoom, Rupak Guni, Maria Kupriyenko
// verificacion.c
#include "verificacion.h"

#define CANT_REGISTROS_BUFFER 256

int main(int argc, char **argv) {
    // Valida la sintaxis
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <nombre_dispositivo> <directorio_simulacion>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *nombre_dispositivo = argv[1];
    const char *directorio_simulacion = argv[2];
    struct INFORMACION info;
    char camino_fichero[200];
    unsigned int offset = 0;
    struct REGISTRO buffer[CANT_REGISTROS_BUFFER];
    char buffer_dir[TAMBUFFER];

    // Monta el dispositivo
    if (bmount(nombre_dispositivo) == -1) {
        fprintf(stderr, RED "Error montando el dispositivo" RESET);
        return FALLO;
    }

    memset(buffer_dir, 0, sizeof(buffer_dir));
    // Obtiene la lista de entradas del directorio
    int numentradas = mi_dir(directorio_simulacion, buffer_dir, 's');

    if (numentradas < 0) {
        fprintf(stderr, RED "Error al listar el directorio\n" RESET);
        bumount();
        return FALLO;
    }

    // Extrae los nombres del buffer
    char *nombres[NUMPROCESOS];
    int num_procesos = 0;

    char *token = strtok(buffer_dir, "\t\n");
    while (token != NULL && num_procesos < NUMPROCESOS) {
        nombres[num_procesos++] = strdup(token);
        token = strtok(NULL, "\t\n");
    }

    if (num_procesos != NUMPROCESOS) {
        fprintf(stderr, RED "Error: número de entradas (%d) != NUMPROCESOS (%d)\n" RESET, num_procesos, NUMPROCESOS);
        bumount();
        return FALLO;
    }

    printf("dir_sim: %s\n", directorio_simulacion);
    printf("numentradas: %d NUMPROCESOS: %d\n", numentradas, NUMPROCESOS);

    // Crea el informe.txt
    char informe_path[200];
    sprintf(informe_path, "%sinforme.txt", directorio_simulacion);
    if (mi_creat(informe_path, 6) < 0) {
        fprintf(stderr, RED "Error creando informe.txt" RESET);
        bumount();
        return FALLO;
    }

    // Recorre cada directorio de proceso
    for (int i = 0; i < num_procesos; i++) {
        // Extrae el PID
        char *ptr = strchr(nombres[i], '_');
        if (!ptr) continue;
        int pid = atoi(ptr + 1);
        info.pid = pid;
        info.nEscrituras = 0;

        // Inicializa las estructuras
        memset(&info.PrimeraEscritura, 0, sizeof(struct REGISTRO));
        memset(&info.UltimaEscritura, 0, sizeof(struct REGISTRO));
        memset(&info.MenorPosicion, 0, sizeof(struct REGISTRO));
        memset(&info.MayorPosicion, 0, sizeof(struct REGISTRO));

        // Abre prueba.dat
        sprintf(camino_fichero, "%s%s/prueba.dat", directorio_simulacion, nombres[i]);
        offset = 0;
        int bytes_leidos;

        while (1) {
            memset(buffer, 0, sizeof(buffer));
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

       // Escribe en informe.txt
        char linea[300];
        struct STAT st;

        // Calcula el offset final para escribir al final
        mi_stat(informe_path, &st);
        unsigned int offset_informe = st.tamEnBytesLog;

        sprintf(linea, "PID: %d\nNumero de escrituras: %d\n", info.pid, info.nEscrituras);
        mi_write(informe_path, linea, offset_informe, strlen(linea));
        offset_informe += strlen(linea);

        struct REGISTRO registros[4] = {info.PrimeraEscritura, info.UltimaEscritura, info.MenorPosicion, info.MayorPosicion};
        const char *etiquetas[4] = {"Primera Escritura", "Ultima Escritura", "Menor Posicion\t", "Mayor Posicion\t"};
        char fecha[80];

        for (int k = 0; k < 4; k++) {
            struct tm *tm = localtime(&registros[k].fecha);
            strftime(fecha, sizeof(fecha), "%Y-%m-%d %H:%M:%S", tm);
            sprintf(linea, "%s\t%d\t%d\t%s\n", etiquetas[k], registros[k].nEscritura, registros[k].nRegistro, fecha);
            mi_write(informe_path, linea, offset_informe, strlen(linea));
            offset_informe += strlen(linea);
        }

        // Escribe el salto de línea al final
        mi_write(informe_path, "\n", offset_informe, 1);


        printf("[%d) %d escrituras validadas en %s]\n", i + 1, info.nEscrituras, camino_fichero);
        
        free(nombres[i]);
    }

    // Desmonta el dispositivo
    bumount();
    return EXIT_SUCCESS;
}
