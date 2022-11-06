#ifndef TLB_H_
#define TLB_H_

#include <commons/collections/list.h>
#include <stdlib.h>

#include "estructurascpu.h"

typedef struct {
    __uint16_t pid;
    int segmento;
    int pagina;
    int marco;
} t_tlb;

t_list* TLB;
int tlb_hits;
int acc_memoria;

int chequear_tlb(__uint16_t pid, int segmento, int pagina);
void actualizar_tlb(__uint16_t pid, int segmento, int pagina, int marco);

#endif