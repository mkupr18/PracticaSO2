// Autores: Kalyarat Asawapoom, Rupak Guni, Maria Kupriyenko
//verificacion.h
#include "simulacion.h"
#include "directorios.h"

struct INFORMACION {
  int pid;
  unsigned int nEscrituras; //validadas 
  struct REGISTRO PrimeraEscritura;
  struct REGISTRO UltimaEscritura;
  struct REGISTRO MenorPosicion;
  struct REGISTRO MayorPosicion;
};

