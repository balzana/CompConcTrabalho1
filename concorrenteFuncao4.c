#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include "timer.h"

#define TRUE 1
#define FALSE 0

// Estrutura que ira guardar o domvalo de integracao.
typedef struct dominio {
    double a;
    double b;
} Dominio;

// Estrutura que ira guardar os intervalos que ainda nao foram
// avaliados, para que as threads possam pega-los quando
// estiverem livre.
typedef struct pilha_no {
	struct pilha_no * prox;
	Dominio * dom;
} PilhaNo;

// Funcao que cria o node cabeca da pilha encadeada
// retorna um ponteiro para a cabeca.
PilhaNo *pilhaCriarCabeca() {
    PilhaNo *cabeca = (PilhaNo*) malloc(sizeof(PilhaNo));
    cabeca->prox = NULL;
    cabeca->dom = NULL;
    return cabeca;
}

// Funcao que implementa o Pop da pilha de dominios.
// Se a cabeca de entrada for nula, a funcao encerra o programa
// com uma mensagem de erro.
// Ela retorna nulo se a pilha estiver vazia e da um aviso ao usuario.
// Senao, ela retorna um ponteiro para o dominio que foi dado Pop.
Dominio *pilhaPop(PilhaNo *cabeca) {

  if(cabeca == NULL){
    printf("Erro na funcao pilhaPop. Node de entrada nulo!\n");
    exit(-1);
  }

	Dominio * dom = NULL;
	PilhaNo * noTopo;

	noTopo = cabeca->prox;
	if(noTopo == NULL) {
    // Pilha vazia! Tratamentos de erro aqui.
	} else {
    cabeca->prox = noTopo->prox;
    dom = noTopo->dom;
  }

	free(noTopo);
	return dom;
}

// Funcao que implementa o Push da pilha de dominios.
// Se a cabeca for nula, a funcao encerra o programa e envia
// uma mensagem de erro.
void pilhaPush(PilhaNo *cabeca, Dominio *dom) {

  if(cabeca == NULL){
    printf("Erro na funcao pilhaPush. Node de entrada nulo!\n");
    exit(-1);
  }

  PilhaNo *noNovo, *noTopo;

	noNovo = (PilhaNo*) malloc(sizeof(PilhaNo));
	noNovo->dom = dom;
	noTopo = cabeca->prox;

	noNovo->prox = noTopo;
	cabeca->prox = noNovo;
}

// Funcao que destroi a pilha. Ela corre pela pilha dando
// free nos nodes.
void pilhaDestruir(PilhaNo *cabeca) {
	Dominio *dom = pilhaPop(cabeca);

	while(dom != NULL) {
		free(dom);
		dom = pilhaPop(cabeca);
	}

	free(cabeca);
}

// Funcao que cria um dominio dadas bordas "a" e "b" como entrada.
Dominio *criarDominio(double a, double b) {

    Dominio *dom;

    dom = (Dominio*)malloc(sizeof(Dominio));
    dom->a = a;
    dom->b = b;

    return dom;
}

pthread_mutex_t mutex_particoes;
pthread_mutex_t mutex_pilha;
pthread_mutex_t mutex_area;

PilhaNo * pilha_dominios;
int particoes_restantes = 0;
double erro;
double area_total = 0.0;


//Calcular o valor da função da letra A no ponto x
double function(double x)
{
	return sin(x*x);
}

// Funcao que calcula efetivamente as integrais da pilha de Dominios
// A thread fica loopando na pilha de dominios e pegando integrais
// para calcular.
// O criterio de parada eh quando a variavel particoes_restantes
// vai para zero, o que significa que nao restam particoes na pilha
// de dominios.
void *IntegrarDominios(void * arg) {

	Dominio *dom;

	// Looping enquanto houver particoes para serem calculadas
	while(particoes_restantes > 0) {

		pthread_mutex_lock(&mutex_pilha);
		dom = pilhaPop(pilha_dominios);
		pthread_mutex_unlock(&mutex_pilha);

		// se o dom eh nulo, entao nao havia dominios na pilha para serem calculados
    // entao a thread volta para o começo do looping ate chegar algum para
    // ela calcular.
		if(dom == NULL)
		  continue;

		// se tiver, ela calcula as areas e faz o seu trabalho usual...
    double ponto_medio = (dom->a + dom->b)/2;
		double area1 = function(ponto_medio) * (dom->b - dom->a);
		double area2 = function((dom->a + ponto_medio) / 2) * (ponto_medio - dom->a) +  function((ponto_medio + dom->b) / 2) * (dom->b - ponto_medio);

		double erro_local = fabs(area2 - area1);

		// se o erro deu certo, ela decrementa o numero de particoes restantes
    // e aumenta a area total.
		if(erro_local <= erro) {
		  pthread_mutex_lock(&mutex_area);
		  area_total += area1;
		  pthread_mutex_unlock(&mutex_area);

			pthread_mutex_lock(&mutex_particoes);
			particoes_restantes--;
			pthread_mutex_unlock(&mutex_particoes);

			continue;
		}

    // se o erro local nao for pequeno o suficiente, entao a thread parte
    // o seu dominio em dois novos e os coloca na pilha para outras threads
    // pegarem.
  	Dominio * dom1 = criarDominio(dom->a, ponto_medio);
    Dominio * dom2 = criarDominio(ponto_medio, dom->b);

    pthread_mutex_lock(&mutex_pilha);
    pilhaPush(pilha_dominios, dom1);
    pilhaPush(pilha_dominios, dom2);
    pthread_mutex_unlock(&mutex_pilha);

    pthread_mutex_lock(&mutex_particoes);
    particoes_restantes++;
    pthread_mutex_unlock(&mutex_particoes);
	}

	pthread_exit(NULL);
}


// Funcao para tratamento de erro da funcao atof
// ela chega se a string fornecida eh um zero em formato de float
// porque o atof retorna zero se a string for invalida OU se for zero.
// Entao eh necessario tratar esse caso especial.
int isZero(char *str) {
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

int main(int argc, char *argv[]) {

	double a      = atof(argv[1]);
	double b      = atof(argv[2]);
	erro          = atof(argv[3]);
	int n_threads = atoi(argv[4]);

  if((argc != 5) || (a == 0 && !(isZero(argv[1]))) || (b == 0 && !(isZero(argv[2]))) || (erro == 0 && !(isZero(argv[3]))) || n_threads <= 0){
    printf("Erro! Chame o programa com: %s <Intervalo A>(double) <Invervalo B>(double) <Erro Aceitavel>(double) <Numero Positivo de Threads>(int)\n", argv[0]);
    return -1;
  }

	pilha_dominios = pilhaCriarCabeca();

	// Criando o dominio inicial, que no caso vai ser o intervalo [a,b].
	Dominio *dominio_inicial = criarDominio(a, b);
	pilhaPush(pilha_dominios, dominio_inicial);
	particoes_restantes++;

  // variaveis para calcular o tempo de duracao do algoritmo
  double ini, fim;
	GET_TIME(ini);

	pthread_t tid_sistema[n_threads];

	for(int i = 0; i < n_threads; i++) {
		pthread_create(&tid_sistema[i], NULL, IntegrarDominios, NULL);
	}

	for(int i = 0; i < n_threads; i++) {
	    pthread_join(tid_sistema[i], NULL);
	}

	GET_TIME(fim);

	printf("Area Total = %lf\n", area_total);
	printf("Tempo de execução = %lf\n", fim-ini);

	pilhaDestruir(pilha_dominios);

	return 0;
}
