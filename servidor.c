/** Servidor
*/

#include <stdint.h>
#include "helpers.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>

#include "crc32.h"

/**
 * @brief Imprime el correcto uso.
 * 
 * @return El código de salida.
 */
static int print_usage(void) {
    puts("Usage: ./server [port]\n");
    return EXIT_SUCCESS;
}

/**
 * @brief Configure 'config' a utilizar TCP/IPv$ y 'port'.
 *        Un nuevo socket es creado y ligado a 'config'.
 * 
 * @param port El puerto deseado.
 * @param config La configuración a ser ingresada al puerto.
 * @return El file descriptor de socket resultante.
 */
static int bind_socket(const int port, struct sockaddr_in* const config) {
    memset(config, '\0', sizeof *config);
    config->sin_family = AF_INET;
    config->sin_port = htons(port);
    config->sin_addr.s_addr = INADDR_ANY;
    // Create the socket and execute `bind()`.
    // Throw away the return value of the call.
    const int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    bind(sock_fd, (struct sockaddr*)config, sizeof *config);
    return sock_fd;
}

/**
 * @brief Obtiene un path de archido de parte del cliente.
 *        Se bloquea hasta que se otorgue un string.
 * 
 * @param sock_fd El file descriptor del socket.
 * @param buffer El buffer a ser escrito.
 *                    Siempre será terminado en nul en éxito.
 * @param buffer_size La máxima capacidad del buffer en bytes.
 * @param client_config La configuración del cliente.
 * @return El número de bytes leídos y por lo tanto escritos en el buffer.
 */
static ssize_t recv_file_path(const int sock_fd, char* const buffer, const size_t buffer_size, struct sockaddr_in* const client_config) {
    socklen_t client_size = sizeof *client_config;
    const ssize_t bytes_read = recvfrom(sock_fd, buffer, buffer_size, MSG_WAITALL, (struct sockaddr*)client_config, &client_size);
    if (bytes_read >= 0)
    {
        buffer[bytes_read] = '\0';
    }
    return bytes_read;
}

/**
 * @brief Envía una respuesta simple al cliente.
 * 
 * @param sock_fd El dile descriptor del socket.
 * @param response La respuesta a ser enviada. Debe ser un string.
 * @param client_config La configuración del cliente al que debe ser enviada la respuesta.
 */
static void send_response(const int sock_fd, const char* const response, const struct sockaddr_in* const client_config) {
    sendto(sock_fd, response, strlen(response), 0, (const struct sockaddr*)client_config, sizeof *client_config);
}

/**
 * @brief Lee un cacho de archivo de un archivo especificado.
 * 
 * @param from El archivo del cuál se va a leer.
 * @param buffer El buffer al cuál se va a escribir.
 * @param bytes El tamaño del cacho en bytes.
 * @return La cantidad de bytes que se leyeron, es decir, el cacho.
 */
static size_t read_file_chunk(FILE* const from, void* const buffer, const size_t bytes) {
    return fread(buffer, sizeof(char), bytes, from);
}

/**
 * @brief Envía un cacho de archivo a un cliente especificado.
 * 
 * @param sock_fd El file descriptor del socket.
 * @param frame El frame a ser enviado.
 * @param client_config El cliente al que debería ser enviada la respuesta.
 * @return El número de bytes enviados, o -1 en error.
 */
static ssize_t send_file_chunk(const int sock_fd, const Frame* const frame, const struct sockaddr_in* const client_config) {
    return sendto(sock_fd, frame, sizeof *frame, 0, (const struct sockaddr*)client_config, sizeof *client_config);
}

/**
 * @brief Obtiene un path de archivo del cliente.
 *        Se bloqueará hasta que se reciba una respuesta.
 * 
 * @param sock_fd El file descriptor del socket.
 * @param frame El frame en el que se escribirá.
 * @param client_config La configuración del cliente.
 * @return El número de bytes leídos y por lo tanto escritos en el buffer.
 */
static ssize_t recv_ack(const int sock_fd, Frame* const frame, struct sockaddr_in* const client_config) {
    socklen_t client_size = sizeof *client_config;
    return recvfrom(sock_fd, frame, sizeof *frame, MSG_WAITALL, (struct sockaddr*)client_config, &client_size);
}

/**
 * @brief Envía los contenidos de un arvhico completo a un cliente.
 * 
 * @param sock_fd El file descriptor del socket.
 * @param filename El nombre el archivo a enviar.
 *                     Se asume que existe, o el servidor fallará.
 * @param client_config La configuración del cliente.
 */
//static void send_file(const int sock_fd, const char* const filename, struct sockaddr_in* const client_config, const int percent, const int time)
//static void send_file(const int sock_fd, const char* const filename, struct sockaddr_in* const client_config, const int miliseconds, double e_percent)
static void send_file(const int sock_fd, const char* const filename, struct sockaddr_in* const client_config, const int miliseconds, double e_percent)
{
    // Nada que hacer si no se puede abrir el archivo de origen
    printf("[+] Sending file \"%s\".\n", filename);
    FILE* const input_file = fopen(filename, "r");
    if (input_file == NULL)
    {
        printf("Unable to open file %s to read\n", filename);
        exit(EXIT_FAILURE);
    }

    // Inicializar en cero ambos frames
    Frame send_frame = {0};
    Frame recv_frame = {0};

    // Establecer el timeout del socket, según el valor recibido
    const struct timeval timeout = {0, miliseconds};
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval)); 

    int msg_counter = 0;
    int ack_counter = 0;

    // Leemos un cacho a la vez del archivo
    while ((send_frame.items = read_file_chunk(input_file, send_frame.packet.data, buff_size)))
    {
        send_frame.FCS = crc32_buffer(buff_size, buff_size % 2, (const unsigned char*)send_frame.packet.data);

/*        if (coin_flip(e_percent) == 0)
        {
            printf("[-] Paquete corrupto.\n");
            send_frame.FCS = crc32_buffer(buff_size, buff_size % 2, (const unsigned char*)send_frame.packet.data) + 1;
        }
        if (coin_flip(e_percent) == 0)
        {
            printf("[-] Paquete corrupto.\n");
            send_frame.FCS = crc32_buffer(buff_size, buff_size % 2, (const unsigned char*)send_frame.packet.data) + 1;
        }
*/

        // Enviamos el cacho al cliente
        send_file_chunk(sock_fd, &send_frame, client_config);
        printf("[+] Mensaje enviado. Seqnum %d, bytes: %zu\n", send_frame.seqnum, send_frame.items);
        msg_counter++;

        // Esperamos el acknowledgement
        // Reenviamos el paquete cada vez que ocurra un timeout
        while (recv_ack(sock_fd, &recv_frame, client_config) < 0)
        {
            fprintf(stderr, "[-] Tiempo agotado, reenviando paquete (%s).\n", strerror(errno));
            send_file_chunk(sock_fd, &send_frame, client_config);
            printf("[+] Mensaje re-enviado. Seqnum %d, bytes: %zu\n", send_frame.seqnum, send_frame.items);
            msg_counter++;
        }
        ack_counter++;

        // Invertimos el seqnum del siguiente cacho
        send_frame.seqnum = send_frame.seqnum ? 0 : 1;

        // Recibir un ack negativo significa que acabamos
        if (recv_frame.ack == -1) 
        {
            printf("[+] Archivo \"%s\" enviado.\n", filename);

            // Imprimimos información sobre el archivo obtenido
            printf("[+] Obtención de archivo \"%s\" finalizada!\n", filename);
            printf("Nombre del archivo: %s\n", filename);
            printf("Tamaño del archivo: %zu bytes.\n", get_file_size(input_file));
            printf("Tamaño del buffer: %d bytes.\n", buff_size);
            printf("Total de mensajes enviados (DATA): %d.\n", msg_counter);
            printf("Total de confirmaciones recibidas (ACK): %d.\n", ack_counter);
            printf("[+] Listo...\n");

            break;
        }
    }
    fclose(input_file);
}

int main(int argc, char **argv)
{
    // Revisamos el correcto funcionamiento
/*    if (argc != 2) {
        return print_usage();
    }
*/

    // Configuramos y activamos el servidor
    struct sockaddr_in server_config = {0};
    const int port = 2020;
    const int sock_fd = bind_socket(port, &server_config);
    printf("SERVIDOR ACTIVO EN EL PUERTO %d\n", port);

    // Inicializamos en cero los campos del cliente
    // El cliente se 'llena' abajo en el recvfrom()
    struct sockaddr_in client_config = {0};
    socklen_t client_len = sizeof(client_config);

    char *program_name = argv[0]; // almacenamos el nombre del programa

    int timeout_val = 0;
    double e_percent = 1;

    // obteniendo argumentos
    while ((opt = getopt(argc, argv, short_options)) != -1)
    {
        switch(opt)
        {
            case 't':
                timeout_val = atoi(optarg);
                continue;
            case ':':
                printf("Argumento %c no proporcionado\n", optopt);
                usage(stdout, program_name);
                exit(EXIT_FAILURE);
            case 'h':
                usage(stdout, program_name);
                exit(EXIT_SUCCESS);
            case 'e':
                e_percent = strtod(optarg, NULL);
                if (e_percent <= 0)
                {
                    e_percent = 1;
                }
                else if(e_percent > 100)
                {
                    printf("porcentaje no valido \n");

                }
                else{
                    e_percent = 1-(e_percent/100);
                }
                break;
            default :
                printf("Argumento desconocido: %c\n", optopt);
                usage(stdout, program_name);
                exit(EXIT_FAILURE);
        }
    }

    // Ciclo infinito, el servidor siempre debe estar "escuchando"
    char buffer[1024];
    while (1)
    {
        // Recibimos el camino del archivo a ser transferido
        const int reply_len = recv_file_path(sock_fd, buffer, sizeof buffer, &client_config);

        // Seguimos escuchando si no se recibe un mensaje
        if (reply_len > 0)
        {
            if (check_file_exists(buffer))
            {
                send_response(sock_fd, "200", &client_config);
                send_file(sock_fd, buffer, &client_config, timeout_val, e_percent);
            }
            else
            {
                send_response(sock_fd, "404", &client_config);
            }
        }
    }
    close(sock_fd);
    return 0;
}
