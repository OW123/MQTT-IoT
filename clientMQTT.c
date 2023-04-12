/*Needed headers*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include "frames.h"


/*The port used for this program*/
#define PORT 1883

char topics[3][100] = {"Stonks", "Social", "Training"};

int fd;
int keepAliveUser;


pthread_mutex_t lock;


sPing sPingReq;
sPing sPingRes;



void *call_setimer();

void timer_handler();

void *call_rx();


int main(int argc, char *argv[])
{
   sPublish sPublishPackage;
   sSubscribe sSubscribePackage;
   sConnectedAck connected_ack_frame;

   int numbytes;
   int option, topic;
   char clientID[50], msgPub[50];

   pthread_t t_timer, t_rx;
   struct sockaddr_in server;

   keepAliveUser = atoi(argv[2]);
   /*Frame structs*/
   printf("Argv %s\n", argv[1]);
   // sConnect connect_frame = connection_building(argv[1], strlen(argv[1]), (uint16_t *)argv[2]);

   sConnect connect_frame = connection_building(argv[1], strlen(argv[1]), keepAliveUser);

   strcpy(clientID, argv[1]);


   // iKeepAlive = (int *)argv[2];

   if ((fd = socket(AF_INET, SOCK_STREAM, 0))==-1){
      printf("socket() error\n");
      exit(-1);
   }

   server.sin_family = AF_INET;
   server.sin_port = htons(PORT);
   server.sin_addr.s_addr = inet_addr("192.168.100.137");//148.239.107.212  
   bzero(&(server.sin_zero),8);

   if(connect(fd, (struct sockaddr *)&server,  sizeof(struct sockaddr))==-1){
      printf("connect() error\n");
      exit(-1);
   }
   printf("Connection Established\n");

   if(pthread_mutex_init(&lock, NULL) != 0){
      perror("pthread_mutex_init failed");
   }

   send(fd,(char *)&connect_frame, sizeof(sConnect),0);

   if ((numbytes=recv(fd,(char *)&connected_ack_frame,sizeof(sConnectedAck),0)) == -1){
      printf("Error en recv() \n");
      close(fd);
      exit(-1);
   }
   printf("The received frame is:\n");

   printf("Type: %X\t msgLenght: %X\t ackFlag: %X\t reasonCode: %X\n", connected_ack_frame.msgType, connected_ack_frame.msgLength, connected_ack_frame.ackFlag, connected_ack_frame.reasonCode);

   // system("sleep 2");
   

   if(connected_ack_frame.reasonCode != 0x00){
      if(connected_ack_frame.reasonCode == 0x01)   
         perror("No more connections available\n");
      
      if(connected_ack_frame.reasonCode == 0x02)
         perror("Frame sent with errors\n");
      
      close(fd);
      exit(-1);
   }else{
      printf("Initiating pings\n\n");
      sPingReq = ping_building();
      sPingReq.msgType = 0xC0; //Request Response = 0xD0
      pthread_create(&t_timer, NULL, call_setimer, NULL);

   }

   // system("clear");

   if(pthread_create(&t_rx, NULL, call_rx, NULL) < 0){
      perror("Failed to create rx thread\n");
      exit(-1);
   }

   while(option != 3){
      printf("======================WELCOME TO CLIENT MENU=================\n\n Select an action to take:\n 1.-Subscribe\t 2.-Publish\n 3.-Exit\n\n");
   
      fflush(stdin);
      scanf("%i", &option);
      switch(option){
         case 1:
            printf("AVAILABLE TOPICS:\n 1.-%s\t 2.-%s\t 3.-%s\n\n", topics[0], topics[1], topics[2]);
            fflush(stdin);
            scanf("%i", &topic);

            if(topic < 1 && topic > 3){
               perror("Invalid topic\n\n");
               break;
            }else{
               pthread_mutex_lock(&lock);
               sSubscribePackage = suscribe_building(topic);
               if(send(fd,(char *)&sSubscribePackage, sizeof(sSubscribe),0) < 0){
                  perror("Error sending Subscribe Packet\n\n");
               }

               pthread_mutex_unlock(&lock);
            }

            break;
         case 2:
               printf("AVAILABLE TOPICS TO PUBLISH:\n 1.-%s\t 2.-%s\t 3.-%s\n\n", topics[0], topics[1], topics[2]);
               fflush(stdin);
               scanf("%i", &topic);

               printf("Enter the message to publish:\n");
               // fflush(stdin);
               if(pthread_mutex_lock(&lock) == 0){
                  scanf("%49[^\n]", msgPub);
                  pthread_mutex_unlock(&lock);
               }

               if(topic < 1 && topic > 3){
                  perror("Invalid topic\n\n");
                  break;
               }else{
                  if(pthread_mutex_lock(&lock) == 0){
                     sPublishPackage = publish_building(topic, (char *)&msgPub);
                     if(send(fd,(char *)&sPublishPackage, sizeof(sPublish),0) < 0){
                     perror("Error sending Subscribe Packet\n\n");
                     }
                     pthread_mutex_unlock(&lock);

                  }
            }

            break;

         case 3:
            close(fd);
            break;

         default:
            perror("Unknown option\n\n");
            break;
      }//End Switch Case
   }//End While Loop Main

   return 0;
}//Close Main Function

void timer_handler()
{
   sPingReq = ping_building();
   sPingReq.msgType = 0xC0;
   if(send(fd,(char *)&sPingReq,sizeof(sPing),0) > 0){
      printf("Ping sent\n");
   }

}//Close TImer Handler

void *call_setimer(){
      struct sigaction sa;
      struct itimerval timer;
      //Llamada a KeepAlive
         memset (&sa, 0, sizeof (sa));
         sa.sa_handler = &timer_handler;
         sigaction (SIGVTALRM, &sa, NULL);
         
         timer.it_value.tv_sec = keepAliveUser / 2;
         
         timer.it_interval.tv_sec = keepAliveUser / 2;

         /* Start a virtual timer. It counts down whenever this process is
         executing. */
         setitimer (ITIMER_VIRTUAL, &timer, NULL);
   while(1){}
}//Close TImer Setup 

void *call_rx(){
   int numbytes;
   sPublish pubRecv;
   sSubsAck subAckRecv;
   sPubAck pubAckRecv;

   while((numbytes=recv(fd,(char *)&pubRecv,sizeof(sPublish),0)) > 0){

      if(pubRecv.msgType == 0xD0){
         sPingRes.msgType = pubRecv.msgType;
         sPingRes.msgLength = 0x00;
         continue;
      }
      else if(pubRecv.msgType == 0x40){
         pubAckRecv.msgType = pubRecv.msgType;
         pubAckRecv. reasonCode = pubRecv.topicNum;

         if(pubAckRecv.reasonCode == 0x00){
            printf("Publish done\n");
         }
         
      }//Close AckPub
      else if(pubRecv.msgType == 0x90){
         subAckRecv.msgType = pubRecv.msgType;
         subAckRecv.reasonCode = pubRecv.topicNum;
         if(subAckRecv.reasonCode == 0x00){
            printf("Subscription done\n");
         }
      }//Close Ack Subscription

      else if(pubRecv.msgType == 0x30){
         if(pubRecv.topicNum == 0x01){
            printf("New message received in topic %s:\n %s\n",topics[0], pubRecv.msgPub);
         }
         else if(pubRecv.topicNum == 0x02){
            printf("New message received in topic %s:\n %s\n",topics[1], pubRecv.msgPub);

         }
         else if(pubRecv.topicNum == 0x03){
            printf("New message received in topic %s:\n %s\n",topics[2], pubRecv.msgPub);

         }
      }

   }//Close While
   return 0;
}


