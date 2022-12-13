// Practica tema 5, Rojo Alvarez, Enrique

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/ip.h>
 
int main(int argc, char *argv[]) {

	// Control de parametros de entrada
	if (argc > 4 || argc == 1 || argc == 3){
	   printf("Numero de parametros invalidos\n");
	   exit(EXIT_FAILURE); 
	}
	
	// Declaracion de la direccion que ira asociada al socket
	struct sockaddr_in address;
	address.sin_addr.s_addr = INADDR_ANY;  // "Cualquier direccion"
	address.sin_family = AF_INET;

	// Creacion de socket y control de errores
	int socketUDP = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketUDP < 0){
	   perror("socket()");
	   exit(EXIT_FAILURE); 
	}

	// Enlace del socket con la direccion "address" y control de errores
	int bindUDP = bind(socketUDP, (struct sockaddr *) &address, sizeof(address)); 
	if (bindUDP < 0){
	   perror("bind()");
	   exit(EXIT_FAILURE); 
	}

	// Conversion de la ip a una direccion de 32 bits en network byte order 
	int validate;
	validate = inet_aton(argv[1], &address.sin_addr);
	if (validate == 0){  // Control de errores
	   printf("Direccion IP invalida\n");
	   close (socketUDP);
	   exit(EXIT_FAILURE); 
	}

	// Si no se incluye un puerto como parametro se asigna el de daytime
	if (argc == 2){
	   struct servent *port = getservbyname("daytime", "udp");
	   address.sin_port = port->s_port;
	}

	// Si se incluye puerto como parametro este se cambia al formato correspondiente y se asigna
	else if (argc == 4){
	   // Comprobacion si se puso la opcion puerto
	   int pRev = strcmp(argv[2], "-p");
	   if (pRev != 0) {
	      printf("Segundo argumento invalido\n");
	      close (socketUDP);
	      exit(EXIT_FAILURE); 
	   }
	   uint16_t portNumber;
	   // Se transforma la cadena en un numero de 16 bits
	   if (sscanf(argv[3], "%" SCNd16, &portNumber) == 0){ 
	      printf("Argumento de numero de puerto invalido\n");
	      close (socketUDP);
	      exit(EXIT_FAILURE); 
	   }
	   // Se convierte a network byte order y se asigna
	   uint16_t portNBO = htons(portNumber);
	   address.sin_port = portNBO;
	}

	// Envio del mensaje al servidor con la nueva direccion address correpondiente
	char* message = "Mensaje udp";
	int send = sendto(socketUDP, message, sizeof(message), 0, (struct sockaddr*) &address, sizeof(address)); 
	if (send < 0){
	   perror("sendto()");
	   close (socketUDP);
	   exit(EXIT_FAILURE); 
	}

	// Recibe el mensaje del servidor
	char messageRecv[256];
	socklen_t addrlen = sizeof(address);
	int recv = recvfrom(socketUDP, messageRecv, sizeof(messageRecv), 0, (struct sockaddr*) &address, &addrlen);
	if (recv < 0){
	   perror("recvfrom()");
	   close (socketUDP);
	   exit(EXIT_FAILURE); 
	}

	// Imprime el mensaje y cierra el socket
	printf("Mensaje recibido: %s", messageRecv);
	close(socketUDP);
	return 0;
}
