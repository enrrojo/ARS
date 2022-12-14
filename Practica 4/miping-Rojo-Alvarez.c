// Practica tema 8, Rojo Alvarez, Enrique

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/ip.h>
#include "ip-icmp-ping.h"
 
int main(int argc, char *argv[]) {

	ECHORequest echoRequest;
	ECHOResponse echoResponse;
	bool informe = false;	// Variable para el parametro -v

	// Control de parametros de entrada
	if (argc > 3 || argc < 2){
	   printf("Numero de parametros invalidos\n");
	   exit(EXIT_FAILURE); 
	}
	
	// Se comprueba si se ha puesto la opcion de -v
	if (argc == 3){
	   if (strcmp(argv[2], "-v") != 0) {
	      printf("Segundo argumento invalido\n");
	      exit(EXIT_FAILURE);
	   } 
	   informe = true;
	}

	// Declaracion de la direccion que ira asociada al socket
	struct sockaddr_in address;
	address.sin_addr.s_addr = INADDR_ANY;  // "Cualquier direccion"
	address.sin_family = AF_INET;

	// Creacion de socket y control de errores
	int socke = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (socke < 0){
	   perror("socket()");
	   exit(EXIT_FAILURE); 
	}

	// Enlace del socket con la direccion "address" y control de errores
	int bindSock = bind(socke, (struct sockaddr *) &address, sizeof(address)); 
	if (bindSock < 0){
	   perror("bind()");
	   close (socke);
	   exit(EXIT_FAILURE); 
	}

	// Conversion de la ip a una direccion de 32 bits en network byte order 
	int validate;
	validate = inet_aton(argv[1], &address.sin_addr);
	if (validate == 0){  // Control de errores
	   printf("Direccion IP invalida\n");
	   close (socke);
	   exit(EXIT_FAILURE); 
	}

	// Construccion del datagrama
	// Comentario de informe por la v flag
	if (informe == 1){
	   printf("-> Generando cabecera ICMP.\n");
	}

	memset(&echoRequest, 0, sizeof(echoRequest));
	echoRequest.icmpHeader.Type = 8;
	// Comentario de informe por la v flag
	if (informe == 1){
	   printf("-> Type: %d\n", echoRequest.icmpHeader.Type);
	}
	echoRequest.icmpHeader.Code = 0;
	// Comentario de informe por la v flag
	if (informe == 1){
	   printf("-> Code: %d\n", echoRequest.icmpHeader.Code);
	}
	echoRequest.ID = getpid();
	// Comentario de informe por la v flag
	if (informe == 1){
	   printf("-> Identifier (pid): %d.\n", echoRequest.ID);
	}
	echoRequest.SeqNumber = 0;
	// Comentario de informe por la v flag
	if (informe == 1){
	   printf("-> Seq. number: %d\n", echoRequest.SeqNumber);
	}
	strcpy(echoRequest.payload, "Este es el payload.");
	// Comentario de informe por la v flag
	if (informe == 1){
	   printf("-> Cadena a enviar: %s\n", echoRequest.payload);
	}

	// Calculo del checksum (primera parte)
	echoRequest.icmpHeader.Checksum = 0;
	int numShorts = sizeof(echoRequest)/2; // Mitad del tamaño del datagrama
	unsigned short int *puntero; // Recorrera los elementos de 16 bits del datagrama ICMP	
	unsigned int acumulador = 0; // Acumulara los resultados parciales
	puntero = (unsigned short int *) &echoRequest; // Puntero al inicio

	int i;
	for (i = 0; i < numShorts; i++){
	    acumulador = acumulador + (unsigned int) *puntero;
	    puntero++;
	}
	// Calculo del checksum (segunda parte)
	// Suma de la parte alta del acumulador con la parte baja	
	acumulador = (acumulador >> 16) + (acumulador & 0x0000ffff);
	acumulador = (acumulador >> 16) + (acumulador & 0x0000ffff);
	// Complemento a 1
	acumulador = ~acumulador;
	// Se guarda el checksum
	echoRequest.icmpHeader.Checksum = (unsigned short int) acumulador;

	// Comentario de informe por la v flag
	if (informe == 1){
	   printf("-> Checksum: 0x%x.\n", echoRequest.icmpHeader.Checksum);
	   printf("-> Tamaño total del paquete ICMP: %ld.\n", sizeof(echoRequest));
	}

	// Envio del paquete
	int send = sendto(socke, &echoRequest, (int) sizeof(echoRequest), 0, (struct sockaddr*) &address, sizeof(address)); 
	if (send < 0){
	   perror("sendto()");
	   close (socke);
	   exit(EXIT_FAILURE); 
	}

	printf("Paquete ICMP enviado a %s\n", argv[1]);

	// Recepcion de la respuesta y control de errores
	socklen_t addrlen = sizeof(address);
	int recv = recvfrom(socke, &echoResponse, sizeof(echoResponse), 0, (struct sockaddr*) &address, &addrlen); 
	if (recv < 0){
	   perror("recvfrom()");
	   close (socke);
	   exit(EXIT_FAILURE);
	}

	// Imprime la ip de la respuesta
	printf("Respuesta recibida desde %s\n", inet_ntoa(address.sin_addr));
	// Imprime el informe con la informacion de la respuesta
	if (informe == 1){
	   printf("-> Tamaño de la respuesta: %d\n", recv);
	   printf("-> Cadena recibida: %s\n", echoResponse.payload);
	   printf("-> Identifier (pid): %d.\n", echoResponse.ID);
	   printf("-> TTL: %d.\n", echoResponse.ipHeader.TTL);
	}

	// Se imprime la descripcion de respuesta correspondiente
	// *Extraidos directamente de wikipedia, ICMP english wikipedia*
	int type = echoResponse.icmpHeader.Type;
	int code = echoResponse.icmpHeader.Code;
	printf("Descripcion de la respuesta: ");
	switch(type){
		case 0:
			if (code == 0){
				printf("respuesta correcta");
			}
			break;
		case 3:
			switch(code){
				case 0:
					printf("destination network unreachable");
					break;
				case 1:
					printf("destination host unreachable");
					break;
				case 2:
					printf("destination protocol unreachable");
					break;
				case 3:
					printf("destination port unreachable");
					break;
				case 4:
					printf("fragmentation required, and DF flag set");
					break;
				case 5:
					printf("source route failed");
					break;
				case 6:
					printf("destination network unknown");
					break;
				case 7:
					printf("destination host unknown");
					break;
				case 8:
					printf("source host isolated");
					break;
				case 9:
					printf("network administratively prohibited");
					break;
				case 10:
					printf("host administratively prohibited");
					break;
				case 11:
					printf("network unreachable for ToS");
					break;
				case 12:
					printf("host unreachable for ToS");
					break;
				case 13:
					printf("communication administratively prohibited");
					break;
				case 14:
					printf("host precedence violation");
					break;
				case 15:
					printf("precedence cutoff in effect");
					break;
			}
			break;
		case 5:
			switch(code){
				case 0:
					printf("redirecting datagram for the network");
					break;
				case 1:
					printf("redirecting datagram for the host");
					break;
				case 2:
					printf("redirecting datagram for the ToS & network");
					break;
				case 3:
					printf("redirecting datagram for the ToS & host");
					break;
			}
			break;
		case 8:
			if (code == 0){
				printf("echo requested (used to ping)");
			}
			break;
		case 9:
			if (code == 0){
				printf("router advertisement");
			}
			break;
		case 10:
			if (code == 0){
				printf("router discovery/selection/solicitation");
			}
			break;
		case 11:
			switch(code){
				case 0:
					printf("TTL expired in transit");
					break;
				case 1:
					printf("fragment reassembly time exceeded");
					break;
			}
			break;
		case 12:
			switch(code){
				case 0:
					printf("pointer indicates the error");
					break;
				case 1:
					printf("missing a required option");
					break;
				case 2:
					printf("bad length");
					break;
			}
			break;
		case 13:
			if (code == 0){
				printf("timestamp");
			}
			break;
		case 14:
			if (code == 0){
				printf("timestamp reply");
			}
			break;
	}

	// Imprime el numero de type y code
	printf("(type %d, code %d).\n", type, code);

	// Se cierra el socket
	close(socke);
	return 0;
}
