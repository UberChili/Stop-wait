#include <getopt.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#define buff_size 512
#define time_default 1000

// variable opt y string y struct para manejar los command line arguments
int opt;
const char *const short_options = ":e:l:i:p:f:t:s:hv";
const struct option long_options[] = {
    {"errpr", 1, NULL, 'e'},
    {"lost", 1, NULL, 'l'},
    {"ip", 1, NULL, 'i'},
    {"port", 1, NULL, 'p'},
    {"file", 1, NULL, 'f'},
    {"size", 1, NULL, 's'},
    {"help", 0, NULL, 'h'},
    {"verbose", 0, NULL, 'v'},
    {NULL, 0, NULL, 0}};

typedef struct {
    char data[buff_size];
}
Packet;


// struct para mensajes de datos
typedef struct {
    int seqnum;
    int ack;
    Packet packet;
    size_t items;
    uint32_t FCS;
}
Frame;

// declaraciones de funciones
static int coin_flip(double percent);
static size_t get_file_size(FILE* const from);
char flip_char(char character);
bool check_file_exists(const char *spath);
int get_argc(const char *string);
void get_argv(char **words, char *string);
void usage(FILE *stream, char program_name[]);
void send_command(char command[]);
int word_coint(char string[]);


// Volado
int coin_flip(double percent) {
    //srand(time(NULL));
    double sim = (double) (rand()) / (double)RAND_MAX;
    if (sim < percent) 
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

// obtiene el tamaño de un archivo
static size_t get_file_size(FILE* const from) {
    fseek(from, 0, SEEK_END);
    size_t size = ftell(from);
    fseek(from, 0, SEEK_SET);
    return size;
}

// comprueba si un archivo existe
bool check_file_exists(const char *path)
{
    if (access(path, F_OK))
    {
        printf("[-] File \"%s\" does not exist.\n", path);
        return false;
    }
    else
    {
        printf("[+] File \"%s\" exists.\n", path);
        return true;
    }
}


// imprime en pantalla el correcto funcionamiento del programa
void usage(FILE *stream, char program_name[])
{
    fprintf(stream, "Usage: %s [options]\n", program_name);
    fprintf(stream,
            " -e --error <1-100>\t\t Porcentaje de error (default: 0) [opcional].\n"
            " -l --lost <1-100>\t\t Porcentaje de pérdida (default: 0) [opcional].\n"
            " -i --ip <X.X.X.X>\t\t Dirección IPv4 (default: 127.0.0.1) [opcional].\n"
            " -p --port <0-65535>\t\t Puerto UDP (default: 4510) [opcional].\n"
            " -f --file <filename> \t\t Ruta del archivo [obligatorio].\n"
            " -s --size <1-65535>\t\t Carga útil (default: 4096) [opcional].\n"
            " -h --help \t\t\t Muestra este mensaje de ayuda [opcional].\n"
            " -v --verbose \t\t\t Imprime mensajes detallados del funcionamiento del programa [opcional].\n");
}

// cuenta las palabras de un string (para utilizarse como 'argc' en getopt())
int get_argc(const char *string)
{
    int words = 0;
    int len;

    for (int i = 0, len = strlen(string); i < len; i++)
    {
       if (string[i] == ' ' || string[i] == '\n' || string[i] == '\0')
       {
           words++;
       }
    }
    return words;
}

// 'convierte' un string en un array de strings, (para utilizarse como 'argv' en getopt())
void get_argv(char **words, char *string)
{
    int windex = 0;

    while ( (*(words+windex) = strsep(&string, " ")) != NULL)
        windex++;
    
    //words[windex] = NULL;
}
