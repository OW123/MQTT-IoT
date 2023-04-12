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
   sPublish pubClient;
   sPubAck pubAck;
   sSubscribe subsClient;
   sSubsAck subAck;


   if(pthread_mutex_lock(&lock) == 0){

      if((read_size = select(numConecctions + 1, &read_FDs, NULL, NULL, NULL))  == -1){
         perror("Select FD failed\n");
      }

      pthread_mutex_unlock(&lock);
   }
   

   while(1){
      for(int i = 0; i < BACKLOG; i++){
         if(FD_ISSET(client_controller[i].fd, &read_FDs)){
            if((read_size = recv(client_controller[i].fd, (char *)&pubClient, sizeof(sPublish) , 0)) < 0){
               printf("Recv %i\n", read_size);
               perror("recv failed topics");
               continue;
            }

            if(read_size <= 0){
               printf("%s beeing disconnected\n", client_controller[i].clientID);
               if(pthread_mutex_lock(&lock) == 0){
                  close(client_controller[i].fd);
                  FD_CLR(client_controller[i].fd, &read_FDs);
                  client_controller[i].fd = 0;
                  pthread_mutex_unlock(&lock);
               }
            }

            if(pubClient.msgType == 0xC0){
               sPingReq.msgType = pubClient.msgType;
               sPingReq.msgLength = 0x00;
               printf("Ping req received.\n Type: %X\t Lenght: %X\n", sPingReq.msgType, sPingReq.msgLength);
               client_controller[i].i_KeepALive = client_controller[i].i_CheckAlive;
               if(send(client_controller[i].fd,(char *)&sPingRes,sizeof(sPing),0) < 0){
                  perror("Send failed in keepalive\n");
               }
            }//Close If ping
            else if(pubClient.msgType == SUBSCRIBE_HEADER){
               subsClient.msgType = pubClient.msgType;
               subsClient.topicNum = pubClient.topicNum;
               subsClient.lenMsg = sizeof(sSubscribe);

               if(subsClient.topicNum == 0x01 && client_controller[i].topic1 == false){
                  client_controller[i].topic1 = true;
                  subAck = subsAck_building(0x00);
               }
               else if(subsClient.topicNum == 0x02 && client_controller[i].topic2 == false){
                  client_controller[i].topic2 = true;
                  subAck = subsAck_building(0x00);
               }
               else if(subsClient.topicNum == 0x03 && client_controller[i].topic3 == false){
                  client_controller[i].topic3 = true;
                  subAck = subsAck_building(0x00);
               }
               else{
                  subAck = subsAck_building(0x01);
               }
               if(client_controller[i].fd != 0){
                  if(send(client_controller[i].fd,(char *)&subAck,sizeof(sSubsAck),0) < 0){
                     perror("Send failed in ackSubs\n");
                  }
               }
            }//Close if Subscribe
            else if(pubClient.msgType == PUBLISH_HEADER){
               if(pubClient.topicNum == 0x01){
                  printf("Publishing message to topic 1 from %s client\n\n", client_controller[i].clientID);
                  for(int k = 0; k < BACKLOG; k++){
                     if(client_controller[k].fd != 0 && client_controller[k].topic1 == true & i != k){
                        if(send(client_controller[i].fd,(char *)&pubClient,sizeof(sPublish),0) < 0){
                           perror("Send failed in publish to others\n");
                        }
                     }
                  }//Close For 0x01

               }//Close If Publish 0x01


               else if(pubClient.topicNum == 0x02){
                  printf("Publishing message to topic 2 from %s client\n\n", client_controller[i].clientID);
                  for(int k = 0; k < BACKLOG; k++){
                     if(client_controller[k].fd != 0 && client_controller[k].topic1 == true & i != k){
                        if(send(client_controller[i].fd,(char *)&pubClient,sizeof(sPublish),0) < 0){
                           perror("Send failed in publish to others\n");
                        }
                     }
                  }//Close For 0x02

               }//Close If Publish 0x02



               else if(pubClient.topicNum == 0x03){
                  printf("Publishing message to topic 3 from %s client\n\n", client_controller[i].clientID);
                  for(int k = 0; k < BACKLOG; k++){
                     if(client_controller[k].fd != 0 && client_controller[k].topic1 == true & i != k){
                        if(send(client_controller[i].fd,(char *)&pubClient,sizeof(sPublish),0) < 0){
                           perror("Send failed in publish to others\n");
                        }
                     }
                  }//Close For 0x03

               }//Close If Publish 0x03

               if(client_controller[i].fd != 0){
                  pubAck = pubAck_building();
                  if(send(client_controller[i].fd,(char *)&pubAck,sizeof(sPubAck),0) < 0){
                     perror("Send failed in ackPub\n");
                  }
               }

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

         if(client_controller[i].i_KeepALive <= 0){
            if(pthread_mutex_lock(&lock) == 0){
               close(client_controller[i].fd);
               FD_CLR(client_controller[i].fd, &read_FDs);
               client_controller[i].fd = 0;
               pthread_mutex_unlock(&lock);
            }

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
