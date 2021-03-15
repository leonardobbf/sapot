/*
* Brasília 11 de dezembro de 2019.
* Autor: Leonardo Brandão Borges de Freitas (contato.leonardobbf@gmail.com)
*
* Unidade de Controle Centralizada SAPoT (UCCSAPoT): 
* 	Construída sobre os protocolos de comunicação MQTT, de tradução e operação SAPoT e de banco de dados SQL. 
*	Tem o intuito de coordenar instruções e armazenar dados em uma rede IoT do tipo UCS (Usuário/Central/Sentinela).
*
*	Documentação: 
* 	
*/

/* Bibliotecas */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <MQTTClient.h>
#include <mysql/mysql.h>
#include "SAPoTCentral.h"

/* Declaração de Objetos */
SAPoTCentral SAPoTcentral;

void signalHandling(int signum){

	printf("Finalizando a UCC...\n");
	SAPoTCentral_end();
	exit(1);

}

/* Função Principal */
int main(int argc, char *argv[]) {

	signal(SIGINT, signalHandling);

	//Definindo o identificador da central
	const char* centralId = "00:00:00:00:00:00";
	
	//Configurando as opções de inicialização da central
	//SAPoTCentral_create_options SAPoTopts = {MQTT, {"10.10.40.84", "1883", "LDAP", NULL}, SQL, {"localhost", "3306", "ucc", "uccpass123", "db_UCC"}};
	//SAPoTCentral_create_options SAPoTopts = {MQTT, {"10.10.40.84", "1883", "LDAP", NULL}, SQL, {"10.10.40.84", "3306", "ucc", "uccpass123", "db_UCC"}};
	SAPoTCentral_create_options SAPoTopts = {MQTT, {"localhost", "1883", NULL, NULL}, SQL, {"localhost", "3306", "ucc", "uccpass123", "db_UCC"}};

	//Iniciando os serviços da Central
	if(SAPoTCentral_begin(&SAPoTcentral, &SAPoTopts, centralId) != SAPOTCENTRAL_SUCCESS){
		printf("SAPoTCentral_begin error (%d)", SAPoTcentral.error);
		return -1;
	}
		
	//Entrando em modo loop	
	SAPoTCentral_loop(); 
			
	return 0;

}
