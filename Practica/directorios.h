// Autores: Kalyarat Asawapoom, Rupak Guni, Maria Kupriyenko

#include "ficheros.h"

#define TAMNOMBRE 60 // Tamaño del nombre de directorio o fichero

#define ERROR_CAMINO_INCORRECTO (-2)
#define ERROR_PERMISO_LECTURA (-3)
#define ERROR_NO_EXISTE_ENTRADA_CONSULTA (-4)
#define ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO (-5)
#define ERROR_PERMISO_ESCRITURA (-6)
#define ERROR_ENTRADA_YA_EXISTENTE (-7)
#define ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO (-8)

// Para mi_ls 
#define TAMFILA 100
#define TAMBUFFER (TAMFILA * 1000)

struct entrada {
  char nombre[TAMNOMBRE];
  unsigned int ninodo;
};


#define TAMNOMBRE 60 // Tamaño del nombre de directorio o fichero
#define PROFUNDIDAD 32 // Profundidad máxima del árbol de directorios
#define CACHESIZE 3 // Tamaño de la caché para escrituras FIFO

struct UltimaEntrada{
   char camino [TAMNOMBRE*PROFUNDIDAD];
   int p_inodo;

};




void mostrar_error_buscar_entrada(int error);
int extraer_camino(const char *camino, char *inicial, char *final, char *tipo);
int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada, char reservar, unsigned char permisos);
int mi_creat(const char *camino, unsigned char permisos);
int mi_chmod(const char *camino, unsigned char permisos);
int mi_stat(const char *camino, struct STAT *p_stat);
int mi_dir(const char *camino, char *buffer, char flag);
int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes);
int mi_read(const char *camino, void *buf, unsigned int offset, unsigned int nbytes);
int mi_link(const char *camino1, const char *camino2);
int mi_unlink(const char *camino);
