#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/*Connection Defines*/
#define PACK_CONNECTION_TYPE 0x10
#define PACK_CONNECTION_VERSION 0x04
#define PACK_CONNECTION_FLAGS 0x02
#define PACK_CONNECTION_PROTOCOL_LENGTH 0x04

/*ConnAck Defines*/
#define CONNACK_TYPE 0x20
#define CONNACK_LENGTH 0x02
#define CONNACK_FLAGS 0x00

/*Publish Defines*/
#define PUBLISH_HEADER 0x30
#define PUBLISH_PROPERTY_LEN 0x00

/*PublishAck Defines*/
#define PUBACK_TYPE 0x40
#define PUBACK_SUCCESS 0x00
#define PUBACK_BAD_TOPIC 0x90
#define PUBACK_UNSPECIFIED_ERROR 0x80
#define PUBACK_PROPERTY_LEN 0x01


/*Subscribe Defines*/
#define SUBSCRIBE_HEADER 0x80
#define SUBSCRIBE_PROPERTY_LEN 0x00
#define SUBSCRIBE_OPTION 0x02

/*Unsubs Defines*/
#define UNSUBSCRIBE_HEADER 0xA0

/*Disconnect Defines*/
#define DISCONNECT_HEADER 0xE0  
#define DISCONNECT_REASON 0x00
#define DISCONNECT_STATIC_LEN 0x05
#define DISCONNECT_EXPIRY_TIME 0x0A

/*SubsAck Defines*/
#define SUBACK_HEADER 0x09


/*Ping Req Defines*/
#define PINGREQ_HEADER 0xC0

/*Ping Resp Defines*/
#define PINGRESP_HEADER 0xD0




typedef struct {
    uint8_t msgType;
    uint16_t msgLength;
    uint16_t lenProtocolName;
    char sProtocolName[5];
    uint8_t protocolVersion;
    uint8_t connectFlag;
    uint16_t lenKeepAlive;
    uint16_t lenClientId;
    char clientID[4];
}sConnect;


typedef struct {
    uint8_t msgType;
    uint8_t msgLength;
    uint8_t ackFlag;
    uint8_t reasonCode;
    //0x00 Conn Accept
    //0x01 No more connections
    //0x02 Malformed Frame;
}sConnectedAck;

typedef struct {
    uint8_t msgType;
    uint8_t msgLength;
}sPing;

typedef struct {
    uint8_t msgType;
    uint8_t topicName;
    uint8_t propertyLength;
    uint16_t lenClientId;
    char clientID[100];
}sPublish;

typedef struct {
    uint8_t msgType;
    uint16_t pubPacketId;
    uint8_t reasonCode;
    uint8_t propertyLength;
}sPubAck;



typedef struct {
    uint8_t msgType;
    uint8_t propertyLength;
    uint8_t topicName;
    uint8_t subsOption;
    uint16_t lenClientId;
    char clientID[100];
}sSubscribe;



typedef struct {
    uint8_t msgType;
    uint16_t topicLength;
    uint8_t topicName;
}sUnsubs;



typedef struct {
    uint8_t msgType;
    uint8_t reasonCode;
    uint8_t staticLength;
    uint8_t intervalId;
    uint16_t expiryInterval;
}sDisconnect;


typedef struct {
    uint8_t msgType;
    uint16_t packetId;
}sSubsAck;




sConnect connection_building(char *clientId, uint16_t clientIdLen, uint16_t keeAlive);

sConnectedAck connAck_building();

sPublish publish_building(uint8_t topic, char *clientName,uint16_t clientIdLen);

sPubAck pubAck_building(uint16_t packetId);

sSubscribe suscribe_building(uint8_t topic, char *clientName,uint16_t clientIdLen);

sUnsubs unsubs_building(uint8_t topic, uint16_t topicLen);

sDisconnect disconnect_building();

sSubsAck subsAck_building(uint16_t packetId);

sPing ping_building();

int connect_validation(sConnect connPackage);
