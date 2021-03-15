
/*
*	SAPoTCentral.c define as funções e subrotinas do protocolo SAPoT
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <MQTTClient.h>
#include <mysql/mysql.h>
#include "SAPoTCentral.h"

/* Global Objects */
SAPoTCentral* handle;
SAPoTCentral_create_options* opts;
volatile MQTTClient_deliveryToken MQTTdeliveredtoken;
int fd; //File Descriptor
void* bff_log; //bufer para mensagens de log
int MQTTstatus = -1; 

/* Function's prototype */

/**
* [Principal] SAPoTCentral_begin
*
*/
int SAPoTCentral_begin(SAPoTCentral* centralHandle, SAPoTCentral_create_options* optsHandle, const char* centralId){

	puts(" Starting a UCC ...");

	/* Inicializa o descritor de arquivo que receberá os LOGs*/
	fd = open("ucc_log.txt", O_WRONLY | O_APPEND | O_CREAT);
	if(fd < 0){
		handle->error = ERROR_LOG_OPERATION;
		return SAPOTCENTRAL_FAILURE;
	} 

	system("chmod 777 ucc_log.txt");
	
	/* Atribuição aos objetos globais */
	opts = (SAPoTCentral_create_options*) optsHandle;
	handle = (SAPoTCentral*) centralHandle;

	/*Inicializa o objeto do tipo SAPoTCentral*/
	handle->id = centralId;
	handle->error = SAPOTCENTRAL_SUCCESS;
	handle->inLoop = 1;
	handle->serial = 0;
	handle->inMessage = NULL;
	handle->outMessage = NULL;
	handle->header = NULL;
	handle->registration = NULL;
	handle->solicitation = NULL;
	//handle->acess = NULL;
	handle->record = NULL;
	handle->modification = NULL;
	
	puts("UCC create options: ");
	printf("Transmission Protocol = %d\n", opts->transmissionProtocol);
	printf("\t host = %s\n", opts->transmission.host);
	printf("\t port = %s\n", opts->transmission.port);
	printf("\t user = %s\n", opts->transmission.user);
	printf("\t pass = %s\n", opts->transmission.pass);
	printf("Database Protocol = %d\n", opts->databaseProtocol);
	printf("\t host = %s\n", opts->database.host);
	printf("\t port = %s\n", opts->database.port);
	printf("\t user = %s\n", opts->database.user);
	printf("\t pass = %s\n", opts->database.pass);
	printf("\t dirr = %s\n", opts->database.dir);	
	
	//Iniciando protocolo de transmissão
	if(opts->transmissionProtocol == UNDEFINED){
		puts("Undefined Transmission Protocol. The function SAPoTCentral_loop() cannot be used.");
	}
	else if(opts->transmissionProtocol == MQTT){ 
		if(MQTTconnect() != SAPOTCENTRAL_SUCCESS){
			handle->error = ERROR_STARTING_TRANSMISSION_PROTOCOL;
			return SAPOTCENTRAL_FAILURE;
		}
		else puts("\t Connected with MQTT's Broker.");
	}	
	else{
		handle->error = ERROR_SETTING_TRANSMISSION_PROTOCOL;
		return SAPOTCENTRAL_FAILURE;
	} 
	
	/*//Iniciando banco de dados
	if(opts->databaseProtocol == UNDEFINED){
		puts("Undefined Data Base Protocol. The function SAPoTCentral_loop() cannot be used.");
	}
	else if(opts->databaseProtocol == SQL){ 
		if(MYSQLconnect() != SAPOTCENTRAL_SUCCESS){
			handle->error = ERROR_STARTING_DATABASE_PROTOCOL;
			return SAPOTCENTRAL_FAILURE; 
		}
		else puts("\t MYSQL database accessed.");
	}
	else{
		handle->error = ERROR_SETTING_DATABASE_PROTOCOL;
		return SAPOTCENTRAL_FAILURE; 
	
	}*/
		
	return SAPOTCENTRAL_SUCCESS;
}

/**
* [Principal] SAPoTCentral_end
*
*/
void SAPoTCentral_end(){

	//Fechando conexão com o server MQTT
	if(opts->transmissionProtocol) MQTTClient_disconnect(handle->MQTTclient, 10000);	

	//Fechando conexão com o server MYSQL
	if(opts->databaseProtocol) mysql_close(&handle->MYSQLclient);

	//Fechando o descritor de arquivos
	close(fd);
}

/**
* [Principal] SAPoTCentral_loop
*
*/
void SAPoTCentral_loop(){

	int i=0;
	while(handle->inLoop == true){
		if(i==30){ 
			MQTTClient_yield();
			i=0;
		}

		//MQTTconnect();
		sleep(1);
		i++;
	}
}

/**
* [Principal] SAPoTCentral_unpack_message
*
*/
int SAPoTCentral_unpack_message(void* message, int messageLen){

	printf("SAPoTCentral_unpack_message: \n");	
	
	//Apontando para o espaço de memoria
	handle->inMessage = (uint8_t*) message;
	//Estruturando o cabeçalho da mensagem recebida
	handle->header = (SAPoTMessage_header*) handle->inMessage;

	//Alocando e Preenchendo o bufer de log
	bff_log = malloc(100);
	sprintf((char*) bff_log, "ReceivedMsg(V=%d, I=%d, A=%d, S=%d, L=%d, EID=%02x:%02x:%02x:%02x:%02x:%02x)\n", handle->header->version, handle->header->instruction, handle->header->ack, handle->header->serial, handle->header->length, handle->header->emitterId[0], handle->header->emitterId[1], handle->header->emitterId[2], handle->header->emitterId[3], handle->header->emitterId[4], handle->header->emitterId[5]);
	//Escrevendo no arquivo de log
	write(fd, bff_log, strlen(bff_log));
	free(bff_log);
	
	//Verificando a versão do pacote recebido
	if(handle->header->version != SAPOT_PROTOCOL_VERSION){
			handle->error = SAPOT_VERSION_ERROR;
			return SAPOTCENTRAL_FAILURE;
	}
	else{

		printf("\t Version: %d \n", handle->header->version);
		if(handle->header->ack == true) printf("\t ACK true \n");
		printf("\t Instruction: %d \n", handle->header->instruction);
		printf("\t Serial: %d \n", handle->header->serial);
		printf("\t length: %d \n", handle->header->length);
		printf("\t ClientId: %02x:%02x:%02x:%02x:%02x:%02x \n", handle->header->emitterId[0], handle->header->emitterId[1], handle->header->emitterId[2], handle->header->emitterId[3], handle->header->emitterId[4], handle->header->emitterId[5]);
	
		//Registration
		if(handle->header->instruction == 0x00){
		
			handle->registration = (SAPoTMessage_registration*) &handle->inMessage[sizeof(SAPoTMessage_header)];
	
		}
		//Solicitation
		else if(handle->header->instruction >= 0x01 && handle->header->instruction <= 0x03){
	
			handle->solicitation = (SAPoTMessage_solicitation*) &handle->inMessage[sizeof(SAPoTMessage_header)];
	
		}
		//Acess
		else if(handle->header->instruction == 0x04){
	
		}
		//Record
		else if(handle->header->instruction == 0x05){
	
			handle->record = (SAPoTMessage_record*) &handle->inMessage[sizeof(SAPoTMessage_header)];
	
		}
		//Modification
		else if(handle->header->instruction == 0x06){
	
			handle->modification = (SAPoTMessage_modification*) &handle->inMessage[sizeof(SAPoTMessage_header)]; 
			handle->modification->macaddr[17] = '\0';
			handle->modification->label[10] = '\0';
	
		}
	}
	
	return SAPOTCENTRAL_SUCCESS;
}

/**
* [Principal] SAPoTCentral_set_operation 
*
*/
int SAPoTCentral_set_operation(int (*publish)(char*, void*, unsigned int)){

	printf("SAPoTCentral_set_operation:\n");

	unsigned int outMessageLength;

	//Registration
	if(handle->header->instruction == 0x00){
	
		outMessageLength = MYSQLregistration();
		
	}
	//Solicitation 0x01
	else if(handle->header->instruction == 0x01){
	
	}
	//Solicitation 0x02
	else if(handle->header->instruction == 0x02){
	
	}
	//Solicitation 0x03
	else if(handle->header->instruction == 0x03){

		outMessageLength = CTRLactuator(publish);
	
	}
	//Access
	else if(handle->header->instruction == 0x04){
	
		outMessageLength = MYSQLaccess();

	}
	//Record
	else if(handle->header->instruction == 0x05){
	
	}
	//Modification
	else if(handle->header->instruction == 0x06){

		outMessageLength = MYSQLmodification();

	}
	
	//Verifica a existencia de erro na operação realizada	
	if(outMessageLength == SAPOTCENTRAL_FAILURE){
		//Alocando e Preenchendo o bufer de log
		bff_log = malloc(25);
		sprintf((char*) bff_log, "SetOperationError=%d\n", handle->error);
		//Escrevendo no arquivo de log
		write(fd, bff_log, strlen(bff_log));
		free(bff_log); 
		return SAPOTCENTRAL_FAILURE;
	}	
	//Se não houver erro envia a mensagem de resposta (ACK) outMessage para ocliente que solicitou a operação 
	else{
		char topicName[18]; 
		sprintf(topicName, "%02x:%02x:%02x:%02x:%02x:%02x", handle->header->emitterId[0], handle->header->emitterId[1], handle->header->emitterId[2], handle->header->emitterId[3], handle->header->emitterId[4], handle->header->emitterId[5]);
		upper_string(topicName);
		if(publish(topicName, handle->outMessage, outMessageLength) != SAPOTCENTRAL_SUCCESS){
			free(handle->outMessage);
			return SAPOTCENTRAL_FAILURE;
		}
	}

	free(handle->outMessage);
	return SAPOTCENTRAL_SUCCESS;
}

/**
* [Principal] SAPoTCentral_error
*
*/
int SAPoTCentral_error(){

	return handle->error;
}

/**
* [Subrotina] MQTTconnect
*
*/
int MQTTconnect(){

	if(MQTTClient_isConnected(handle->MQTTclient) != true){

		//Caso a conexão entre a central e servidor MQTT seja encerrada, o MQTTclient é destruido e o MQTTConnect é totalmente refeito
		if(MQTTstatus == MQTTCLIENT_SUCCESS){ 
			MQTTClient_destroy(handle->MQTTclient);
			write(fd, "MQTTClient isn't connected, rebuild a MQTTclient and creating this connection\n", strlen("MQTTClient isn't connected, rebuild a MQTTclient and creating this connection\n"));
		}

		printf("MQTTconnect: \n");
	
		//MQTTClient_connectOptions MQTTopts = { {'M', 'Q', 'T', 'C'}, 6, 60, 0, 1, NULL, opts->transmission.user, opts->transmission.pass, 30, 0, NULL, 0, NULL, MQTTVERSION_DEFAULT, {NULL, 0, 0}, {0, NULL}, -1, 0}; //MQTTClient_connectOptions_initializer;
	  	MQTTClient_connectOptions MQTTopts = MQTTClient_connectOptions_initializer;
	  	MQTTopts.keepAliveInterval = 20;
    	MQTTopts.cleansession = 1;
    	MQTTopts.username = opts->transmission.user;
    	MQTTopts.password = opts->transmission.pass;

	  	/* tcp://10.10.40.84:1883 */
	  	char* serverURI = malloc(40);
	  	sprintf(serverURI, "tcp://%s:%s",opts->transmission.host, opts->transmission.port);
	  	printf("\t serverURI: %s\n", serverURI);
	  	
	  	if(MQTTClient_create(&handle->MQTTclient, serverURI, handle->id, MQTTCLIENT_PERSISTENCE_NONE, NULL) != MQTTCLIENT_SUCCESS){
	  		puts("MQTTconnect error: unable to create client\n");
			write(fd, "MQTTconnect error: unable to create the client\n", strlen("MQTTconnect error: unable to create the client\n"));
	  		free(serverURI);
	  		return 0;
	  	}
	  		
	  	free(serverURI);
	  	
	  	puts("\t MQTTClient_create ready.");
	  	
	  	if(MQTTClient_setCallbacks(handle->MQTTclient, NULL, MQTTconnectionLost, MQTTmessageArrived, MQTTdeliveryComplete) != MQTTCLIENT_SUCCESS){
			printf("MQTTconnect error: unable to set call back message\n");
			write(fd, "MQTTconnect error: unable to set call back message\n", strlen("MQTTconnect error: unable to set call back message\n"));
			return 0;
		}
		
	 	puts("\t MQTTClient_setCallbacks ready.");
	 
	   	if((MQTTstatus = MQTTClient_connect(handle->MQTTclient, &MQTTopts)) != MQTTCLIENT_SUCCESS){
		   	printf("MQTTconnect error: unable to connect with broker\n");
		   	bff_log = malloc(70);
		   	sprintf(bff_log, "MQTTconnect error (%d): unable to connect with broker\n", MQTTstatus);
			write(fd, bff_log, strlen(bff_log));
			free(bff_log);
	  		return 0;
	   	}
	   	
	   	puts("\t MQTTClient_connect ready.");
	   	
	   	if(MQTTClient_subscribe(handle->MQTTclient, handle->id, 0) != MQTTCLIENT_SUCCESS){
			printf("MQTTconnect error: unable to subscribe on topic %s\n", handle->id);
			bff_log = malloc(70);
			sprintf(bff_log, "MQTTconnect error: unable to subscribe on topic %s\n", handle->id);
			write(fd, bff_log, strlen(bff_log));
			free(bff_log);
			return 0;
		}
		
		puts("\t MQTTClient_subscribe ready.");
		
	 
	}  
	
	return SAPOTCENTRAL_SUCCESS; 	
}

/**
* [Subrotina] MQTTmessageArrived
*
*/
int MQTTmessageArrived(void* context, char* topicName, int topicLen, MQTTClient_message* MQTTmsg){


	if(SAPoTCentral_unpack_message(MQTTmsg->payload, MQTTmsg->payloadlen) != SAPOTCENTRAL_SUCCESS){
		printf("MQTTmessageArrived error: unable to unpack SAPoT's message (%d)\n", handle->error);
	}
	else{
		
		if(SAPoTCentral_set_operation(MQTTpublish) != SAPOTCENTRAL_SUCCESS){
			printf("MQTTmessageArrived error: unable to set operation on SAPoTCentral (%d)\n", handle->error);
		}	
		
	}

	handle->error = SAPOTCENTRAL_SUCCESS;
	MQTTClient_freeMessage(&MQTTmsg);
    MQTTClient_free(topicName);
	
	return 1;
}

/**
* [Subrotina] MQTTdeliveryComplete
*
*/
void MQTTdeliveryComplete(void* context, MQTTClient_deliveryToken dt){

	printf("Message with token value %d delivery confirmed\n", dt);
	MQTTdeliveredtoken = dt;

	//Escrevendo no arquivo de log
	bff_log = malloc(55);
	sprintf(bff_log, "Message with token value %d delivery confirmed\n", dt);
	write(fd, bff_log, strlen(bff_log));
	free(bff_log);
}

/**
* [Subrotina] MQTTconnectionLost
*
*/
void MQTTconnectionLost(void* context, char* cause){

	printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);

    //Escrevendo no arquivo de log
	bff_log = malloc(500);
	sprintf(bff_log, "Connection lost: %s\n", cause);
	write(fd, bff_log, strlen(bff_log));
	free(bff_log);
}

/**
* [Subrotina] MQTTpublish
*
*/
int MQTTpublish(char* topic, void* payload, unsigned int payloadLen){

	printf("MQTTPublish on topic: %s\n", topic);
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    pubmsg.payload = payload;
    pubmsg.payloadlen = payloadLen;
    pubmsg.qos = 0;
    pubmsg.retained = 0;
    MQTTClient_deliveryToken token;
    
   	if(MQTTClient_publishMessage(handle->MQTTclient, topic, &pubmsg, &token) != MQTTCLIENT_SUCCESS){
   		printf("\t MQTTpublish error: unable to publish the message\n");
   		handle->error = ERROR_MQTT_PUBLISH;
   		return SAPOTCENTRAL_FAILURE;
   	}
   	else{ 
    	MQTTClient_waitForCompletion(handle->MQTTclient, token, 1000L);
    	printf("\t Published \n");
    	return SAPOTCENTRAL_SUCCESS;
    }	
}

/**
* [Subrotina] MYSQLconnect
*
*/
int MYSQLconnect(){

	printf("MYSQLconnect: \n");
		
	//Inicializa o cliente SQL
	if(mysql_init(&handle->MYSQLclient) == NULL){
		printf("MySQL Erro(%d): %s\n", mysql_errno(&handle->MYSQLclient), mysql_error(&handle->MYSQLclient));
		mysql_close(&handle->MYSQLclient);
		return SAPOTCENTRAL_FAILURE;
	}
	
	puts("\t mysql_init ready.");
	
	//Conecta o cliente ao servidor SQL. 
	if(mysql_real_connect(&handle->MYSQLclient, opts->database.host, opts->database.user, opts->database.pass, opts->database.dir, 0, NULL, 0 ) == NULL){
		printf("MySQL Erro(%d): %s\n", mysql_errno(&handle->MYSQLclient), mysql_error(&handle->MYSQLclient));		
		mysql_close(&handle->MYSQLclient);
		return SAPOTCENTRAL_FAILURE; 
	}
	
	puts("\t mysql_real_connect ready.");
	
	return SAPOTCENTRAL_SUCCESS;
}

/**
* [Subrotina] MYSQLregistration
*
*/
int MYSQLregistration(){

	//Abrindo conexão com o banco de dados
	if(opts->databaseProtocol == SQL){ 
		if(MYSQLconnect() != SAPOTCENTRAL_SUCCESS){
			handle->error = ERROR_STARTING_DATABASE_PROTOCOL;
			return SAPOTCENTRAL_FAILURE; 
		}
	}	

	puts("MYSQLregistration: ");

	char query[200] = {};
	int querylen = 0;
	char newId[18];
	/** Estrutura que representa o resultado de uma query solicitada */
  	MYSQL_RES* sqlResult;
	/** Dá acesso a uma linha do resultado de uma query solicitada */
	MYSQL_ROW sqlRow;	
	
	//Formatando o id do possível novo cliente
	sprintf(newId, "%02x:%02x:%02x:%02x:%02x:%02x", handle->header->emitterId[0], handle->header->emitterId[1], handle->header->emitterId[2], handle->header->emitterId[3], handle->header->emitterId[4], handle->header->emitterId[5]);
	upper_string(newId);
	printf("\t newId = %s\n", newId);
	
	//Formatando a query para verificar se o novo cliente já está na tabela tb_cadastrados
	querylen = sprintf(query, "SELECT id FROM tb_cadastrados WHERE macaddr like '%s';", newId);
	printf("\t query = %s\n", query);
	printf("\t querylen = %d\n", querylen);
	
    //Solicitando a query ao servidor 
	if( mysql_real_query(&handle->MYSQLclient, (const char*) query, (unsigned int) querylen) != 0 ){
		printf("MYSQL Erro(%d): %s\n", mysql_errno(&handle->MYSQLclient), mysql_error(&handle->MYSQLclient));
		handle->error =  ERROR_DATABASE_INQUIRY;
		mysql_close(&handle->MYSQLclient);
		return SAPOTCENTRAL_FAILURE; 
	}
		
	//sqlResult recebe o retorno da query solicitada ao banco de dados:
	sqlResult = mysql_store_result(&handle->MYSQLclient);
	//Se o cliente já está cadastrados o retorno da query diferente de null
	//Então o sqlRow receberá o id em que ele foi cadastrado na tabela tb_cadastrados.
	if((sqlRow = mysql_fetch_row(sqlResult)) != NULL){
		//formatando uma query de update para o cliente que já está cadastrado.
		querylen = sprintf(query, "UPDATE tb_cadastrados SET type='%d', sensor='%d', actuator='%d' WHERE id='%s';", handle->registration->clientType, handle->registration->sensorQuantity, handle->registration->actuatorQuantity, sqlRow[0]);
	}
	//Contudo, se o cliente não está cadastrados o retorno da query será falso 
	else{
		//formatando uma query para inserir o novo cliente
		querylen = sprintf(query, "INSERT tb_cadastrados(label, macaddr, type, sensor, actuator) VALUES('xxxxxxxxxx', '%s', '%d', '%d', '%d');", newId, handle->registration->clientType, handle->registration->sensorQuantity, handle->registration->actuatorQuantity);
	}
	
	printf("\t query = %s\n", query);
	printf("\t querylen = %d\n", querylen);
	
	//Solicita ao servidor a query de atualização do cliente ja existente ou a inserção do novo cliente  
	if(mysql_real_query(&handle->MYSQLclient, (const char*) query, querylen) != 0){
		printf("MYSQL Erro(%d): %s\n", mysql_errno(&handle->MYSQLclient), mysql_error(&handle->MYSQLclient));
		handle->error =  ERROR_DATABASE_INQUIRY;
		mysql_close(&handle->MYSQLclient);
		return SAPOTCENTRAL_FAILURE; 
	}

	//Livrando o espaço de memória do resultado da query
	mysql_free_result(sqlResult);

	//Definindo a mensagem de resposta
	int outMessageLength = sizeof(SAPoTMessage_header);
	handle->outMessage = malloc(outMessageLength);
	SAPoTMessage_header* header = (SAPoTMessage_header*) handle->outMessage;
	header->version = SAPOT_PROTOCOL_VERSION;
	header->ack = 1;
	header->rsv1 = 0;
	header->rsv2 = 0;
	header->rsv3 = 0;
	header->instruction = handle->header->instruction;
	header->serial = handle->header->serial;
	header->length = outMessageLength;
	getmacID((const char*) handle->id, header->emitterId); 

	//Fechando conexão com o banco de dados
	mysql_close(&handle->MYSQLclient);
	
	//Retornando o tamanho da mensagem a ser publicada
	return outMessageLength;
}

/**
* [Subrotina] MYSQLmodification
*
*/
int MYSQLmodification(){

	//Abrindo conexão com o banco de dados
	if(opts->databaseProtocol == SQL){ 
		if(MYSQLconnect() != SAPOTCENTRAL_SUCCESS){
			handle->error = ERROR_STARTING_DATABASE_PROTOCOL;
			return SAPOTCENTRAL_FAILURE; 
		}
	}

	char query[100] = {};
	int querylen = 0;
	
	printf("MYSQLmodification: \n");
	//Formatando o id do cliente a ser etiquetado
	upper_string((char*)handle->modification->macaddr);
	printf("\t macaddr = %s\n", handle->modification->macaddr);
	printf("\t label = %s\n", handle->modification->label);

	//formatando uma query de update para etiquetar o cliente que já está cadastrado.
	querylen = sprintf(query, "UPDATE tb_cadastrados SET label='%s' WHERE macaddr='%s';", handle->modification->label, handle->modification->macaddr);
	printf("\t query = %s\n", query);
	printf("\t querylen = %d\n", querylen);
	
	//Solicita ao servidor a query de atualização do cliente ja existente ou a inserção do novo cliente  
	if(mysql_real_query(&handle->MYSQLclient, (const char*) query, querylen) != 0){
		printf("MYSQL Erro(%d): %s\n", mysql_errno(&handle->MYSQLclient), mysql_error(&handle->MYSQLclient));
		handle->error =  ERROR_DATABASE_INQUIRY;
		mysql_close(&handle->MYSQLclient);
		return SAPOTCENTRAL_FAILURE; 
	}

	//Alocando memória para a mensagem de retorno.
	int outMessageLength = sizeof(SAPoTMessage_header); 
	handle->outMessage = malloc(outMessageLength);

	//Preenchendo o cabeçalho fixo
	SAPoTMessage_header* header = (SAPoTMessage_header*) handle->outMessage;
	header->version = SAPOT_PROTOCOL_VERSION;
	header->ack = 1;
	header->rsv1 = 0;
	header->rsv2 = 0;
	header->rsv3 = 0;
	header->instruction = handle->header->instruction;
	header->serial = handle->header->serial;
	header->length = outMessageLength;
	getmacID((const char*) handle->id, header->emitterId);

	//Fechando conexão com o banco de dados
	mysql_close(&handle->MYSQLclient);

	return outMessageLength;	
}

/**
* [Subrotina] MYSQLacess
*
*/
int MYSQLaccess(){

	//Abrindo conexão com o banco de dados
	if(opts->databaseProtocol == SQL){ 
		if(MYSQLconnect() != SAPOTCENTRAL_SUCCESS){
			handle->error = ERROR_STARTING_DATABASE_PROTOCOL;
			return SAPOTCENTRAL_FAILURE; 
		}
	}

	printf("MYSQLaccess:\n");

	char query[] = "SELECT * FROM tb_cadastrados;";
	int querylen = strlen(query);

	//Estrutura que representa o resultado de uma query solicitada 
  	MYSQL_RES* sqlResult;
	//Dá acesso as linhas do resultado de uma query solicitada 
	MYSQL_ROW sqlRow;

	printf("\t query = %s\n", query);

	//Solicita ao servidor uma query de consulta sobre as informações da tabela tb_cadastrados  
	if(mysql_real_query(&handle->MYSQLclient, (const char*) query, querylen) != 0){
		printf("MYSQL Erro(%d): %s\n", mysql_errno(&handle->MYSQLclient), mysql_error(&handle->MYSQLclient));
		handle->error =  ERROR_DATABASE_INQUIRY;
		mysql_close(&handle->MYSQLclient);
		return SAPOTCENTRAL_FAILURE; 
	}

	//sqlResult recebe o retorno da query solicitada ao banco de dados
	sqlResult = mysql_store_result(&handle->MYSQLclient);
	if(sqlResult == NULL){
		printf("MYSQL Erro(%d): %s\n", mysql_errno(&handle->MYSQLclient), mysql_error(&handle->MYSQLclient));
		handle->error =  ERROR_DATABASE_INQUIRY;
		mysql_close(&handle->MYSQLclient);
		return SAPOTCENTRAL_FAILURE; 		
	}

	//Conta a quantidade de linhas de registro existentes na tabela tb_cadastrados, para definir o tamanho do retorno desta operação.
	int rowQuantity = 0;
	while((sqlRow = mysql_fetch_row(sqlResult)) != NULL) rowQuantity++;

	//Reinicia o ponteiro para o início do sqlResult
	mysql_data_seek(sqlResult, 0);
		
	printf("\t rowQuantity = %d\n", rowQuantity);

	//Alocando memória para a mensagem de retorno.
	int outMessageLength = sizeof(SAPoTMessage_header) + (rowQuantity*sizeof(SAPoTMessage_access)); 
	handle->outMessage = malloc(outMessageLength);

	//Preenchendo o cabeçalho fixo
	SAPoTMessage_header* header = (SAPoTMessage_header*) handle->outMessage;
	header->version = SAPOT_PROTOCOL_VERSION;
	header->ack = 1;
	header->rsv1 = 0;
	header->rsv2 = 0;
	header->rsv3 = 0;
	header->instruction = handle->header->instruction;
	header->serial = handle->header->serial;
	header->length = outMessageLength;
	getmacID((const char*) handle->id, header->emitterId);


	//Ponteiro do tipo SAPoTMessage_access para auxiliar na concatenação das informações de payload
	SAPoTMessage_access* access;

	//Retirando as informações do banco de dados
	int i=0;
	while((sqlRow = mysql_fetch_row(sqlResult)) != NULL){
		access = (SAPoTMessage_access*) (handle->outMessage + sizeof(SAPoTMessage_header) + (i*sizeof(SAPoTMessage_access)));
		strncpy((char*)access->label, (char*)sqlRow[1], (size_t) 10);
		strncpy((char*)access->id, (char*)sqlRow[2], (size_t) 17);
		access->id[17] = 0;
		access->type = (uint16_t*) atoi(sqlRow[3]);
		access->sensor = (uint8_t*) atoi(sqlRow[4]);
		access->actuator = (uint8_t*) atoi(sqlRow[5]);
		i++;	
	}

	//Limpa os resultados da query anterior
	mysql_free_result(sqlResult);

	//Fechando conexão com banco de dados
	mysql_close(&handle->MYSQLclient);

	printf("antes do retorno de MYSQLaccess\n");

	return outMessageLength;	
}

/**
* [Controle de Clientes] CTRLactuator 
*
*/
int CTRLactuator(int (*publish)(char*, void*, unsigned int)){

	//Abrindo conexão com o banco de dados
	if(opts->databaseProtocol == SQL){ 
		if(MYSQLconnect() != SAPOTCENTRAL_SUCCESS){
			handle->error = ERROR_STARTING_DATABASE_PROTOCOL;
			return SAPOTCENTRAL_FAILURE; 
		}
	}

	printf(" CTRLactuator: \n");

	char query[100] = {};
	int querylen = 0;
	char macaddr[18] = {};
	SAPoTMessage_header* header;
	int outMessageLength;
	/** Estrutura que representa o resultado de uma query solicitada */
  	MYSQL_RES* sqlResult;
	/** Dá acesso a uma linha do resultado de uma query solicitada */
	MYSQL_ROW sqlRow;

	//Verifica no banco de dados qual é o macaddr referente à label recebida via SAPoTMessage_solicitation
	printf("\t label = %s\n", handle->solicitation->label);
	querylen = sprintf(query, "SELECT macaddr FROM tb_cadastrados WHERE label like '%s';", handle->solicitation->label);
	printf("\t query = %s\n", query);
	printf("\t querylen = %d\n", querylen);

	//Solicitando a query ao servidor 
	if(mysql_real_query(&handle->MYSQLclient, (const char*) query, (unsigned int) querylen) != 0 ){
		printf("MYSQL Erro(%d): %s\n", mysql_errno(&handle->MYSQLclient), mysql_error(&handle->MYSQLclient));
		handle->error =  ERROR_DATABASE_INQUIRY;
		mysql_close(&handle->MYSQLclient);
		return SAPOTCENTRAL_FAILURE; 
	}

	//sqlResult recebe o retorno da query solicitada ao banco de dados:
	sqlResult = mysql_store_result(&handle->MYSQLclient);

	//Se a label está cadastrada no banco de dados o retorno da query será verdadeiro (result == 1). 
	//Então o sqlRow receberá o macaddr que refere-se a label.
	if((sqlRow = mysql_fetch_row(sqlResult)) != NULL){

		//Insere o sqlRow em macaddr
		strncpy(macaddr, sqlRow[0], 17);
		upper_string(macaddr);
		printf("\t macaddr = %s\n", macaddr); 

		//Livrando o espaço de memória do resultado da query
		mysql_free_result(sqlResult);

		//aloca espaço de memória para uma solicitação do tipo SAPoTMessage_actuatorDrive 
		int msglen = sizeof(SAPoTMessage_header) + sizeof(SAPoTMessage_actuatorDrive);
		void* msg = malloc(msglen);

		//preenchendo o cabeçalho fixo
		header = (SAPoTMessage_header*) msg;
		header->version = SAPOT_PROTOCOL_VERSION;
		header->ack = 0;
		header->rsv1 = 0;
		header->rsv2 = 0;
		header->rsv3 = 0;
		header->instruction = 3;
		header->serial = ++handle->serial;
		header->length = msglen;
		getmacID((const char*) handle->id, header->emitterId);

		//preenchendo o payload
		SAPoTMessage_actuatorDrive* actuatorDrive = (SAPoTMessage_actuatorDrive*) (msg + sizeof(SAPoTMessage_header));
		actuatorDrive->actuatorId = handle->solicitation->sensorOrActuatorId;
		actuatorDrive->timeSet = handle->solicitation->timeSet;
		actuatorDrive->degreeOfPerformance = handle->solicitation->degreeOfPerformance;

		if(publish(macaddr, msg, msglen) != SAPOTCENTRAL_SUCCESS){
			free(msg);
			return SAPOTCENTRAL_FAILURE;
		}

		free(msg);

	}else{

		printf("\t Label não cadastrada!\n");
		handle->error = ERROR_LABEL_NOT_REGISTERED;
		//Fechando conexão com banco de dados
		mysql_close(&handle->MYSQLclient);
		return SAPOTCENTRAL_FAILURE;  

	}

	//alocando espaço de memoria para o ack ao usuário que solicitou o acionamento do atuador
	outMessageLength = sizeof(SAPoTMessage_header);
	handle->outMessage = malloc(outMessageLength);
	header = (SAPoTMessage_header*) handle->outMessage;
	header->version = SAPOT_PROTOCOL_VERSION;
	header->ack = 1;
	header->rsv1 = 0;
	header->rsv2 = 0;
	header->rsv3 = 0;
	header->instruction = handle->header->instruction;
	header->serial = handle->header->serial;
	header->length = outMessageLength;
	getmacID((const char*) handle->id, header->emitterId);

	//Fechando conexão com banco de dados
	mysql_close(&handle->MYSQLclient);

	return outMessageLength;	
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
