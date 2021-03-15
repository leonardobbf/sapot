/**************************************************** Bibliotecas ************************************************/

#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

/********************************************* Defines ***************************************/

#define TRUE 1
#define FALSE 0
//#define CMD_TOPIC "GC/CMD"
//#define THING_TOPIC "GC/things/"
//Versão
#define SAPOT_VERSION 1
//Thing Type
#define CMD 0x00
#define SMCAI 0x01
#define SMCAE 0x02
#define SMRE 0x03
#define SMRH 0x04
#define SRF 0x05 
//Sensor Type
#define TMP 0x0000
#define UMD 0x0001 
#define LUX 0x0002 
#define DTP 0x0003
#define CAM 0x0004
#define VCA 0x0005
#define IAC 0x0006
#define PAC 0x0007
#define FP 0x0008
#define KWH 0x0009
//Actuator Type
#define CAC 0x0000
#define CDC 0x0001
//Request Type
#define ALL 0x0000
#define NON 0xffff

/************************************************ Estrutura de dados *********************************************/

/* SAPoT Client: Classe de um Cliente SAPoT  */
typedef struct{
  uint16_t serial; //Número serial identificador de processos realizados pelo cliente
  uint8_t id[6]; // Número identificador do cliente
  uint8_t type; //Tipo do cliente
  const char* centralID; //Identificador da Central de comandos
  uint8_t status; //Identifica a situação do cliente
  uint16_t sensorRequestID; //Identifica qual sensor será requisitado
  uint32_t sensorRequestTime; //Tempo para próxima requisição do sensor
  uint16_t actuatorRequestID; //Identifica qual atuador será acionado
  uint32_t actuatorRequestTime; //Tempo que o atuador permanece em execução 
  uint16_t actuatorRequestDegree; //Grau de atuação requerido para acionamento do atuador
  uint16_t* sensorVector; //Ponteiro para o vetor de sensores
  uint16_t* actuatorVector; //Ponteiro para o vetor de atuadores
  uint8_t amountOfSensors; //Quantidade de Sensores instalados no cliente
  uint8_t amountOfActuators; //Quantidade de Atuadores instalados no cliente
  unsigned long currentTime; //Tempo atual
  void* previousTime; //Tempo anterior
}SAPoTClient;

/* SAPoT Network: Variáveis para atuação na rede SAPoT */
typedef struct{
  const char* WiFi_ssid;  
  const char* WiFi_pass;
  const char* MQTT_server; 
  int MQTT_port;
  const char* MQTT_user;
  const char* MQTT_pass; 
}SAPoTNetwork;

/* SAPoT Message: Variáveis para manipulação de um pacote SAPoT*/
typedef struct{
  uint16_t length; //tamanho da mensagem
  void* data; //Dados da mensagem 
}SAPoTMessage;

/* Cabeçalho fixo */
typedef struct{
  uint8_t rsv3:1, rsv2:1, rsv1:1, ack:1, version:4; //rsv3 menos significativo e Version mais significativo
  uint8_t instruction;
  uint16_t serial;
  uint16_t length;
  uint8_t client_id[6];
}fixed_header_SAPoT;

/* Comando 00: Registro de nova coisa */
typedef struct{
  uint16_t thing_type; //Tipo de coisa
  uint8_t sensor_quantity; //Quantidade de sensores
  uint8_t actuator_quantity; //Quantidade de atuadores
  uint16_t sensor_actuator_type[]; //Array contendo os tipos de Sensores e atuadores
}registration_SAPoT;

/* ACK 00: Confirmação de registro (payload nulo)*/
typedef struct{
}registration_ack_SAPoT;

/* Comando 01: Requisição dos dados de todos os sensores disponíveis  */
typedef struct{
  uint16_t get_time;
}request_of_all_SAPoT;

/* ACK 01: Informações de todos os sensores disponíveis */
typedef struct{
  float sensors_info[]; 
}request_of_all_ack_SAPoT;

/* Comando 02: Requisição do dado de um sensor em específico */
typedef struct{
  uint16_t sensor_type;
  uint16_t get_time;
}sensor_request_SAPoT;

/* ACK 02: Informação do sensor em específico */
typedef struct{
  float sensor_info;
}sensor_request_ack_SAPoT;

/* Comando 03: Acionamento de um atuador em específico */ 
typedef struct{
  uint16_t actuator_type; 
  uint16_t time_set; 
  uint16_t degree_of_performance; 
}actuator_drive_SAPoT;

/* ACK03: Confirmação do acionamento do atuador em específico*/
typedef struct{
}actuator_drive_ack_SAPoT;


/************************************************ Objetos e Variáveis *************************************************************/

/* Objetos SAPoT */
SAPoTClient SAPoTclient;
SAPoTNetwork SAPoTnetwork;
SAPoTMessage SAPoTmessage;
unsigned long* sensorPreviousTime;
unsigned long* actuatorPreviousTime;
uint8_t actuatorTimeCounter;

/* Objetos de acesso a rede (Wifi e MQTT) */
WiFiClient WiFiclient;
PubSubClient MQTTclient(WiFiclient);

/************************************************ Subrotinas ***********************************************************/

/*
 * Função: Inicia cliente SAPoT
 *  @parâmetros: tipo de cliente SAPoT, Quantidade de sensores, Quantidade de atuadores, vetor de sensores, vetor de atuadores e UTF offset regional.
 *  @retorno: TRUE se o Cliente for iniciado com sucesso, se não retorna FALSE.
 */
bool SAPoTClient_begin(const char* central_id, uint8_t type, uint8_t howManySensors, uint8_t howManyActuators, uint16_t* whichSensors, uint16_t* whichActuators){

  //Verificando qual tipo de cliente será inicializado.
  if(type == CMD)Serial.println("SAPoTClient_begin: Starting a CMD"); 
  else if(type == SMCAI)Serial.println("SAPoTClient_begin: Starting a SMCAI");
  else if(type == SMCAE)Serial.println("SAPoTClient_begin: Starting a SMCAE");
  else if(type == SMRE)Serial.println("SAPoTClient_begin: Starting a SMRE");
  else if(type == SMRH)Serial.println("SAPoTClient_begin: Starting a SMRH");
  else if(type == SRF)Serial.println("SAPoTClient_begin: Starting a SRF");
  else{
    Serial.println("SAPoTClient_begin: client type not found !");
    SAPoTclient.type = 0xff;
    return FALSE;
  }
    
  //Preenchendo as informações do objeto global SAPoTclient
  SAPoTclient.currentTime = millis();
  SAPoTclient.previousTime = malloc((howManySensors + howManyActuators) * sizeof(unsigned long));
  SAPoTclient.serial = 0;
  getmacID(WiFi.macAddress().c_str(), SAPoTclient.id); //A função getmacID transforma o macaddr da placa em um identificador SAPoT
  SAPoTclient.centralID = central_id;
  SAPoTclient.type = type;
  SAPoTclient.status = 0;
  SAPoTclient.sensorRequestTime = 0; 
  SAPoTclient.actuatorRequestTime = 0; 
  SAPoTclient.sensorRequestID = NON; 
  SAPoTclient.actuatorRequestID = NON; 
  SAPoTclient.sensorVector = whichSensors; 
  SAPoTclient.actuatorVector = whichActuators;
  SAPoTclient.amountOfSensors = howManySensors;
  SAPoTclient.amountOfActuators = howManyActuators;

  //Inicializando os vetores temporais para o controle dos sensores e atuadores.
  sensorPreviousTime = (unsigned long*) SAPoTclient.previousTime;
  actuatorPreviousTime = (unsigned long*) (SAPoTclient.previousTime + (howManySensors * sizeof(unsigned long)));
  int i;
  for(i=0; i<howManySensors; i++) sensorPreviousTime[i] = 0;
  for(i=0; i<howManyActuators; i++) actuatorPreviousTime[i] = 0;
  actuatorTimeCounter = 0;

  Serial.println();
  Serial.println("SAPoTClient:");
  Serial.println("\t Serial: " + String(SAPoTclient.serial));
  Serial.println("\t ID: " + String(SAPoTclient.id[0], HEX) + ":" + String(SAPoTclient.id[1], HEX) + ":" + String(SAPoTclient.id[2], HEX) + ":" + String(SAPoTclient.id[3], HEX) + ":" + String(SAPoTclient.id[4], HEX) + ":" + String(SAPoTclient.id[5], HEX));
  Serial.println("\t Type: " + String(SAPoTclient.type));
  Serial.println("\t Central ID: " + String(SAPoTclient.centralID));
  Serial.println("\t Status: " + String(SAPoTclient.status));
  Serial.println("\t Sensor Request ID: " + String(SAPoTclient.sensorRequestID));
  Serial.println("\t Sensor Request Time: " + String(SAPoTclient.sensorRequestTime));
  Serial.println("\t Actuator Request ID: " + String(SAPoTclient.actuatorRequestID));
  Serial.println("\t Actuator Request Time: " + String(SAPoTclient.actuatorRequestTime));
  Serial.println("\t Actuator Request Degree: " + String(SAPoTclient.actuatorRequestDegree));
  Serial.println("\t Amount of Sensors: " + String(SAPoTclient.amountOfSensors));
  for(i=0; i<SAPoTclient.amountOfSensors; i++) Serial.println("\t Sensor Vector[" + String(i) + "]: " + String(SAPoTclient.sensorVector[i])); 
  Serial.println("\t Amount of Actuators: " + String(SAPoTclient.amountOfActuators));
  for(i=0; i<SAPoTclient.amountOfActuators; i++) Serial.println("\t Actuator Vector[" + String(i) + "]: " + String(SAPoTclient.actuatorVector[i]));
  Serial.println("\t Current Time: " + String(SAPoTclient.currentTime, DEC));
  for(i=0; i<(SAPoTclient.amountOfSensors + SAPoTclient.amountOfActuators); i++) Serial.println("\t Previous Time: " + String(((unsigned long*) SAPoTclient.previousTime)[i], DEC)); 
  
  return TRUE;
  
}

/*
 * Função: Conecta o Cliente SAPoT com a rede e solicita registro na Cental 
 *  @parametros: ssid da rede Wifi, senha da rede wifi, endereço do server MQTT, porta de escuta da broker MQTT, login para acessar o broker, senha para acessar o broker.
 *  @retorno: TRUE se o cliente for registrado na central com sucesso, se não retorna FALSE
 */
bool SAPoTClient_connect(const char* ssid, const char* password, const char* mqtt_server,const int broker_port, const char* broker_login, const char* broker_pass){

    
  //Verificando o tipo de cliente que será registrado 
  if(SAPoTclient.type == SMCAI) Serial.println("SAPoTClient_connect: Preparing the record of a SMCAI");
  else if(SAPoTclient.type == SMCAE)Serial.println("SAPoTClient_connect: Preparing the record of a SMCAE");
  else if(SAPoTclient.type == SMRE)Serial.println("SAPoTClient_connect: Preparing the record of a SMRE");
  else if(SAPoTclient.type == SMRH)Serial.println("SAPoTClient_connect: Preparing the record of a SMRH");
  else if(SAPoTclient.type == SRF)Serial.println("SAPoTClient_connect: Preparing the record of a SRF");
  else{
    Serial.println("SAPoTClient_connect: client type not found !");
    return FALSE;
  }

  SAPoTnetwork.WiFi_ssid = ssid;
  SAPoTnetwork.WiFi_pass = password;
  SAPoTnetwork.MQTT_server = mqtt_server;
  SAPoTnetwork.MQTT_port = broker_port;
  SAPoTnetwork.MQTT_user = broker_login;
  SAPoTnetwork.MQTT_pass = broker_pass;
  
  
  do{
    //Verifica/Realiza a conexão com a rede via Wifi
    WiFiconnect();
  
    //Verifica/Realiza a conexão com o servidor MQTT
    MQTTconnect();
  
    //Definindo o tamanho do pacote de registro.
    SAPoTmessage.length = (sizeof(fixed_header_SAPoT)+ sizeof(registration_SAPoT)+2*SAPoTclient.amountOfSensors+2*SAPoTclient.amountOfActuators);
    Serial.println("SAPoTmessage.length " + String(SAPoTmessage.length));
  
    //Concatenando as estruturas do cabeçalho fixo (fh) e do payload (pl) no mesmo espaço de memória (SAPoTmessage.data).
    SAPoTmessage.data = malloc(SAPoTmessage.length);
    fixed_header_SAPoT *fh = (fixed_header_SAPoT*) SAPoTmessage.data;
    registration_SAPoT *pl = (registration_SAPoT*) (SAPoTmessage.data + sizeof(fixed_header_SAPoT));
  
    //Preenchendo o espaço de memória (fixed header)
    fh->version = SAPOT_VERSION;
    fh->ack = 0;
    fh->rsv1 = 0;
    fh->rsv2 = 0;
    fh->rsv3 = 0;
    fh->instruction = 0;
    fh->serial = SAPoTclient.serial;
    fh->length = SAPoTmessage.length;
    int i;
    for(i=0; i<6; i++) fh->client_id[i] = SAPoTclient.id[i];
  
    //Preenchendo o espaço de memória (payload: Registration)
    pl->thing_type = SAPoTclient.type;
    pl->sensor_quantity = SAPoTclient.amountOfSensors;
    pl->actuator_quantity = SAPoTclient.amountOfActuators;
    for(i=0; i<SAPoTclient.amountOfSensors; i++) pl->sensor_actuator_type[i] = SAPoTclient.sensorVector[i]; 
    for(i=0; i<SAPoTclient.amountOfActuators; i++) pl->sensor_actuator_type[i+SAPoTclient.amountOfSensors] = SAPoTclient.actuatorVector[i];
  
    //Print da mensagem para verificação dos dados.
    Serial.println("SAPoTmessage: " + String(SAPoTmessage.length) + "Bytes");
    for(i=0; i<SAPoTmessage.length; i++){ 
       Serial.print("["+ String(i) +"]: ");
       Serial.println(((uint8_t*)SAPoTmessage.data)[i], HEX);
    } 
  
    //Publicando o pacote de registro no tópico de escuta da Central.
    Serial.println("Publishing on topic: " + String(SAPoTclient.centralID));
    if(MQTTclient.publish(SAPoTclient.centralID, (byte*)SAPoTmessage.data,(byte)SAPoTmessage.length, 0) < 0){
        Serial.println("SAPoTClient_connect Error: unable to post on broker");
        free(SAPoTmessage.data);
        return FALSE; 
    }
  
    //Livrando o espaço de memória referente ao pacote enviado
    free(SAPoTmessage.data);
  
    //Esperando o retorno da central por 30 segundos, se não realiza o cadastro novamente.
    Serial.print("Waiting for a Central response !");    
    for(i=0; i<60; i++){
      Serial.print(".");
      WiFiconnect();
      MQTTconnect();
      if(SAPoTclient.status == 1) return TRUE;
      delay(500);
    }
    Serial.println();
    if(SAPoTclient.status != 1) Serial.println("No response from Central. Requesting registration again !");
    
  }while(SAPoTclient.status != 1);
}

/*
 * Função: Executa as operações com os sensores e/ou atuadores de acordo com o que for solicitado pela Central
 *  @parâmetros: float* sensorControl(uint16 sensorID), void actuatorControl(uint16_t actuatorID, uint16_t degreeOfPerformance).  
 *  @retorno: #CRIAR TABELA DE RETORNO#
 */
int SAPoTloop(float* (*sensorControl)(uint16_t), void (*actuatorControl)(uint16_t, uint16_t)){

  float* sensorInfo;
  int duty;
  
  WiFiconnect();
  MQTTconnect();
  SAPoTclient.currentTime = millis();
  
  if(SAPoTclient.sensorRequestID != NON){
      if((SAPoTclient.currentTime - sensorPreviousTime[SAPoTclient.sensorRequestID]) >= SAPoTclient.sensorRequestTime){
          sensorInfo = sensorControl(SAPoTclient.sensorRequestID);
          
          SAPoTclient.serial++;
          
          //Definindo o tamanho da mensagem de resposta
          if(SAPoTclient.sensorRequestID == ALL) SAPoTmessage.length = sizeof(fixed_header_SAPoT) + SAPoTclient.amountOfSensors * sizeof(float);
          else SAPoTmessage.length = sizeof(fixed_header_SAPoT) + sizeof(float);
          
          //Preenchendo o cabeçalho da mensagem
          SAPoTmessage.data = malloc(SAPoTmessage.length);
          fixed_header_SAPoT* fh = (fixed_header_SAPoT*) SAPoTmessage.data;
          fh->version = SAPOT_VERSION;
          fh->ack = 1;
          fh->rsv1 = 0;
          fh->rsv2 = 0;
          fh->rsv3 = 0;
          fh->serial = SAPoTclient.serial;
          fh->length = SAPoTmessage.length;
          int i;
          for(i=0; i<6; i++) fh->client_id[i] = SAPoTclient.id[i];

          //Preenchendo o payload da mensagem
          if(SAPoTclient.sensorRequestID == ALL){
            fh->instruction = 1;
            request_of_all_ack_SAPoT* pl = (request_of_all_ack_SAPoT*) (SAPoTmessage.data + sizeof(fixed_header_SAPoT));
            for(i=0; i<SAPoTclient.amountOfSensors; i++) pl->sensors_info[i] = sensorInfo[i];
          }
          else{
            fh->instruction = 2;
            sensor_request_ack_SAPoT* pl = (sensor_request_ack_SAPoT*) (SAPoTmessage.data + sizeof(fixed_header_SAPoT));
            pl->sensor_info = sensorInfo[0]; 
          }

          //Enviando as informações para a Central
          if(MQTTclient.publish(SAPoTclient.centralID, (byte*) SAPoTmessage.data, (byte) SAPoTmessage.length, 0) < 0) Serial.println("SAPoTloop Error: unable to post on broker"); 
          
          free(SAPoTmessage.data);
          
          sensorPreviousTime[SAPoTclient.sensorRequestID] = millis();
      }
  }

  
  SAPoTclient.currentTime = millis();
  if(SAPoTclient.actuatorRequestID != NON){
       if((SAPoTclient.currentTime - actuatorPreviousTime[SAPoTclient.actuatorRequestID]) >= SAPoTclient.actuatorRequestTime){
           actuatorTimeCounter++;
           if(actuatorTimeCounter == 1){ //Liga
             duty = (int) SAPoTclient.actuatorRequestDegree;
             duty = map(duty, 0, 65535, 0, 1023); 
             actuatorControl(SAPoTclient.actuatorRequestID, duty);
           }
           if(actuatorTimeCounter == 2){ //Desliga
             actuatorControl(SAPoTclient.actuatorRequestID, 0);
             actuatorTimeCounter = 0;
             SAPoTclient.actuatorRequestID = NON;
           }
           actuatorPreviousTime[SAPoTclient.actuatorRequestID] = millis();
     
       }
  }
  
}

/*
 * Função: Verifica e efetua a conexão do Cliente WiFi com a rede
 *  @parametro: identificador da rede (ssid) e senha da rede (pass)
 *  @retorno: FALSE se não conectado à rede, TRUE se conectado à rede.
 */
bool WiFiconnect(){
  
  if (WiFi.status() == WL_CONNECTED){
    //Serial.println("Wifi Client remains connected.");
    return TRUE;
  }
  else{
      WiFi.begin(SAPoTnetwork.WiFi_ssid, SAPoTnetwork.WiFi_pass); 
      Serial.println();
      Serial.println("Connecting to the network: " + String(SAPoTnetwork.WiFi_ssid) + ". Wait please!");
      while(WiFi.status() != WL_CONNECTED){ 
        Serial.print(".");
        delay(1000);      
      }
  }  
}

/*
 * Função: Verifica e efetua a conexão do Cliente MQTT com o broker 
 *  @parâmetros: endereço do broker, porta de escuta do broker, login e senha para se conectar ao broker, identificador do cliente
 *  @retorno: FALSE se não estiver conectado ao servidor, TRUE se estiver.
 *  obs: Deve ser usado junto com MQTTclient.loop() para persistência da conexão com o servidor.
 */
bool MQTTconnect(){

  MQTTclient.loop();
  int state = MQTTclient.state();
  if(state != 0){
    Serial.println("MQTTclient.state(): " + String(state));
    Serial.print("Connecting to Broker: " + String(SAPoTnetwork.MQTT_server) + ":" + String(SAPoTnetwork.MQTT_port)); 
    Serial.print(", With login: " + String(SAPoTnetwork.MQTT_user) + ":" + String(SAPoTnetwork.MQTT_pass)); 
    Serial.println(", With ID: " + WiFi.macAddress());
    MQTTclient.setServer(SAPoTnetwork.MQTT_server, SAPoTnetwork.MQTT_port);
    MQTTclient.setCallback(MQTTCallBackMessage); 
     
    if(MQTTclient.connect(WiFi.macAddress().c_str(), SAPoTnetwork.MQTT_user, SAPoTnetwork.MQTT_pass) == 0){
        Serial.println("MQTTconnect error: unable to connect on broker mqtt");
        return FALSE;  
    }
    
    Serial.print("Subscription topic: " + WiFi.macAddress());
    if(MQTTclient.subscribe(WiFi.macAddress().c_str()) == 0){
        Serial.print("MQTTconnect error: unable to subscribe on topic");
        return FALSE;
    }

    Serial.println();
    Serial.println("MQTTconnect successfully executed !");
    return TRUE;      
    }
   else return TRUE;  
}

/*
 * Função: Tratar as messagens recebidas via MQTT, estruturando-as de acordo com o SAPoT.
 *  @parametrôs(padronizados pela biblioteca PubSubClient.h): Tópico de onde a mensagem está vindo, careira de bytes da messagem, tamanho da mensagem 
 *  @retorno: Nenhum.
 */ 
void MQTTCallBackMessage(char* topic, byte* message, unsigned int message_length){

  Serial.println();
  Serial.println("Message recieved from topic: " + String(topic) + ", with " + String(message_length) + " bytes"); 
  
  //Estruturando a mensagem recebida.
  fixed_header_SAPoT p;
  p.version = (message[0]) >> 4;
  p.ack = (message[0] & 0x0f) >> 3;
  p.rsv1 = (message[0] & 0x07) >> 2;
  p.rsv2 = (message[0] & 0x02) >> 1;
  p.rsv3 = (message[0] & 0x01);
  p.instruction = message[1];
  p.serial = (message[2] & 0xffff) | ((message[3] & 0xffff) << 8);
  p.length = (message[4] & 0xffff) | ((message[5] & 0xffff) << 8);
  int i;
  for(i=0; i<6; i++) p.client_id[i] = message[i+6];

  //Printando o cabeçalho fixo da messagem recebida para verificação dos dados.
  Serial.println("Version = "+ String(p.version, HEX));
  if(p.ack == TRUE) Serial.println("ACK = TRUE");
  if(p.rsv1 == TRUE) Serial.println("RSV1 = TRUE");
  if(p.rsv2 == TRUE) Serial.println("RSV2 = TRUE");
  if(p.rsv3 == TRUE) Serial.println("RSV3 = TRUE");
  Serial.println("Instruction = "+ String(p.instruction, HEX));
  Serial.println("Serial = "+ String(p.serial, HEX));
  Serial.println("Length = "+ String(p.length, HEX));
  Serial.println("Client ID = " + String(p.client_id[0], HEX) + ":" + String(p.client_id[1], HEX) + ":" + String(p.client_id[2], HEX) + ":" + String(p.client_id[3], HEX) + ":" + String(p.client_id[4], HEX) + ":" + String(p.client_id[5], HEX));
  Serial.println();

  //Tratando as informações da mensagem recebida
  if((p.version) != SAPOT_VERSION) Serial.println("Packet with incompatible verison!");
  else{
    if(p.ack == TRUE) ackHandling(p.instruction);
    else{
      if(p.instruction == 0){
        Serial.println("Instruction recieved: Register a new client.");
      }
      else if(p.instruction == 1){
        Serial.println("Instruction recieved: Request for all sensors");
        SAPoTclient.sensorRequestID = 0;
        SAPoTclient.sensorRequestTime = getTime((message[sizeof(fixed_header_SAPoT)] & 0xffff) | ((message[sizeof(fixed_header_SAPoT)+1] & 0xffff) << 8));   
      }
      else if(p.instruction == 2){
        Serial.println("Instruction recieved: Request for a specific sensor");
        SAPoTclient.sensorRequestID = (message[sizeof(fixed_header_SAPoT)] & 0xffff) | ((message[sizeof(fixed_header_SAPoT)+1] & 0xffff) << 8); 
        SAPoTclient.sensorRequestTime = getTime((message[sizeof(fixed_header_SAPoT)+2] & 0xffff) | ((message[sizeof(fixed_header_SAPoT)+3] & 0xffff) << 8));
      }
      else if(p.instruction == 3){
        Serial.println("Instruction recieved: Acting request");
        SAPoTclient.actuatorRequestID = (message[sizeof(fixed_header_SAPoT)] & 0xffff) | ((message[sizeof(fixed_header_SAPoT)+1] & 0xffff) << 8);
        SAPoTclient.actuatorRequestTime = getTime(((message[sizeof(fixed_header_SAPoT)+2] & 0xffff) | ((message[sizeof(fixed_header_SAPoT)+3] & 0xffff) << 8)));
        SAPoTclient.actuatorRequestDegree = (message[sizeof(fixed_header_SAPoT)+4] & 0xffff) | ((message[sizeof(fixed_header_SAPoT)+5] & 0xffff) << 8);
      }
      else if(p.instruction == 4){
        Serial.println("Instruction recieved: Stop sensor request");
        SAPoTclient.sensorRequestID = NON;
      }
    }
  }
}

/*
 * Função: Trata as mensagens de reconhecimento (Acknowledgment).
 *  @parâmetros: De qual instrução o ack provêm.
 *  @retorno: Nenhum.
 */
void ackHandling(uint8_t instruction){
  //Função para tratar os ack's. A princípio, essa função só é necessária para a operação da central.

  if(instruction == 0) SAPoTclient.status = 1;
  
  
}

/*
 * Função: Transformar o MacAddress (const char*) no vetor ID (uint8_t[6])
 *  @parâmetros: MacAddress da placa e um vetor de bytes ID
 *  @retorno: Nenhum.
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
   //Serial.println(thingID[j], HEX);
   j++;
  }
  
} 

/*
 * Função: Trata o tempo recebido via SAPoT.
 *  @parâmetros: Tempo estruturado em SAPoT (1º byte = Escala de tempo; 2º, 3º e 4º bytes = Quantidade de tempo).
 *  @retorno: tempo em milisegundos.
 */
unsigned long getTime(uint16_t _time){

    uint16_t scale = (_time>>12);
    uint16_t quantity = _time & 0x0FFF;

    Serial.println();
    Serial.println("Time = " + String(_time));
    Serial.println("Scale = " + String(scale));
    //Escala de milisegundos
    if(scale == 0x0000){
      Serial.println("GetTime: " + String(quantity) + "ms"); 
      return quantity;
    }
    //Escala de Segundos
    else if(scale == 0x0001){
      Serial.println("GetTime: " + String(quantity) + "s"); 
      return (quantity * 1000);
    }
    //Escala de Minutos
    else if(scale == 0x0002){ 
      Serial.println("GetTime: " + String(quantity) + "min" + " = " + String(quantity * 60) + "s");
      return (quantity * 1000 * 60);
    }
    //Escala de horas
    else if(scale == 0x0003){
      Serial.println("GetTime: " + String(quantity) + "hours" + " = " + String(quantity * 60 * 60) + "s");
      return (quantity * 1000 * 60 * 60);
    }
    else if(scale == 0x0004){ 
      Serial.println("GetTime: " + String(quantity) + "Days" + " = " + String(quantity * 60 * 60 * 24) + "s");
      return (quantity * 1000 * 60 * 60 * 24);
    }

}

/************************************************************************************************************/

#define PIN_PWR D5
#define PIN_RST D8

/* Constantes de acesso a rede (Wifi, MQTT e NTP) */
const char* ssid = "GPC";  //"Brandao_111_24";"Gigacandanga";    
const char* password = "gpcpass123"; //"98411837";  "Aldebaran2@18noc316316";
const char* mqtt_server = "192.168.5.1"; //"10.10.40.84"; "10.10.20.104"; 

const int broker_port = 1883;
const char* broker_user = NULL; //"thing1"; //"ucc";
const char* broker_pass = NULL; //"thingpass123"; //"jilo71jilo71";
const char* central_id = "00:00:00:00:00:00"; //"78:E4:00:8C:65:77";

/* Preenchendo os objetos referentes aos e atuadores */
uint16_t actuatorVector[] = {CDC, CDC};
/* Subrotinas para o controle dos sensores e atuadores */
float* sensorControl(uint16_t sensorID);
void actuatorControl(uint16_t actuatorID, uint16_t degreeOfPerformance);

void setup(){

  //Inicia comunicação serial
  Serial.begin(9600);
  delay(1000);

  //Inicializando a pinagem dos atuadores
  pinMode(PIN_PWR, OUTPUT);
  digitalWrite(PIN_PWR, LOW);
  pinMode(PIN_RST, OUTPUT);
  digitalWrite(PIN_RST, LOW);  
 
  
  //Inicializa cliente SAPoT
  SAPoTClient_begin(central_id, SMCAI, 0, 2, NULL, actuatorVector);

  //Conecta o cliente à rede e faz seu registro na central
  SAPoTClient_connect(ssid, password, mqtt_server, broker_port, broker_user, broker_pass);
 
}

void loop(){

  //Inicia o loop de processos SAPoT
  SAPoTloop(NULL, actuatorControl);

}

/* Funções das Subrotinas */

//float* sensorControl(uint16_t sensorID){}

void actuatorControl(uint16_t actuatorID, uint16_t degreeOfPerformance){
  
    //Atuador 1
    if(actuatorID == 1) analogWrite(PIN_PWR, degreeOfPerformance);
    //Atuador 2
    else if(actuatorID == 2) analogWrite(PIN_RST, degreeOfPerformance);
           
}
