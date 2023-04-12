MQTT Server/Client

This is a collection of C functions for building and validating MQTT messages, including:

    Connection Package
    ConnAck Package
    Publish Package
    PubAck Package
    Subscribe Package
    Unsubscribe Package
    Disconnect Package
    Subscribe Acknowledgment Package
    Ping Package

The code is organized as follows:
Header Files

The following header files are included:

    stdio.h
    stdlib.h
    stdint.h
    stdbool.h

Defines

The code includes various #define statements for defining constants, including connection, ConnAck, publish, PubAck, subscribe, unsubscribe, disconnect, subscribe acknowledgment, and ping messages.
Structs

The code defines various structs for holding different MQTT message types, including sConnect, sConnectedAck, sPing, sPublish, sPubAck, sSubscribe, sUnsubs, sDisconnect, and sSubsAck.
Functions

The code includes various functions for building MQTT messages, including:

    connection_building for building connection packages
    connAck_building for building ConnAck packages
    publish_building for building publish packages
    pubAck_building for building PubAck packages
    subscribe_building for building subscribe packages
    unsubs_building for building unsubscribe packages
    disconnect_building for building disconnect packages
    subsAck_building for building subscribe acknowledgment packages
    ping_building for building ping packages
    connect_validation for validating connection packages

Variables

The code defines a struct s_ClientFD for holding client information, including the file descriptor, client ID, keep alive time, check alive time, and subscribed topics.

Usage

  To use this code, simply compile the files with the Makefile that is provided and run each program in one terminal. The client programm needs two parameters: UserID and the time for the KeepAlive

Author
  Wing Manuel Ortiz Uribe
  César Padilla Lazos
