// Practica tema 6, Rojo Alvarez Enrique

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/ip.h>

// Variables globales
int socketTCP; 

// Funciones definidas
void signal_handler(int sig);

int main(int argc, char *argv[]) {
	
	signal(SIGINT, signal_handler);
	char messageRecv[256];  // Variable que contendra el mensaje recivido 
	#define SIZEBUFF 40
	char buff[SIZEBUFF];  // Variable para guardar la cadena daytime
	FILE *fich;	      // Se usara un fichero para obtener el daytime
	// Dos direcciones una para el la ubicacion de servidor y otra para el cliente
	struct sockaddr_in address;
	struct sockaddr_in clientAddress;
	socklen_t clientAddressSize;

	// Control de los parametros de entrada
	if (argc > 3 || argc == 2){
	   printf("Numero de parametros invalidos");
	   exit(EXIT_FAILURE); 
	}

	// Si no se indica puerto se asigna el correspondiente a daytime
	if (argc == 1){
	   struct servent *port = getservbyname("daytime", "tcp");
	   address.sin_port = port->s_port;
	}

	// Si se indica puerto este sera el asignado al socket
	else if (argc == 3){
	   // Comprobacion de la opcion de puerto
	   int pRev = strcmp(argv[1], "-p");
	   if (pRev != 0) {
	      printf("Segundo argumento invalido");
	      exit(EXIT_FAILURE); 
	   }
	   // Se convierte la cadena del puerto en un numero de 16 bits	
	   uint16_t portNumber;
	   if (sscanf(argv[2], "%" SCNd16, &portNumber) == 0){ 
	      printf("Argumento de numero de puerto invalido");
	      exit(EXIT_FAILURE); 
	   }
	   // Se convierte el numero en network byte order
	   uint16_t portNBO = htons(portNumber);
	   address.sin_port = portNBO;  // Se asigna a la direccion del server
	}

	// Declaracion del socket y control de errores
	socketTCP = socket(AF_INET, SOCK_STREAM, 0);
	if (socketTCP == -1){
	   perror("socket()");
	   exit(EXIT_FAILURE); 
	}

	// Se asignan los parametros correspondientes de la direccion de server
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	// Enlace de la direccion servidor con el socket
	int bindTCP = bind(socketTCP, (struct sockaddr *) &address, sizeof(address)); 
	if (bindTCP < 0){
	   perror("bind()");
	   recv(socketTCP, messageRecv, sizeof(messageRecv), 0);
	   shutdown(socketTCP, SHUT_RDWR);
	   close(socketTCP);
	   exit(EXIT_FAILURE); 
	}

	// Servidor puesto para recibir conexiones
	int listenTCP = listen(socketTCP, 10);
	if (listenTCP < 0){
	   perror("listen()");
	   recv(socketTCP, messageRecv, sizeof(messageRecv), 0);
	   shutdown(socketTCP, SHUT_RDWR);
	   close(socketTCP);
	   exit(EXIT_FAILURE); 
	}

	char messageA [50];  // Variable que contendra el mensaje de respuesta
	char complementoA[5] = ": ";  // Complemento para el mensaje respuesta
	int newSocket;

	// Bucle infinito, donde espera una respuesta del cliente constantemente, en caso de recibirla enviara un mensaje
	while (1){

	   // Espera la conexion de un cliente
	   newSocket = accept(socketTCP, (struct sockaddr*) &clientAddress, &clientAddressSize);
	   if (newSocket < 0){
	      recv(newSocket, messageRecv, sizeof(messageRecv), 0);
	      shutdown(socketTCP, SHUT_RDWR);
	      close(socketTCP);
	      exit(EXIT_FAILURE); 
	   }
	   // Si recibe una conexion crea un hijo y este envia el mensaje 
	   int child;
	   child = fork();
	   // Proceso del hijo
	   if (child == 0){
	      // Se ponen las variables como vacias
	      memset(messageA, '\0', sizeof(messageA));
	      memset(buff, '\0', sizeof(buff));
	      memset(messageRecv, '\0', sizeof(messageRecv));  // Mensaje respuesta vacio
	      // Se anade el nombre del servidor al mensaje
	      gethostname(messageA, 30);
	      // Concatenacion del nombre del servidor y el complemento
	      strcat(messageA, complementoA);
	      // Llamada a sistema date, se escribira e el fichero descrito en /tmp, se recuperara el contenido del fichero y se añade en la variable buff
	      system("date > /tmp/responseUDP.txt");
	      fich = fopen("/tmp/responseUDP.txt", "r");
	      if (fgets(buff,SIZEBUFF, fich) == NULL) {
		 printf("Error en system(), en fopen() o fgets()\n");
		 exit(EXIT_FAILURE);
	      }

	      // Concatenacion del mensaje A con buff
	      strcat(messageA, buff);
	      // Envio del mensaje
	      int sendTCP = send(newSocket, messageA, sizeof(messageA), 0);
	      if (sendTCP < 0){
	      	 perror("send()");
		 recv(newSocket, messageRecv, sizeof(messageRecv), 0);
	    	 shutdown(newSocket, SHUT_RDWR);
	 	 close(newSocket);
	   	 exit(0); 
	      } 

	      // Recibe por si hay datos pendientes y cierra conexion
	      recv(newSocket, messageRecv, sizeof(messageRecv), 0);
	      shutdown(newSocket, SHUT_RDWR);
	      close(newSocket);
	      exit(0);
	   }
        }

	return 0;
}

void signal_handler(int sig){
	if (sig == SIGINT){
	   char messageRecvClose[256]; 
	   // Recibe por si hay datos pendientes
	   recv(socketTCP, messageRecvClose, sizeof(messageRecvClose), 0);
	   shutdown(socketTCP, SHUT_RDWR);
	   close(socketTCP);
	   exit(EXIT_SUCCESS);
	}
}
