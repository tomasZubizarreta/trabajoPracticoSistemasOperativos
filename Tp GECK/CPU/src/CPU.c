#include "./include/cpu.h"
int main(int argc, char **argv)
{

    if (argc < 2)
    {
        puts("ERROR - Arhcivo de configuracion no especificado");
        return EXIT_FAILURE;
    }
    else if (argc > 2)
    {
        puts("ERROR - Demasiados argumentos");
        return EXIT_FAILURE;
    }

    t_config *config = cargar_config(argv[1]);
    TLB = list_create();
    iniciar_conexiones(); // inicia las conexiones con memoria y con el kernel y crea 2 hilos encargados del manejo
    // de interrupciones y pcbs. Termina cuando el kernel se desconecta de ambos puertos (dispatch e interrupt)
    config_destroy(config);
    log_destroy(memoria_log);
    log_info(cpu_log, "El CPU finalizo correctamente");
    log_destroy(cpu_log);
};