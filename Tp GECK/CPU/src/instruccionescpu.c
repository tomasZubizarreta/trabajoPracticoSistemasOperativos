#include "./include/instruccionescpu.h"

bool proceso_terminado;
t_instruccion fetch(t_pcb *pcb);
bool decode(t_instruccion instruccion);
uint32_t fetch_operand(int tabla_paginas, uint32_t direccion_logica);
uint32_t leer_desde_memoria(int tabla_paginas, uint32_t direccion_logica, uint16_t PID);
uint32_t execute(t_instruccion instruccion, t_pcb *p, uint32_t regs[]);
void borrarRegistrosTlb(__uint16_t pid);


void ciclo_de_instruccion(t_pcb *pcb)
{

	log_info(cpu_log, "Comenzando ciclo con nuevo PCB...");
	//log_info(cpu_log, "PID: %d \t TP 1: %X", pcb->id, pcb->tabla_paginas);
	t_instruccion instruccion_actual;
	uint32_t hayIO;
	uint32_t operand;
	uint32_t registros[4];
	for (int i = 0; i < 4; i++)
	{
		registros[i] = pcb->valorRegistros[i];
	}

	t_buffer *buffer = malloc(sizeof(t_buffer));
	tlb_hits = 0;
	acc_memoria = 0;

	while (pcb)
	{
		instruccion_actual = fetch(pcb);
		log_info(cpu_log, "Instruccion nº%d: %d", pcb->program_counter, instruccion_actual.identificador);
		if (decode(instruccion_actual))
			usleep(1000 * cpuconfig.retardo_instr);//retardo para instrucciones q no acceden a memoria
		hayIO = execute(instruccion_actual, pcb, registros);
		if (proceso_terminado)
		{
			proceso_terminado = false;
			borrarRegistrosTlb(pcb->id);
			log_info(cpu_log, "Proceso %d TERMINADO", pcb->id);
			log_info(cpu_log, "Devolviendo PCB actualizado del PID %d...", pcb->id);
			chequear_instrucciones(pcb->instrucciones, pcb->cant_instrucciones);
			enviar_pcb(pcb, socket_dispatch, 0);
			liberar_pcb(pcb);
			pcb = NULL; // demas?
			continue;
		}
		pcb->program_counter++;
		if (hayIO)
		{
			log_info(cpu_log, "Operacion de IO por %dms solicitada", hayIO);
			log_info(cpu_log, "Devolviendo PCB actualizado del PID %d...", pcb->id);
			chequear_instrucciones(pcb->instrucciones, pcb->cant_instrucciones);
			enviar_pcb_IO(pcb, socket_dispatch, hayIO, instruccion_actual.parametros[0], instruccion_actual.parametros[1]);
			liberar_pcb(pcb);
			pcb = NULL; // demas?
			continue;
		}

		sem_wait(&mutex_interrupt);
		if (check_interrupt)
		{
			log_warning(interrupt_log, "Se detecto una interrupcion!!!");
			log_info(interrupt_log, "Enviando PCB del PID %d...", pcb->id);
			check_interrupt = false;

			chequear_instrucciones(pcb->instrucciones, pcb->cant_instrucciones); // para?
			enviar_pcb(pcb, socket_dispatch, 0);
			liberar_pcb(pcb);
			pcb = NULL; // demas?
		}
		sem_post(&mutex_interrupt);
	}
	//log_info(cpu_log, "TLB HITS: %d  ACCESOS MEMORIA: %d", tlb_hits, acc_memoria);
	free(buffer);
}

t_instruccion fetch(t_pcb *pcb)
{
	t_instruccion instruccion_nueva;
	instruccion_nueva = *((t_instruccion *)list_get(pcb->instrucciones, pcb->program_counter));

	log_info(cpu_log, "Program Counter: %d + 1", pcb->program_counter);	
	return instruccion_nueva;
}

bool decode(t_instruccion instruccion)
{
	if (instruccion.identificador == SET || instruccion.identificador == ADD)
		return true;
	return false;
}



char *regsitro(uint32_t reg)
{
	switch (reg)
	{
	case 1:
		return "AX";
	case 2:
		return "BX";
	case 3:
		return "CX";
	case 4:
		return "DX";
	}
}

//Cambiar como se escribe memoria
/*
bool escribir_a_memoria(int tabla_paginas, uint32_t direccion_logica, void *dato_a_escribir)
{
	int nro_de_pagina = floor(((double)direccion_logica) / ((double)*tam_de_pagina));
	int desplazamiento = direccion_logica - nro_de_pagina * (*tam_de_pagina);
	int direccion_fisica = obtener_direccion_fisica(tabla_paginas, direccion_logica, *entradas_por_tabla, *tam_de_pagina);

	int diferencia = desplazamiento + 4 - *tam_de_pagina;
	bool confirmacion;

	if (diferencia <= 0)
		confirmacion = pedir_escritura(4, dato_a_escribir, direccion_fisica);

	else
	{
		confirmacion = pedir_escritura(4 - diferencia, dato_a_escribir, direccion_fisica);
		if (confirmacion == false)
			return confirmacion;
		nro_de_pagina += 1;
		int nueva_direccion_logica = nro_de_pagina * (*tam_de_pagina);
		direccion_fisica = obtener_direccion_fisica(tabla_paginas, nueva_direccion_logica, *entradas_por_tabla, *tam_de_pagina);
		confirmacion = pedir_escritura(diferencia, dato_a_escribir + 4 - diferencia, direccion_fisica);
	}
	return confirmacion;
}
*/
// Cambiar como se lee de memoria
uint32_t leer_desde_memoria(int tabla_paginas, uint32_t direccion_logica, const uint16_t PID)
{
	uint32_t retorno;
	int direccion_fisica = obtener_direccion_fisica(tabla_paginas, direccion_logica, *entradas_por_tabla, *tam_de_pagina, PID);
	/*
	int nro_de_pagina = floor(((double) direccion_logica) / ((double) *tam_de_pagina));
	int desplazamiento = direccion_logica - nro_de_pagina * (*tam_de_pagina);
	int direccion_fisica = obtener_direccion_fisica(tabla_paginas, direccion_logica, *entradas_por_tabla, *tam_de_pagina);
	void* valor_leido;
	int diferencia = desplazamiento + 4 - *tam_de_pagina;
	uint32_t retorno;
	if (diferencia <= 0)
		valor_leido = pedir_lectura(4, direccion_fisica);

	else {
		valor_leido = pedir_lectura(4 - diferencia, direccion_fisica);
		valor_leido = realloc(valor_leido, 4);
		nro_de_pagina += 1;
		void *nuevo_valor_leido;
		int nueva_direccion_logica = nro_de_pagina * (*tam_de_pagina);
		direccion_fisica = obtener_direccion_fisica(tabla_paginas, nueva_direccion_logica, *entradas_por_tabla, *tam_de_pagina);
		nuevo_valor_leido = pedir_lectura(diferencia, direccion_fisica);
		memcpy(valor_leido + 4 - diferencia, nuevo_valor_leido, diferencia);
		free(nuevo_valor_leido);
	}
	*/
	// completar el log despues de cambiar la lectura
	// log_info(cpu_log, "PID: %d - Acción: LEER - Segmento: %d - Pagina: %d - Dirección Fisica: %d",PID,,nro_de_pagina, direccion_fisica);

	return retorno;
}

uint32_t execute(const t_instruccion instruccion, t_pcb *p, uint32_t regs[])
{	
	uint32_t valor;
	const int tabla_paginas = p->tabla_paginas;
	switch (instruccion.identificador)
	{
	case SET:
		p->valorRegistros[instruccion.parametros[0] - 1] = instruccion.parametros[1];
		regs[instruccion.parametros[0] - 1] = instruccion.parametros[1];
		// valor_leido = leer_desde_memoria(tabla_paginas, instruccion.parametros[0]);
		log_info(cpu_log, "El valor asignado al registro %s es: %d", registro(instruccion.parametros[0]), instruccion.parametros[1]);		
		break;

	case ADD:
		regs[instruccion.parametros[0] - 1] += regs[instruccion.parametros[1] - 1];
		p->valorRegistros[instruccion.parametros[0] - 1] = regs[instruccion.parametros[0] - 1];
		log_info(cpu_log, "El valor de la suma es %d y se almaceno en el registro %s", regs[instruccion.parametros[0] - 1], registro(instruccion.parametros[0]));
		// escribir_a_memoria(tabla_paginas, instruccion.parametros[0], &(instruccion.parametros[1]));		
		break;

	case MOVE_IN:
		valor = leer_desde_memoria(tabla_paginas, instruccion.parametros[1], p->id);
		if(valor == -1){
			log_info(cpu_log, "El valor de la suma es %d y se almaceno en el registro %s", regs[instruccion.parametros[0] - 1], registro(instruccion.parametros[0]));
		}
		regs[instruccion.parametros[0] - 1] = valor;
		// escribir_a_memoria(tabla_paginas, instruccion.parametros[0], (void*) &operando);
		break;

	case MOVE_OUT:
		log_info(cpu_log, "NO_OP");
		break;

	case IO: // CAMBIAR
		return 1;
		break;

	case EXIT:
		proceso_terminado = true;
		break;

	default:
		break;
	}
	return 0;
}

void borrarRegistrosTlb(__uint16_t pid){
	t_list_iterator* iterador_tlb;

    iterador_tlb = list_iterator_create(TLB);

    while(list_iterator_has_next(iterador_tlb)){
        if( ( (t_tlb*)list_iterator_next(iterador_tlb) )->pid == pid ){
            list_remove_and_destroy_element(TLB, iterador_tlb->index, free);            
        }
    }	
}