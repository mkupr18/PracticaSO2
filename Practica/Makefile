CC = gcc
CFLAGS = -c -g -Wall -std=gnu99
SOURCES = bloques.c ficheros_basico.c mi_mkfs.c leer_sf.c ficheros.c leer.c escribir.c permitir.c# Agrega aquí los .c necesarios
LIBRARIES = bloques.o ficheros_basico.o  ficheros.o # Agrega otros .o 
PROGRAMS = mi_mkfs leer_sf leer escribir permitir # Agrega otros programas aquí

OBJS = $(SOURCES:.c=.o)

all: $(PROGRAMS)

$(PROGRAMS): %: %.o $(LIBRARIES)
	$(CC) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -rf *.o *~ $(PROGRAMS) disco* ext*
