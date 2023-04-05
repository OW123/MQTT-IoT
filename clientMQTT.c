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

int threadsFlag = 0;

int fd;

char clientID[100];

int lengthID;

int connCount = 10;


pthread_mutex_t lock_pub, lock_subs, lock_timer;

char topics[3][100] = {"Stonks", "Social", "Training"};


sPublish sPublishPackage;
sSubscribe sSubscribePackage;

sPing sPingReq;
sPing sPingRes;

struct sigaction sa;
struct itimerval timer;


void *call_publish();

void *call_subscribe();

void *call_setimer();


void timer_handler();

void *call_rx();


int main(int argc, char *argv[])
{
   int numbytes;
   int userOption;
   pthread_t t_pub, t_sub, t_timer, t_rx;
   pthread_mutex_t lock;
   
   struct sockaddr_in server;


   /*Frame structs*/
   printf("Argv %s\n", argv[1]);
   // sConnect connect_frame = connection_building(argv[1], strlen(argv[1]), (uint16_t *)argv[2]);

   sConnect connect_frame = connection_building(argv[1], strlen(argv[1]), 100);

   strcpy(clientID, argv[1]);
   lengthID = strlen(clientID);

   sConnectedAck connected_ack_frame;

   // iKeepAlive = (int *)argv[2];

   if ((fd = socket(AF_INET, SOCK_STREAM, 0))==-1){
      printf("socket() error\n");
      exit(-1);
   }

   server.sin_family = AF_INET;
   server.sin_port = htons(PORT);
   server.sin_addr.s_addr = inet_addr("148.239.99.167");//148.239.107.212  
   bzero(&(server.sin_zero),8);

   if(connect(fd, (struct sockaddr *)&server,  sizeof(struct sockaddr))==-1){
      printf("connect() error\n");
      exit(-1);
   }
   printf("Connection Established\n");

   send(fd,(char *)&connect_frame, sizeof(sConnect),0);

   if ((numbytes=recv(fd,(char *)&connected_ack_frame,sizeof(sConnectedAck),0)) == -1){
      printf("Error en recv() \n");
      close(fd);
      exit(-1);
   }
   printf("The received frame is:\n");

   printf("Type: %X\t msgLenght: %X\t ackFlag: %X\t reasonCode: %X\n", connected_ack_frame.msgType, connected_ack_frame.msgLength, connected_ack_frame.ackFlag, connected_ack_frame.reasonCode);

   // system("sleep 5");
   

   if(connected_ack_frame.reasonCode != 0x00){
      if(connected_ack_frame.reasonCode == 0x01)   
         perror("No more connections available\n");
      
      if(connected_ack_frame.reasonCode == 0x02)
         perror("Frame sent with errors\n");
      
      close(fd);
      exit(-1);
   }

   system("clear");

   pthread_create(&t_timer, NULL, call_setimer, NULL);
   pthread_create(&t_rx, NULL, call_rx, NULL);


   while(1){
      printf("=================MQTT MENU=================\n");
      printf("Select an option:\n 1.- Publish\t 2.-Subscribe\n");
      scanf("%i", &userOption);

      if(userOption < 1 && userOption > 2){
         perror("Invalid option");
         threadsFlag = 0;
         continue;
      }
      if(userOption == 1){
         pthread_create(&t_pub, NULL, call_publish, NULL);
         pthread_join(t_pub, NULL);
      }

      if(userOption == 2){
         pthread_create(&t_sub, NULL, call_subscribe, NULL);
         pthread_join(t_sub, NULL);
      }

      system("clear");
         
   }
   return 0;
}

void *call_rx(){
   int numbytes;
   char *buffer;
   if ((numbytes=recv(fd,buffer,1024,0)) == -1){
      printf("Error en recv() rx\n");
      close(fd);
      exit(-1);
   }
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

//Hacer lock de los dos threads, o jutar las dos opciones en un mismo thread
void *call_publish(){
   pthread_mutex_lock(&lock_pub);
   int option;
   printf("===================== AVAILABLE TOPICS =======================\n");
   printf("Select a topic to publish\n");
   for(int i = 0; i < 3; i++){
      printf("%i.- %s\n", i, topics[i]);
   }
   scanf("%i", &option);

   sPublishPackage = publish_building(option, clientID, lengthID);

   if(send(fd,(char *)&sPublishPackage,sizeof(sPublish),0) < 0){
      perror("Bad send in publish");
   }

   pthread_mutex_unlock(&lock_pub);
}

void *call_subscribe(){
   pthread_mutex_lock(&lock_subs);
   int option;
   printf("===================== AVAILABLE TOPICS =======================\n");
   printf("Select a topic to publish\n");
   for(int i = 0; i < 3; i++){
      printf("%i.- %s\n", i, topics[i]);
   }
   scanf("%i", &option);

   sSubscribePackage = suscribe_building(option, clientID, lengthID);
   printf("Size of subscription %li\n", sizeof(sSubscribePackage));

   if(send(fd,(char *)&sSubscribePackage,sizeof(sSubscribe),0) < 0){
      perror("Bad send in subscribe");
   }
   pthread_mutex_unlock(&lock_subs);
}

void timer_handler()
{  
   pthread_mutex_lock(&lock_timer);
   sPingReq = ping_building();

	printf("timer expired %d times\n", connCount);

   sPingReq.msgType = 0xC0; //Request Response = 0xD0

   if(send(fd,(char *)&sPingReq,sizeof(sPing),0) > 0){
      printf("Ping sent\n");
      connCount = 10;
   }


   // if ((recv(fd,(char *)&sPingRes,sizeof(sPing),0)) < 0){
   //    close(fd);
   // }
   
   if(connCount == 0){
      perror("Keep ALive Time Exceeded");
      close(fd);
      exit(-1);
   }

    connCount--;

    pthread_mutex_unlock(&lock_timer);
}