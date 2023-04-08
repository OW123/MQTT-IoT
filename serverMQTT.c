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


sPing sPingReq;
sPing sPingRes;

fd_set read_FDs;

s_ClientFD client_controller[BACKLOG];

int numConecctions = 0;

/*Array of thread for each client*/
pthread_t t_rx, t_tx;
pthread_mutex_t lock;


/*Declaring the function prototype*/
void *handle_client(void *socket_desc);

void timer_handler(int signum);

void *call_setimer();

void *call_rx();

void *call_tx();


int main(int argc, char **argv) {
   int sockfd, newfd;
   struct sockaddr_in host_addr, client_addr;
   socklen_t sin_size;

   pthread_t t_handlerClients, t_timerHandler;

   sConnect connection_frame;
   sConnectedAck connected_ack_frame = connAck_building();

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
   sin_size = sizeof(struct sockaddr_in);

   if(pthread_create(&t_handlerClients, NULL, handle_client, NULL) < 0){
      perror("pthread_create failed in handle_client");
      exit(-1);
   }

   if(pthread_create(&t_timerHandler, NULL, call_setimer, NULL) < 0){
      perror("pthread_create failed in set_timer");
      exit(-1);
   }


   FD_ZERO(&read_FDs);
   FD_SET(sockfd, &read_FDs);

   numConecctions = sockfd;

   while(1) {
      if((newfd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size)) < 0) {
         perror("Accept failed");
         continue;
      }else{
         printf("Connection from %s received\n", inet_ntoa(client_addr.sin_addr));

         if((recv(newfd ,(char *)&connection_frame , sizeof(sConnect) , 0)) == -1){
               perror("recv failed");
         }
        printf("ID: %s\n",connection_frame.clientID);
        printf("Protocol: %X\t msgLen:%X\t lenProtocolName: %X\t protocolName: %s\t protocolVersion: %X\t connectFlag: %X\t lenKeepAlive: %X\t lenClientID: %X\n", connection_frame.msgType, connection_frame.msgLength, connection_frame.lenProtocolName, connection_frame.sProtocolName, connection_frame.protocolVersion, connection_frame.connectFlag, connection_frame.lenKeepAlive, connection_frame.lenClientId);
         if(connect_validation(connection_frame) != 0){
            connected_ack_frame.reasonCode = 0x02;
            if(send(newfd, &connected_ack_frame, sizeof(sConnectedAck), 0) < 0) {
               
               perror("Send failed ack\n");
            }
            close(newfd);
         }else{
            connected_ack_frame.reasonCode = 0x00;

            for(int i = 0; i < BACKLOG; i++) {
               if(client_controller[i].fd == 0){
                  client_controller[i].fd = newfd;
                  strcpy(client_controller[i].clientID, connection_frame.clientID);
                  client_controller[i].i_KeepALive = connection_frame.lenKeepAlive;
                  client_controller[i].i_CheckAlive = connection_frame.lenKeepAlive;
                  client_controller[i].topic1 = false;
                  client_controller[i].topic2 = false;
                  client_controller[i].topic3 = false;
                  FD_SET(client_controller[i].fd, &read_FDs);
                  if(client_controller[i].fd > numConecctions){
                     numConecctions = client_controller[i].fd;
                  }

                  if(client_controller[BACKLOG - 1].fd != 0){
                     printf("Clients Limit reached\n\n");
                     connected_ack_frame.reasonCode = 0x01;
                     if(send(newfd, &connected_ack_frame, sizeof(sConnectedAck), 0) < 0) {
                           perror("Send failed conn > 5\n");
                     }

                     break;
                  }
                  if(send(newfd, &connected_ack_frame, sizeof(sConnectedAck), 0) < 0) {
                     perror("Send failed ack success\n");
                  }
                  break;
               }//Close main if of the For loop
            }
         }



      }//Close Else Accept
      
   }

   if(close(sockfd) < 0) {
      perror("Close socket failed\n");
      exit(1);
   }

   return 0;
}//Close Main function

void *handle_client(void *socket_desc) {
   int read_size;

   sPingRes = ping_building();
   sPingRes.msgType = 0xD0; //Request Response = 0xC0
   
   if((read_size = select(numConecctions + 1, &read_FDs, NULL, NULL, NULL) < 0) == -1){
      perror("Select FD failed\n");
   }

   while(1){
      for(int i = 0; i < BACKLOG; i++){
         if(FD_ISSET(client_controller[i].fd, &read_FDs)){
            if((read_size = recv(client_controller[i].fd, (char *)&sPingReq, sizeof(sPing) , 0)) < 0){
               printf("Recv %i\n", read_size);
               perror("recv failed topics");
               continue;
            }
            printf("Ping req received.\n Type: %X\t Lenght: %X\n", sPingReq.msgType, sPingReq.msgLength);

            client_controller[i].i_KeepALive = client_controller[i].i_CheckAlive;

            if(send(client_controller[i].fd,(char *)&sPingRes,sizeof(sPing),0) < 0){
               perror("Send failed in keepalive\n");
            }
         }
      }
   }

   if(close(client_controller[0].fd) < 0){
      perror("Close failed in handle_client\n");
   }
   pthread_exit(NULL);
}//Close Handle Client

void timer_handler(int signum)
{
   for(int i = 0; i < BACKLOG; i++){
      if(client_controller[i].fd != 0){
         client_controller[i].i_KeepALive--;
         
         printf("The client %d is still alive.\t Remaining time: %X\n\n", client_controller[i].fd, client_controller[i].i_KeepALive);

         if(client_controller[i].i_KeepALive == 0){
            close(client_controller[i].fd);
            client_controller[i].fd = 0;
         }
      }
   }
}//Close Timer Handler

void *call_setimer(){
   struct sigaction sa;
   struct itimerval timer;
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
}//Close Timer Setup 

// void *call_tx(){
//    if(flagRcv == 1){
//       if ((recv(fd,(char *)&sPingReq,sizeof(sPing),0)) > 0){
//          printf("Ping received\n");
//          timerCount = 10;
//          flagRcv = 0;
//       }
//    }


// }

// void *call_rx(){
//    int read_size;
//    char *buffer;
//    sSubscribe *sSubsPack;
//    sPublish *sPublishPack;
//    buffer = malloc(1024);

//    if(flagRcv == 1){

//       if((read_size = recv(fd, buffer, 1024 , 0)) < 0){
//             printf("Recv %i\n", read_size);
//             perror("recv failed topics");
//         }
        
//         if(*buffer == 0xC0){
//          sPingReq = malloc(sizeof(sPing));
//          sPingReq = (sPing *)buffer;
//          printf("Ping received\n");
//          if(sPingReq->msgType != 0xC0){
//             close(fd);
//          }else{
//             timerCount = 10;
//             flagRcv = 0;
//             flagPing = 1;
//          }
//          free(buffer);
//          free(sPingReq);
//         }

//         if(*buffer == PUBLISH_HEADER){
//             sPublishPack = (sPublish *)buffer;        
//         }

//         if(*buffer == SUBSCRIBE_HEADER){
//             sSubsPack = (sSubscribe *)buffer;
//         }
//    /*Always receiving messages form the clients on each thread*/
    
//       if(read_size == 0) {
//          perror("Client disconnected\n");
//       } else if(read_size == -1) {
//          perror("Receive failed\n");
//       }
//    }
// }