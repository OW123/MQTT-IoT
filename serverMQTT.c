/*All the headers needed for this program*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

#include "frames.h"

/*The BACKLOG is the maximun number of allowed connections*/
#define BACKLOG 5
#define PORT 1883

int connCount = 0;

int timerCount = 10;

int fd, newfd;

int flagRcv = 1;
int flagPing = 0;

sPing *sPingReq;
sPing sPingRes;

/*Array of thread for each client*/
pthread_t threads[BACKLOG], t_timer, t_rx, t_tx;
pthread_mutex_t lock;

struct sigaction sa;
struct itimerval timer;


/*Declaring the function prototype*/
void *handle_client(void *socket_desc);

void timer_handler(int signum);

void *call_setimer();

void *call_rx();

void *call_tx();


int main(int argc, char **argv) {
   int sockfd;
   struct sockaddr_in host_addr, client_addr;
   socklen_t sin_size;

   if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      perror("Socket failed");
      exit(1);
   }

   host_addr.sin_family = AF_INET;
   host_addr.sin_port = htons(PORT);
   host_addr.sin_addr.s_addr = INADDR_ANY;
   memset(&(host_addr.sin_zero), '\0', 8);

   if(bind(sockfd, (struct sockaddr *)&host_addr, sizeof(struct sockaddr)) == -1) {
      perror("Bind failed");
      exit(1);
   }

   if(listen(sockfd, BACKLOG) == -1) {
      perror("Listen failed");
      exit(1);
   }

   printf("Server ready, listening...\n");


   while(1) {
      sin_size = sizeof(struct sockaddr_in);
      if((newfd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size)) == -1) {
         perror("Accept failed");
         continue;
      }
      connCount++;
      fd = newfd;
      printf("Connection from %s received\n", inet_ntoa(client_addr.sin_addr));

      /*A thread is created with the ID connection*/
      if(pthread_create(&threads[newfd], NULL, handle_client, &newfd) < 0) {
         perror("Thread creation failed");
         continue;
      }
   }

   if(close(sockfd) < 0) {
      perror("Close socket failed\n");
      exit(1);
   }

   return 0;
}


void *handle_client(void *socket_desc) {
    int sock = *(int*)socket_desc;
    int argv_timeHandler[2];
   int read_size;

    /*Frame structs*/
    sConnect connection_frame;
    sConnectedAck connected_ack_frame = connAck_building();

   if(connCount > 5){
      perror("Too many connections");
      connected_ack_frame.reasonCode = 0x01;
      if(send(sock, &connected_ack_frame, sizeof(sConnectedAck), 0) < 0) {
            perror("Send failed conn > 5\n");
        }
      connCount--;
      close(sock);
   }

        if((read_size = recv(sock ,(char *)&connection_frame , sizeof(sConnect) , 0)) == -1){
            perror("recv failed");
        }
        printf("ID: %s\n",connection_frame.clientID);
        printf("Protocol: %X\t msgLen:%X\t lenProtocolName: %X\t protocolName: %s\t protocolVersion: %X\t connectFlag: %X\t lenKeepAlive: %X\t lenClientID: %X\n", connection_frame.msgType, connection_frame.msgLength, connection_frame.lenProtocolName, connection_frame.sProtocolName, connection_frame.protocolVersion, connection_frame.connectFlag, connection_frame.lenKeepAlive, connection_frame.lenClientId);

      //Connection frame validation
         if(connect_validation(connection_frame) != 0){
            connected_ack_frame.reasonCode = 0x02;
            if(send(sock, &connected_ack_frame, sizeof(sConnectedAck), 0) < 0) {
               
               perror("Send failed ack\n");
            }
            connCount--;
            close(sock);
         }else{

            connected_ack_frame.reasonCode = 0x00;
            if(send(sock, &connected_ack_frame, sizeof(sConnectedAck), 0) < 0) {
               perror("Send failed ack success\n");
            }
         }

/*Always receiving messages form the clients on each thread*/
    
      if(read_size == 0) {
         perror("Client disconnected\n");
      } else if(read_size == -1) {
         perror("Receive failed\n");
      }
   pthread_create(&t_timer, NULL, call_setimer, NULL);
   pthread_create(&t_rx, NULL, call_rx, NULL);
   


      pthread_mutex_unlock(&lock);

   if(close(sock) < 0) {
      perror("Close socket failed\n");
   }
   connCount--;

   pthread_exit(NULL);
}



void *call_setimer(){
   //Llamada a KeepAlive
   memset (&sa, 0, sizeof (sa));
	sa.sa_handler = &timer_handler;
	sigaction (SIGVTALRM, &sa, NULL);
	
	/* Configure the timer to expire after 250 msec... */
	timer.it_value.tv_sec = 1;
	
	/* ... and every 250 msec after that. */
	timer.it_interval.tv_sec = 1;

   /* Start a virtual timer. It counts down whenever this process is
	executing. */
	setitimer (ITIMER_VIRTUAL, &timer, NULL);
	
   while(1){}

}

// void *call_tx(){
//    if(flagRcv == 1){
//       if ((recv(fd,(char *)&sPingReq,sizeof(sPing),0)) > 0){
//          printf("Ping received\n");
//          timerCount = 10;
//          flagRcv = 0;
//       }
//    }


// }

void *call_rx(){
   int read_size;
   char *buffer;
   sSubscribe *sSubsPack;
   sPublish *sPublishPack;
   buffer = malloc(1024);

   if(flagRcv == 1){

      if((read_size = recv(fd, buffer, 1024 , 0)) < 0){
            printf("Recv %i\n", read_size);
            perror("recv failed topics");
        }
        
        if(*buffer == 0xC0){
         sPingReq = malloc(sizeof(sPing));
         sPingReq = (sPing *)buffer;
         printf("Ping received\n");
         if(sPingReq->msgType != 0xC0){
            close(fd);
         }else{
            timerCount = 10;
            flagRcv = 0;
            flagPing = 1;
         }
         free(buffer);
         free(sPingReq);
        }

        if(*buffer == PUBLISH_HEADER){
            sPublishPack = (sPublish *)buffer;        
        }

        if(*buffer == SUBSCRIBE_HEADER){
            sSubsPack = (sSubscribe *)buffer;
        }
   /*Always receiving messages form the clients on each thread*/
    
      if(read_size == 0) {
         perror("Client disconnected\n");
      } else if(read_size == -1) {
         perror("Receive failed\n");
      }
   }
}
void timer_handler(int signum)
{
   pthread_mutex_lock(&lock);
	
   printf("timer expired %d times\n", timerCount);

   sPingRes = ping_building();
   sPingRes.msgType = 0xD0; //Request Response = 0xC0
   
   if(timerCount == 5 && flagRcv == 1){
      if(send(fd,(char *)&sPingRes,sizeof(sPing),0) > 0){
         timerCount = 10;

      }else{         
         close(fd);
         perror("No ping sent to client\n");
      }
   }

   printf("File read is %i\n", fd);

   
   if(timerCount == 0){
      timerCount = 11;
   }

    timerCount--;

   pthread_mutex_unlock(&lock);
}