/* 
	Aplicacao do PB2 no Raspberry
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define IN  0
#define OUT 1

#define LOW  0
#define HIGH 1

// Global
struct gpio
{	int rs;
	int rw;
	int enable;
	int data[3];
};

struct gpio pins;

// Pinagem 
void pinagem()
{
	pins.rs = 22;       // Register Select
	pins.rw = 3;        // Register Write
	pins.enable = 17;   // Enable
	pins.data[0] = 25;	// Data 4 em 8 bit    LSB
	pins.data[1] = 24;	// Data 5 em 8 bit
	pins.data[2] = 23; 	// Data 6 em 8 bit
	pins.data[3] = 18; 	// Data 7 em 8 bit    MSB
}

// Exportando o pino
static int
GPIOExport(int pin)
{
#define BUFFER_MAX 3
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open export for writing!\n");
		return(-1);
	}

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return(0);
}

// Unexport do pino
static int
GPIOUnexport(int pin)
{
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/gpio/unexport", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open unexport for writing!\n");
		return(-1);
	}

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return(0);
}

// Configurando o pino como INPUT ou OUTPUT
static int
GPIODirection(int pin, int dir)
{
	int arquivo = 0;
	char buffer[3];
	char path[35];

    snprintf(path, 35, "/sys/class/gpio/gpio%d/direction", pin);
    arquivo = open (path, O_WRONLY);
    if (arquivo==-1)
    {
    	return -1;
    }
    snprintf(buffer, 3, "%d", pin);
    if (write( arquivo, ((dir == IN)?"in":"out"), 3 )==-1)
    {
    	close(arquivo);
    	return -1;
    }

    close(arquivo);
    return 0;
}

// Efetuando a leitura no pino
static int
GPIORead(int pin)
{
#define VALUE_MAX 30
	char path[VALUE_MAX];
	char value_str[3];
	int fd;

	usleep(120);
	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_RDONLY);
	if (-1 == fd) {
		fprintf(stderr, "Falha em Abrir a gpio para a Leitura!\n");
		return(-1);
	}

	if (-1 == read(fd, value_str, 3)) {
		fprintf(stderr, "Falha na leitura do Valor!\n");
		return(-1);
	}

	close(fd);

	return(atoi(value_str));
}

// Escrita no pino
static int
GPIOWrite(int pin, int value)
{
	usleep(120);
	static const char s_values_str[] = "01";

	//printf("Pino: %d;\nValor: %d;\n", pin, value); //Checando valor do pino
	char path[VALUE_MAX];
	int fd;

	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Falha em Abrir a gpio para Escrita!\n");
		return(-1);
	}

	if (1 != write (fd, ((value == HIGH)?"1":"0"), 1)) {
		fprintf(stderr, "Falha em escrever o Valor!\n");
		return(-1);
	}

	close(fd);
	return(0);
}

// Intrucao de 4 bit com 2 de rs e rw
void instruction4bit(int rs, int rw, int data1, int data2, int data3, int data4)
{
	GPIOWrite(pins.enable, 1);
	GPIOWrite(pins.rs, rs);
	GPIOWrite(pins.rw, rw);
	GPIOWrite(pins.data[0], data4); //	LSB
	GPIOWrite(pins.data[1], data3);
	GPIOWrite(pins.data[2], data2);
	GPIOWrite(pins.data[3], data1); // MSB
	usleep(500);
	GPIOWrite(pins.enable, 0);
	usleep(500);
}

// Configurando os Pinos
void setupPins()
{
 	pinagem();

	GPIOExport(pins.rs);
	GPIODirection(pins.rs, OUT);
	GPIOExport(pins.rw);
	GPIODirection(pins.rw, OUT);
	GPIOExport(pins.enable);
	GPIODirection(pins.enable, OUT);

	int i = 0;
	for(i = 0; i < 4; i++)
	{
		GPIOExport(pins.data[i]);
		GPIODirection(pins.data[i], OUT);
	}
}

// Desconfigurando os Pinos
void unsetPins()
{
	pinagem();

	GPIOUnexport(pins.rs);
	GPIOUnexport(pins.rw);
	GPIOUnexport(pins.enable);

	int i = 0;
	for(i = 0; i < 4; i++)
	{
		GPIOUnexport(pins.data[i]);
	}
}

// Inicializacao do LCD de 4bits 1 
void initialize4bit()
{
	usleep(100000); // in miliseconds
	instruction4bit(0,0,0,0,1,1); // Function Set 3H
	usleep(5000); // in miliseconds
	instruction4bit(0,0,0,0,1,1); // Function Set 3H
	usleep(100); // in microseconds
	instruction4bit(0,0,0,0,1,1); // Function Set 3H
	usleep(100); // in microseconds
	instruction4bit(0,0,0,0,1,0); // Function Set 2H
	usleep(100); // in microseconds

	/*Controlador do LCD configurado no modo de 4-bit.*/

	instruction4bit(0,0,0,0,1,0); // Function Set 2H
	instruction4bit(0,0,1,0,0,0); // Function Set 8H
	usleep(100); // in microseconds
	instruction4bit(0,0,0,0,0,0); // Function Set 0H
	instruction4bit(0,0,1,0,0,0); // Function Set 8H
	usleep(100); // in microseconds
	instruction4bit(0,0,0,0,0,0); // Function Set 0H
	instruction4bit(0,0,0,0,0,1); // Function Set 1H
	usleep(3000); // in miliseconds
	instruction4bit(0,0,0,0,0,0); // Function Set 0H
	instruction4bit(0,0,0,1,1,0); // Function Set 6H
	usleep(100); // in microseconds

	instruction4bit(0,0,0,0,0,0); // Function Set 0H
	instruction4bit(0,0,1,1,0,0); // Function Set CH - Liga o Display, Desliga o Cursor sem piscar
	usleep(100); // in microseconds
}


// Funcao teste do Escrita
void testeTexto()
{
	instruction4bit(0,0,0,0,0,0); // Home p/ Cursor
    instruction4bit(0,0,0,0,1,0);
	usleep(3000); // em microsegundos
	instruction4bit(0,0,0,0,0,1);
    instruction4bit(0,0,0,1,0,0); // 		Cursor para direita 14H
	usleep(3000); // em microsegundos
	instruction4bit(0,0,0,0,0,0);
    instruction4bit(0,0,1,1,1,1); // Liga o Display, Liga o Cursor piscando
	usleep(3000); // em microsegundos
	instruction4bit(1,0,0,1,0,1);
    instruction4bit(1,0,0,0,0,0); // Escrita de digito 48H - P
	usleep(3000); // em microsegundos
	instruction4bit(0,0,0,0,0,1);
    instruction4bit(0,0,0,1,0,0); // 		Cursor para direita 14H
	usleep(3000); // em microsegundos
	instruction4bit(1,0,0,1,1,0);
    instruction4bit(1,0,0,0,1,0); // Escrita de digito 45H - r
	usleep(3000); // em microsegundos
	instruction4bit(0,0,0,0,0,1);
    instruction4bit(0,0,0,1,0,0); // 		Cursor para direita 14H
	usleep(3000); // em microsegundos
	instruction4bit(1,0,0,1,1,0);
    instruction4bit(1,0,0,1,0,1); // Escrita de digito 45H - e
	usleep(3000); // em microsegundos
	instruction4bit(0,0,0,0,0,1);
    instruction4bit(0,0,0,1,0,0); // 		Cursor para direita 14H
	usleep(3000); // em microsegundos
	instruction4bit(1,0,0,1,1,1);
    instruction4bit(1,0,0,0,1,1); // Escrita de digito 45H - s
	usleep(3000); // em microsegundos
	instruction4bit(1,0,0,1,1,1);
    instruction4bit(1,0,0,0,1,1); // Escrita de digito 45H - s
	usleep(3000); // em microsegundos
	instruction4bit(0,0,0,0,0,1);
    instruction4bit(0,0,0,1,0,0); // 		Cursor para direita 14H
	usleep(3000); // em microsegundos
	instruction4bit(1,0,0,1,0,1);
    instruction4bit(1,0,0,0,1,1); // Escrita de digito 45H - S
	usleep(3000); // em microsegundos
	instruction4bit(0,0,0,0,0,1);
    instruction4bit(0,0,0,1,0,0); // 		Cursor para direita 14H
	usleep(3000); // em microsegundos
	instruction4bit(1,0,0,1,1,0);
    instruction4bit(1,0,0,1,0,0); // Escrita de digito 45H - t
	usleep(3000); // em microsegundos
	instruction4bit(0,0,0,0,0,1);
    instruction4bit(0,0,0,1,0,0); // 		Cursor para direita 14H
	usleep(3000); // em microsegundos
	instruction4bit(1,0,0,1,1,0);
    instruction4bit(1,0,0,0,0,1); // Escrita de digito 45H - a
	usleep(3000); // em microsegundos
	instruction4bit(0,0,0,0,0,1);
    instruction4bit(0,0,0,1,0,0); // 		Cursor para direita 14H
	usleep(3000); // em microsegundos
	instruction4bit(1,0,0,1,1,0);
    instruction4bit(1,0,0,0,1,0); // Escrita de digito 45H - r
	usleep(3000); // em microsegundos
	instruction4bit(0,0,0,0,0,1);
    instruction4bit(0,0,0,1,0,0); // 		Cursor para direita 14H
	usleep(3000); // em microsegundos
	instruction4bit(1,0,0,1,1,0);
    instruction4bit(1,0,0,1,0,0); // Escrita de digito 45H - t
	usleep(3000); // em microsegundos
}

//Teste do boneco
void testeBoneco()
{
	instruction4bit(0,0,0,0,0,0); // Home p/ Cursor
    instruction4bit(0,0,0,0,1,0);
	usleep(3000); // em microsegundos
	instruction4bit(0,0,0,0,0,0); 
    instruction4bit(0,0,0,0,0,1);// Clear Display
	usleep(3000); // em microsegundos
	
	/*Acesso aos Caracteres Especiais*/
	
	instruction4bit(0,0,0,0,0,0); 
    instruction4bit(0,0,0,0,0,0);// Display Primeiro Caracter Customizado 00H
	
}

//Numeracao
static int 
numeracao(int numero)
{
	switch (numero)
	{
		case 0:
	instruction4bit(0,0,0,0,1,1); 
    instruction4bit(0,0,0,0,0,0); // Numero 0
     
	break;
		case 1:
	instruction4bit(0,0,0,0,1,1); 
    instruction4bit(0,0,0,0,0,1); // Numero 1
     
	break;	
		case 2:
	instruction4bit(0,0,0,0,1,1); 
    instruction4bit(0,0,0,0,1,0); // Numero 2
     
	break;
		case 3:
	instruction4bit(0,0,0,0,1,1); 
    instruction4bit(0,0,0,0,1,1); // Numero 3
     
	break;
		case 4:
	instruction4bit(0,0,0,0,1,1); 
    instruction4bit(0,0,0,1,0,0); // Numero 4
     
	break;
		case 5:
	instruction4bit(0,0,0,0,1,1); 
    instruction4bit(0,0,0,1,0,1); // Numero 5
     
	break;
		case 6:
	instruction4bit(0,0,0,0,1,1); 
    instruction4bit(0,0,0,1,1,0); // Numero 6
     
	break;
		case 7:
	instruction4bit(0,0,0,0,1,1); 
    instruction4bit(0,0,0,1,1,1); // Numero 7
     
	break;
		case 8:
	instruction4bit(0,0,0,0,1,1); 
    instruction4bit(0,0,1,0,0,0); // Numero 8
     
	break;
		case 9:
	instruction4bit(0,0,0,0,1,1); 
    instruction4bit(0,0,1,0,0,1); // Numero 9
     
	break;

	default
     printf("Algo de errado não esta certo com os numeros\n");
	 return 0;
	}
	
}

//Configuração do Boneco
void boneco()
{
	/*Acessar a CGROM*/
	instruction4bit(0,0,0,1,0,0);
    instruction4bit(0,0,0,0,0,0); // Regiao da Memoria pra Caracteres Esp. 40H
	
	/*Caracter especial Boneco*/
	
	instruction4bit(0,0,0,0,0,0);
    instruction4bit(0,0,1,1,0,0); // CH
	instruction4bit(0,0,0,0,0,0);
    instruction4bit(0,0,1,1,0,0); // CH
	instruction4bit(0,0,0,0,0,0);
    instruction4bit(0,0,0,0,0,0); // 0H
	instruction4bit(0,0,0,0,0,0);
    instruction4bit(0,0,1,1,1,0); // EH
	instruction4bit(0,0,0,0,0,1);
    instruction4bit(0,0,1,1,0,0); // 1CH
	instruction4bit(0,0,0,0,0,0);
    instruction4bit(0,0,1,1,0,0); // CH
	instruction4bit(0,0,0,0,0,1);
    instruction4bit(0,0,0,0,1,0); // 12H
	instruction4bit(0,0,0,0,0,1);
    instruction4bit(0,0,0,0,1,1); // 13H	
}

//Configuração do obstaculo
void obstaculo()
{
	/*Acessar a CGROM*/
	instruction4bit(0,0,0,1,0,0);
    instruction4bit(0,0,0,0,0,0); // Regiao da Memoria pra Caracteres Esp. 40H
	
	/*Caracter especial do obstaculo*/
	
	instruction4bit(0,0,0,0,0,1);
    instruction4bit(0,0,1,1,1,1); // 1FH
	instruction4bit(0,0,0,0,0,1);
    instruction4bit(0,0,1,1,1,1); // 1FH
	instruction4bit(0,0,0,0,0,1);
    instruction4bit(0,0,1,1,1,1); // 1FH
	instruction4bit(0,0,0,0,0,1);
    instruction4bit(0,0,1,1,1,1); // 1FH
	instruction4bit(0,0,0,0,0,1);
    instruction4bit(0,0,1,1,1,1); // 1FH
	instruction4bit(0,0,0,0,0,1);
    instruction4bit(0,0,1,1,1,1); // 1FH
	instruction4bit(0,0,0,0,0,1);
    instruction4bit(0,0,1,1,1,1); // 1FH
	instruction4bit(0,0,0,0,0,1);
    instruction4bit(0,0,1,1,1,1); // 1FH
}


// Matriz que percorre as 32 posicoes do LCD att seus componentes #Opc01
void matrizLCD()
{//Percorre de 0 a 15, quando pula mais uma posicao ele vai pra linha de baixo
	instruction4bit(0,0,0,0,0,0);
    instruction4bit(0,0,0,0,1,0); // Home Cursor
}

//Deslocamento Obstáculo
void deslocObs() //Ultima posicao da segunda tela e se desloca para a esquerda
{
	
}

//Colisão do boneco com o obstaculo
int colisao()
{
	
}

// Onde tudo começa
// FPS 410 milisegundos 
int main(int argc, char *argv[])
{
	//Utilizar matriz ou fila pra mapear as 16 telas do LCD para poder manipular melhor
	int state = 0;
	if(state == 0)
	{// Espera para apertar o botao e comecar o jogo
		printf("Tela de Inicio\n"); // Comecar Piscando
	}
	else if(state == 1)
	{// Durante a aplicacao
		printf("jogatina\n"); //Dchecar movimentacao do boneco e deslocamento do obstaculo
	}
	else if(state == 2)
	{// Aconteceu a colisao do boneco com o obstaculo e pausa
		printf("Game Over\n");
	}
	
	/*Testando funcoes separadas*/
	printf("Configurando a pinagem....\n");
	setupPins();	
	sleep(10);
	
	printf("Inicializando o LCD....\n");	
	initialize4bitLinha();
	testeTexto();
	
	printf("Teste Score....\n");
	pontuacao();	
	sleep(10);	
	
	printf("Finalizando programa, desconfigurando os Pinos....\n");
	unsetPins();
}