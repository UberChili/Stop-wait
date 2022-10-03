/** Cliente
*/

#include <stdint.h>
#include "helpers.h"

#include <stdbool.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "crc32.h"

/**
 * @brief Recibe un cacho del archivo enviado por el servidor.
 *        Esta llamada se bloquea hasta que los datos sean leídos.
 *
 * @param sock_fd el file descriptor del socket.
 * @param frame El frame (header)  cual escribir.
 * @param server_config La configuración del servidor.
 * @return El número de bytes leído en ok, -1 si error.
 */
static ssize_t recv_file_chunk(const int sock_fd, Frame *const frame, struct sockaddr_in *const server_config)
{
    socklen_t server_size = sizeof *server_config;
    return recvfrom(sock_fd, frame, sizeof *frame, MSG_WAITALL, (struct sockaddr *)server_config, &server_size);
}

/**
 * @brief Escribe un cacho de archivo a un archivo.
 *
 * @param to El archivo hacia cuál escribir.
 * @param buffer El buffer a copiar.
 * @param buffer_size El tamaño del buffer en bytes.
 */
static size_t write_file_chunk(FILE *const to, const char *const buffer, const size_t buffer_size)
{
    return fwrite(buffer, sizeof(char), buffer_size, to);
}

/**
 * Envía un acknowledgement al servidor.
 *
 * @param sock_fd El file descriptor del socket.
 * @param frame El frame a ser enviado.
 * @param server_config La configuración del servidor.
 */
static void send_ack(const int sock_fd, const Frame *const frame, const struct sockaddr_in *const server_config)
{
    sendto(sock_fd, frame, sizeof *frame, 0, (struct sockaddr *)server_config, sizeof *server_config);
}

/**
 * @brief Descarga el archivo por su nombre dado.
 *
 * @param sock_fd El file descriptor del socket.
 * @param filename El path del archivo destino.
 * @param server_config La configuración del servidor.
 */
static void get_file(const int sock_fd, const char *const filename, struct sockaddr_in *const server_config, double p_percent)
{
    // Abrimos el archivo para escribir, sobreescribiendo si existe
    printf("[+] Obteniendo archivo \"%s\"\n", filename);
    FILE *const fp = fopen(filename, "w");

    // Inicializamos en cero ambos frames
    Frame recv_frame = {0};
    Frame send_frame = {0};

    int msg_counter = 0;
    int ack_counter = 0;
    int lost_packets = 0;
    int crc = 0;

    // Cambiamos a 'true' en la últime iteración
    bool done = false;
    do
    {
        // Si recibimos un cacho exitosamente...
        if (recv_file_chunk(sock_fd, &recv_frame, server_config) > 0)
        {
            printf("[+] Mensaje recibido. Seqnum: %d, bytes: %zu\n", recv_frame.seqnum, recv_frame.items);
            msg_counter++;

            if (recv_frame.FCS == (crc32_buffer(buff_size, buff_size % 2, (const unsigned char*)recv_frame.packet.data)))
            {
                if (coin_flip(p_percent) == 0)
                {
                    printf("[-] Paquete descartado.\n");
                    lost_packets++;
                    continue;
                }

                //else
                //{
                    //printf("[+] Mensaje recibido. Seqnum: %d, bytes: %zu\n", recv_frame.seqnum, recv_frame.items);
                    //msg_counter++;

                    // Sólo escribimos el cacho en el archivo en caso de ser nuevo
                    if (recv_frame.seqnum == send_frame.ack)
                    {
                        write_file_chunk(fp, recv_frame.packet.data, recv_frame.items);
                        send_frame.ack = send_frame.ack ? 0 : 1;
                    }

                    // Enviamos último ack (-1) con esta condición
                    if (recv_frame.items < buff_size)
                    {
                        done = true;
                        send_frame.ack = -1;

                        // Imprimimos información sobre el archivo obtenido
                        printf("[+] Obtención de archivo \"%s\" finalizada!\n", filename);
                        printf("Nombre del archivo: %s\n", filename);
                        printf("Tamaño del archivo: %zu bytes.\n", get_file_size(fp));
                        printf("Tamaño del buffer: %d bytes.\n", buff_size);
                        printf("Total de mensajes recibidos (DATA): %d.\n", msg_counter);
                        printf("Mensajes escritos (DATA): %d.\n", msg_counter - lost_packets);
                        printf("Total de mensajes perdidos: %d.\n", lost_packets);
                        printf("Total de confirmaciones enviadas (ACK): %d.\n", ack_counter);
                    }

                    send_ack(sock_fd, &send_frame, server_config);
                    printf("[+] Mensaje enviado. Ack: %d, bytes: %zu\n", send_frame.ack, send_frame.items);
                    ack_counter++;
                //}
            }
        }

    } while (!done);

    close(sock_fd);
    fclose(fp);
}

int main(int argc, char **argv)
{
    srand(time(NULL));

    char message[1024] = {0};
    const char *filename = NULL;
    int port = 0;                  // puerto
    int sockfd = 0;                // socket
    struct sockaddr_in serverAddr; // estructura sockaddr_in ya definida
    socklen_t addr_len = sizeof(serverAddr);

    sockfd = socket(PF_INET, SOCK_DGRAM, 0); // el socket
    memset(&serverAddr, '\0', sizeof(serverAddr));

    // configurando el cliente
    serverAddr.sin_family = AF_INET;                     // familia TCP/IP
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // determine cuales clientes pueden enviar mensajes en es caso es la red local

    char *program_name = argv[0]; // almacenamos el nombre del programa

    double p_percent = 1;

    while ((opt = getopt(argc, argv, short_options)) != -1)
    {
        switch (opt)
        {
        case 'f':
            filename = optarg;
            break;
        case 'p':
            port = atoi(optarg); // Asignamos el puerto
            serverAddr.sin_port = htons(port);
            break;
        case 'l':
            p_percent = strtod(optarg, NULL);
            if (p_percent <= 0)
            {
                p_percent = 1;
            }
            else if(p_percent > 100)
            {
                printf("porcentaje no valido \n");

            }
            else{
                p_percent = 1-(p_percent/100);
            }
            break;
        case ':':
            printf("Argumento %c no proporcionado\n", optopt);
            usage(stdout, program_name);
            exit(EXIT_FAILURE);
        case 'h':
            usage(stdout, program_name);
            exit(EXIT_SUCCESS);
        default:
            printf("Argumento desconocido: %c\n", optopt);
            usage(stdout, program_name);
            exit(EXIT_FAILURE);
        }
    }

    // Revisamos si se proporcionaron los argumentos necesarios
    if (port != 0 && filename != NULL)
    {
        if (check_file_exists(filename))
        {
            printf("File already exists. Aborting.\n");
            exit(EXIT_SUCCESS);
        }
        sendto(sockfd, filename, strlen(filename), 0, (struct sockaddr *)&serverAddr, addr_len);
    }
    else
    {
        printf("[-] Not enough arguments.\n");
        usage(stdout, program_name);
        exit(EXIT_FAILURE);
    }

    int reply_len = 0;
    char reply[1024];
    reply_len = recvfrom(sockfd, reply, 1024, MSG_WAITALL, (struct sockaddr *)&serverAddr, &addr_len);
    reply[reply_len] = '\0';
    printf("%s\n", reply);

    if (strcmp(reply, "200") == 0)
    {
        get_file(sockfd, filename, &serverAddr, p_percent);

        exit(EXIT_SUCCESS);
    }

    return 0;
}
