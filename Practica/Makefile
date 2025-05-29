# Autores: Kalyarat Asawapoom, Rupak Guni, Maria Kupriyenko

CFLAGS=-c -g -Wall -std=gnu99
LDFLAGS=-pthread
 

SOURCES=mi_mkfs.c bloques.c ficheros_basico.c leer_sf.c ficheros.c directorios.c mi_mkdir.c mi_touch.c mi_ls.c mi_chmod.c mi_stat.c mi_escribir.c mi_cat.c mi_link.c mi_rm.c mi_rmdir.c mi_rm_r.c semaforo_mutex_posix.c simulacion.c verificacion.c 
LIBRARIES=bloques.o ficheros_basico.o ficheros.o directorios.o semaforo_mutex_posix.o 
INCLUDES=bloques.h ficheros_basico.h ficheros.h directorios.h semaforo_mutex_posix.h simulacion.h verificacion.h
PROGRAMS=mi_mkfs leer_sf mi_mkdir mi_touch mi_ls mi_chmod mi_stat mi_escribir mi_cat mi_link mi_rm mi_rmdir mi_rm_r simulacion verificacion 


OBJS=$(SOURCES:.c=.o)


all: $(OBJS) $(PROGRAMS)


$(PROGRAMS): $(LIBRARIES) $(INCLUDES)
	$(CC) $(LDFLAGS) $(LIBRARIES) $@.o -o $@
 

%.o: %.c $(INCLUDES)
	$(CC) $(CFLAGS) -o $@ -c $<
 

.PHONY: clean
clean:
	rm -rf *.o *~ $(PROGRAMS) disco* ext* res*



# ANTIGUO MAKEFILE (Niveles 0-10)
# CC = gcc
# CFLAGS = -c -g -Wall -std=gnu99
# SOURCES = bloques.c ficheros_basico.c mi_mkfs.c leer_sf.c ficheros.c leer.c escribir.c permitir.c truncar.c directorios.c mi_mkdir.c mi_stat.c mi_ls.c mi_chmod.c mi_touch.c mi_cat.c mi_escribir.c mi_escribir_varios.c mi_link.c mi_rm.c mi_rmdir.c mi_rm_r.c# Agrega aquí los .c necesarios
# LIBRARIES = bloques.o ficheros_basico.o  ficheros.o directorios.o # Agrega otros .o 
# PROGRAMS = mi_mkfs leer_sf leer escribir permitir truncar mi_mkdir mi_stat mi_ls mi_chmod mi_touch mi_cat mi_escribir mi_escribir_varios mi_link mi_rm mi_rmdir mi_rm_r# Agrega otros programas aquí

# OBJS = $(SOURCES:.c=.o)

# all: $(PROGRAMS)

# $(PROGRAMS): %: %.o $(LIBRARIES)
# 	$(CC) $^ -o $@

# %.o: %.c
# 	$(CC) $(CFLAGS) -o $@ $<

# .PHONY: clean
# clean:
# 	rm -rf *.o *~ $(PROGRAMS) disco* ext*
