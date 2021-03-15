/* Bibliotecas */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <MQTTClient.h>
#include "SAPoTClient.h"

/* Declaração de Objetos */
SAPoTClient SAPoTclient;

void signalHandling(int signum){

	printf("Finalizando o cliente SAPoT...\n");
	SAPoTClient_end();
	exit(1);

}

int main(int argc, char *argv[]){

	//Tratando o sinal SIGINT
	signal(SIGINT, signalHandling);

	if(argc<2){

		printf("Argumento de execução invalido. Tente:\n");
		printf("./gpc access\n");
		printf("./gpc modification \"$macaddr\" \"$label\" \n");
		printf("./gpc solicitation \"$label\" \"$operation\"\n");
		printf("\t $operation: ON, OFF, RST \n");
		exit(1);	

	}

	//Definindo o id para cliente 
	const char* clientId = "78:E4:00:8C:65:77";

	//Configurando as opções de inicialização do cliente
	//SAPoTClient_create_options SAPoTopts = {"00:00:00:00:00:00", MQTT, {"10.10.40.84", "1883", "LDAP", NULL}};
	//SAPoTClient_create_options SAPoTopts = {"00:00:00:00:00:00", MQTT, {"localhost", "1883", NULL, NULL}};
	//SAPoTClient_create_options SAPoTopts = {"00:00:00:00:00:00", MQTT, {"192.168.4.1", "1883", NULL, NULL}};
	SAPoTClient_create_options SAPoTopts = {"00:00:00:00:00:00", MQTT, {"10.10.20.205", "1883", NULL, NULL}};


	//Iniciando o cliente
	if(SAPoTClient_begin(&SAPoTclient, &SAPoTopts, clientId) == SAPOTCLIENT_FAILURE){
		printf("Erro (%d) ao iniciar o cliente SAPoT", SAPoTClient_error());
		exit(1);
	}

	//printf("Cliente iniciado\n");

	void* message;
	int messageLen;

	if(!strcmp(argv[1], "access")){

		printf("Requested Access \n");
		
		//Alocando espaço de memória para a mensagem
		messageLen = sizeof(SAPoTMessage_header);
		message = malloc(messageLen);

		//Preenchendo cabeçalho da mensagem
		SAPoTMessage_header* header = (SAPoTMessage_header*) message;
		header->version = SAPOT_PROTOCOL_VERSION;
		header->ack = 0;
		header->rsv1 = 0;
		header->rsv2 = 0;
		header->rsv3 = 0;
		header->instruction = 4;
		header->serial = 0;
		header->length = messageLen;
		getmacID(clientId, header->emitterId);

	}
	else if(!strcmp(argv[1], "modification")){

		printf("Requested Modification \n");

		//Alocando espaço de memória para a mensagem
		messageLen = sizeof(SAPoTMessage_header) + sizeof(SAPoTMessage_modification);
		message = malloc(messageLen);

		//Preenchendo cabeçalho da mensagem
		SAPoTMessage_header* header = (SAPoTMessage_header*) message;
		header->version = SAPOT_PROTOCOL_VERSION;
		header->ack = 0;
		header->rsv1 = 0;
		header->rsv2 = 0;
		header->rsv3 = 0;
		header->instruction = 6;
		header->serial = 0;
		header->length = messageLen;
		getmacID(clientId, header->emitterId);

		//Preenchendo payload
		SAPoTMessage_modification* modification = (SAPoTMessage_modification*) (message + sizeof(SAPoTMessage_header));
		strncpy(modification->macaddr, argv[2], 17);
		strncpy(modification->label, argv[3], 10);

	}
	else if(!strcmp(argv[1], "solicitation")){

		printf("Requested Solicitation \n");

		//Alocando espaço de memória para a mensagem
		messageLen = sizeof(SAPoTMessage_header) + sizeof(SAPoTMessage_solicitation);
		message = malloc(messageLen);

		//Preenchendo cabeçalho da mensagem
		SAPoTMessage_header* header = (SAPoTMessage_header*) message;
		header->version = SAPOT_PROTOCOL_VERSION;
		header->ack = 0;
		header->rsv1 = 0;
		header->rsv2 = 0;
		header->rsv3 = 0;
		header->instruction = 3;
		header->serial = 0;
		header->length = messageLen;
		getmacID(clientId, header->emitterId);

		//Preenchendo payload
		SAPoTMessage_solicitation* solicitation = (SAPoTMessage_solicitation*) (message + sizeof(SAPoTMessage_header));
		strncpy((char*) solicitation->label, argv[2], 10);
		solicitation->degreeOfPerformance = 0xffff;
		if(!strcmp(argv[3], "ON")){
			solicitation->sensorOrActuatorId = 1;
			solicitation->timeSet = 0x1001;

		}
		else if(!strcmp(argv[3], "OFF")){
			solicitation->sensorOrActuatorId = 1;
			solicitation->timeSet = 0x1003;

		}
		else if(!strcmp(argv[3], "RST")){
			solicitation->sensorOrActuatorId = 2;
			solicitation->timeSet = 0x1001;
			
		}
		else{ 
			printf("Invalid operation !\n");
			SAPoTClient_end();
			exit(1);
		}	

	}
	else{
		SAPoTClient_end();
		exit(1);
	} 

	//Publicando mensagem via MQTT
	MQTTpublish(SAPoTopts.centralId, message, messageLen);

	//Limpando espaço de memória da mensagem enviada
	free(message);

	//Entrando em modo de loop para esperar a resposta da central
	SAPoTClient_loop();	

	return 0;
}
