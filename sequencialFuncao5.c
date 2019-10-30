#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include "timer.h"

#define TRUE 1
#define FALSE 0

double function(double input){
  return cos(pow(M_E,input*(-1)));
}

// Funcao para tratamento de erro da funcao atof
// ela chega se a string fornecida eh um zero em formato de float
// porque o atof retorna zero se a string for invalida OU se for zero.
// Entao eh necessario tratar esse caso especial.
int IsZero(char *str) {
  if(str[0] != '0' && str[0] != '+' && str[0] != '-')
    return FALSE;

  int i = 1;
  int has_dot = 0;

  while(str[i] != '\0') {
    if(str[i] == '.')
      has_dot++;
    else if(str[i] != '0')
      return FALSE;
    i++;
  }

  if(has_dot > 1)
    return FALSE;
  else
    return TRUE;
}

// Funcao que computa a area do retangulo no dominio fornecido.
// Ele recebe o "a" e o "b" do intervalo e devolve a area do retangulo
// correspondente.
double CalcularRetanguloMedio(double a, double b) {
  return (function((b+a)/2))*(b-a);
}

// Funcao que calcula a integral recursivamente. Ela recebe o erro, a intervalo
// a partir das bordas "a" e "b" e checa a condicao de parada. Se ela nao
// for atendida, a funcao e chamada recursivamente dois nos novos dominios.
double Integrar(double a, double b, double e) {
  double area_1 = CalcularRetanguloMedio(a, b);
  double area_2 = CalcularRetanguloMedio(a, (a+b)/2) + CalcularRetanguloMedio((a+b)/2, b);
  if(fabs(area_1-area_2) <= e)
    return area_1;
  else
    return Integrar(a, (a+b)/2, e) + Integrar ((a+b)/2, b, e);
}

int main(int argc, char *argv[]) {

  double a = atof(argv[1]);
  double b = atof(argv[2]);
  double e = atof(argv[3]);

  if( (a == 0 && !(IsZero(argv[1]))) || (b == 0 && !(IsZero(argv[2]))) || (e == 0 && !(IsZero(argv[3])))){
    printf("Erro! Chame o programa com: %s <Intervalo A>(double) <Invervalo B>(double) <Erro Aceitavel>(double)\n", argv[0]);
    return -1;
  }

  // variaveis para calcular o tempo de duracao do algoritmo
  double ini, fim;
  GET_TIME(ini);

  double area = Integrar(a, b, e);

  GET_TIME(fim);

  printf("Area total = %lf\n", area);
  printf("Tempo de execução = %lf\n", fim-ini);


  return 0;
}
