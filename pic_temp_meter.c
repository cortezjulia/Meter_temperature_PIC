#include <16f887.h>
#include <regs_887.h>
#use delay (clock=4MHz)
#fuses INTRC_IO
#include <display.h>
#include <teclado.h>

void interrupt_timer1(void);      //fun��o para interrup��o do timer1
void ConfiguraAD(void);           //fun��o onde h� os bits de configura��o do AD
void conf_timer1(int H, int L);   //fun��o onde h� os bits de configura��o do timer1 de acordo com os diferentes valores carregados no TMR1
int16 AD_select_convert(int ANX); //fun��o para altern�ncia dos canais do AD
int canal_AD=0;                   //vari�vel que � incrementada para controle do canal que ser� convertido 
int16 Temperatura1=0, Temperatura2=0, Temperatura3=0, Temperatura4=0, Temperatura5=0, Media=0,contatempo; //vari�vel para receber o valor convertido do Lm35
int  OpcaoPeriodo=0, OpcaoIntervalo=0; //vari�vel para escolha de per�odo e intervalo entre as medi��es
int flag=0;

void ConfiguraAD(void)
{  //rel�gio gerado por oscilador interno no microcontrolador; resultado justificado � direita; tens�o de alimenta��o interna  
   ADCS1=1;    ADCS0=1;
   ADFM=1;
   VCFG1=0;
   VCFG0=0; 

}

void conf_timer1(int H, int L)
{
   //prescaler do TMR1 1:8; oscilador de baixa pot�ncia desligado; n�o sincroniza a entrada externa de clock; modo temporizador
   //TMR1L e TMR1H s�o carregados com valores para 1s e 5s
   T1CKPS0=1; T1CKPS1=1;  //1:8
   T1OSCEN=0;
   T1SYNC=1;
   TMR1CS=0;

   TMR1ON=0;
   TMR1H=H;  TMR1L=L;
}

int16 AD_select_convert(int ANX)
{
//A cada valor passado para ANX no void main, um canal � selecionado
   int16 Temp=0, Palavra=0;
   ADON=1;//AD ligado
   if(ANX==0)
   {
      CHS3=0; CHS2=0; CHS1=0; CHS0=0; delay_ms(1);//Canal 0
   }
   else if(ANX==1)
   {
      CHS3=0; CHS2=0; CHS1=0; CHS0=1; delay_ms(1);//Canal 1
   }
   else if(ANX==2)
   {
      CHS3=0; CHS2=0; CHS1=1; CHS0=0; delay_ms(1);//Canal 2
   }
   else if(ANX==3)
   {
      CHS3=0; CHS2=0; CHS1=1; CHS0=1; delay_ms(1);//Canal 3
   }
   else if(ANX==4)
   {
      CHS3=0; CHS2=1; CHS1=0; CHS0=0; delay_ms(1);//Canal 4
   }
   
   GO_DONE=1; //conversor em curso
   
   while(GO_DONE);//aguarda fim da convers�o
   
   Palavra=ADRESH*256+ADRESL; //concatena os valores dos registros de dados do AD em uma vari�vel
   Temp=500*Palavra/1023;     //c�lculo para convers�o da T do Lm35
   //EX: P/ 22�C - 220mV na entrada do AD, devido ao Lm35 (1mV por �C), esse valor ao passar para os registros do AD,
   //� concatenado na vari�vel palavra,que � multiplicada por 500 e dividida por 1023(10^2), resultando na T final
   
   ADON=0;                    // AD desligado
   
   return(Temp); //entrega para o main a temperatura
}

void main(void)
{
   
   TRISA=0xFF; //canais AD como entrada
   ANS0=1; ANS1=1; ANS2=1; ANS3=1; ANS4=1; //pinos configurados como anal�gicos
   TRISD=0b11110000; //configura��o de entrada/saida dos pinos do teclado
   TRISC=0; //saida para o display
   TRISE=0; //saida para o display
   inicializa(); //inicializa o display
   limpa();      //garante a limpeza do mesmo
   
   enable_interrupts(GLOBAL); //habilita a interrup��o, equivalente ao bit GIE e PEIE 
   enable_interrupts(INT_TIMER1); //habilita a interrup��o, equivalente ao bit TMR1IE
   ConfiguraAD(); //fun��o de configura��o do AD � chamada

   //a seguir, h� a estrutura de menu de op��es para o periodo para o usu�rio
   strcpy(texto,"Sensor de T - Lm35:");
   escreve(0x80, texto);
   strcpy(texto, "Periodo:  ");
   escreve(0xC1, texto); 
   strcpy(texto, " 1 segundo  -> 1    ");
   escreve(0x94, texto); 
   strcpy(texto, " 5 segundo  -> 4    ");
   escreve(0xD4, texto); 
      
   OpcaoPeriodo=teclado(); //opcaoperiodo aguarda receber um valor do usuario atrav�s do teclado

   limpa(); //limpa visor do teclado

   //a seguir, h� a estrutura de menu de op��es para o periodo para o intervalo
   strcpy(texto," Sensor de T - Lm35:");
   escreve(0x80, texto);
   strcpy(texto, "Intervalo:  ");
   escreve(0xC1, texto); 
   strcpy(texto, " 30 segundos -> 1    ");
   escreve(0x94, texto); 
   strcpy(texto, " 60 segundos -> 4    ");
   escreve(0xD4, texto); 

 
   OpcaoIntervalo=teclado(); //opcaointervalo aguarda receber um valor do usuario atrav�s do teclado
   delay_ms(1050); // garante um tempo de espera, para depois prosseguir com as opera��es
 
   //a seguir, dependendo da op��o do usu�rio o timer1 � carregado com diferentes valores, correspondentes a 1s e 5s
   if(OpcaoPeriodo==1)
   {
      conf_timer1(0xCF, 0x2C); //1s
   }
   else if(OpcaoPeriodo==4)
   {
      conf_timer1(0x0B, 0xDC); //5s
   }
   limpa(); //limpa visor do teclado
   TMR1ON=1;//TIMER1 � ligado
   
   while(true) //programa repete o mesmo ciclo sempre
   {
   //enquanto o timer1 n�o gerar overflow e entrar na interrup��o, nada acontece
   //ao entrar na interrup��o a flag � setada
   if(flag!=0)     
      {
      contatempo++; //variavel apenas para aux�lio do programador
      LCD_p(0xE1, contatempo); //imprime o valor na tela do display
      TMR1ON=0; //timer1 desligado
      canal_AD++; //incrementa variavel de contagem do AD
      //temperaturas impressas sequencialmente
      strcpy(texto, "T1");
      escreve(0x80, texto); 
      strcpy(texto, "T2");
      escreve(0x84, texto); 
      strcpy(texto, "T3");
      escreve(0x89, texto);
      strcpy(texto, "T4");
      escreve(0x8D, texto); 
      strcpy(texto, "T5");
      escreve(0x91, texto); 
      //a cada valor incrementado do canal_AD, um canal � convertido e mostrado no display
   if(canal_AD==1)
       {
      Temperatura1=AD_select_convert(0);
         
      LCD_p(0xC0, Temperatura1); 
      flag=0;// flag zerada para nova interrup��o
      TMR1ON=1;// timer1 novamente ligado
       }
   else if(canal_AD==2)
       {
      Temperatura2=AD_select_convert(1);
        
      LCD_p(0xC4, Temperatura2);  
      flag=0;
      TMR1ON=1;
       }
   else if(canal_AD==3)
       {
      Temperatura3=AD_select_convert(2);
          
      LCD_p(0xC9, Temperatura3);
      flag=0;
      TMR1ON=1;
       }
   else if(canal_AD==4)
       {
      Temperatura4=AD_select_convert(3);
          
      LCD_p(0xCD, Temperatura4); 
      flag=0;
      TMR1ON=1;
       }
   else if(canal_AD==5)
       {
      Temperatura5=AD_select_convert(4);
         
         
      LCD_p(0xD1, Temperatura5);
         
      Media=(Temperatura1+Temperatura2+Temperatura3+Temperatura4+Temperatura5)/5; //c�lculo para m�dia das temperaturas
 
      //m�dia � printada
      strcpy(texto, "Media="); 
      escreve(0xD4, texto); 
      LCD_p(0xDA, Media);
  
      //aqui, avalia-se o intervalo escolhido pelo usu�rio
      if(OpcaoIntervalo==1)
         {
         flag=0; //flag zerada para nova interrup��o
         canal_AD=0; //canal AD pronto para novas convers�es e para novo ciclo
         TMR1ON=0; //timer1 desligado
         delay_ms(30000); //tempo correspondente a meio min
         TMR1ON=1; //timer1 ligado
         }
   else if(OpcaoIntervalo==4)
       {
         flag=0; //flag zerada para nova interrup��o
         canal_AD=0; //canal AD pronto para novas convers�es e para novo ciclo
         TMR1ON=0; //timer1 desligado
         delay_ms(60000); //tempo correspondente a 1 min
         
       }
         TMR1ON=1; //timer1 ligado
       }
       }
       }
       }

#INT_TIMER1
void interrupt_timer1(void)
{
   static UNSIGNED int x=0;
   TMR1ON=0;
   x++;//incremento de overflow
   //de acordo com a op��o do usu�rio, um dos valores para o TIMER � carregado
   if(OpcaoPeriodo==1)
   {
      TMR1H=0xCF;  TMR1L=0x2C;
   }
   else if(OpcaoPeriodo==2)
   {
      TMR1H=0x0B;  TMR1L=0xDC;
   }
   if(x==10)//valor calculado para overflow
   {  
      flag=1;//flag setada para ser poss�vel fazer as convers�es no main
      x=0; //vari�vel de overflow zerada para novas contagens
   }
   if(flag==0)//garante que o programa no main fique esperando o overflow final
   {
      TMR1ON=1;//timer1 segue ligado
   }
   clear_interrupt(INT_TIMER1); //limpa flag TMR1IF
}

