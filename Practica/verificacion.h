// Autores: Kalyarat Asawapoom, Rupak Guni, Maria Kupriyenko

#include "simulacion.h"
#include "directorios.h"

struct INFORMACION {
  int pid;
  unsigned int nEscrituras; // Validadas 
  struct REGISTRO PrimeraEscritura;
  struct REGISTRO UltimaEscritura;
  struct REGISTRO MenorPosicion;
  struct REGISTRO MayorPosicion;
};

