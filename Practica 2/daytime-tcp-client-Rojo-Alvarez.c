// Practica tema 6, Rojo Alvarez, Enrique

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

	char messageRecv[256];

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
	int socketTCP = socket(AF_INET, SOCK_STREAM, 0);
	if (socketTCP < 0){
	   perror("socket()");
	   exit(EXIT_FAILURE); 
	}

	// Enlace del socket con la direccion "address" y control de errores
	int bindTCP = bind(socketTCP, (struct sockaddr *) &address, sizeof(address)); 
	if (bindTCP < 0){
	   perror("bind()");
	   shutdown(socketTCP, SHUT_RDWR);
	   recv(socketTCP, messageRecv, sizeof(messageRecv), 0);
	   close (socketTCP);
	   exit(EXIT_FAILURE); 
	}

	// Conversion de la ip a una direccion de 32 bits en network byte order 
	int validate;
	validate = inet_aton(argv[1], &address.sin_addr);
	if (validate == 0){  // Control de errores
	   printf("Direccion IP invalida\n");
	   shutdown(socketTCP, SHUT_RDWR);
	   recv(socketTCP, messageRecv, sizeof(messageRecv), 0);
	   close (socketTCP);
	   exit(EXIT_FAILURE); 
	}

	// Si no se incluye un puerto como parametro se asigna el de daytime
	if (argc == 2){
	   struct servent *port = getservbyname("daytime", "tcp");
	   address.sin_port = port->s_port;
	}

	// Si se incluye puerto como parametro este se cambia al formato correspondiente y se asigna
	else if (argc == 4){
	   // Comprobacion si se puso la opcion puerto
	   int pRev = strcmp(argv[2], "-p");
	   if (pRev != 0) {
	      printf("Segundo argumento invalido\n");
	      shutdown(socketTCP, SHUT_RDWR);
	      recv(socketTCP, messageRecv, sizeof(messageRecv), 0);
	      close (socketTCP);
	      exit(EXIT_FAILURE); 
	   }
	   uint16_t portNumber;
	   // Se transforma la cadena en un numero de 16 bits
	   if (sscanf(argv[3], "%" SCNd16, &portNumber) == 0){ 
	      printf("Argumento de numero de puerto invalido\n");
	      shutdown(socketTCP, SHUT_RDWR);
	      recv(socketTCP, messageRecv, sizeof(messageRecv), 0);
	      close(socketTCP);
	      exit(EXIT_FAILURE); 
	   }
	   // Se convierte a network byte order y se asigna
	   uint16_t portNBO = htons(portNumber);
	   address.sin_port = portNBO;
	}

	// Conexion del socket con el servidor
	int conexion = connect(socketTCP, (struct sockaddr*) &address, sizeof(address));
	if (conexion < 0) {
	   perror("connect()");
	   shutdown(socketTCP, SHUT_RDWR);
	   recv(socketTCP, messageRecv, sizeof(messageRecv), 0);
	   close(socketTCP);
	}

	// Recibe el mensaje del servidor
	int recvTCP = recv(socketTCP, messageRecv, sizeof(messageRecv), 0);
	if (recvTCP < 0){
	   perror("recv()");
	   shutdown(socketTCP, SHUT_RDWR);
	   recv(socketTCP, messageRecv, sizeof(messageRecv), 0);
	   close(socketTCP);
	   exit(EXIT_FAILURE); 
	}

	// Imprime el mensaje y cierra el socket
	printf("Mensaje recibido: %s", messageRecv);
	shutdown(socketTCP, SHUT_RDWR);
	// Recibe por si quedan datos pendientes
	recvTCP = recv(socketTCP, messageRecv, sizeof(messageRecv), 0);
	if (recvTCP < 0){
	   perror("recv()");
	   shutdown(socketTCP, SHUT_RDWR);
	   close(socketTCP);
	   exit(EXIT_FAILURE); 
	}
	close(socketTCP);
	return 0;
}
