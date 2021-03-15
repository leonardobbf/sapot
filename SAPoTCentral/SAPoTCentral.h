/**
 * @file SAPoTCentral.h
 * @author Leonardo Brandão Borges de Freitas (contato.leonardobbf@gmail.com)
 * @date 12 Fev 2020
 * @brief Definições, estruturas e subrotinas para execução de uma central SAPoT em C.
 *
 * @mainpage Coordenador de operações para redes IoT.
 * &copy; Copyright Associação GigaCandanga 2019, 2020.
 * 
 * @section intro Introdução 
 *
 * O Protocolo para Sensoriamento e Atuação de Coisas (SAPoT) é um leve e
 * enxuto protocolo de fonte aberta desenvolvido para traduzir as mensagens 
 * em uma rede IoT no modelo Central/Sentinela (vide <a href="https://drive.google.com/drive/u/2/my-drive">SAPoTv1.0 Especificação de Protocolo</a> ). 
 * Na atual versão, a Central SAPoT opera em sistemas Linux e sua rotina de operação local-padrão  
 * depende dos serviços: de transmissão de dados MQTTv3.1.1 e de banco de dados MySQLv8.0.
 * De qualquer forma, a Central SAPoT suporta outros tipos de protocolos para transmissão 
 * e armazenamento de dados que podem ser configurados manualmente pelo usuário. 
 * Para maiores detalhes veja, <a href="https://www.google.com/">Documentação SAPoTCentral</a>.
 * @see http://mqtt.org/documentation
 * @see https://dev.mysql.com/doc/refman/8.0/en/
 * 
 * @section utilizacao Utilizando a Central
 *
 * As aplicações que utilizam essa biblioteca geralmente são implementadas da seguinte maneira:
 * <ul>
 * <li> Cria-se uma entidade SAPoTCentral </li>
 * <li> Crie-se uma constante SAPoTCentral_create_options, inicializando-a através da definição SAPOTCENTRAL_OPTS_STDLOCAL </li>
 * <li> Inicia-se as operações da Central através da função SAPoTCentral_begin()</li>
 * <li> Inicia-se a rotina de repetição através de SAPoTCentral_loop() que opera da seguinte maneira:</li>
 *		<ul>
 *			<li> Recebe a mensagem do broker via MQTTmessageArrived() </li>
 *			<li> Estrutura a mensagem via SAPoTCentral_unpack_message() </li>
 *  		<li> Executa a operação referente à instrução recebida via SAPoTCentral_set_operation() </li>
 *		</ul>
 * <li> Finaliza-se os serviços da Central através da função SAPoTCentral_end()</li> 
 * </ul>
 *
 * O código a seguir mostra uma simples aplicação que utiliza da SAPoTCentral.h em sua configuração local-padrão:
 *
 * @code{.c}
 * #include <stdio.h>
 * #include <stdlib.h>
 * #include <signal.h>
 * #include "SAPoTCentral.h"
 *
 * #define MQTT_USER "user"
 * #define MQTT_PASS "password"
 * #define MYSQL_USER "user"
 * #define MYSQL_PASS "password"
 * #define MYSQL_DIR "directory"
 *
 * //Objeto para manipulação da Central
 * SAPoTCentral handle;
 *
 * void signalHandling(int signum){
 *
 *	printf("Finalizando a Central SAPoT...\n");
 *	SAPoTCentral_end();
 *	exit(1);
 *
 * }
 *
 * int main(int argc, char *argv[]) {
 *
 *  //Tratrando o sinal de terminação (Ctrl + C)
 *	signal(SIGINT, signalHandling);
 *
 *	//Definindo o identificador da Central
 *	const char* centralId = "00:00:00:00:00:00";
 * 	
 *	//Configurando as opções de inicialização da central
 *	SAPoTCentral_create_options opts = SAPOTCENTRAL_OPTS_STDLOCAL;
 *  opts.transmission.user = MQTT_USER;
 *  opts.transmission.pass = MQTT_PASS;
 *  opts.database.user = MYSQL_USER;
 *  opts.database.pass = MYSQL_PASS;
 *  opts.database.dir = MYSQL_DIR;
 *	
 *  //Iniciando os serviços da Central
 *	if(SAPoTCentral_begin(&handle, &opts, centralId) != SAPOTCENTRAL_SUCCESS){
 *		printf("SAPoTCentral_begin error (%d)", handle.error);
 *		return -1;
 *	}
 *		
 *	//Entrando em modo loop	
 *	SAPoTCentral_loop(); 
 *			
 *	return 0;
 *
 * }
 * @endcode
 * 
 * @section pre_req Pré-requisitos
 * 
 * O usuário com a inteção de utilizar a central no modelo local-padrão deve instalar na mesma máquina que rodará sua aplicação
 * um broker MQTTv3.1.1, bem como um servidor MYSQLv8.0. Para o servidor MQTT não existe configurações adicionais além das 
 * demandadas em sua instalação. Portanto apenas instale seu broker de preferência e inicie o serviço na máquina. 
 *	
 *	<b>Configurando o servidor MySQL</b><br>
 *	<ul>
 *	<li> Após a instalação do servidor MySQLv8.0, abra um terminal e acesse o serviço </li>
 *	@code
 *	mysql -u root -p 	
 *	@endcode
 *	<li> Após digitar a senha de root e acessar o banco de dados, crie um diretorio chamado db_UCC </li>
 *	@code{.sql}
 *	CREATE DATABASE db_UCC;
 *	@endcode
 *	<li> Verifique se o diretório foi criado corretamente e então o acesse</li>
 *	@code{.sql}
 *	SHOW DATABASES;
 *	USE db_UCC;
 *	@endcode
 *	<li> Uma vez dentro do diretório crie uma tabela chamada tb_cadastrados</li>
 *	@code{.sql}
 *	CREATE TABLE tb_cadastrados(
 *   id INT NOT NULL AUTO_INCREMENT,
 *   label VARCHAR(11) NOT NULL,
 *   macaddr VARCHAR(18) NOT NULL,
 *   type INT(6) NOT NULL,
 *   sensor INT(4) NOT NULL,
 *   actuator INT(4) NOT NULL,
 *   PRIMARY KEY (id)
 *   );
 *	@endcode
 *	<li> Com tabela devidamente configurada, deve-se criar um usuário chamado guest com senha de mesmo nome e 
 *	dar a ele permissões para modificar a tabela tb_cadastrados </li>
 *  @code{.sql}
 *	CREATE USER 'guest'@'localhost' IDENTIFIED BY 'guest';
 *	GRANT ALL PRIVILEGES ON db_UCC.* TO 'guest'@'localhost';
 *	@endcode 	
 *  </ul>
 *	 
 *	<b>Observações:</b> As configurações de login para acessar tanto o broker MQTT quanto o banco de dados podem ser 
 *	alteradas, desde que se atente em modifica-las nas opções de configurações da Central. Assim como pode ser 
 *	observado no código da aplicação de exemplo da seção @ref utilizacao.
 *
 */

#ifndef SAPOTCENTRAL_H
#define SAPOTCENTRAL_H

								/************************* Headers ******************************/
								
#include <stdio.h>
#include <stdint.h>
#include <MQTTClient.h>
#include <mysql/mysql.h>

								/************************* Defines ******************************/

/**
* Indicador da Versão do protocolo: Uma definição utilizada para verificar a versão do protocolo SAPoT da mensagem recebida.
*	
*/
#define SAPOT_PROTOCOL_VERSION 1

/**
* Código de Retorno: Sucesso: Indica sucesso na operação realizada pela Central SAPoT. 
*
*/
#define SAPOTCENTRAL_SUCCESS 0

/**
* Código de Retorno: Fracasso: Indica fracasso na operação realizada pela Central SAPoT. 
*
*/
#define SAPOTCENTRAL_FAILURE -1

/**
* Código de Erro: Versão Invalida. Indica que o pacote recebido possui uma versão diferente 
* de SAPOT_PROTOCOL_VERSION.   
*
*/
#define SAPOT_VERSION_ERROR -2

/**
* Código de Erro: Banco de dados configurado incorretamente. Indica que a SAPoTCentral.h não 
* suporta o protocolo de banco de dados ou que esse foi configurado incorretamente pelo usuário.  
*
*/
#define ERROR_SETTING_DATABASE_PROTOCOL -3

/**
* Código de Erro: Protocolo de transmissão de dados configurado incorretamente. Indica que a 
* SAPoTCentral.h não suporta o protocolo de transmissão de dados ou que esse foi configurado 
* incorretamente pelo usuário.  
*
*/
#define ERROR_SETTING_TRANSMISSION_PROTOCOL -4

/**
* Código de Erro: Problemas ao iniciar a comunicação com o serviço de banco de dados.
* Indica que a central não conseguiu iniciar o serviço de banco de dados.
* Esse cógido de erro decorre geralmente de uma falha na comunicação da Central SAPoT com o servidor
* de banco de dados. Como por exemplo, quando a Central opera em uma máquina diferente do servidor de 
* banco de dados e existe um problema com rede aonde a comunicação entre essas duas entidades é interrompida. 
* Contudo, esse erro também se propaga de uma incorreta configuração dos paramêtros do serviço de bando de dados. 
* Para evita-lo é aconselhado que o usuário utilize um servidor local MySQLv8.0 e inicialize as opções de criação 
* da Central (SAPoTCentral_create_options) através da definição SAPOTCENTRAL_OPTS_LOCAL. 
*		  
*/
#define ERROR_STARTING_DATABASE_PROTOCOL -5

/**
* Código de Erro: Problemas ao iniciar a comunicação com o serviço de transmissão de dados.  
* Indica que a central não conseguiu iniciar o serviço de transmissão de dados.
* Esse cógido de erro decorre geralmente de uma falha na comunicação da Central SAPoT com o servidor
* de transmissão de dados. Como por exemplo, quando a Central opera em uma máquina diferente do servidor de 
* transmissão de dados e existe um problema com rede aonde a comunicação entre essas duas entidades é interrompida. 
* Contudo, esse erro também se propaga de uma incorreta configuração dos paramêtros do serviço de transmissão de dados. 
* Para evita-lo é aconselhado que o usuário utilize um servidor local MQTTv3.1.1 e inicialize as opções de criação da 
* Central (SAPoTCentral_create_options) através da definição SAPOTCENTRAL_OPTS_LOCAL. 
*		  
*/
#define ERROR_STARTING_TRANSMISSION_PROTOCOL -6

/**
* Código de Erro: Problemas na solicitação de uma query ao banco de dados.
* Indica que a central não conseguiu se comunicar com o serviço de banco de dados.
* Esse cógido de erro decorre geralmente de uma falha na comunicação da Central SAPoT com o servidor
* de banco de dados. Como por exemplo, quando existe um problema com rede e a comunicação entre 
* a Central e o servidor de banco de dados é interrompida. Contudo, esse erro também tem origem da
* corrupção de dados no momento em que uma query é estruturada nas funções MYSQLRegistration, 
* MYSQLaccess, MYSQLmodification e CTRLactuator.
*		  
*/
#define ERROR_DATABASE_INQUIRY -7

/**
* Código de Erro: Problemas na publicação de uma query ao banco de dados   
* Indica que a central não conseguiu se comunicar com o serviço de banco de dados.
* Esse cógido de erro decorre geralmente de uma falha na comunicação da Central SAPoT com o servidor
* de banco de dados. Como por exemplo, quando existe um problema com rede e a comunicação entre 
* a Central e o servidor de banco de dados é interrompida. Contudo, esse erro também tem origem da
* corrupção de dados na momento em que é estruturada uma query.
*		  
*/
#define ERROR_MQTT_PUBLISH -8

/**
* Código de Erro: Indica fracasso ao operar com arquivo de log.
*  
*
*/
#define ERROR_LOG_OPERATION -9

/**
* Código de Erro: Indica que a label solicitada não esta cadastradas no banco de dados. 
*
*/
#define ERROR_LABEL_NOT_REGISTERED -10 

/**
* Código de Configuração: Indica que o usuário irá utilizar um protocolo não padronizado na SAPoTCentral.h.
* E portanto a função SAPoTCentral_loop() não será utilizada.
*/
#define UNDEFINED 0

/**
* Código de Configuração: Indica que o protocolo de transmissão que será usado pela central será o MQTT
*
*/
#define MQTT 1

/**
* Código de Configuração: Indica que o protocolo de banco de dados que será usado pela central será o MySQL
*
*/
#define SQL 1

/**
* Opção de inicialização (Servidores Locais).
* Define as opções de inicialização SAPoTCentral_create_options. Indicando que o protocolo de transmissão será o MQTTv3.1.1 
* e que o protocolo de banco de dados será o MySQLv8.0. Além disso, define que os servidores estão localizados na mesma máquina 
* em que a Central SAPoT está operando. Para o servidor MQTT não será definido nenhum parâmetro de login (user e pass) e será 
* definida a porta de escuta padrão como 1883. Para o servidor MySQL os parâmetros de login serão definidos como convidado 
* (user='guest', pass='guest') e o diretório da base de dados será definido como db_UCC (dir='db_UCC').    
* 
*/
#define SAPOTCENTRAL_OPTS_STDLOCAL {1, {"localhost", "1883", NULL, NULL}, 1, {"localhost", "3306", "guest", "guest", "db_UCC"}}

/**
* Opção de inicialização (Servidores Indefinidos) 
* Define as opções de inicialização SAPoTCentral_create_options. Indicando a indefinição dos protocolos de transmissão de 
* dados e de banco de dados. Isso faz com que a função SAPoTCentral_loop() não possa ser utilizada. Além disso, indica que 
* o usuário utilizará outros protocolos não padronizados na SAPoTCentral.h.   
* 
*/
#define SAPOTCENTRAL_OPTS_UNDEFINED_PROTOCOLS {0, {NULL, NULL, NULL, NULL}, 0, {NULL, NULL, NULL, NULL, NULL}}





					/************************* Structs for SAPoTMessage *************************/

/**
* @brief Cabeçalho fixo das mensagem SAPoT
*
* Todas as mensagens ​SAPoT possuem um cabeçalho fixo com 12 bytes de
* comprimento organizados em: 4 bits de flags, 4 bits identificadores 
* da versão do protocolo SAPoT que gerou essa mensagem, 8 bits que definem 
* a instrução da operação, 16 bits para o numero serial da mensagem, 16 bits 
* que determinam o tamanho total da mensagem e os últimos 6 bytes indentificadores 
* do emissor da mensagem. 
*
*/
typedef struct{
	
	/** Flag reservada para uso futuro */
	uint8_t rsv3:1, 

	/** Flag reservada para uso futuro */
	rsv2:1, 

	/** Flag reservada para uso futuro */
	rsv1:1, 

	/** Flag identificadora de mensagens de retorno */
	ack:1,

	/** Identificador de versão de protocolo  */
	version:4;

  	/** Identificador da instrução de operação: \n
  	* 0x00: Cadastro de um novo Cliente (Registration) \n
  	* 0x01: Requsição dos dados de todos os sensores (Solicitation) \n
  	* 0x02: Requisição dos dados de um sensor em específico (Solicitation) \n
  	* 0x03: Acionamento de um atuador em específico (Solicitation) \n
  	* 0x04: Acesso à informação dos clientes cadastrados no banco de dados da Central (Acess) \n
  	* 0x05: Registro de informação proveniente de sensores e atuadores (Record) \n
  	* 0x06: Etiquetagem de um cliente que está cadastrado no banco de dados da Central (Modification) \n 
  	**/
  	uint8_t instruction;
  	
  	/** Número serial da mensagem. Refere-se a ordem de emissão da origem. */
  	uint16_t serial;
  	
  	/** Comprimento total da mensagem (Header + Payload) */
  	uint16_t length;
  	
	/** Identificador do Cliente emissor da mensagem (6 bytes referentes ao endereço MAC da interface de rede do emissor) */
  	uint8_t emitterId[6];
  	
}SAPoTMessage_header;

/**
* @brief Payload para cadastro de novo cliente
*
* Payload emitido por um novo cliente que tem o intuito de se cadastrar no banco de dados da Central.
* Organiza-se em: 16 bits definidores do tipo de cliente, 8 bits para quantidade de sensores que operam 
* neste cliente, 8 bits para quantidade de atuatores que operam neste cliente, um vetor com posições de 
* 16 bits para identificar os tipos de sensores e atuadores que operam neste cliente. 
*  
*/
typedef struct{

	/** Tipo de Cliente:\n 
	* 0x00: Interface do Usuário (USR) \n
	* 0x01: Sistema de Monitoramento e Controle para Ambientes Internos (SMCAI) \n
	* 0x02: Sistema de Monitoramento e Controle para Ambientes Externos (SMCAE) \n
	* 0x03: Sistema de Monitoramento de Rede Elétrica (SMRE) \n
	* 0x04: Sistema de Monitoramento de Rede Hidráulica (SMRH) \n
	* 0x05: Sistema de Reconhecimento Facial (SRF) \n
	* 0x06 a 0xFF: Reservado para uso futuro. \n
	*
	*/
  	uint16_t clientType;
  	
  	/** Quantidade de sensores em operação no Cliente */ 
  	uint8_t sensorQuantity;
  	
  	/** Quantidade de atuadores em operação no Cliente */ 
  	uint8_t actuatorQuantity;
  	
  	/** Ponteiro identificador dos tipos de sensores e atuadores em operação no Cliente: \n
  	* 0x0000: Sensor de Temperatura (TMP)\n
  	* 0x0001: Sensor de Umidade (UMD)\n
  	* 0x0002: Sensor de Luminosidade (LUX)\n
  	* 0x0003: Sensor de detecção de presença (DTP)\n
  	* 0x0004: Câmera de captação de imagens (CAM)\n
  	* 0x0005: Sensor de tensão em rede elétrica CA (VCA)\n
  	* 0x0006: Sensor de corrente em rede elétrica CA (ICA)\n
  	* 0x0007: Sensor de potência em rede elétrica CA (PCA)\n
  	* 0x0008: Sensor de fator de potêncial em rede elétrica CA (FP)\n
  	* 0x0009: Sensor de consumo energético em rede elétrica CA (KWH)\n
  	* 0x000A a 0xFFFF: reservado para uso futuro. \n 
  	*
  	*/ 
  	uint16_t* sensorActuatorType;
  	 
}SAPoTMessage_registration;

/**
* @brief Payload para solicitar o acionamento ou sensoriamento de algum cliente cadastrado no banco de dados.
* 
* Payload emitido pelo usuário e recebido pela central. Tem o intuito de solicitar o acionamento de um atuador 
* ou requisitar os dados provenientes de um sensor que estejam operando em algum cliente cadastrado no banco de
* dados da Central. São 17 bytes divididos em: 11 bytes referentes a etiqueta do cliente cadastrado no banco de 
* dados da Central, 16 bits para identificar o sensor ou o atuador a ser acionado, 16 bits para o temporizador de
* acionamento do atuador ou para o intervalo de tempo em que o cliente deve enviar as informações do sensor pra 
* a central e os ultimos 16 bits referentes à intensidade de atuação a qual o atuador será submetido. 
*
*/
typedef struct{

	/** Etiqueta do cliente cadastrado que será acionado pelo usuário */
  	uint8_t label[11];
  	
  	/** Identificador do Sensor ou do Atuador */
  	uint16_t sensorOrActuatorId;
  	
  	/** Tempo de requisição do sensor ou tempo de acionamento do atuador (16 bits): \n
  	*	4 bits MSB:\n 
  	*	0x0 Milisegundos\n
  	*	0x1 Segundos\n
  	*	0x2 Minutos\n
  	*	0x3 Horas\n
  	*	0x4 Dias\n
  	*   12 bits LSB: de 0x000 (0b10) a 0xFFF (4096b10)
  	*/ 
  	uint16_t timeSet;
  	
  	/** Grau de performace do atuador: de 0x0000 (0%) a 0xffff (100%)*/ 
  	uint16_t degreeOfPerformance; 
  	 
}SAPoTMessage_solicitation;

/**
* @brief Estrutura para acesso à informação dos clientes cadastrados.
*
* Em uma solicitação de acesso realizada pelo usuário (instrução = 0x04) a Central busca
* em seu banco de dados as informações dos clientes cadastrados. Dessa forma, ela precisa
* organizar essas informações em uma carga útil para a mensagem de resposta ao usuário. 
* Portanto, a SAPoTMessage_access funciona como um ponteiro tanto para escrever, quanto 
* para ler as informações daquela carga útil. As informações pertinentes à requisição de
* acesso são estruturadas em seções de 32 bytes, para cada cliente cadastrado na central, da 
* seguinte maneira: 10 bytes para etiqueta, 18 bytes para o endereço MAC (XX:XX:XX:XX:XX:XX),
* 2 bytes para o tipo de cliente, 1 byte para quantidade de sensores e o último byte para
* quantidade de atuadores.    
*
*/
typedef struct{

	/** Etiqueta referente ao cadastrado */
	uint8_t label[10];

	/** Identificador de Macaddr */
	uint8_t id[18];

	/** Tipo de Cliente:\n 
	* 0x00: Interface do Usuário (USR) \n
	* 0x01: Sistema de Monitoramento e Controle para Ambientes Internos (SMCAI) \n
	* 0x02: Sistema de Monitoramento e Controle para Ambientes Externos (SMCAE) \n
	* 0x03: Sistema de Monitoramento de Rede Elétrica (SMRE) \n
	* 0x04: Sistema de Monitoramento de Rede Hidráulica (SMRH) \n
	* 0x05: Sistema de Reconhecimento Facial (SRF) \n
	* 0x06 a 0xFF: Reservado para uso futuro. \n
	*
	*/
	uint16_t type;

	/** Quantidade de sensores */ 
  	uint8_t sensor;
  	
  	/** Quantidade de atuadores */ 
  	uint8_t actuator;
  	 
}SAPoTMessage_access;

/**
* @brief Estrutura para acessar as de informação enviadas pelo cliente, dentre as quais: Dados dos Sensores, Status 
* dos Atuadores e possíveis erros de operação.
*
* A partir de uma solicitação do Usuário à Central que posteriormente é encaminhada ao Cliente, esse emite 
* uma mensagem de resposta para a Central com as informações pertinentes à instrução recebida:
* 0x01: Requsição dos dados de todos os sensores (Solicitation) \n
* 0x02: Requisição dos dados de um sensor em específico (Solicitation) \n
* 0x03: Acionamento de um atuador em específico (Solicitation) \n  
* Ou em casos de erro em alguma operação, o Cliente enviará uma mensagem de log para a central com a instrução
* 0x07.
* Dessa maneira, a Central utiliza da estrutura SAPoTMessage_record para interpretar as informações contidadas
* No pacote enviado pelo Cliente. 
*
*/
typedef struct{


	/** Tamanho referente à quantidade de informação recebida  */ 
  	uint8_t dataLen;
  	
	/** Ponteiro identificador da informação a ser guardada no bando de dados. \n
	*  de acordo com a instrução recebida: \n
	*  0x01: (float*) Dados de todos os sensores \n
	*  0x02: (float) Dados de um sensor em específico \n
	*  0x03: (uint8_t) Status do atuador acionado \n
	*  0x07: (int) Log de erro \n	
	**/
  	void* data;
  	 
}SAPoTMessage_record;

/**
* @brief Payload para etiquetagem de clientes no banco de dados da Central.
*
* Payload enviado pelo Usuário e recebido pela Central. Tem o intuito de
* relacionar o endereço MAC de um Cliente já cadastrado a uma etiqueta 
* definida pelo usuário. Dessa forma, a resolução de nomes dos Clientes
* fica mais intuitiva para as solicitações do Usuário . Esse payload possui 29 bytes de comprimento
* divididos em: 18 bytes identificadores do endereço MAC cadastrado, seguidos por 
* 11 bytes referentes a etiqueta definida pelo usuário.
*	
*/
typedef struct{

	/** Identificador do macaddr que será etiquetado */
  	uint8_t macaddr[18];
  	
  	/** Etiqueta */ 
  	uint8_t label[11];
  	 
}SAPoTMessage_modification;


/**
* @brief Payload para acionar um atuador específico em um Cliente cadastrado.
*
* Payload Enviado pela Central para um Cliente. Após uma solicitação do Usuário 
* com instrução 0x03 (Acionamento de um atuador em específico) a Central encaminha
* essa solicitaão para o Cliente através desse Payload que é composto por 6 bytes
* divididos em: 2 bytes para identificar qual será o atuador acionado, 2 bytes para
* definir por quanto tempo esss atuador será acionado e os ultimos dois bytes para
* definir a potência de atuação.
*
*/
typedef struct {

  	/** Identificador do atuador */
  	uint16_t actuatorId;
  	
  	/** Tempo de acionamento do atuador (16 bits): \n
  	*	4 bits MSB:\n 
  	*	0x0 Milisegundos\n
  	*	0x1 Segundos\n
  	*	0x2 Minutos\n
  	*	0x3 Horas\n
  	*	0x4 Dias\n
  	*   12 bits LSB: de 0x000 (0) a 0xfff (4096)
  	*	 
  	*/ 
  	uint16_t timeSet;
  	
  	/** Grau de performace do atuador: tem alcance de 0 (0%) a 65535(100%). */ 
  	uint16_t degreeOfPerformance;

	
}SAPoTMessage_actuatorDrive;



							/************************* Structs for SAPoTCentral *************************/

/**
* @brief Estrutura para definir as opções de criação de uma Central SAPoT
*
* Essa estrutura armazena as informações pertinentes aos serviços de transmissão e banco de dados.
* Na atual versão de protocolo, a Central SAPoT tem suporte apenas para o protocolo MQTT e SQL.
*
*/
typedef struct{

	/** identificador de protocolo de transmissão: \n 
	* 	1: MQTT \n 
	*/
	uint8_t transmissionProtocol;
	
	/** Informações referente ao servidor de transmissão de mensagens */
	struct{
		
		char* host; /*!< Endereço do servidor de transmissão */
		
		char* port; /*!< Porta de escuta do servidor de transmissão */
		
		char* user; /*!< Usuário para acessar o servidor de transmissão */
		
		char* pass; /*!< Senha para acessar o servidor de transmissão */
		
	}transmission;
	
	/** identificador de protocolo de banco de dados: \n 
	* 	1: SQL \n 
	*/
	uint8_t databaseProtocol;
	
	/** Informações referente ao servidor de banco de dados */
	struct{
		
		char* host; /*!< Endereço do servidor de banco de dados */
		
		char* port; /*!< Porta de escuta do servidor de banco de dados */
		
		char* user; /*!< Usuário para acessar o servidor de banco de dados */
		
		char* pass; /*!< Senha para acessar o servidor de banco de dados */
		
		char* dir; /*!< Diretório da base de dados */
		
	}database;
	
}SAPoTCentral_create_options;

/**
* @brief Principal estrutura para operar uma Central SAPoT.
*
* Essa estrutura armazena as ferramentas necessárias para recepção, manipulação e envio 
* de mensagem na Central SAPoT, bem como os maniuladores dos clientes MQTT e MySQL. 
* Além disso, existem outras informações e flags importantes como: o identificador da 
* Central no formato de endereço MAC, o indicador de erro, o contador serial de mensagens 
* enviadas pela central e a flag de autorização da rotina de repetição SAPoTCentral_loop().
*
*/
typedef struct{

	/** identificador da Central no formato macaddr (xx:xx:xx:xx:xx:xx) */
	const char* id;
	
	/** Indicador de numero de erro */
	int error;

	/** 
	* Flag que permite a execução da SAPoTCentral_loop() se a Central
	* for inciada corretamente via SAPoTCentral_begin().
	*/
	bool inLoop;

	/** Número serial referente as mensagem enviadas pela central*/
	uint16_t serial;	

	/** Ponteiro indicador da mensagem recebida */
	uint8_t* inMessage;

	/** Ponteiro indicador da mensagem a ser enviada para o Usuário */
	void* outMessage;
	
	/** Ponteiro para o cabeçalho da mensagem recebida */
	SAPoTMessage_header* header;
	
	/** Ponteiro para o payload de cadastro de novo cliente */
	SAPoTMessage_registration* registration;
	
	/** Ponteiro para o payload de registro de informação */
	SAPoTMessage_record* record;
	
	/** Ponteiro para o payload de modificação de informação */
	SAPoTMessage_modification* modification;
	
	/** Ponteiro para o payload de solicitação de uso de algum cliente cadastrado */
	SAPoTMessage_solicitation* solicitation;
	
	/** Objeto referente ao cliente MQTT*/
	MQTTClient MQTTclient;
	
	/** Objeto referente ao cliente MYSQL*/
	MYSQL MYSQLclient;
	
	
}SAPoTCentral;


					/************************* Functions for SAPoTCentral *************************/

/** 
* Essa função inicia a central SAPoT preenchendo as informações passadas pelo usuário através das estruturas SAPoTCentral
* e SAPoTCentral_create_options. Além disso, inicia o arquivo de log e se o usuário optar por utilizar o modo local-padrão
* essa função inicia os clientes para comunicação com os serviços MQTT e MySQL.
*
* @param userPointer Um ponteiro para o espaço de memória do tipo SAPoTCentral que deseja ser iniciado como Central.
* @param opts Um ponteiro para o espaço de memória do tipo SAPoTCentral_create_options que contém as configurações iniciais
* para operação da Central.
* @param centralId Uma string no formato de um endereço MAC (XX:XX:XX:XX:XX:XX) que identifica a Central. Esse parâmetro 
* deve ser único em uma rede que possua mais de uma Central SAPoT em operação.  
*  
* @return Essa função retorna #SAPOTCENTRAL_FAILURE no caso de não conseguir iniciar a Central, ou retorna #SAPOTCENTRAL_SUCCESS 
* em caso de sucesso.
*
*/
int SAPoTCentral_begin(SAPoTCentral* userPointer , SAPoTCentral_create_options* opts, const char* centralId);

/** 
* Essa função finaliza os serviços da central SAPoT. Fecha o arquivo de log e se estiver operando no modo local-padrão, 
* encerra os serviços MQTT e MySQL. Não possue parâmetros de entrada nem retorno.
* 
*/
void SAPoTCentral_end();

/** 
* Essa função pode ser utilizada se, e somente se a Central for configurada no modelo local-padrão através da definição 
* #SAPOTCENTRAL_OPTS_STDLOCAL (veja também SAPoTCentral_create_options). Além disso, essa função mantém a conexão entre 
* a Central e o broker MQTT realizando um ping no servidor MQTT a cada 30 segundos. Não possue parâmetros de entrada nem 
* retorno.  
*
*/
void SAPoTCentral_loop(); 

/**
* Essa função recebe uma mensagem e à estrutura utilizando os ponteiros SAPoTMessage do manipulador SAPoTCentral de acordo com 
* a instrução contida em seu cabeçalho. No caso da Central operar em modeo local-padrão (veja SAPoTCentral_create_options 
* e #SAPOTCENTRAL_OPTS_STDLOCAL) as mensagem são recebidas via MQTTmessageArrived() através do procotocolo MQTT. Não obstante,
* essa função também armazena no arquivo de log os parâmetros do cabeçalho de todas as mensagem recebidas. (veja SAPoTMessage_header).
*
* @param message Ponteiro @c void* para o espaço de memória da mensagem.
* @param messageLen Um número inteiro referente ao comprimento da mensagem. 
*
* @return Essa função retorna #SAPOTCENTRAL_FAILURE para o caso da versão de protocolo da mensagem não coincidir com versão de 
* protocolo da Central, ou retorna #SAPOTCENTRAL_SUCCESS caso contrário.  
*
*/
int SAPoTCentral_unpack_message(void* message, int messageLen);

/**
* Essa função pode ser utilizada se, e somente se a Central for configurada no modelo local-padrão através da definição 
* #SAPOTCENTRAL_OPTS_STDLOCAL (veja também SAPoTCentral_create_options). Após a execução da SAPoTCentral_unpack_message() 
* essa função utiliza da instrução contida no cabeçalho do manipulador SAPoTCentral para operar com tais possibilidades: 
* <ul>
* <li> 0x00: MYSQLregistration() </li> 
* <li> 0x01: </li>
* <li> 0x02: </li>
* <li> 0x03: CTRLactuator() </li>
* <li> 0x04: MYSQLaccess() </li>
* <li> 0x05: </li>
* <li> 0x06: MYSQLmodification() </li>
* </ul> 
*
* Além disso, após operar com sucesso envia-se uma mensagem de reconhecimento para o emissor da instrução.
*
* @param publish Ponteiro para uma função que realiza a transmissão de mensagens para comunicar a Central com os Clientes e 
* Usuários. Essa função deve receber uma string com o endereço do destinatário da mensagem, um @c void* com o conteúdo da 
* mensagem e um inteiro positivo com o comprimento total da mensagem.   
* 
* @return Essa função retorna #SAPOTCENTRAL_FAILURE em dois casos: Primeiramente se ocorrer algum erro na operação realizada, 
* ou em segundo momento se não for possível enviar a mensagem de reconhecimento. Contudo, essa função retorna #SAPOTCENTRAL_SUCCESS
* se a operação for realizada com sucesso e se a mensagem de reconhecimento for enviada ao emissor da instrução. 
*
*/
int SAPoTCentral_set_operation(int (*publish)(char*, void*, unsigned int));

/**
* Essa função mostra na tela através de um printf a definição do error contido no manipulador SAPoTCentral.
* 
* @retrun retorna o número do erro contido no manipulador SAPoTCentral. 
*
*/
int SAPoTCentral_error();


					/************************* Functions for MQTT **************************/
/**
* Essa função foi implementada através da biblioteca MQTTClient.h desenvolvida pelo projeto Eclipse Paho.  
* Ela tem o intuito de conectar a Central SAPoT ao broker MQTT, através de um objeto MQTTClient, da seguinte 
* maneira: Primeiramente, ela verifica a existência de conexão com o broker e se caso a conexão não exista, ela destrói 
* o possivel objeto MQTTClient que esteja com problemas na conexão e o cria novamente. 
* (veja <a href="https://www.eclipse.org/paho/files/mqttdoc/MQTTClient/html/_m_q_t_t_client_8h.html#a6e8231e8c47f6f67f7ebbb5dcb4c69c0">MQTTClient_isConnected()</a>, 
* <a href="https://www.eclipse.org/paho/files/mqttdoc/MQTTClient/html/_m_q_t_t_client_8h.html#ae700c3f5cfea3813264ce95e7c8cf498">MQTTClient_destroy()</a>, 
* <a href="https://www.eclipse.org/paho/files/mqttdoc/MQTTClient/html/_m_q_t_t_client_8h.html#a9a0518d9ca924d12c1329dbe3de5f2b6">MQTTClient_create()</a>). 
* Após criar e configurar um novo MQTTClient, essa função configura o recebimento de mensagens em modo assíncrono a partir da função 
* <a href="https://www.eclipse.org/paho/files/mqttdoc/MQTTClient/html/_m_q_t_t_client_8h.html#aad27d07782991a4937ebf2f39a021f83">MQTTClient_setCallbacks()</a> 
* que recebe a função MQTTmessageArrived() como parâmetro para tratar as mensagens recebidas.
* Em seguida, o MQTTClient requisita a conexão com o broker via 
* <a href="https://www.eclipse.org/paho/files/mqttdoc/MQTTClient/html/_m_q_t_t_client_8h.html#aaa8ae61cd65c9dc0846df10122d7bd4e">MQTTClient_connect()</a> 
* e se for estabelecida com sucesso, realiza-se a subscrição no tópico de mesmo nome do identificador da central (veja SAPoTCentral).      
*
* @see <a href="https://www.eclipse.org/paho">Projeto Eclipse Paho </a>   
*
*/
int MQTTconnect();

/**
* Essa função foi implementada baseada na biblioteca MQTTClient.h do projeto Eclipse Paho. Ela possui um padrão de argumentos 
* definidos para manipular as mensagem recebidas via MQTT, quando a função MQTTClient_setCallbacks() configura a recepção 
* assíncrona das mensagem (veja MQTTconnect()). Em sua rotina de execução, quando uma mensagem é recebida, ela primeiro extrai o 
* payload da mensagem MQTT e o estrutura em uma mensagem SAPoT via SAPoTCentral_unpack_message(). Em sequência, 
* se ocorrer tudo certo com a extração, ela evoca a função SAPoTCentral_set_operation() que utiliza da mensagem SAPoT estruturada
* anteriormente para realizar a instrução recebida. 
*
* @see <a href="https://www.eclipse.org/paho/files/mqttdoc/MQTTClient/html/_m_q_t_t_client_8h.html#aa42130dd069e7e949bcab37b6dce64a5"> Projeto Eclipse Paho: MQTTClient_messageArrived </a> 
*
*/
int MQTTmessageArrived(void* context, char* topicName, int topicLen, MQTTClient_message* MQTTmsg);

/**
* 
* Executa a conclusão do recebimento da mensagem via MQTTmessageArrived 
*
* @see <a href="https://www.eclipse.org/paho/files/mqttdoc/MQTTClient/html/_m_q_t_t_client_8h.html#abef83794d8252551ed248cde6eb845a6">Projeto Eclipse Paho: MQTTClient_deliveryComplete </a> 
*
*/
void MQTTdeliveryComplete(void* context, MQTTClient_deliveryToken dt);

/**
* 
* Trata a perda de conexão entre o MQTTClient e o servidor MQTT 
*
* @see <a href="https://www.eclipse.org/paho/files/mqttdoc/MQTTClient/html/_m_q_t_t_client_8h.html#a6bb253f16754e7cc81798c9fda0e36cf">Projeto Eclipse Paho: MQTTClient_connectionLost </a> 
*
*/
void MQTTconnectionLost(void* context, char* cause);

/**
* Essa função foi implementada baseada nas ferramentas disponibilizadas pela biblioteca MQTTClient.h do projeto Eclipse Paho.
* 
*
*/
int MQTTpublish(char* topic, void* payload, unsigned int payloadLen);
					
					
					/************************* Functions for MySQL *************************/
					
/**
* Função: Conecta o cliente ao servidor MySQL
*
*/					
int MYSQLconnect();

/**
* Função: Realiza as operações necessárias no banco de dados MYSQL 
* para cadastrar ou atualizar as informações de cadastrado de um cliente SAPoT.
*
*/
int MYSQLregistration();

/**
* Função: Realiza a etiquetagem de um cliente SAPoT ja cadastrados no banco de dados MYSQL.
*
*/
int MYSQLmodification();

/**
* Função: Acessa os dados de todos os clientes cadastrados no banco de dados MYSQL.
*
*/
int MYSQLaccess();


					/************************* Client control functions *************************/


int CTRLactuator(int (*publish)(char*, void*, unsigned int));


		

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


#endif /* SAPOTCENTRAL_H */
