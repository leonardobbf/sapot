
/*
* SAPoTClient.c define as funções e subrotinas do protocolo SAPoT
*
*
*   
*/


      /************************* Headers ******************************/
                
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <MQTTClient.h>
#include "SAPoTClient.h"

/* Global Objects */
SAPoTClient* handle;
SAPoTClient_create_options* opts;

/* Function's prototype */


/**
* [Principal] SAPoTClient_begin
*
*/
int SAPoTClient_begin(SAPoTClient* clientHandle, SAPoTClient_create_options* optsHandle, const char* clientId){

  //puts(" Starting a SAPoT Client ...");
  
  /* Atribuição aos objetos globais */
  opts = (SAPoTClient_create_options*) optsHandle;
  handle = (SAPoTClient*) clientHandle;

  /*Inicializa o objeto do tipo SAPoTClient*/
  handle->id = clientId;
  handle->error = SAPOTCLIENT_SUCCESS;
  handle->inLoop = true;
  handle->inMessage = NULL;
  handle->outMessage = NULL;
  handle->header = NULL;
  
  /*puts("SAPoT Client create options: ");
  printf("\t Central ID = %s\n", opts->centralId);
  printf("Transmission Protocol = %d\n", opts->transmissionProtocol);
  printf("\t host = %s\n", opts->transmission.host);
  printf("\t port = %s\n", opts->transmission.port);
  printf("\t user = %s\n", opts->transmission.user);
  printf("\t pass = %s\n", opts->transmission.pass);*/
  
  //Iniciando protocolo de transmissão
  if(opts->transmissionProtocol == UNDEFINED){
    puts("Undefined Transmission Protocol. The function SAPoTClient_loop() cannot be used.");
  }
  else if(opts->transmissionProtocol == MQTT){ 
    if(MQTTconnect() != SAPOTCLIENT_SUCCESS){
      handle->error = ERROR_STARTING_TRANSMISSION_PROTOCOL;
      return SAPOTCLIENT_FAILURE;
    }
    else; //puts("\t Connected with MQTT's Broker.\n");
  } 
  else{
    handle->error = ERROR_SETTING_TRANSMISSION_PROTOCOL;
    return SAPOTCLIENT_FAILURE;
  } 
  
    
  return SAPOTCLIENT_SUCCESS;

}

/**
* [Principal] SAPoTClient_end
*
*/
void SAPoTClient_end(){

  //Fechando conexão com o server MQTT
  handle->inLoop = false;
  MQTTClient_disconnect(handle->MQTTclient, 10000);
  //MQTTClient_destroy(handle->MQTTclient);
  sleep(1);
  exit(1);

}

/**
* [Principal] SAPoTClient_loop
*
*/

int SAPoTClient_loop(){

  //printf("SAPoTClient_loop \n");
  while(handle->inLoop == true){
    MQTTconnect();
    usleep(500000);
  }

  return 1;

}

/**
* [Principal] SAPoTClient_unpack_message
*
*/
int SAPoTClient_unpack_message(void* message, int messageLen){

  //printf("SAPoTClient_unpack_message: \n");  
  
  //Apontando para o espaço de memoria
  handle->inMessage = (uint8_t*) message;
  //Estruturando o cabeçalho da mensagem recebida
  handle->header = (SAPoTMessage_header*) handle->inMessage;
  
  //Verificando a versão do pacote recebido
  if(handle->header->version != SAPOT_PROTOCOL_VERSION){
      handle->error = SAPOT_VERSION_ERROR;
      return SAPOTCLIENT_FAILURE;
  }
  else{

    /*int i;
    for(i=0; i<messageLen; i++) printf("\t %x\n", handle->inMessage[i]);
    */

    /*
    printf("\t Version: %d \n", handle->header->version);
    if(handle->header->ack == true) printf("\t ACK true \n");
    printf("\t Instruction: %d \n", handle->header->instruction);
    printf("\t Serial: %d \n", handle->header->serial);
    printf("\t length: %d \n", handle->header->length);
    printf("\t Version: %d \n", handle->header->version);
    printf("\t emitterId: %02x:%02x:%02x:%02x:%02x:%02x \n", handle->header->emitterId[0], handle->header->emitterId[1], handle->header->emitterId[2], handle->header->emitterId[3], handle->header->emitterId[4], handle->header->emitterId[5]);
    */
  }
  
  return SAPOTCLIENT_SUCCESS;
}

/**
* [Principal] SAPoTClient_set_operation 
*
*/
int SAPoTClient_set_operation(int (*publish) (char*, void*, int)){


  //Registration
  if(handle->header->instruction == 0x00){
    
  }
  //Solicitation 0x01
  else if(handle->header->instruction == 0x01){
  
  }
  //Solicitation 0x02
  else if(handle->header->instruction == 0x02){
  
  }
  //Solicitation 0x03
  else if(handle->header->instruction == 0x03){

    if(handle->header->ack == true){
      SAPoTClient_end();
    }
  
  }
  //Acess
  else if(handle->header->instruction == 0x04){

    if(handle->header->ack == true){
      SAPoTClient_printAcess();
      SAPoTClient_end();
    }
    
  
  }
  //Record
  else if(handle->header->instruction == 0x05){
  
  }
  //Modification
  else if(handle->header->instruction == 0x06){
    if(handle->header->ack == true){
      //printf("Modification confirmed !\n");
      SAPoTClient_end();
    }

  }
  
  //Verifica a existência de erro na operação realizada 
  if(handle->error != SAPOTCLIENT_SUCCESS) return SAPOTCLIENT_FAILURE; 
  else return SAPOTCLIENT_SUCCESS;  
  
  
  return SAPOTCLIENT_SUCCESS;
}

/**
* [Principal] SAPoTClient_error
*
*/
int SAPoTClient_error(){

  return handle->error;
}

/**
* [Subrotina] MQTTconnect
*
*/
int MQTTconnect(){

  if(MQTTClient_isConnected(handle->MQTTclient) != true){
  
    //printf("MQTTconnect: \n");
  
    MQTTClient_connectOptions MQTTopts = { {'M', 'Q', 'T', 'C'}, 6, 60, 1, 1, NULL, opts->transmission.user, opts->transmission.pass, 30, 0, NULL, 0, NULL, MQTTVERSION_DEFAULT, {NULL, 0, 0}, {0, NULL}, -1, 0}; //MQTTClient_connectOptions_initializer;
      
      /* tcp://10.10.40.84:1883 */
      char* serverURI = malloc(50);
      sprintf(serverURI, "tcp://%s:%s",opts->transmission.host, opts->transmission.port);
      //printf("\t serverURI: %s\n", serverURI);
      
      if(MQTTClient_create(&handle->MQTTclient, serverURI, handle->id, MQTTCLIENT_PERSISTENCE_NONE, NULL) != MQTTCLIENT_SUCCESS){
        puts("MQTTconnect error: unable to create client\n");
        free(serverURI);
        return 0;
      }
        
      free(serverURI);
      
      //puts("\t MQTTClient_create ready.");
      
      if(MQTTClient_setCallbacks(handle->MQTTclient, NULL, NULL, MQTTmessageArrived, NULL) != MQTTCLIENT_SUCCESS){
      printf("MQTTconnect error: unable to set call back message\n");
      return 0;
      }
    
      //puts("\t MQTTClient_setCallbacks ready.");
   
      if(MQTTClient_connect(handle->MQTTclient, &MQTTopts) != MQTTCLIENT_SUCCESS){
        printf("MQTTconnect error: unable to connect with broker\n");
        return 0;
      }
      
      //puts("\t MQTTClient_connect ready.");
      
      if(MQTTClient_subscribe(handle->MQTTclient, handle->id, 0) != MQTTCLIENT_SUCCESS){
      printf("MQTTconnect error: unable to subscribe on topic %s\n", handle->id);
      return 0;
    }
    
    //puts("\t MQTTClient_subscribe ready.");
    
   
  }  
  
  return SAPOTCLIENT_SUCCESS;   

}

/**
* [Subrotina] MQTTcallback
*
*/
int MQTTmessageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *MQTTmsg){
  
  if(SAPoTClient_unpack_message(MQTTmsg->payload, MQTTmsg->payloadlen) != SAPOTCLIENT_SUCCESS){
    printf("MQTTmessageArrived error: unable to unpack SAPoT's message (%d)\n", handle->error);
  }
  else{
    if(SAPoTClient_set_operation(MQTTpublish) != SAPOTCLIENT_SUCCESS){
      printf("MQTTmessageArrived error: unable to set SAPoT's operation (%d)\n", handle->error);
    }
  } 

  //free(handle->outMessage);

  MQTTClient_freeMessage(&MQTTmsg);
  
  MQTTClient_free(topicName);
  
  return 1;
}

/**
* [Subrotina] MQTTpublish
*
*/
int MQTTpublish(char* topic, void* payload, int payloadLen){

  //printf("MQTTPublish on topic: %s\n", topic);
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    pubmsg.payload = payload;
    pubmsg.payloadlen = payloadLen;
    pubmsg.qos = 0;
    pubmsg.retained = 0;
    MQTTClient_deliveryToken token;
    
    if(MQTTClient_publishMessage(handle->MQTTclient, topic, &pubmsg, &token) != MQTTCLIENT_SUCCESS){
      printf("\t MQTTpublish error: unable to publish the message\n");
      handle->error = ERROR_MQTT_PUBLISH;
      return SAPOTCLIENT_FAILURE;
    }
    else{ 
      MQTTClient_waitForCompletion(handle->MQTTclient, token, 1000L);
      //printf("\t Published \n");
      return SAPOTCLIENT_SUCCESS;
    } 
}


/**
* [Subrotina] printAcess
*
*/
void SAPoTClient_printAcess(){

    //printf("SAPoTClient_printAcess: \n\n");

    int accessPackLen = (handle->header->length - sizeof(SAPoTMessage_header)) / sizeof(SAPoTMessage_access);

    //printf("accessPackLen = %d\n", accessPackLen);

    printf("\t  Label\t\t   Macaddr\t\tType\tSensors\tActuators\n");

    int i, j;
    for(i=0; i<accessPackLen; i++){

      SAPoTMessage_access* access = (SAPoTMessage_access*) (handle->inMessage + sizeof(SAPoTMessage_header) + (i*sizeof(SAPoTMessage_access)));
      printf("\t");
      for(j=0; j<10; j++) printf("%c", access->label[j]);
      printf("\t");
      for(j=0; j<17; j++) printf("%c", access->id[j]); 
      printf("\t %d", (int)access->type);
      printf("\t %d", (int)access->sensor);
      printf("\t %d\n",(int)access->actuator);

    }
    
}

/**
* [Utilitário] upper_string
*
*/

void upper_string(char s[]){
   int c = 0;
   
   while (s[c] != '\0') {
      if (s[c] >= 'a' && s[c] <= 'z') {
         s[c] = s[c] - 32;
      }
      c++;
   }
}

/**
* [Utilitário] getmacID
*
*/

void getmacID(const char* MAC, uint8_t ID[6]){

  int i;
  int j=0;
  uint8_t bff[12];
    
  for(i=0; i<(strlen(MAC)); i++){
    
    if(MAC[i] == ':');
    else if(MAC[i] >= 'A'){
      bff[j] = (uint8_t) (MAC[i]- 55);
      //Serial.println(bff[j], HEX);
      j++;
    }
    else{
      bff[j] =(uint8_t) (MAC[i]-0x30);
      //Serial.println(bff[j], HEX);
      j++;    
    }
    
  } 
  
  j=0;
  for(i=0; i<11; i++){
   ID[j] =  (uint8_t)((bff[i]<<4) | (bff[++i]));
   //printf("ID[%d] = %x\n", j, ID[j]);
   j++;
  }
  
} 


/**
*
*
*/
