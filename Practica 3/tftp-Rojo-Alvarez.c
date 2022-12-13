// Practica tema 7, Rojo Alvarez, Enrique

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
 
int main(int argc, char *argv[]) {

	bool informe = false;	// Variable para el parametro -v
	int opCode;	// Almacenara el codigo de operacion
	FILE *file;	// Leera o escribira en fichero

	// Control de parametros de entrada
	if (argc > 5 || argc < 4){
	   printf("Numero de parametros invalidos\n");
	   exit(EXIT_FAILURE); 
	}
	
	// Comprobacion de parametro lectura/escritura
	if (strcmp(argv[2], "-r") == 0){
	   opCode = 1;
	} else if (strcmp(argv[2], "-w") == 0){
	   opCode = 2;
	} else {
	   printf("Tercer argumento invalido\n");
	   exit(EXIT_FAILURE);
	}
	
	// Comprobacion del numero de caracteres del nombre del archivo
	if (strlen(argv[3]) > 100){
	   printf("Excedido el numero de caracteres maximos del nombre del archivo.\n");
	   exit(EXIT_FAILURE);
	} 

	// Se comprueba si se ha puesto la opcion de -v
	if (argc == 5){
	   if (strcmp(argv[4], "-v") != 0) {
	      printf("Quinto argumento invalido\n");
	      exit(EXIT_FAILURE);
	   } 
	   informe = true;
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

	// Se asigna el puerto correspondiente a tftp
	struct servent *port = getservbyname("tftp", "udp");
	address.sin_port = port->s_port;

	// Construccion del datagrama
	char request[516];	// 512 bytes de carga util y 4 de opcode y EOS

	memset(request, 0, 516);
	memcpy(request+1, &opCode, 1);
	strcpy(request+2, argv[3]);
	strcpy(request+3+strlen(argv[3]), "octet");

	// Envio de la solicitud de lectura/escritura al servidor
	int send = sendto(socketUDP, request, sizeof(request), 0, (struct sockaddr*) &address, sizeof(address)); 
	if (send < 0){
	   perror("sendto()");
	   close (socketUDP);
	   exit(EXIT_FAILURE); 
	}

	// Elaboracion del informe
	if (informe && opCode == 1){
	   printf("Enviada solicitud de lectura de %s a servidor tftp en %s.\n", argv[3], argv[1]);
	} else if (informe && opCode == 2){
	   printf("Enviada solicitud de escritura de %s a servidor tftp en %s.\n", argv[3], argv[1]);
	}

	// La solicitud es de lectura
	if (opCode == 1){
	   // Apertura del fichero
	   file = fopen(argv[3], "w");
	   if (file == NULL){
	      perror("sendto()");
	      close (socketUDP);
	      exit(EXIT_FAILURE); 
	   }

	   // Parametros para recibir los mensajes y envio de ACK
	   char messageRecv[516];	//512 de carga util, resto de opcode y numero de bloque
	   memset(messageRecv, 0, 516);
	   int tam = 516;	// Variable para controlar el tamaño de los mensajes recibidos
	   socklen_t addrlen = sizeof(address);
	   char ack[4];
	   int currentNumber = 1;	// Variable para controlar el numero de bloque que se esperaba
	   int blockType, blockNumber;

	   // Al recibir un paquete con menos carga util que 512, significara que era el ultimo
	   while (tam >= 516){
		 // Recibe mensaje del servidor con el paquete de datos
	      	 int recv = recvfrom(socketUDP, &messageRecv, tam, 0, (struct sockaddr*) &address, &addrlen);
	     	 if (recv < 0){
	   	     perror("recvfrom()");
	   	     close (socketUDP);
	   	     exit(EXIT_FAILURE); 
	      	 }
		 if (informe){
		    printf("Recibido bloque del servidor tftp.\n");
		 }

		 // Se actualiza el tamaño del paquete recibido
		 tam = recv;
		 // Salida de informe

		 blockType = (unsigned char) messageRecv[0]*256 + (unsigned char)messageRecv[1];
		 blockNumber = (unsigned char) messageRecv[2]*256 + (unsigned char)messageRecv[3];
		 if (blockType == 3){
		    if (informe){
	   	       if (blockNumber == 1){
	       	          printf("Es el primer bloque (numero de bloque 1).\n");
	   	       } else {
	       	          printf("Es el bloque con codigo %d.\n", blockNumber);
	 	       }
		    } 
		 }

		 // Recibido paquete de datos
		 if (blockType == 3 && currentNumber == blockNumber){
	   	     ack[0] = '\0';
	   	     ack[1] = '\4';
	   	     ack[2] = messageRecv[2];
	   	     ack[3] = messageRecv[3];

	  	     if (informe) {
	      	    	printf("Enviamos el ACK del bloque %d.\n", blockNumber);
	   	     }
		     // Envio del ACK
	   	     int send = sendto(socketUDP, &ack, 4, 0, (struct sockaddr*) &address, sizeof(address));
	   	     if (send < 0){
	       	    	perror("sendto()");
	       	    	close (socketUDP);
	      	    	exit(EXIT_FAILURE); 
	   	     }
		     // Escribe los datos recibidos en el fichero
		     fwrite(messageRecv+4, 1, tam-4, file); 
		     currentNumber++;	// Incrementa el contador con el numero del proximo paquete
		 }
		 // Recibido paquete de error
		 else if (blockType == 5){
	   		 printf("Ha ocurrido un error de tipo %d.\n", blockNumber);
	       	    	 close (socketUDP);
	      	    	 exit(EXIT_FAILURE); 
		 // Se recibe un tipo de paquete inesperado
		 } else{
	   	   printf("Ha ocurrido un error al recibir el paquete");
	       	   close (socketUDP);
	      	   exit(EXIT_FAILURE); 
		 }
	   }
	   if (informe){
	      printf("El bloque %d era el ultimo: cerramos el fichero.\n", blockNumber);
	   }

	// La solicitud enviada fue de escritura
	} else if (opCode == 2){
	   // Parametros para recibir los ACK y enviar datos 
	   char dataSend[516];	//512 de carga util, resto de opcode y numero de bloque
	   memset(dataSend, 0, 516);
	   int tam = 516;	// Variable para controlar el tamaño de los mensajes recibidos
	   socklen_t addrlen = sizeof(address);
	   char ack[4];
	   int currentNumber = 0;	// Variable para controlar el numero de bloque que se esperaba
	   int ackType, ackNumber;

	   // Apertura del fichero
	   file = fopen(argv[3], "r");
	   if (file == NULL){
	      perror("sendto()");
	      close (socketUDP);
	      exit(EXIT_FAILURE); 
	   }

	   // Al recibir un paquete con menos carga util que 512, significara que era el ultimo
	   while (tam >= 512){
		 // Recibe mensaje del servidor con el ACK
	      	 int recv = recvfrom(socketUDP, &ack, 4, 0, (struct sockaddr*) &address, &addrlen);
	     	 if (recv < 0){
	   	     perror("recvfrom()");
	   	     close (socketUDP);
	   	     exit(EXIT_FAILURE); 
	      	 }
		 if (informe){
		    printf("Recibido bloque del servidor tftp.\n");
		 }
		 ackType = (unsigned char) ack[0]*256 + (unsigned char) ack[1];
		 ackNumber = (unsigned char) ack[2]*256 + (unsigned char) ack[3];
		 if (ackType == 4 && currentNumber == ackNumber){
		    if (informe && ackNumber == 0){
		       printf("Es el primer ACK (numero de ACK 0).\n");
		    } else {
		       printf("Es el ACK con codigo %d.\n", ackNumber);
		    }
		 }
		 else if (ackType == 5){
	   	       printf("Ha ocurrido un error de tipo %d.\n", ackNumber);
	       	       close (socketUDP);
	      	       exit(EXIT_FAILURE); 
		 } else {
	   	       printf("Ha ocurrido un error inesperado.\n");
	       	       close (socketUDP);
	      	       exit(EXIT_FAILURE); 
		 }
		 // Preparacion del datagrama con los datos del fichero
		 dataSend[0] = 0;
		 dataSend[1] = 3;
		 currentNumber++;
		 dataSend[2] = currentNumber/256;	// Se pasa de entero a achar
		 dataSend[3] = currentNumber%256;
		 tam = fread(dataSend+4, 1, 512, file);

	  	     if (informe) {
	      	    	printf("Enviamos el bloque %d.\n", currentNumber);
	   	     }
		     // Envio del bloque 
	   	     int send = sendto(socketUDP, &dataSend, tam+4, 0, (struct sockaddr*) &address, sizeof(address));
	   	     if (send < 0){
	       	    	perror("sendto()");
	       	    	close (socketUDP);
	      	    	exit(EXIT_FAILURE); 
	   	     }
	   }
	   // Recibe el ultimo mensaje del servidor con el ACK
	   int recv = recvfrom(socketUDP, &ack, 4, 0, (struct sockaddr*) &address, &addrlen);
	   if (recv < 0){
	      perror("recvfrom()");
	      close (socketUDP);
	      exit(EXIT_FAILURE); 
	   }
	   if (informe){
	      printf("Recibido bloque del servidor tftp.\n");
	   }
	   ackType = (unsigned char) ack[0]*256 + (unsigned char) ack[1];
	   ackNumber = (unsigned char) ack[2]*256 + (unsigned char) ack[3];
	   if (ackType == 4 && currentNumber == ackNumber){
	         printf("Es el ultimo ACK con codigo %d.\n", ackNumber);
	   }
	   else if (ackType == 5){
	   	 printf("Ha ocurrido un error de tipo %d.\n", ackNumber);
	       	 close(socketUDP);
	      	 exit(EXIT_FAILURE); 
	   } else {
	         printf("Ha ocurrido un error inesperado.\n");
	       	 close(socketUDP);
	      	 exit(EXIT_FAILURE); 
	   }
	}

	   // Se cierra fichero
	   int closeFile = fclose(file);
	   if (closeFile == EOF){
	      perror("fclose()");
	      close (socketUDP);
	      exit(EXIT_FAILURE); 
	   }
	   // Se cierra el socket
	   close(socketUDP);
	   return 0;
}
