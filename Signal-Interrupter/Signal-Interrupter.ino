#include <TimerOne.h>
#include "Timer2.h"

//definindo pinos
#define pinSignal       9
#define softStart       6
#define stepStart       7

//definindo expressões relativas ao chaveamento
#define Fc              5000.0 //Hz
#define Tc              ( 1.0/Fc )
#define Tc_us           ( (unsigned long)(1000000*Tc) )

//definindo expressões relativas ao ciclo de trabalho
#define t               ( micros()/1000000.0 + t_crr) //seg
#define dutyCycle       ( ( amplitude*sin(2.0*M_PI*Ff*t)+1.0 )/2.0 )
#define dutyCycle_1024  ( (unsigned int)(dutyCycle*1023) )

//definindo expressões relativas à frequência fundamental
#define Tf              ( 1.0/Ff )
#define Tf_us           ( (unsigned long)(1000000*Tf) )

//definindo constantes relativas à rampa de amplitude e de frequência
#define Tr              10.0   //seg
#define amplitude0      0.0   //%
#define amplitudeInf    1.0   //%
#define Ff0             0.01  //Hz
#define FfInf           50.0   //Hz

//definindo variáveis
float amplitude = amplitude0, Ff = Ff0, t_crr = 0.0;
//definindo variáveis auxiliares
bool  needUpdateDuty = false, inSoftStart = true;
//informando o uso de uma variável externa relativa à contagem de tempo
extern volatile unsigned long timer0_overflow_count;

//declarando função reset
void(* reset) (void) = 0;
//declarando funções auxiliares
inline void resetMicros()   __attribute__((always_inline));
inline void updatePwmDuty() __attribute__((always_inline));

void setup(){
  //configurando o pino de saída
  pinMode(pinSignal, OUTPUT);
  //configurando os pinos de entrada
  pinMode(stepStart, INPUT);
  pinMode(softStart, INPUT);

  //aguardando sinal de step ou soft start
  while(!digitalRead(stepStart) && !digitalRead(softStart));
  //desabilitando rampa de amplitude e de frequência para o caso de step start
  if(digitalRead(stepStart)){
    amplitude = amplitudeInf;
    Ff = FfInf;
    inSoftStart = false;
  }

  //inicializando o Timer1
  Timer1.initialize();
  //indicando função de chamada para a interrupção
  Timer1.attachInterrupt(updatePwmDuty);
  //inicializando e configurando o Timer2,
  //o período de interrupção é o produto entre os dois primeiros argumentos,
  //a função de chamada para a interrupção é o terceiro argumento
  Timer2::set(10000/FfInf, 1.0/10000, resetMicros);

  //reinicinado a contagem de tempo
  resetMicros();
  
  //inicializando o Timer2 para o caso de step start
  //a fim de saturar o tempo no período da frequência fundamental
  if(!inSoftStart)
    Timer2::start();
  
  //inicializando chaveamento PWM,
  //o primeiro argumento representa o pino de saída do sinal gerado,
  //o segundo  argumento representa o ciclo de trabalho do sinal (0 para 0% e 1024 para 100%),
  //o terceiro argumento representa o período de chaveamento em microssegundos
  Timer1.pwm(pinSignal, dutyCycle_1024, Tc_us);
}

void loop(){
  //verificando necessidade de atualização do ciclo de trabalho do sinal PWM
  if(needUpdateDuty){
    //atualizando o ciclo de trabalho do sinal PWM,
    //o primeiro argumento representa o pino de saída do sinal gerado,
    //o segundo  argumento representa o ciclo de trabalho do sinal (0 para 0% e 1023 para 100%)
    Timer1.setPwmDuty(pinSignal, dutyCycle_1024);
    //indicando que a atualização foi realizada
    needUpdateDuty = false;
  }

  //verificando se o sinal está em rampa de amplitude e de frequência
  if(inSoftStart){
    //verificando se o tempo de rampa foi ultrapassado
    if(t<=Tr){
      //atualizando a amplitude do sinal para gerar a rampa
      amplitude = ((amplitudeInf-amplitude0)/Tr)*t + amplitude0;
      //atualizando a frequência do sinal para gerar a rampa
      Ff =        ((FfInf-Ff0)/(2*Tr))*t + Ff0;
    }
    else{
      //incluindo correção no tempo para evitar descontinuidade ao término da rampa
      t_crr = (((FfInf+Ff0)/(2*FfInf))*Tr);
      //atribuindo o valor final de frequência
      Ff = FfInf;
      //indicando o término da rampa
      inSoftStart = false;
      //iniciando o Timer2 para saturar o tempo no período da frequência fundamental
      Timer2::start();
    }
  }

  //resetando o micro controlador para gerar efeito de step stop
  //if(digitalRead(stepStart))
    //reset();
}

void resetMicros(){
  //reinicializando contagem de tempo
  timer0_overflow_count = TCNT0 = 0;
}

void updatePwmDuty(){
  //habilitando atualização do ciclo de trabalho do sinal PWM
  needUpdateDuty = true;
}
