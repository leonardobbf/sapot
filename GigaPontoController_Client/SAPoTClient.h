#ifndef SAPOTCLIENT_H
#define SAPOTCLIENT_H


								/************************* Headers ******************************/
								
#include <stdio.h>
#include <stdint.h>
#include <MQTTClient.h>


								/************************* Defines ******************************/

/**
* Indicador de Versão 
*
*/
#define SAPOT_PROTOCOL_VERSION 1
/**
* Código de Retorno: Indica sucesso em alguma operação sobre o SAPoTClient
*/
#define SAPOTCLIENT_SUCCESS 0
/**
* Código de Retorno: Indica fracasso em alguma operação sobre o SAPoTClient 
*
*/
#define SAPOTCLIENT_FAILURE -1
/**
* Código de Retorno: Indica que o pacote recebido está fora de versão 
*
*/
#define SAPOT_VERSION_ERROR -2
/**
* Código de Retorno: Indica fracasso em alguma operação sobre o bando de dados 
*
*/
#define ERROR_SETTING_DATABASE_PROTOCOL -3
/**
* Código de Retorno: Indica fracasso em definir o protocolo de transmissão 
*
*/
#define ERROR_SETTING_TRANSMISSION_PROTOCOL -4
/**
* Código de Retorno: Indica fracasso ao iniciar o cliente de transmissão 
*
*/
#define ERROR_STARTING_TRANSMISSION_PROTOCOL -5
/**
* Código de Retorno: Indica fracasso ao iniciar o cliente de banco de dados 
*
*/
#define ERROR_STARTING_DATABASE_PROTOCOL -6
/**
* Código de Retorno: Indica fracasso ao realizar uma query ao banco de dados 
*
*/
#define ERROR_DATABASE_INQUIRY -7
/**
* Código de Retorno: Indica fracasso ao publicar em um tópico no broker MQTT 
*
*/
#define ERROR_MQTT_PUBLISH -8


/**
* Protocolo Indefinido: Indica que o usuário irá utilizar um protocolo não padronizado na SAPoTClient.h.
* 
*/
#define UNDEFINED 0

/**
* Protocolo de Transmissão: Indica que o protocolo de transmissão que será usado pela cliente será o MQTT
*
*/
#define MQTT 1



					/************************* Structs for SAPoTMessage *************************/

/**
* Estrutura: Cabeçalho fixo das mensagem SAPoT 
*
*/
typedef struct{
	
	/** Flags de definição da mensagem */
	uint8_t rsv3:1, rsv2:1, rsv1:1, ack:1, version:4;

  	/** Instrução de operação 
  	* 0x00: Cadastro de um novo Cliente (Registration) 
  	* 0x01: Requsição de todos os sensores no cliente (Solicitation)
  	* 0x02: Requisição de um sensor em específico no cliente (Solicitation)
  	* 0x03: Acionamento de um atuador em específico no cliente (Solicitation)
  	* 0x04: Acesso à informação dos clientes cadastrados (Acess)
  	* 0x05: Registro de informação proveniente de sensores e atuadores (Record)
  	* 0x06: Etiquetagem de um cliente ja cadastrado (Modification)  
  	**/
  	uint8_t instruction;
  	
  	/** Número serial da mensagem. Refere-se a origem */
  	uint16_t serial;
  	
  	/** Comprimento total da mensagem (Header + Payload) */
  	uint16_t length;
  	
	/** Identificador do Cliente emissor da mensagem */
  	uint8_t emitterId[6];
  	
}SAPoTMessage_header;

/**
* Estrutura: Payload para cadastro de novo cliente
*
*/
typedef struct{

	/** Tipo de Cliente */
  	uint16_t clientType;
  	
  	/** Quantidade de sensores em operação no Cliente */ 
  	uint8_t sensorQuantity;
  	
  	/** Quantidade atuadores em operação no Cliente */ 
  	uint8_t actuatorQuantity;
  	
  	/** Vetor identificador dos tipos de sensores e atuadores, respectivamente, em operação no Cliente */ 
  	uint16_t* sensorActuatorType;
  	 
}SAPoTMessage_registration;

/**
* Estrutura: Payload para solicitar o acionamento de alguma cliente cadastrado
* Utilizado pelo usuário.
*
*/
typedef struct{

	/** Etiqueta do cliente cadastrado que será acionado pelo usuário */
  	uint8_t label[11];
  	
  	/** Identificador do Sensor ou do atuador */
  	uint16_t sensorOrActuatorId;
  	
  	/** Tempo de requisição do sensor ou tempo de acionamento do atuador */ 
  	uint16_t timeSet;
  	
  	/** Grau de performace do atuador*/ 
  	uint16_t degreeOfPerformance; 
  	 
}SAPoTMessage_solicitation;

/**
* Estrutura: Payload para acesso à informação dos clientes cadastrados
*
*/
typedef struct{

	/** Etiqueta referente ao cadastrado */
	uint8_t label[10];

	/** Identificador Macaddr */
	uint8_t id[18];

	/** Tipo de Cliente */
	uint16_t type;

	/** Quantidade de sensores */ 
  	uint8_t sensor;
  	
  	/** Quantidade atuadores */ 
  	uint8_t actuator;
  	 
}SAPoTMessage_access;

/**
* Estrutura: Payload para registro de informação dos clientes (informação dos Sensores, status dos Atuadores e possiveis erros)
*
*/
typedef struct{


	/** Tamanho referente à quantidade e informação em data  */ 
  	uint8_t dataLen;
  	
	/** Ponteiro identificador da informação a ser guardada no bando de dados. 
	* 	Se SAPoTMessage_header.instruction for :
	*  0x01: (float*) data
	*  0x02: (float) data
	*  0x03: (uint8_t) data
	**/
  	void* data;
  	 
}SAPoTMessage_record;

/**
* Estrutura: Payload para modificação das informações de cadastro dos clientes
* Utilizada para colocar uma etiqueta no cliente cadastrado, a partir de um clientId.	
*
*/
typedef struct{

	/** Identificador do clientId que será etiquetado */
  	char macaddr[18];
  	
  	/** Etiqueta */ 
  	char label[11];
  	 
}SAPoTMessage_modification;

							/************************* Structs for SAPoTClient *************************/

/**
* Estrutura: Classe de um objeto para definir as opções de criação de um cliente SAPoT
*
*/
typedef struct{

	/** Identificador da central a qual o cliente responde */
	char* centralId;

	/** identificador de protocolo de transmissão (MQTT, TCP, UDP ou HTTP) */
	uint8_t transmissionProtocol;
	
	/** Objeto referente as informações do servidor de transmissão de mensagens */
	struct{
		
		char* host; /** Endereço do servidor de transmissão */
		
		char* port; /** Porta de acesso ao servidor de transmissão */
		
		char* user; /** Usuário para acessar o servidor de transmissão */
		
		char* pass; /** Senha para acessar o servidor de transmissão */
		
	}transmission;
	
	
}SAPoTClient_create_options;

/**
* Estrutura: Classe de um objeto SAPoTClient. É o principal objeto para manipulação de um cliente SAPoT.
*
*/
typedef struct{

	/** identificador do Client no formato macaddr (xx:xx:xx:xx:xx:xx) */
	const char* id;
	
	/** Indicador de Erro */
	int error;

	/** Flag que permite a execução da SAPoTClient_loop() */
	bool inLoop;
		
	/** Ponteiro indicador da mensagem recebida */
	uint8_t* inMessage;
	
	/** Ponteiro indicador da mensagem a ser enviada */
	void* outMessage;
	
	/** Cabeçalho da mensagem recebida */
	SAPoTMessage_header* header;
	
	/** Objeto referente ao cliente MQTT*/
	MQTTClient MQTTclient;
	
	
}SAPoTClient;


/************************* Functions for SAPoTClient *************************/

/** 
* Função: Inicia o cliente SAPoT preenchendo as informações no objeto SAPoTClient
*
*/
int SAPoTClient_begin(SAPoTClient* userPointer , SAPoTClient_create_options* opts, const char* clientId);

/** 
* Função: Finaliza o client SAPoT encerrando o serviço de transmissão de dados MQTT 
*
*/
void SAPoTClient_end();

/** 
* Função: Coordena a rotina de execução de uma UCCSAPoT de acordo com as opções definidas no 
* objeto SAPoTClient_create_options. É o meio mais fácil e simples de se implementar uma UCCSAPoT. 
*
*/
int SAPoTClient_loop(); 

/**
* Função: Traduz uma mensagem SAPoT e a armazena no espaço de memoria do objeto SAPoTClient
*
*/
int SAPoTClient_unpack_message(void* message, int messageLen);

/**
* Função: A partir da mensagem traduzida em SAPoTClient_unpack_message() a função SAPoTClient_set_operation()
* realiza as operações indicadas pela instrução recebida
*
*/
int SAPoTClient_set_operation(int (*publish) (char*, void*, int));

/**
* Função: Retorna o numero do erro referente a alguma operação mal sucedida sobre o objeto SAPoTClient. 
*
*/
int SAPoTClient_error();

/**
* Função: Printa para o usuário a mensagem de resposta da central referente a solicitação de acesso ao banco de dados 
*
*/
void SAPoTClient_printAcess();


/************************* Functions for MQTT **************************/
/**
* Função: Conecta o cliente ao servidor MQTT 
*
*/
int MQTTconnect();

/**
* Função: Define a rotina de recebimento de mensagens via MQTT 
*
*/
int MQTTmessageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *MQTTmsg);

/**
* Função: Publica a mensagem em um topico especifico no broker MQTT
*
*/
int MQTTpublish(char* topic, void* payload, int payloadLen);
					

/************************* Utility Functions **************************/

/**
* Função: Deixa as letras de uma string em caixa alta
*/
void upper_string(char s[]);

/**
* Função: Transforma o endereço MAC (string) em ID(uint8_t*) 
*
*/
void getmacID(const char* MAC, uint8_t ID[6]);


#endif
