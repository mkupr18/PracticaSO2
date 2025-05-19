# Autores: Kalyarat Asawapoom, Rupak Guni, Maria Kupriyenko

CC = gcc
CFLAGS = -c -g -Wall -std=gnu99
SOURCES = bloques.c ficheros_basico.c mi_mkfs.c leer_sf.c ficheros.c leer.c escribir.c permitir.c truncar.c directorios.c mi_mkdir.c mi_stat.c mi_ls.c mi_chmod.c mi_touch.c mi_cat.c mi_escribir.c mi_escribir_varios.c mi_link.c mi_rm.c mi_rmdir.c mi_rm_r.c# Agrega aquí los .c necesarios
LIBRARIES = bloques.o ficheros_basico.o  ficheros.o directorios.o # Agrega otros .o 
PROGRAMS = mi_mkfs leer_sf leer escribir permitir truncar mi_mkdir mi_stat mi_ls mi_chmod mi_touch mi_cat mi_escribir mi_escribir_varios mi_link mi_rm mi_rmdir mi_rm_r# Agrega otros programas aquí

OBJS = $(SOURCES:.c=.o)

all: $(PROGRAMS)

$(PROGRAMS): %: %.o $(LIBRARIES)
	$(CC) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -rf *.o *~ $(PROGRAMS) disco* ext*
