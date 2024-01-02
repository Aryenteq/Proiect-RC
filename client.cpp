#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cerrno>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <cerrno>
#include <iostream>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>

#include <vector>
#include "map.cpp"
#include "utilities.cpp"

int port;
int sd;
std::string constantMessage("speed");

std::atomic<bool> stopThread(false);
std::condition_variable cv;
std::mutex cvMutex;

void ParallelMessage()
{
    char buf[100];
    while (!stopThread)
    {
        strcpy(buf, constantMessage.c_str());
        if (write(sd, &buf, sizeof(buf)) <= 0)
        {
            std::cerr << "[client]Eroare la write() spre server: " << strerror(errno) << std::endl;
            return;
        }

        // Black magic to sleep for 60'' or until notified
        {
            std::unique_lock<std::mutex> lock(cvMutex);
            cv.wait_for(lock, std::chrono::seconds(60), [&] { return stopThread.load(); });
        }
    }
}

void StopThread()
{
    {
        std::lock_guard<std::mutex> lock(cvMutex);
        stopThread = true;
    }
    cv.notify_one(); // Notify the thread to wake up immediately
}

int main(int argc, char *argv[])
{
    createGraph();
    struct sockaddr_in server;
    int nr = 0;
    char buf[1024];

    if (argc != 3) 
    {
        printf("Syntax: %s <server address> <port>\n", argv[0]);
        return -1;
    }

    port = atoi(argv[2]);

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        std::cerr << "[client]Eroare la socket(): " << strerror(errno) << std::endl;
        return errno;
    }

    server.sin_family = AF_INET;
    if (inet_pton(AF_INET, argv[1], &server.sin_addr) < 0)
    {
        perror("[client] Eroare la inet_pton.\n");
        return errno;
    }
    server.sin_port = htons(port);

    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        std::cerr << "[client]Eroare la connect(): " << strerror(errno) << std::endl;
        return errno;
    }

    // The requirment to send updates of speed every 60''
    std::thread sendThread(ParallelMessage);

    while (1) 
    {
        fd_set readfds, writefds;
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);

        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sd, &readfds);

        if (select(sd + 1, &readfds, &writefds, NULL, NULL) == -1) 
        {
            std::cerr << "[client]Eroare la select(): " << strerror(errno) << std::endl;
            return errno;
        }

        // To read from the client input & send the message
        if (FD_ISSET(STDIN_FILENO, &readfds))
        {
            read(0, buf, sizeof(buf));
            int index = 0;
            while (buf[index] != '\n')
                index++;
            buf[index] = '\0';

            if (write(sd, &buf, sizeof(buf)) <= 0)
            {
                std::cerr << "[client]Eroare la write() spre server: " << strerror(errno) << std::endl;
                return errno;
            }
        }

        // Broadcasted messages + client requested messages
        if (FD_ISSET(sd, &readfds)) 
        {
            ssize_t bytesRead = read(sd, &buf, sizeof(buf));
            if (bytesRead < 0)
            {
                std::cerr << "[client]Eroare la read(): " << strerror(errno) << std::endl;
                return errno;
            } else if (bytesRead == 0)
            {
                std::cout << "[client]Server has closed the connection." << std::endl;
                StopThread();
                break;
            }

            printf("\n[client]Mesajul primit este: %s\n", buf);

            if (strcmp(buf, "Closing") == 0)
            {
                StopThread();
                break;
            }   
        }
    }

    // W8 for thread to finish
    sendThread.join();

    close(sd);
    return 0;
}
