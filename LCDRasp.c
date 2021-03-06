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
#include <time.h>

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
	
	int pausa;//
	int pulo;//
	int reset;//
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
	
	pins.pausa = 5;//           pausa
	pins.pulo = 19;//           pulo
	pins.reset = 26;//          reset
	
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

//Funcionamento qunado for IN, para os botoess

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
	
	GPIOExport(pins.pausa);
	GPIODirection(pins.pausa, IN);
	GPIOExport(pins.pulo);
	GPIODirection(pins.pulo, IN);
	GPIOExport(pins.reset);
	GPIODirection(pins.reset, IN);

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

// Inicializacao do LCD de 4bits 
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

// Funcao Escrita do Inicio
void texto_press_start()
{
	instruction4bit(0,0,0,0,0,0); // Limpar Display 
    instruction4bit(0,0,0,0,0,1);
	
	instruction4bit(0,0,0,0,0,0); // Home p/ Cursor
    instruction4bit(0,0,0,0,1,0);
	
	usleep(3000); // em microsegundos
	
	instruction4bit(1,0,0,1,0,1);
    instruction4bit(1,0,0,0,0,0); // Escrita de digito  P
	
	usleep(3000); // em microsegundos
	
	instruction4bit(1,0,0,1,1,1);
    instruction4bit(1,0,0,0,1,0); // Escrita de digito  r
	
	usleep(3000); // em microsegundos
	
	instruction4bit(1,0,0,1,1,0);
    instruction4bit(1,0,0,1,0,1); // Escrita de digito  e
	
	usleep(3000); // em microsegundos
	
	instruction4bit(1,0,0,1,1,1);
    instruction4bit(1,0,0,0,1,1); // Escrita de digito  s
	
	usleep(3000); // em microsegundos
	
	instruction4bit(1,0,0,1,1,1);
    instruction4bit(1,0,0,0,1,1); // Escrita de digito  s
	
	usleep(3000); // em microsegundos
	
	instruction4bit(0,0,0,0,0,1);
    instruction4bit(0,0,0,1,0,0); // 		Cursor para direita 14H
	
	usleep(3000); // em microsegundos
	
	instruction4bit(1,0,0,1,0,1);
    instruction4bit(1,0,0,0,1,1); // Escrita de digito  S
	
	usleep(3000); // em microsegundos
	
	instruction4bit(1,0,0,1,1,1);
    instruction4bit(1,0,0,1,0,0); // Escrita de digito  t
	
	usleep(3000); // em microsegundos
	
	instruction4bit(1,0,0,1,1,0);
    instruction4bit(1,0,0,0,0,1); // Escrita de digito  a
	
	usleep(3000); // em microsegundos
	
	instruction4bit(1,0,0,1,1,1);
    instruction4bit(1,0,0,0,1,0); // Escrita de digito  r
	
	usleep(3000); // em microsegundos
	
	instruction4bit(1,0,0,1,1,1);
    instruction4bit(1,0,0,1,0,0); // Escrita de digito  t
	
	usleep(3000); // em microsegundos
}

// Funcao Escrita da Perda
void texto_game_over()
{
	instruction4bit(0,0,0,0,0,0); // Limpar Display 
    instruction4bit(0,0,0,0,0,1);
	
	instruction4bit(0,0,0,0,0,0); // Home p/ Cursor
    instruction4bit(0,0,0,0,1,0);
	
	usleep(3000); // em microsegundos
	
	instruction4bit(1,0,0,1,0,0);
    instruction4bit(1,0,0,1,1,1); // Escrita de digito  G
	
	usleep(3000); // em microsegundos
	
	instruction4bit(1,0,0,1,1,0);
    instruction4bit(1,0,0,0,0,1); // Escrita de digito  a
	
	usleep(3000); // em microsegundos
	
	instruction4bit(1,0,0,1,1,0);
    instruction4bit(1,0,1,1,0,1); // Escrita de digito  m
	
	usleep(3000); // em microsegundos
	
	instruction4bit(1,0,0,1,1,0);
    instruction4bit(1,0,0,1,0,1); // Escrita de digito  e
	
	usleep(3000); // em microsegundos
	
	instruction4bit(0,0,0,0,0,1);
    instruction4bit(0,0,0,1,0,0); // 		Cursor para direita 14H
	
	usleep(3000); // em microsegundos
	
	instruction4bit(1,0,0,1,0,0);
    instruction4bit(1,0,1,1,1,1); // Escrita de digito  O
	
	usleep(3000); // em microsegundos
	
	instruction4bit(1,0,0,1,1,1);
    instruction4bit(1,0,0,1,1,0); // Escrita de digito  v
	
	usleep(3000); // em microsegundos
	
	instruction4bit(1,0,0,1,1,0);
    instruction4bit(1,0,0,1,0,1); // Escrita de digito  e
	
	usleep(3000); // em microsegundos
	
	instruction4bit(1,0,0,1,1,0);
    instruction4bit(1,0,0,0,1,0); // Escrita de digito  r
	
	usleep(3000); // em microsegundos	
}

//Numeracao 
static int 
numeracao(int numero)
{
	switch (numero)
	{
		case 0:
	instruction4bit(1,0,0,0,1,1); 
    instruction4bit(1,0,0,0,0,0); // Numero 0
     
	break;
		case 1:
	instruction4bit(1,0,0,0,1,1); 
    instruction4bit(1,0,0,0,0,1); // Numero 1
     
	break;	
		case 2:
	instruction4bit(1,0,0,0,1,1); 
    instruction4bit(1,0,0,0,1,0); // Numero 2
     
	break;
		case 3:
	instruction4bit(1,0,0,0,1,1); 
    instruction4bit(1,0,0,0,1,1); // Numero 3
     
	break;
		case 4:
	instruction4bit(1,0,0,0,1,1); 
    instruction4bit(1,0,0,1,0,0); // Numero 4
     
	break;
		case 5:
	instruction4bit(1,0,0,0,1,1); 
    instruction4bit(1,0,0,1,0,1); // Numero 5
     
	break;
		case 6:
	instruction4bit(1,0,0,0,1,1); 
    instruction4bit(1,0,0,1,1,0); // Numero 6
     
	break;
		case 7:
	instruction4bit(1,0,0,0,1,1); 
    instruction4bit(1,0,0,1,1,1); // Numero 7
     
	break;
		case 8:
	instruction4bit(1,0,0,0,1,1); 
    instruction4bit(1,0,1,0,0,0); // Numero 8
     
	break;
		case 9:
	instruction4bit(1,0,0,0,1,1); 
    instruction4bit(1,0,1,0,0,1); // Numero 9
     
	break;

	default:
     printf("Algo de errado não esta certo com os numeros\n");
	 return 0;
	}	
}

//Desenhar a pontuacao no LCD
void desenhar_score(int score)
{	
	instruction4bit(0,0,0,0,0,0); // Home p/ Cursor
    instruction4bit(0,0,0,0,1,0);
	
	for(int i = 0; i < 15; i++)
	{
		desloc_cursor(1);
	}
	numeracao(score);	
}
//Desena
void desenhar_score_Desena(int score)
{	
	instruction4bit(0,0,0,0,0,0); // Home p/ Cursor
    instruction4bit(0,0,0,0,1,0);
	
	for(int i = 0; i < 14; i++)
	{
		desloc_cursor(1);
	}
	numeracao(score);	
}

//Configuração do Boneco
void boneco()
{
	/*Acessar a CGRAM*/
	instruction4bit(0,0,0,1,0,0);
    instruction4bit(0,0,0,0,0,0); // Regiao da Memoria pra Caracteres Esp. 40H
	
	/*Caracter especial Boneco*/
	
	/* Andar 00 */
		
	instruction4bit(1,0,0,0,0,0);
    instruction4bit(1,0,1,1,0,0); // CH
	
	instruction4bit(1,0,0,0,0,0);
    instruction4bit(1,0,1,1,0,0); // CH
	
	instruction4bit(1,0,0,0,0,0);
    instruction4bit(1,0,0,0,0,0); // 0H
	
	instruction4bit(1,0,0,0,0,0);
    instruction4bit(1,0,1,1,1,0); // EH	
	
	instruction4bit(1,0,0,0,0,1);
    instruction4bit(1,0,1,1,0,0); // 1CH
	
	instruction4bit(1,0,0,0,0,0);
    instruction4bit(1,0,1,1,0,0); // CH
	
	instruction4bit(1,0,0,0,0,1);
    instruction4bit(1,0,0,0,1,0); // 12H
	
	instruction4bit(1,0,0,0,0,1);
    instruction4bit(1,0,0,0,1,1); // 13H	
	
	/* Andar 01 */....
	
	/*Acessar a CGRAM*/
	instruction4bit(0,0,0,1,0,0);
    instruction4bit(0,0,1,0,0,0); // Regiao da Memoria pra Caracteres Esp. 48H
	
	
	instruction4bit(1,0,0,0,0,0);
    instruction4bit(1,0,1,1,0,0); // CH
	
	instruction4bit(1,0,0,0,0,0);
    instruction4bit(1,0,1,1,0,0); // CH
	
	instruction4bit(1,0,0,0,0,0);
    instruction4bit(1,0,0,0,0,0); // 0H
	
	instruction4bit(1,0,0,0,0,1);
    instruction4bit(1,0,1,1,0,0); // EH	
	
	instruction4bit(1,0,0,0,0,0);
    instruction4bit(1,0,1,1,1,0); // 1CH
	
	instruction4bit(1,0,0,0,0,0);
    instruction4bit(1,0,1,1,0,0); // CH
	
	instruction4bit(1,0,0,0,0,1);
    instruction4bit(1,0,1,1,0,0); // 12H
	
	instruction4bit(1,0,0,0,0,0);
    instruction4bit(1,0,1,1,1,0); // 13H	
	
	
	/* Pular */...
	
	/*Acessar a CGRAM*/
	instruction4bit(0,0,0,1,0,1);
    instruction4bit(0,0,0,1,1,0); // Regiao da Memoria pra Caracteres Esp. 56H
	
	
	instruction4bit(1,0,0,0,0,0);
    instruction4bit(1,0,1,1,0,0); // 
	
	instruction4bit(1,0,0,0,0,0);
    instruction4bit(1,0,1,1,0,0); // 
	
	instruction4bit(1,0,0,0,0,0);
    instruction4bit(1,0,0,0,0,0); // 
	
	instruction4bit(1,0,0,0,0,1);
    instruction4bit(1,0,1,1,0,0); // 	
	
	instruction4bit(1,0,0,0,0,0);
    instruction4bit(1,0,1,1,1,0); // 
	
	instruction4bit(1,0,0,0,0,0);
    instruction4bit(1,0,1,1,0,0); // 
	
	instruction4bit(1,0,0,0,0,1);
    instruction4bit(1,0,1,1,0,0); // 
	
	instruction4bit(1,0,0,0,0,1);
    instruction4bit(1,0,0,0,0,0); //
}

//Deslocamento cursor a variar do sentido
void desloc_cursor(int sentido)
{
	instruction4bit(0,0,0,0,0,1);
    instruction4bit(0,0,0,sentido,0,0); // 		1 para direita, 0 para esquerda
}

//Deslocamento mensagem a variar do sentido, resulta no deslocamento da tela toda
void desloc_mem(int sentido)
{
	instruction4bit(0,0,0,0,0,1);
    instruction4bit(0,0,1,sentido,0,0); // 		1 para direita, 0 para esquerda
}



//Error
//Mostrar no LCD o boneco salvo
void desenhar_Boneco()
{
	instruction4bit(0,0,0,0,0,0); // Home p/ Cursor
    instruction4bit(0,0,0,0,1,0);
	
	for(int i = 0; i < 41; i++)
	{
		desloc_cursor(1);
	}		
	instruction4bit(1,0,0,0,0,0); 
    instruction4bit(1,0,0,0,0,0);// Display Primeiro Caracter Customizado 00H
}

//Desenhar no LCD o obstaculo 
void desenhar_bloco()
{
	instruction4bit(0,0,0,0,0,0); // Home p/ Cursor
    instruction4bit(0,0,0,0,1,0);
	
	for(int i = 0; i < 56; i++)
	{
		desloc_cursor(1);
	}		
    instruction4bit(1, 0, 1, 1, 1, 1);
    instruction4bit(1, 0, 1, 1, 1, 1);

	for(int j = 55; j >49 ; j--)
	{
		//desloc_mem(0);
	}

}


//Funcao do botao
int pressButton(int pino)
{	
	return GPIORead(pino);
}


//Funcao do contador de 1s para o score
void tempoSegundos()
{
	clock_t inicio, espera;
  
    espera = (1000/1000) * CLK_TCK;// 1 segundo = 1000 em milisegundos
    inicio = clock( );

       while( (clock( ) - inicio) < espera ){};	
}

//Gerador randomico
int randomBlock()  
{
        rand.seed(int(tempoSegundos()))
        return rand.next(3) + 3
}

// Onde tudo começa
// FPS 410 milisegundos 
int main(int argc, char *argv[])
{
	int state = 0;	
	int tempo = 0;
	int pause = 0;
	int telaInit = 0;
	int ar = 0;
	int ondebloco = 0;
	int comprimentoBloco = randomBlock();
	int animacao = 0;
	int buttPulo = 0;
	int buttPausa = 0;
	int andar = 0;	
	 
	printf("Configurando a pinagem....\n");
	setupPins();	
	sleep(3);
	printf("Inicializando o LCD....\n");	
	initialize4bit();	
	printf("Criando boneco....\n");
	boneco();// Cria o boneco customizado		
		
	
	for(;;)
	{			
		if(state == 0)// Texto de inicio
			{
				tempo++;
				usleep(50000); // em microsegundos
				if (tempo > 4) 
				{
					if (telaInit) 
					{
						texto_press_start( );
					} else {
						instruction4bit(0,0,0,0,0,0); // Limpar Display 
						instruction4bit(0,0,0,0,0,1);
					}
					tempo = 0;
					telaInit = !telaInit;
                }
				usleep(50000); // em microsegundos
				if (pressButton(5) == 0) //Apertar o botao para começar
				{
					state = 1;
				}
			}
		
		else if(state == 1)// Tempo de execucao
			{		
				if (pressButton(5) == 0 && !animacao)
				{
					buttPulo = 1;
					animacao = 0;
                }
				
				tempo = pressButton(19) == 0;
				if (tempo && tempo != buttPausa) 
				{
					pause = !pause;
                }
				
				buttPausa = tempo;
						
				if (pause != 0)
				{
					if(pressButton(5)== 1)
					{
						//Contador e desenhar bloco
						if()//pulo e altura = mudar boneco
						{
							
						}
					}
					if (pressButton(26) == 0) 
						{
							game_state = 0;
                        }
					
				}
				if(pause && pressButton(5) == 1) {//Apertar o botao para começar
					pause = !pause;
				}
				
				// o Que vai aparecer no LCD 
				
				instruction4bit(0,0,0,0,0,0); // Limpar Display 
				instruction4bit(0,0,0,0,0,1);
				
				printf("Teste Score....\n");
				for(int t = 0; t < 10; t++)
				{
					for(int k = 0; k < 10; k++)
					{
						tempoSegundos();
						desenhar_score(k);
						desenhar_score_Desena(t);	
					}			
				}
				desenhar_Boneco();// Desenha o boneco
				desenhar_bloco();// Deslocar mensagem leva a tela toda
				usleep(300); // em microsegundos	
				if air > 0
				{
					button_pressed = false
                }
			}
		
		
		else if(state == 2)// Pausar
			{
				if (pressButton(5) == 0) 
					{                      
						state = 1;
					}				
			}
		
		else
			{
				instruction4bit(0,0,0,0,0,0); // Limpar Display 
				instruction4bit(0,0,0,0,0,1);
				
				texto_game_over();
				
				return(0);
								
			}		
	}
		
		printf("Finalizando programa, desconfigurando os Pinos....\n");
		//unsetPins(); //Funcionando?
}