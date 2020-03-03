/* 
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

//Global
struct gpio
{	int rs;
	int rw;
	int enable;
	int data[7];
};

int score = 0;

struct gpio pins;

//Pinagem 
void pinning()
{
	pins.rs = 22;       // P1-03
	pins.rw = 3;        // P1-05
	pins.enable = 17;   // P1-07
	pins.data[0] = 25;  // P1-11 LSB
	pins.data[1] = 24;  // P1-13
	pins.data[2] = 23;  // P1-15
	pins.data[3] = 18;  // P1-19 MSB
}

//Exportando o pino
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

//Unexport do pino
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

//Configurando o pino como INPUT ou OUTPUT
static int
GPIODirection(int pin, int dir)
{
	int arquive = 0;
	char buffer[3];
	char path[35];

    snprintf(path, 35, "/sys/class/gpio/gpio%d/direction", pin);
    arquive = open (path, O_WRONLY);
    if (arquive==-1)
    {
    	return -1;
    }
    snprintf(buffer, 3, "%d", pin);
    if (write( arquive, ((dir == IN)?"in":"out"), 3 )==-1)
    {
    	close(arquive);
    	return -1;
    }

    close(arquive);
    return 0;
}

//Efetuando a leitura no pino
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
		fprintf(stderr, "Failed to open gpio value for reading!\n");
		return(-1);
	}

	if (-1 == read(fd, value_str, 3)) {
		fprintf(stderr, "Failed to read value!\n");
		return(-1);
	}

	close(fd);

	return(atoi(value_str));
}

//Escrita no pino
static int
GPIOWrite(int pin, int value)
{
	usleep(120);
	static const char s_values_str[] = "01";

	//printf("Pino: %d;\nValor: %d;\n", pin, value);
	char path[VALUE_MAX];
	int fd;

	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio value for writing!\n");
		return(-1);
	}

	if (1 != write (fd, ((value == HIGH)?"1":"0"), 1)) {
		fprintf(stderr, "Failed to write value!\n");
		return(-1);
	}

	close(fd);
	return(0);
}

/* Intrução de 8 bit + 2 de rs e rw
void instruction(int rs, int rw, int data1, int data2, int data3, int data4, int data5, int data6, int data7, int data8)
{
	GPIOWrite(pins.enable, 0);
	usleep(5);
	GPIOWrite(pins.enable, 1);// falling edge
	GPIOWrite(pins.rs, rs);
	GPIOWrite(pins.rw, rw);
	GPIOWrite(pins.data[0], data8); // LSB
	GPIOWrite(pins.data[1], data7);
	GPIOWrite(pins.data[2], data6);
	GPIOWrite(pins.data[3], data5);
	GPIOWrite(pins.data[4], data4);
	GPIOWrite(pins.data[5], data3);
	GPIOWrite(pins.data[6], data2);
	GPIOWrite(pins.data[7], data1); // MSB
	usleep(1);
	GPIOWrite(pins.enable, 0);
	usleep(5);
}*/

// Intrução de 4 bit + 2 de rs e rw
void instruction4bit(int rs, int rw, int data1, int data2, int data3, int data4)
{
	GPIOWrite(pins.enable, 1);
	GPIOWrite(pins.rs, rs);
	GPIOWrite(pins.rw, rw);
	GPIOWrite(pins.data[0], data4); //LSB
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
 	pinning();

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
	pinning();

	GPIOUnexport(pins.rs);
	GPIOUnexport(pins.rw);
	GPIOUnexport(pins.enable);

	int i = 0;
	for(i = 0; i < 4; i++)
	{
		GPIOUnexport(pins.data[i]);
	}
}

//Reset
void restartPins()
{
	unsetPins();
	setupPins();
}

//Inicialização do LCD de 4bits
void initialize4bitNovo()
{
	usleep(100000); // in miliseconds
	instruction4bit(0,0,0,0,1,0); // Real Function Set 2H
	usleep(100); // in microseconds

	/*The LCD controller is now in the 4-bit mode.*/

	instruction4bit(0,0,0,0,1,0); // Real Function Set 2H
	instruction4bit(0,0,1,0,0,0); // Real Function Set 8H
	usleep(100); // in microseconds
	instruction4bit(0,0,0,0,0,0); // Real Function Set 0H
	instruction4bit(0,0,1,1,1,1); // Real Function Set 8H
	usleep(100); // in microseconds
	instruction4bit(0,0,0,0,0,0); // Real Function Set 0H
	instruction4bit(0,0,0,0,0,1); // Real Function Set 8H
	usleep(3000); // in miliseconds
	instruction4bit(0,0,0,0,0,0); // Real Function Set 0H
	instruction4bit(0,0,0,1,1,0); // Real Function Set 6H
	usleep(100); // in microseconds

	/*Initializing end - display off*/
	instruction4bit(0,0,0,0,0,0); // Real Function Set 0H
	instruction4bit(0,0,1,1,1,1); // Real Function Set 6H
	usleep(100);
	instruction4bit(0,0,0,1,0,0); // Real Function Set 0H
	instruction4bit(0,0,0,0,0,1); // Real Function Set 1H
	usleep(100); // in microseconds
}

//Inicialização do LCD de 4bits
void initialize4bit()
{
	usleep(100000); // in miliseconds
	instruction4bit(0,0,0,0,1,1); // Function Set 3H
	usleep(5000); // in miliseconds
	instruction4bit(0,0,0,0,1,1);
	usleep(100); // in microseconds
	instruction4bit(0,0,0,0,1,1);
	usleep(100); // in microseconds
	instruction4bit(0,0,0,0,1,0); // Real Function Set 2H
	usleep(100); // in microseconds

	/*The LCD controller is now in the 4-bit mode.*/

	instruction4bit(0,0,0,0,1,0); // Real Function Set 2H
	instruction4bit(0,0,1,0,0,0); // Real Function Set 8H
	usleep(100); // in microseconds
	instruction4bit(0,0,0,0,0,0); // Real Function Set 0H
	instruction4bit(0,0,1,0,0,0); // Real Function Set 8H
	usleep(100); // in microseconds
	instruction4bit(0,0,0,0,0,0); // Real Function Set 0H
	instruction4bit(0,0,0,0,0,1); // Real Function Set 1H
	usleep(3000); // in miliseconds
	instruction4bit(0,0,0,0,0,0); // Real Function Set 0H
	instruction4bit(0,0,0,1,1,0); // Real Function Set 6H
	usleep(100); // in microseconds

	/*Initializing end - display off*/
	instruction4bit(0,0,0,0,0,0); // Real Function Set 0H
	instruction4bit(0,0,1,1,0,0); // Real Function Set 6H
	usleep(100); // in microseconds
}

//Função teste do Escrita
void helloWorld()
{
	instruction(0,0,0,0,0,0,0,0,1,0); // HOME
	usleep(3000); // in microseconds
	instruction(0,0,0,0,0,1,0,1,0,0); // Cursor para direita 14H
	usleep(3000); // in microseconds
	instruction(0,0,0,0,0,0,1,1,1,1); // Liga display Liga Cursor
	usleep(3000); // in microseconds
	instruction(1,0,0,1,0,0,1,0,0,0); // Escrita de digito 48H - H
	usleep(3000); // in microseconds
	//instruction(0,0,0,0,0,1,0,1,0,0); // Cursor para direita 14H
	//usleep(3000); // in microseconds
	instruction(1,0,0,1,0,0,0,1,0,1); // Escrita de digito 45H - E
	usleep(3000); // in microseconds
	//instruction(0,0,0,0,0,1,0,1,0,0); // Cursor para direita 14H
	//usleep(3000); // in microseconds

}

/*
//Contador de Unidade 
void counterUN()
{
	instruction(0,0,0,0,0,0,0,0,1,0); // HOME
	usleep(3000); // in microseconds

}

//Contador de Dezena
void counterDE()
{
	instruction(0,0,0,0,0,0,0,0,1,0); // HOME
	usleep(3000); // in microseconds

}

//Contador de Centena
void counterCE()
{
	instruction(0,0,0,0,0,0,0,0,1,0); // HOME
	usleep(3000); // in microseconds

}

// Junção dos Contadores
void loop()
{
	
score++;
}
*/

//Onde tudo começa
int main(int argc, char *argv[])
{
	printf("Setting up pins..\n");
	setupPins();
	
	printf("Initializing 4bit..\n");
	initialize4bit();
	
	sleep(10);
	restartPins();
	printf("Initializing new 4bit..\n");
	initialize4bitNovo();

	sleep(10);
	
	printf("Ending program, unsetting pins...\n");
	unsetPins();
}