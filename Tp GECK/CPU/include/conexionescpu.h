#ifndef CONEXIONESCPU_H_
#define CONEXIONESCPU_H_

#include <pthread.h>
#include <semaphore.h>
#include <assert.h>

//#include "estructurascpu.h"
#include "../../shared/include/sockets.h"

#include "instruccionescpu.h"
 
void iniciar_conexiones();    

#endif