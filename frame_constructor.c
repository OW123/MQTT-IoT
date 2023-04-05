#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "frames.h"




sConnect connection_building(char *clientId, uint16_t clientIdLen, uint16_t keeAlive){
    sConnect sConnectPack;

    sConnectPack.msgType = PACK_CONNECTION_TYPE;
    sConnectPack.lenProtocolName = PACK_CONNECTION_PROTOCOL_LENGTH;
    // strcpy(sConnectPack.sProtocolName, "MQTT");
    sConnectPack.sProtocolName[0] = 'M';
    sConnectPack.sProtocolName[1] = 'Q';
    sConnectPack.sProtocolName[2] = 'T';
    sConnectPack.sProtocolName[3] = 'T';

    sConnectPack.protocolVersion = PACK_CONNECTION_VERSION;
    sConnectPack.connectFlag = PACK_CONNECTION_FLAGS;
    sConnectPack.lenKeepAlive = keeAlive;
    sConnectPack.lenClientId = clientIdLen;    
    strcpy(sConnectPack.clientID, clientId);

    sConnectPack.msgLength = sizeof(sConnect);
    printf("Connect Frame\n");
    printf("Length frame %X\n", sConnectPack.msgLength);
    printf("Message type %X\n", sConnectPack.msgType);
    printf("Protocol Lenght %X\n", sConnectPack.lenProtocolName);

    printf("Protocol Name %s\n", sConnectPack.sProtocolName);
    printf("Protocol version %X\n", sConnectPack.protocolVersion);
    printf("Flags %X\n", sConnectPack.connectFlag);
    printf("Keep Alive %X\n", sConnectPack.lenKeepAlive);
    printf("Length of client %X\n", sConnectPack.lenClientId);
    printf("client id: %s\n", sConnectPack.clientID);



    // system("sleep 10");
    return sConnectPack;
}

int connect_validation(sConnect connPackage){
    int flag = 0;

    if(connPackage.msgType != PACK_CONNECTION_TYPE)
        flag = -1;
    if(connPackage.lenProtocolName != PACK_CONNECTION_PROTOCOL_LENGTH)
        flag = -1;
    // if(strcmp(connPackage.sProtocolName, "MQTT") != 0) 
    //     flag = -1;
    
    if(connPackage.protocolVersion != PACK_CONNECTION_VERSION)
        flag = -1;

    if(connPackage.connectFlag != PACK_CONNECTION_FLAGS)
        flag = -1;

    return flag;
}

sConnectedAck connAck_building(){
    sConnectedAck sConnectedAckPack;

    sConnectedAckPack.msgType = CONNACK_TYPE;
    sConnectedAckPack.msgLength = CONNACK_LENGTH;
    sConnectedAckPack.ackFlag = CONNACK_FLAGS;

    return sConnectedAckPack;
}

int connAck_validation(sConnectedAck connAckPackage){
    int flag = 0;

    if(connAckPackage.msgType != CONNACK_TYPE)
        flag = -1;
    if(connAckPackage.msgLength != CONNACK_LENGTH)
        flag = -1;
    
    if(connAckPackage.ackFlag != CONNACK_FLAGS)
        flag = -1;

    return flag;
}


sPing ping_building(){
    sPing sPingPack;

    sPingPack.msgLength = 0x00;

    return sPingPack;
}


sPublish publish_building(uint8_t topic, char *clientName,uint16_t clientIdLen){
    sPublish sPublishPack;
    sPublishPack.msgType = PUBLISH_HEADER;
    sPublishPack.topicName = topic;
    sPublishPack.propertyLength = PUBLISH_PROPERTY_LEN;
    sPublishPack.lenClientId = clientIdLen;
    strcpy(sPublishPack.clientID, clientName);

    return sPublishPack;
}


sPubAck pubAck_building(uint16_t packetId){
    sPubAck sPubAckPack;

    sPubAckPack.msgType = PUBACK_TYPE;
    sPubAckPack.pubPacketId = packetId;
    sPubAckPack.reasonCode = PUBACK_SUCCESS;
    sPubAckPack.propertyLength = PUBACK_PROPERTY_LEN;

    return sPubAckPack;
}

sSubscribe suscribe_building(uint8_t topic, char *clientName,uint16_t clientIdLen){
    sSubscribe sSubscribePack;

    sSubscribePack.msgType = SUBSCRIBE_HEADER;
    sSubscribePack.propertyLength = SUBSCRIBE_PROPERTY_LEN;
    sSubscribePack.topicName = topic;
    sSubscribePack.subsOption = SUBSCRIBE_OPTION;
    sSubscribePack.lenClientId = clientIdLen;
    strcpy(sSubscribePack.clientID, clientName);

    return sSubscribePack;
}

sUnsubs unsubs_building(uint8_t topic, uint16_t topicLen){
    sUnsubs sUnsubsPack;

    sUnsubsPack.msgType = UNSUBSCRIBE_HEADER;
    sUnsubsPack.topicLength = topicLen;
    sUnsubsPack.topicName = topic;

    return sUnsubsPack;
}

sDisconnect disconnect_building(){
    sDisconnect sDisconnectPack;

    sDisconnectPack.msgType = DISCONNECT_HEADER;
    sDisconnectPack.reasonCode = DISCONNECT_REASON;
    sDisconnectPack.staticLength = DISCONNECT_STATIC_LEN;
    sDisconnectPack.intervalId = 0x11;
    sDisconnectPack.expiryInterval = DISCONNECT_EXPIRY_TIME;

    return sDisconnectPack;
}

sSubsAck subsAck_building(uint16_t packetId){
    sSubsAck sSubsAckPack;

    sSubsAckPack.msgType = SUBACK_HEADER;
    sSubsAckPack.packetId = packetId;

    return sSubsAckPack;
}


