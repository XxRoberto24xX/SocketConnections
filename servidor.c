/*
**Fichero: servidor.c
**Autores:
**Roberto González Navas DNI 70965621H
**Miguel González Herranz DNI 70835278Q
*/


/*
 *          		S E R V I D O R
 *
 *	This is an example program that demonstrates the use of
 *	sockets TCP and UDP as an IPC mechanism.  
 *
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
/* LIBRERIAS AÑADIDAS */
#include <regex.h>

#define PUERTO 5621
#define ADDRNOTFOUND	0xffffffff	/* return address for unfound host */
#define BUFFERSIZE	512	/* maximum size of packets to be received */
#define TAM_BUFFER 512
#define MAXHOST 128
#define TIMEOUT 6

extern int errno;

/*
 *			M A I N
 *
 *	This routine starts the server.  It forks, leaving the child
 *	to do all the work, so it does not have to be run in the
 *	background.  It sets up the sockets.  It
 *	will loop forever, until killed by a signal.
 *
 */
 
void serverTCP(int s, struct sockaddr_in peeraddr_in, FILE* logFile);
void serverUDP(int s, char * buffer, struct sockaddr_in clientaddr_in, FILE* logFile);
void errout(char *);		/* declare error out routine */

int FIN = 0;             /* Para el cierre ordenado */
void finalizar(){ FIN = 1; }

int main(argc, argv)
int argc;
char *argv[];
{

    int s_TCP, s_UDP;		/* connected socket descriptor */
    int ls_TCP;				/* listen socket descriptor */
    
    int cc;				    /* contains the number of bytes read */
     
    struct sigaction sa = {.sa_handler = SIG_IGN}; /* used to ignore SIGCHLD */
    
    struct sockaddr_in myaddr_in;	/* for local socket address */
    struct sockaddr_in clientaddr_in;	/* for peer socket address */
	int addrlen;
	
    fd_set readmask;
    int numfds,s_mayor;
    
    char buffer[BUFFERSIZE];	/* buffer for packets to be read into */
    
    struct sigaction vec;



    /* Variables añadidas */
    int newSocket;
    FILE* logFile;
    int status;
    char hostname[MAXHOST];


	/* Create the listen socket. */
	ls_TCP = socket (AF_INET, SOCK_STREAM, 0);
	if (ls_TCP == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket TCP\n", argv[0]);
		exit(1);
	}
	/* clear out address structures */
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
   	memset ((char *)&clientaddr_in, 0, sizeof(struct sockaddr_in));

    addrlen = sizeof(struct sockaddr_in);

		/* Set up address structure for the listen socket. */
	myaddr_in.sin_family = AF_INET;
		/* The server should listen on the wildcard address,
		 * rather than its own internet address.  This is
		 * generally good practice for servers, because on
		 * systems which are connected to more than one
		 * network at once will be able to have one server
		 * listening on all networks at once.  Even when the
		 * host is connected to only one network, this is good
		 * practice, because it makes the server program more
		 * portable.
		 */
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	myaddr_in.sin_port = htons(PUERTO);

	/* Bind the listen address to the socket. */
	if (bind(ls_TCP, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to bind address TCP\n", argv[0]);
		exit(1);
	}
		/* Initiate the listen on the socket so remote users
		 * can connect.  The listen backlog is set to 5, which
		 * is the largest currently supported.
		 */
	if (listen(ls_TCP, 5) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to listen on socket\n", argv[0]);
		exit(1);
	}
	
	
	/* Create the socket UDP. */
	s_UDP = socket (AF_INET, SOCK_DGRAM, 0);
	if (s_UDP == -1) {
		perror(argv[0]);
		printf("%s: unable to create socket UDP\n", argv[0]);
		exit(1);
	   }
	/* Bind the server's address to the socket. */
	if (bind(s_UDP, (struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		printf("%s: unable to bind address UDP\n", argv[0]);
		exit(1);
	    }

		/* Now, all the initialization of the server is
		 * complete, and any user errors will have already
		 * been detected.  Now we can fork the daemon and
		 * return to the user.  We need to do a setpgrp
		 * so that the daemon will no longer be associated
		 * with the user's control terminal.  This is done
		 * before the fork, so that the child will not be
		 * a process group leader.  Otherwise, if the child
		 * were to open a terminal, it would become associated
		 * with that terminal as its control terminal.  It is
		 * always best for the parent to do the setpgrp.
		 */
	setpgrp();

	switch (fork()) {
	case -1:		/* Unable to fork, for some reason. */
		perror(argv[0]);
		fprintf(stderr, "%s: unable to fork daemon\n", argv[0]);
		exit(1);

	case 0:     /* The child process (daemon) comes here. */

			/* Close stdin and stderr so that they will not
			 * be kept open.  Stdout is assumed to have been
			 * redirected to some logging file, or /dev/null.
			 * From now on, the daemon will not report any
			 * error messages.  This daemon will loop forever,
			 * waiting for connections and forking a child
			 * server to handle each one.
			 */
		fclose(stdin);
		fclose(stderr);

			/* Set SIGCLD to SIG_IGN, in order to prevent
			 * the accumulation of zombies as each child
			 * terminates.  This means the daemon does not
			 * have to make wait calls to clean them up.
			 */
		if ( sigaction(SIGCHLD, &sa, NULL) == -1) {
            perror(" sigaction(SIGCHLD)");
            fprintf(stderr,"%s: unable to register the SIGCHLD signal\n", argv[0]);
            exit(1);
            }
            
		    /* Registrar SIGTERM para la finalizacion ordenada del programa servidor */
        vec.sa_handler = (void *) finalizar;
        vec.sa_flags = 0;
        if ( sigaction(SIGTERM, &vec, (struct sigaction *) 0) == -1) {
            perror(" sigaction(SIGTERM)");
            fprintf(stderr,"%s: unable to register the SIGTERM signal\n", argv[0]);
            exit(1);
        }

        logFile = fopen("peticiones.log", "w");

		while (!FIN) {
            /* Meter en el conjunto de sockets los sockets UDP y TCP */
            FD_ZERO(&readmask);
            FD_SET(ls_TCP, &readmask);
            FD_SET(s_UDP, &readmask);
            /* 
            Seleccionar el descriptor del socket que ha cambiado. Deja una marca en 
            el conjunto de sockets (readmask)
            */ 
    	    if (ls_TCP > s_UDP) s_mayor=ls_TCP;
    		else s_mayor=s_UDP;

            if ( (numfds = select(s_mayor+1, &readmask, (fd_set *)0, (fd_set *)0, NULL)) < 0) {
                if (errno == EINTR) {
                    FIN=1;
		            close (ls_TCP);
		            close (s_UDP);
                    perror("\nFinalizando el servidor. SeÃal recibida en select\n "); 
                }
            } else { 

                /* Comprobamos si el socket seleccionado es el socket TCP */
                if (FD_ISSET(ls_TCP, &readmask)) {
                    /* Note that addrlen is passed as a pointer
                     * so that the accept call can return the
                     * size of the returned address.
                     */
    				/* This call will block until a new
    				 * connection arrives.  Then, it will
    				 * return the address of the connecting
    				 * peer, and a new socket descriptor, s,
    				 * for that connection.
    				 */
    			s_TCP = accept(ls_TCP, (struct sockaddr *) &clientaddr_in, &addrlen);
    			if (s_TCP == -1) exit(1);
    			switch (fork()) {
        			case -1:	/* Can't fork, just exit. */
        				exit(1);
        			case 0:		/* Child process comes here. */
                    	close(ls_TCP); /* Close the listen socket inherited from the daemon. */
                    	status = getnameinfo((struct sockaddr *)&clientaddr_in,sizeof(clientaddr_in), hostname,MAXHOST,NULL,0,0);
                    	fprintf(logFile, "Comunicación Realizada: hostname=%s, IP=%s, TCP, PuertoEfimero=%u\n", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port));
        				serverTCP(s_TCP, clientaddr_in, logFile);
        				fprintf(logFile, "Comunicación Finalizada: hostname=%s, IP=%s, TCP, PuertoEfimero=%u\n", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port));
        				close(s_TCP);
        				exit(0);
        			default:	/* Daemon process comes here. */
        					/* The daemon needs to remember
        					 * to close the new accept socket
        					 * after forking the child.  This
        					 * prevents the daemon from running
        					 * out of file descriptor space.  It
        					 * also means that when the server
        					 * closes the socket, that it will
        					 * allow the socket to be destroyed
        					 * since it will be the last close.
        					 */
        				close(s_TCP);
        			}
            	} /* Fin De TCP*/

	            /* Comprobamos si el socket seleccionado es el socket UDP */
	            if (FD_ISSET(s_UDP, &readmask)) {

	            	/* Recibimos la peticion de conexión */
	                cc = recvfrom(s_UDP, buffer, BUFFERSIZE - 1, 0, (struct sockaddr *)&clientaddr_in, &addrlen);
	                if ( cc == -1) {
	                    perror(argv[0]);
	                    printf("%s: recvfrom error\n", argv[0]);
	                    exit (1);
	                }
	                buffer[cc] = '\0';

	                status = getnameinfo((struct sockaddr *)&clientaddr_in,sizeof(clientaddr_in), hostname,MAXHOST,NULL,0,0);

	                /* Creamos un hijo de manera que el sea el que se encargue de seguir
	                 * con la peticion que acaba de llegar y el padre simplemente continue con la ejecucion del programa
	                 */
	                switch(fork()){
	                case -1:  /* Error en la creacion del hijo */
	                	exit(1);
	                case 0:   /* Codigo del hijo */
						/* El hijo cierra el socket primigeneo y llama a la funcion serverUPD con el socket nuevo creado y una vez acabada la iteracion con el cliente mueree*/
						close(s_UDP);

						/* Creaamos un nuevo socket para atender de forma persoalizada la peticion*/
	                	newSocket = socket(AF_INET, SOCK_DGRAM, 0);
	                	if(newSocket == -1){
	                		perror(argv[0]);
							fprintf(stderr, "%s: unable to create socket TCP\n", argv[0]);
							exit(1);
	                	}

	                	/* Lenamos la estructura y hacemos el bind */
	                	myaddr_in.sin_family = AF_INET;
	                	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	                	myaddr_in.sin_port = htons(0);

	                	if (bind(newSocket, (struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
							perror(argv[0]);
							fprintf(stderr, "%s: unable to bind address TCP\n", argv[0]);
							exit(1);
						}

						fprintf(logFile, "Comunicación Realizada: hostname=%s, IP=%s, UDP, PuertoEfimero=%u\n", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port));
						serverUDP (newSocket, buffer, clientaddr_in, logFile);
						fprintf(logFile, "Comunicación Finalizada: hostname=%s, IP=%s, UDP, PuertoEfimero=%u\n", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port));
						exit(0);

	                case 1:   /* Codigo del padre */
	                	continue;
	                }
	            }/* Fin De UPD*/
          }
		}   /* Fin del bucle infinito de atención a clientes */
        /* Cerramos los sockets UDP y TCP */
        close(ls_TCP);
        close(s_UDP);
        fclose(logFile);
    
        printf("\nFin de programa servidor!\n");
        
	default:		/* Parent process comes here. */
		exit(0);
	}

}

/*
 *				S E R V E R T C P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */
void serverTCP(int s, struct sockaddr_in clientaddr_in, FILE* logFile)
{
	int reqcnt = 0;		/* keeps count of number of requests */
	char buf[TAM_BUFFER];		/* This example uses TAM_BUFFER byte messages. */
	char hostname[MAXHOST];		/* remote host's name string */

	int len, len1, status;
    struct hostent *hp;		/* pointer to host info for remote host */
    long timevar;			/* contains time returned by time() */
    
    struct linger linger;		/* allow a lingering, graceful close; */
    				            /* used when setting SO_LINGER */

	/* VARIABLES AÑADIDAS */
	regex_t regexHelo;
	regex_t regexMailFrom;
	regex_t regexRcpt;
	regex_t regexData;
	regex_t regexPunto;
	regex_t regexQuit;

	int reti;

	int valHelo = 1;
	int valMailFrom = 1;
	int valRcpt = 1;
	int valinfo = 1;
	int valbucle = 1;

	int cont = 0;
	int numrecpt = 0;

	char ok[] = "250 OK";
	char error[] = "500 ERROR de sintaxis";
	char cierre[] = "221 Cerrando el Servicio";

    				
	/* Look up the host information for the remote host
	 * that we have connected with.  Its internet address
	 * was returned by the accept call, in the main
	 * daemon loop above.
	 */
	 
     status = getnameinfo((struct sockaddr *)&clientaddr_in,sizeof(clientaddr_in),
                           hostname,MAXHOST,NULL,0,0);
     if(status){
           	/* The information is unavailable for the remote
			 * host.  Just format its internet address to be
			 * printed out in the logging information.  The
			 * address will be shown in "internet dot format".
			 */
			 /* inet_ntop para interoperatividad con IPv6 */
            if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
            	perror(" inet_ntop \n");
             }
    /* Log a startup message. */
    time (&timevar);
		/* The port number must be converted first to host byte
		 * order before printing.  On most hosts, this is not
		 * necessary, but the ntohs() call is included here so
		 * that this program could easily be ported to a host
		 * that does require it.
		 */
	printf("Startup from %s port %u at %s",
		hostname, ntohs(clientaddr_in.sin_port), (char *) ctime(&timevar));

		/* Set the socket for a lingering, graceful close.
		 * This will cause a final close of this socket to wait until all of the
		 * data sent on it has been received by the remote host.
		 */
	linger.l_onoff  =1;
	linger.l_linger =1;
	if (setsockopt(s, SOL_SOCKET, SO_LINGER, &linger,
					sizeof(linger)) == -1) {
		errout(hostname);
	}

	/* COMIENZO DEL INTERCAMBIO DE INFORMACION */
	/*-----------------------------------------*/

	/* HELO */
	cont = 0;
	do{
		/* Recepcion de datos */
		len = recv(s, buf, TAM_BUFFER, 0);
		if(len == -1){
			//error en la recepcion del mensaje
			errout(hostname);
		}
		while(len < TAM_BUFFER){
			len1 = recv(s, &buf[len], TAM_BUFFER, 0);
			if(len == -1){
				//error en la recepcion del mensaje
				errout(hostname);
			}
			len+=len1;
		}
		fprintf(logFile, "Peticion: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);

		/* Comprobacion de contenido */
		reti = regcomp(&regexHelo, "HELO [a-zA-Z][a-zA-Z]*.[a-zA-Z][a-zA-Z]*", 0);
		if(reti){
			errout(hostname);
		}

		reti = regexec(&regexHelo, buf, 0, NULL, 0);
		if(!reti){
			valHelo = 0;
			strcpy(buf, ok);
			if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER) errout(hostname);
			fprintf(logFile, "Respuesa: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);
		}else if (reti == REG_NOMATCH){
			strcpy(buf, error);
			if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER) errout(hostname);
			fprintf(logFile, "Respuesa: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);
			cont++;
			if(cont >= 3){
				errout(hostname);
			}
		}
	}while(valHelo);

	/* Recepcion de datos */
		len = recv(s, buf, TAM_BUFFER, 0);
		if(len == -1){
			//error en la recepcion del mensaje
			errout(hostname);
		}
		while(len < TAM_BUFFER){
			len1 = recv(s, &buf[len], TAM_BUFFER, 0);
			if(len == -1){
				//error en la recepcion del mensaje
				errout(hostname);
			}
			len+=len1;
		}
		fprintf(logFile, "Peticion: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);

	while(valbucle){

		/* MAIL FROM */
		cont = 0;
		do{
			/* Comprobacion de contenido */
			reti = regcomp(&regexMailFrom, "MAIL FROM: <[a-zA-Z0-9][a-zA-Z0-9]*@[a-zA-Z][a-zA-Z]*.[a-zA-Z][a-zA-Z]*>", 0);
			if(reti){
				errout(hostname);
			}

			reti = regexec(&regexMailFrom, buf, 0, NULL, 0);
			if(!reti){
				valMailFrom = 0;
				strcpy(buf, ok);
				if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER) errout(hostname);
				fprintf(logFile, "Respuesa: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);

			}else if (reti == REG_NOMATCH){
				strcpy(buf, error);
				if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER) errout(hostname);
				fprintf(logFile, "Respuesa: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);
				cont++;
				if(cont >= 3){
					errout(hostname);
				}

				len = recv(s, buf, TAM_BUFFER, 0);
				if(len == -1){
					//error en la recepcion del mensaje
					errout(hostname);
				}
				while(len < TAM_BUFFER){
					len1 = recv(s, &buf[len], TAM_BUFFER, 0);
					if(len == -1){
						//error en la recepcion del mensaje
						errout(hostname);
					}
					len+=len1;
				}
				fprintf(logFile, "Peticion: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);
			}
			
		}while(valMailFrom);

		/* RCPT TO */
		cont = 0;
		numrecpt = 0;
		do{
			/* Recepcion de datos */
			len = recv(s, buf, TAM_BUFFER, 0);
			if(len == -1){
				//error en la recepcion del mensaje
				errout(hostname);
			}
			while(len < TAM_BUFFER){
				len1 = recv(s, &buf[len], TAM_BUFFER, 0);
				if(len == -1){
					//error en la recepcion del mensaje
					errout(hostname);
				}
				len+=len1;
			}
			fprintf(logFile, "Peticion: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);

			/* Comprobacion de contenido */
			reti = regcomp(&regexRcpt, "RCPT TO: <[a-zA-Z0-9][a-zA-Z0-9]*@[a-zA-Z][a-zA-Z]*.[a-zA-Z][a-zA-Z]*", 0);
			if(reti){
				errout(hostname);
			}

			reti = regexec(&regexRcpt, buf, 0, NULL, 0);
			if(!reti){
				numrecpt++;
				strcpy(buf, ok);
				if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER) errout(hostname);
				fprintf(logFile, "Respuesa: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);
			}else if (reti == REG_NOMATCH){

				reti = regcomp(&regexData, "DATA", 0);
				if(reti){
					errout(hostname);
				}

				reti = regexec(&regexData, buf, 0, NULL, 0);
				if(!reti){
					if(numrecpt > 0){
						valRcpt = 0;
						strcpy(buf, ok);
						if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER) errout(hostname);
						fprintf(logFile, "Respuesa: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);
					}else{
						strcpy(buf, error);
						if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER) errout(hostname);
						fprintf(logFile, "Respuesa: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);
						cont++;
						if(cont >= 3){
							errout(hostname);
						}
					}
				}else if(reti == REG_NOMATCH){
					strcpy(buf, error);
					if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER) errout(hostname);
					fprintf(logFile, "Respuesa: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);
					cont++;
					if(cont >= 3){
						errout(hostname);
					}
				}
			}
			
		}while(valRcpt);

		/* Informacion */
		do{
			/* Recepcion de datos */
			len = recv(s, buf, TAM_BUFFER, 0);
			if(len == -1){
				//error en la recepcion del mensaje
				errout(hostname);
			}
			while(len < TAM_BUFFER){
				len1 = recv(s, &buf[len], TAM_BUFFER, 0);
				if(len == -1){
					//error en la recepcion del mensaje
					errout(hostname);
				}
				len+=len1;
			}
			fprintf(logFile, "Peticion: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);

			/* Comprobacion de contenido */
			reti = regcomp(&regexPunto, "^[.]", 0);
			if(reti){
				errout(hostname);
			}

			reti = regexec(&regexPunto, buf, 0, NULL, 0);
			if(!reti){
				valinfo = 0;
				strcpy(buf, ok);
				if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER) errout(hostname);
				fprintf(logFile, "Respuesa: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);
			}else if(reti == REG_NOMATCH){
				if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER) errout(hostname);
				fprintf(logFile, "Respuesa: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);
			}
			
		}while(valinfo);

		/* QUIT */
		len = recv(s, buf, TAM_BUFFER, 0);
		if(len == -1){
			//error en la recepcion del mensaje
			errout(hostname);
		}
		while(len < TAM_BUFFER){
			len1 = recv(s, &buf[len], TAM_BUFFER, 0);
			if(len == -1){
				//error en la recepcion del mensaje
				errout(hostname);
			}
			len+=len1;
		}
		fprintf(logFile, "Peticion: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);

		/* Comprobacion de contenido */

		reti = regcomp(&regexQuit, "QUIT", 0);
		if(reti){
			errout(hostname);
		}

		reti = regexec(&regexQuit, buf, 0, NULL, 0);
		if(!reti){
			valbucle = 0;
			strcpy(buf, cierre);
			if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER) errout(hostname);
			fprintf(logFile, "Respuesa: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);
		}else if(reti == REG_NOMATCH){
			valMailFrom = 1;
			valRcpt = 1;
			valinfo = 1;
		}
	}

	while(len = recv(s, buf, TAM_BUFFER, 0)){
		if(len == -1){
			//error en la recepcion del mensaje
			errout(hostname);
		}
		while(len < TAM_BUFFER){
			len1 = recv(s, &buf[len], TAM_BUFFER, 0);
			if(len == -1){
				//error en la recepcion del mensaje
				errout(hostname);
			}
			len+=len1;
		}
	}
	
	close(s);

		/* Log a finishing message. */
	time (&timevar);
		/* The port number must be converted first to host byte
		 * order before printing.  On most hosts, this is not
		 * necessary, but the ntohs() call is included here so
		 * that this program could easily be ported to a host
		 * that does require it.
		 */
	printf("Completed %s port %u, %d requests, at %s\n",
		hostname, ntohs(clientaddr_in.sin_port), reqcnt, (char *) ctime(&timevar));

}

/*
 *	This routine aborts the child process attending the client.
 */
void errout(char *hostname)
{
	printf("Connection with %s aborted on error\n", hostname);
	exit(1);     
}


/*
 *				S E R V E R U D P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */
void serverUDP(int s, char * buffer, struct sockaddr_in clientaddr_in, FILE* logFile)
{
    struct in_addr reqaddr;	/* for requested host's address */
    struct hostent *hp;		/* pointer to host info for requested host */
    int nc, errcodem, status;

    struct addrinfo hints, *res;

	int addrlen;

	/* Variables añadidas */
	regex_t regexHelo;
	regex_t regexMailFrom;
	regex_t regexRcpt;
	regex_t regexData;
	regex_t regexPunto;
	regex_t regexQuit;

	int reti;
	int len;

	int valHelo = 1;
	int valMailFrom = 1;
	int valRcpt = 1;
	int valinfo = 1;
	int valbucle = 1;

	int cont = 0;
	int numrecpt = 0;

	char ok[] = "250 OK";
	char error[] = "500 ERROR de sintaxis";
	char cierre[] = "221 Cerrando el Servicio";
	char hostname[MAXHOST];

	char buf[BUFFERSIZE];

	addrlen = sizeof(struct sockaddr_in);
	status = getnameinfo((struct sockaddr *)&clientaddr_in,sizeof(clientaddr_in), hostname ,MAXHOST,NULL,0,0);


	/* CONFIRMACION DE LA CONEXION */
	/*-----------------------------*/
   	strcpy(buf, "Empecemos\r\n");
	nc = sendto (s, buf, BUFFERSIZE, 0, (struct sockaddr *) & clientaddr_in, addrlen);
	if ( nc == -1) {
        perror("serverUDP");
        printf("%s: sendto error\n", "serverUDP");
        return;
    }
    fprintf(logFile, "Respuesa: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);


    /* COMIENZO DEL INTERCAMBIO DE INFORMACION */
	/*-----------------------------------------*/
	/* HELO */
	cont = 0;
	do{
		/* Recepcion de datos */
		alarm(TIMEOUT);
        /* Wait for the reply to come in. */
        if (( len = recvfrom (s, buf, BUFFERSIZE, 0, (struct sockaddr *)& clientaddr_in, &addrlen)) == -1) {
            printf("Unable to get response from");
            exit(1);
        } else {
           /* Llega el mensaje y desactivamos la alarma*/
           alarm(0);
        }
        fprintf(logFile, "Peticion: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);

		/* Comprobacion de contenido */
		reti = regcomp(&regexHelo, "HELO [a-zA-Z][a-zA-Z]*.[a-zA-Z][a-zA-Z]*", 0);
		if(reti){
			exit(1);
		}

		reti = regexec(&regexHelo, buf, 0, NULL, 0);
		if(!reti){
			valHelo = 0;
			strcpy(buf, ok);
			nc = sendto (s, buf, BUFFERSIZE, 0, (struct sockaddr *) & clientaddr_in, addrlen);
			if ( nc == -1) {
		        perror("serverUDP");
		        printf("%s: sendto error\n", "serverUDP");
		        return;
		    }
		    fprintf(logFile, "Respuesa: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);
		}else if (reti == REG_NOMATCH){
			strcpy(buf, error);
			nc = sendto (s, buf, BUFFERSIZE, 0, (struct sockaddr *) & clientaddr_in, addrlen);
			if ( nc == -1) {
		        perror("serverUDP");
		        printf("%s: sendto error\n", "serverUDP");
		        return;
		    }
		    fprintf(logFile, "Respuesa: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);
			cont++;
			if(cont >= 3){
				exit(1);
			}
		}
	}while(valHelo);

	/* Recepcion de datos */
		alarm(TIMEOUT);
        /* Wait for the reply to come in. */
        if (( len = recvfrom (s, buf, BUFFERSIZE, 0, (struct sockaddr *)& clientaddr_in, &addrlen)) == -1) {
            printf("Unable to get response from");
           exit(1);
        } else {
           /* Llega el mensaje y desactivamos la alarma*/
           alarm(0);
        }
        fprintf(logFile, "Peticion: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);

	while(valbucle){

		/* MAIL FROM */
		cont = 0;
		do{
			/* Comprobacion de contenido */
			reti = regcomp(&regexMailFrom, "MAIL FROM: <[a-zA-Z0-9][a-zA-Z0-9]*@[a-zA-Z][a-zA-Z]*.[a-zA-Z][a-zA-Z]*>", 0);
			if(reti){
				exit(1);
			}

			reti = regexec(&regexMailFrom, buf, 0, NULL, 0);
			if(!reti){
				valMailFrom = 0;
				strcpy(buf, ok);
				nc = sendto (s, buf, BUFFERSIZE, 0, (struct sockaddr *) & clientaddr_in, addrlen);
				if ( nc == -1) {
			        perror("serverUDP");
			        printf("%s: sendto error\n", "serverUDP");
			        return;
			    }
			    fprintf(logFile, "Respuesa: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);
			}else if (reti == REG_NOMATCH){
				strcpy(buf, error);
				nc = sendto (s, buf, BUFFERSIZE, 0, (struct sockaddr *) & clientaddr_in, addrlen);
				if ( nc == -1) {
			        perror("serverUDP");
			        printf("%s: sendto error\n", "serverUDP");
			        return;
			    }
			    fprintf(logFile, "Respuesa: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);
				cont++;
				if(cont >= 3){
					exit(1);
				}

				alarm(TIMEOUT);
		        /* Wait for the reply to come in. */
		        if (( len = recvfrom (s, buf, BUFFERSIZE, 0, (struct sockaddr *)& clientaddr_in, &addrlen)) == -1) {
		            printf("Unable to get response from");
		           exit(1);
		        } else {
		           /* Llega el mensaje y desactivamos la alarma*/
		           alarm(0);
		        }
		        fprintf(logFile, "Peticion: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);
			}
			
		}while(valMailFrom);

		/* RCPT TO */
		cont = 0;
		numrecpt = 0;
		do{
			/* Recepcion de datos */
			alarm(TIMEOUT);
	        /* Wait for the reply to come in. */
	        if (( len = recvfrom (s, buf, BUFFERSIZE, 0, (struct sockaddr *)& clientaddr_in, &addrlen)) == -1) {
	            printf("Unable to get response from");
	           exit(1);
	        } else {
	           /* Llega el mensaje y desactivamos la alarma*/
	           alarm(0);
	        }
	        fprintf(logFile, "Peticion: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);

			/* Comprobacion de contenido */
			reti = regcomp(&regexRcpt, "RCPT TO: <[a-zA-Z0-9][a-zA-Z0-9]*@[a-zA-Z][a-zA-Z]*.[a-zA-Z][a-zA-Z]*", 0);
			if(reti){
				exit(1);
			}

			reti = regexec(&regexRcpt, buf, 0, NULL, 0);
			if(!reti){
				numrecpt++;
				strcpy(buf, ok);
				nc = sendto (s, buf, BUFFERSIZE, 0, (struct sockaddr *) & clientaddr_in, addrlen);
				if ( nc == -1) {
			        perror("serverUDP");
			        printf("%s: sendto error\n", "serverUDP");
			        return;
			    }
			    fprintf(logFile, "Respuesa: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);
			}else if (reti == REG_NOMATCH){

				reti = regcomp(&regexData, "DATA", 0);
				if(reti){
					exit(1);
				}

				reti = regexec(&regexData, buf, 0, NULL, 0);
				if(!reti){
					if(numrecpt > 0){
						valRcpt = 0;
						strcpy(buf, ok);
						nc = sendto (s, buf, BUFFERSIZE, 0, (struct sockaddr *) & clientaddr_in, addrlen);
						if ( nc == -1) {
					        perror("serverUDP");
					        printf("%s: sendto error\n", "serverUDP");
					        return;
					    }
					    fprintf(logFile, "Respuesa: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);
					}else{
						strcpy(buf, error);
						nc = sendto (s, buf, BUFFERSIZE, 0, (struct sockaddr *) & clientaddr_in, addrlen);
						if ( nc == -1) {
					        perror("serverUDP");
					        printf("%s: sendto error\n", "serverUDP");
					        return;
					    }
					    fprintf(logFile, "Respuesa: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);
						cont++;
						if(cont >= 3){
							exit(1);
						}
					}
					
				}else if(reti == REG_NOMATCH){
					strcpy(buf, error);
					nc = sendto (s, buf, BUFFERSIZE, 0, (struct sockaddr *) & clientaddr_in, addrlen);
					if ( nc == -1) {
				        perror("serverUDP");
				        printf("%s: sendto error\n", "serverUDP");
				        return;
				    }
				    fprintf(logFile, "Respuesa: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);
					cont++;
					if(cont >= 3){
						exit(1);
					}
				}
			}
			
		}while(valRcpt);

		/* Informacion */
		do{
			/* Recepcion de datos */
			alarm(TIMEOUT);
	        /* Wait for the reply to come in. */
	        if (( len = recvfrom (s, buf, BUFFERSIZE, 0, (struct sockaddr *)& clientaddr_in, &addrlen)) == -1) {
	            printf("Unable to get response from");
	           exit(1);
	        } else {
	           /* Llega el mensaje y desactivamos la alarma*/
	           alarm(0);
	        }
	        fprintf(logFile, "Peticion: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);

			/* Comprobacion de contenido */
			reti = regcomp(&regexPunto, "^[.]", 0);
			if(reti){
				exit(1);
			}

			reti = regexec(&regexPunto, buf, 0, NULL, 0);
			if(!reti){
				valinfo = 0;
				strcpy(buf, ok);
				nc = sendto (s, buf, BUFFERSIZE, 0, (struct sockaddr *) & clientaddr_in, addrlen);
				if ( nc == -1) {
			        perror("serverUDP");
			        printf("%s: sendto error\n", "serverUDP");
			        return;
			    }
			    fprintf(logFile, "Respuesa: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);
			}else if(reti == REG_NOMATCH){
				nc = sendto (s, buf, BUFFERSIZE, 0, (struct sockaddr *) & clientaddr_in, addrlen);
				if ( nc == -1) {
			        perror("serverUDP");
			        printf("%s: sendto error\n", "serverUDP");
			        return;
			    }
			    fprintf(logFile, "Respuesa: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);
			}
			
		}while(valinfo);

		/* QUIT */
		alarm(TIMEOUT);
        /* Wait for the reply to come in. */
        if (( len = recvfrom (s, buf, BUFFERSIZE, 0, (struct sockaddr *)& clientaddr_in, &addrlen)) == -1) {
            printf("Unable to get response from");
           exit(1);
        } else {
           /* Llega el mensaje y desactivamos la alarma*/
           alarm(0);
        }
        fprintf(logFile, "Peticion: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);

		/* Comprobacion de contenido */
		reti = regcomp(&regexQuit, "QUIT", 0);
		if(reti){
			exit(1);
		}

		reti = regexec(&regexQuit, buf, 0, NULL, 0);
		if(!reti){
			valbucle = 0;
			strcpy(buf, cierre);
			nc = sendto (s, buf, BUFFERSIZE, 0, (struct sockaddr *) & clientaddr_in, addrlen);
			if ( nc == -1) {
			    perror("serverUDP");
			    printf("%s: sendto error\n", "serverUDP");
			    return;
			}
			fprintf(logFile, "Respuesa: hostname=%s, IP=%s, TCP, PuertoEfimero=%u, Contenido:%s", hostname, inet_ntoa(clientaddr_in.sin_addr), ntohs(clientaddr_in.sin_port), buf);
		}else if(reti == REG_NOMATCH){
			valMailFrom = 1;
			valRcpt = 1;
			valinfo = 1;
		}
	}
 }
