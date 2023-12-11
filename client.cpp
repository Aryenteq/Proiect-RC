#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
//#include <errno.h>
#include <cerrno>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>

/* Added */
#include <cerrno> /* for cpp errors */
#include <iostream> // cerr
#include <arpa/inet.h>  // IP addres (inet_pton)


/* codul de eroare returnat de anumite apeluri */
//extern int errno;

/* portul de conectare la server*/
int port;

int main (int argc, char *argv[])
{
    int sd;			// descriptorul de socket
    struct sockaddr_in server;	// structura folosita pentru conectare 

    // mesajul trimis
    int nr=0;
    char buf[100];

    /* exista toate argumentele in linia de comanda? */
    if (argc != 3)
    {
        printf ("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
        return -1;
    }

    /* stabilim portul */
    port = atoi (argv[2]);

    /* cream socketul */
    if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
        std::cerr << "[client]Eroare la socket(): " << strerror(errno) << std::endl;
        //perror ("Eroare la socket().\n");
        return errno;
    }

    /* umplem structura folosita pentru realizarea conexiunii cu serverul */
    /* familia socket-ului */
    server.sin_family = AF_INET;
    /* adresa IP a serverului */
    if (inet_pton(AF_INET, argv[1], &server.sin_addr) < 0) {
        perror("[client] Eroare la inet_pton.\n");
        return errno;
    }
    //server.sin_addr.s_addr = inet_addr(argv[1]);
    /* portul de conectare */
    server.sin_port = htons (port);
  
    /* ne conectam la server */
    if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
        std::cerr << "[client]Eroare la connect(): " << strerror(errno) << std::endl;
        //perror ("[client]Eroare la connect().\n");
        return errno;
    }
    while(1)
    {
        /* citirea mesajului */
        printf ("[client]Introduceti o comanda: ");
        fflush (stdout);
        read (0, buf, sizeof(buf));
        
        // Get rid of endl character
        int index=0;
        while(buf[index]!='\n')
            index++;
        buf[index]='\0';
        //printf("[client]Am citit: %s\n", buf);

        /* trimiterea mesajului la server */
        if (write (sd,&buf,sizeof(buf)) <= 0)
        {
            std::cerr << "[client]Eroare la write() spre server: " << strerror(errno) << std::endl;
            //perror ("[client]Eroare la write() spre server.\n");
            return errno;
        }

        /* citirea raspunsului dat de server 
        (apel blocant pina cind serverul raspunde) */
        if (read (sd, &buf,sizeof(buf)) < 0)
        {
            std::cerr << "[client]Eroare la read(): " << strerror(errno) << std::endl;
            //perror ("[client]Eroare la read() de la server.\n");
            return errno;
        }
        /* afisam mesajul primit */
        printf ("[client]Mesajul primit este: %s\n", buf);

        // Close
        if (strcmp(buf, "Closing") == 0)
            break;
    }  
    /* inchidem conexiunea, am terminat */
    close (sd);
}