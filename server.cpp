#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
// #include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

/* Added */
#include <stdint.h> // intptr_t
#include <cerrno> /* for cpp errors */
#include <iostream> // cerr
#include <vector>
#include <string>


using namespace std;

#include "map.cpp"
#include "process.cpp"

/* portul folosit */
#define PORT 2908

/* codul de eroare returnat de anumite apeluri */
//extern int errno;

static void *treat(void *); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */
void raspunde(void *);

int main ()
{
  createGraph();
  struct sockaddr_in server;	// structura folosita de server
  struct sockaddr_in from;	
  int nr;		//mesajul primit de trimis la client 
  int sd;		//descriptorul de socket 
  int pid;
  pthread_t th[100];    //Identificatorii thread-urilor care se vor crea
	int i=0;
  

  /* crearea unui socket */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      std::cerr << "[server]Eroare la socket(): " << strerror(errno) << std::endl;
      //perror ("[server]Eroare la socket().\n");
      return errno;
    }
  /* utilizarea optiunii SO_REUSEADDR */
  int on=1;
  setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
  
  /* pregatirea structurilor de date */
  bzero (&server, sizeof (server));
  bzero (&from, sizeof (from));
  
  /* umplem structura folosita de server */
  /* stabilirea familiei de socket-uri */
    server.sin_family = AF_INET;	
  /* acceptam orice adresa */
    server.sin_addr.s_addr = htonl (INADDR_ANY);
  /* utilizam un port utilizator */
    server.sin_port = htons (PORT);
  
  /* atasam socketul */
  if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
    {
      std::cerr << "[server]Eroare la bind(): " << strerror(errno) << std::endl;
      //perror ("[server]Eroare la bind().\n");
      return errno;
    }

  /* punem serverul sa asculte daca vin clienti sa se conecteze */
  if (listen (sd, 2) == -1)
    {
      std::cerr << "[server]Eroare la listen(): " << strerror(errno) << std::endl;
      //perror ("[server]Eroare la listen().\n");
      return errno;
    }
  /* servim in mod concurent client// Save the changes to the XML file
    std::ofstream file("users.xml");
    file << doc;
    file.close();ii...folosind thread-uri */
  while (1)
    {
      int client;
      thData * td; //parametru functia executata de thread     
      int length = sizeof (from);

      printf ("[server]Asteptam la portul %d...\n",PORT);
      fflush (stdout);

      // client= malloc(sizeof(int));
      /* acceptam un client (stare blocanta pina la realizarea conexiunii) */
      if ( (client = accept (sd, (struct sockaddr *) &from, (socklen_t*)&length)) < 0)
	    {
        std::cerr << "[server]Eroare la accept(): " << strerror(errno) << std::endl;
	      //perror ("[server]Eroare la accept().\n");
	      continue;
	    }
	
        /* s-a realizat conexiunea, se astepta mesajul */

	td=(struct thData*)malloc(sizeof(struct thData));	
  
  initialiseThread(td, i, client);
	pthread_create(&th[i], NULL, &treat, td);	      	
	}//while    
};



static void *treat(void *arg)
{
    struct thData tdL;
    tdL = *((struct thData *)arg);
    printf("[thread]- %d - Asteptam mesajul...\n", tdL.idThread);
    fflush(stdout);
    pthread_detach(pthread_self());

    //while(!shouldStop[tdL.idThread])
    while(!tdL.userInfo.shouldStop)
    {
        //raspunde((struct thData *)arg);
        raspunde((struct thData *)&tdL);
    }

    // Close the connection
    close((intptr_t)arg);
    return NULL;
}

void raspunde(void *arg)
{
  int i = 0;
  char buf[100];
  //struct thData tdL;
  //tdL = *((struct thData *)arg);
  struct thData *tdL = (struct thData *)arg;

  int bytesRead = read(tdL->cl, &buf, sizeof(buf));

  // Client was closed in unnatural ways - Ctrl+C, crash, etc
  if(bytesRead == 0)
  {
    //closeClient(buf, tdL);
    closeClient(buf, *tdL);
    return;
  }
  else if (bytesRead < 0)
  {
    std::cerr << "[server]Eroare la read() de la client: " << strerror(errno) << std::endl;
    return;
  }

  printf("[Thread %d]Mesajul a fost receptionat...%s\n", tdL->idThread, buf);
  fflush (stdout);
  processCommand(buf, *tdL);
  printf("[Thread %d]Trimitem mesajul inapoi...%s\n", tdL->idThread, buf);

  /* returnam mesajul clientului */
  if (write(tdL->cl, &buf, sizeof(buf)) <= 0)
  {
    printf("[Thread %d] ", tdL->idThread);
    std::cerr << "[Thread]Eroare la write() catre client: " << strerror(errno) << std::endl;
    return;
  }
  else
    printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL->idThread);
}