#include <iostream>
#include <functional>
#include <pthread.h>
#include <cmath>
#include <chrono>
#include <stack>
#include <utility>
#include <fstream>

using namespace std;

static double Epsilon;   // Margem de erro
static int N_THREADS;   // Número de threads

struct Retangulo;
template<typename T>
struct Pilha;

pthread_mutex_t mutex_nthreads; //mutex de nthreads, para identificar em qual buffer a thread trabalhará
pthread_mutex_t mutex_soma;   // mutex de soma, para não ocorrer condição de corrida quando as threads terminarem
pthread_mutex_t mutex_buffer[8]; // vetor de mutexes, um para cada buffer
pthread_mutex_t mutex_ladrao;  // Mutex correspondente a função rouba retangulo

double somathreads;   // Variavel global que guarda o resultado da integral
int nthreads;

template <typename A , typename B> // Função que retorna o ponto médio de dois pontos
double ptMed (A a, B b){
	return (a+b)/2;
}

// Função que retorna o modulo de um número
template<typename A>
double Modulo( A x){
	if ( x < 0 ) return -x;
	else return x;
}


// Pilha criada para ser usada como buffer
template <typename T>
struct Pilha { 
    Pilha() {
      topo = 0;
    }
    void push( T valor ) {
      tab[topo++] = valor;   
    }
    T pop() { 
    	return tab[--topo]; 
    }
    bool empty(){
    	return topo ? false : true;
		}
		
  T tab[1024];
  int topo;
};

// Struct usada para representar os retangulos usados para aproximar a integral

struct Retangulo{
	
	Retangulo(){	}
	Retangulo(double x0, double x1, function<double(double)> y){
		this->x0 = x0;
		this->x1 = x1;
		this->func = y;
		area = (x1-x0)*y(ptMed(x0,x1));
	}
	
	pair<Retangulo,Retangulo> split(){
		Retangulo esq{this->x0,ptMed(this->x0,this->x1),func};
		Retangulo dir{ptMed(this->x0,this->x1),this->x1,func};
		auto rets = make_pair(esq,dir);
		return rets;
	}
	double getArea(){
		return this->area;
	}

	double x0;
	double x1;
	function<double(double)> func;
	double area;
};

// Redefinição de operadores + e - para somar a area dos retangulos, ou seja,
// Se somarmamos um Retangulo A com um Retangulo B o resultado será a soma de suas áreas

double operator -(Retangulo a, Retangulo b){
	return a.getArea()-b.getArea();
}
double operator +(Retangulo a, Retangulo b){
	return a.getArea()+b.getArea();
}
double operator -(double a, Retangulo b){
	return a-b.getArea();
}
double operator +(double a, Retangulo b){
	return a+b.getArea();
}
double operator -(Retangulo a, double b){
	return a.getArea()-b;
}
double operator +(Retangulo a, double b){
	return a.getArea()+b;
}

Pilha<Retangulo> buffer[8];    // Declaração da variável global buffer, que é uma Pilha de Retangulos.

int RoubaRetangulo(int tid){    // Função que busca no buffer de outras threads um retangulo para processar
	pthread_mutex_lock(&mutex_ladrao);      // Mutex ladrão serve para apenas uma thread estar roubando, caso várias estejam roubando ao mesmo tempo,
	pthread_mutex_lock(&mutex_buffer[tid]); // é possivel que todos os buffers se tranquem e o programa entre em deadlock
	for ( int i = (tid+1)%N_THREADS ; i <= N_THREADS%8 ; i = ((i+1)%8)){
		if (i == tid) {
			pthread_mutex_unlock(&mutex_buffer[tid]);
			pthread_mutex_unlock(&mutex_ladrao);
			return 0;
		}
		
		pthread_mutex_lock(&mutex_buffer[i]);
		if (!buffer[i].empty()){
			buffer[tid].push(buffer[i].pop());
			pthread_mutex_unlock(&mutex_buffer[i]);
			pthread_mutex_unlock(&mutex_buffer[tid]);
			pthread_mutex_unlock(&mutex_ladrao);
			return 1;
		}
		pthread_mutex_unlock(&mutex_buffer[i]);
	}
	pthread_mutex_unlock(&mutex_ladrao);
	return 0;
}

void* IntegralAux( void* nada){     // Função que processa os retangulos e realiza de fato a integral
	pthread_mutex_lock(&mutex_nthreads);
	int tid = nthreads%8;
	nthreads++;
	pthread_mutex_unlock(&mutex_nthreads);
	Retangulo Maior;
	pair<Retangulo,Retangulo> Dividido;
	Retangulo MenorEsq;
	Retangulo MenorDir;
	double soma = 0;
	
	while (1){
		pthread_mutex_lock(&mutex_buffer[tid]);
		if (buffer[tid].empty()){						// Se o buffer estiver vazio, tenta roubar um retangulo de outros buffers
			pthread_mutex_unlock(&mutex_buffer[tid]);
			if (RoubaRetangulo(tid)){    // Se conseguiu roubar, continua processando
				continue;     
			}
			else {																// se não conseguiu ( todos os buffers estão vazios), soma o resultado ao total e encerra
				pthread_mutex_unlock(&mutex_buffer[tid]);
				pthread_mutex_lock(&mutex_soma);
				somathreads += soma;
				pthread_mutex_unlock(&mutex_soma);
				pthread_exit(0);
			}
		}
		else {
			Maior = Retangulo{buffer[tid].pop()};   // Cria um retangulo com o topo da pilha
			pthread_mutex_unlock(&mutex_buffer[tid]);
			MenorEsq = Retangulo(Maior.x0,ptMed(Maior.x0,Maior.x1),Maior.func);   // Divide o maior retangulo em dois
			MenorDir = Retangulo(ptMed(Maior.x0,Maior.x1),Maior.x1,Maior.func);
			if (Modulo((Maior-MenorEsq-MenorDir)) > Epsilon){     // Caso a diferença seja significativa, os dois retangulos são enviados ao buffer
				pthread_mutex_lock(&mutex_buffer[tid]);							// Para serem divididos mais ainda
				buffer[tid].push(MenorEsq);
				buffer[tid].push(MenorDir);
				pthread_mutex_unlock(&mutex_buffer[tid]);
			}
			else {
				soma += MenorEsq+MenorDir;
			}
		}
	}
	pthread_exit(0);
}

template<typename F>      // Função que divide a tarefa em blocos, passando os retangulos iniciais para o buffer
double IntegralConc(double x0, double x1, F func){
	int i;
	nthreads = 0;
	somathreads = 0;
	pthread_t tid[N_THREADS];
	pthread_mutex_init(&mutex_soma,0);
	pthread_mutex_init(&mutex_nthreads,0);
	pthread_mutex_init(&mutex_ladrao,0);
	for (i = 0 ; i < N_THREADS ; i++)	pthread_mutex_init(&mutex_buffer[i],0);    // Inicializando os mutex
	
	double incremento = (x1-x0)/(N_THREADS);
	for ( i = 0 ; i < N_THREADS ; i++) buffer[i%8].push(Retangulo{x0+incremento*i,x0+incremento*(i+1),func});
	
	for ( i = 0 ; i < N_THREADS ; i++) pthread_create(&tid[i],0,IntegralAux,0);
	
	for ( i = 0 ; i < N_THREADS ; i++) pthread_join(tid[i],0);
	
	cout << "O valor da integral e de " << somathreads << " u.m. " << endl;
	
	pthread_mutex_destroy(&mutex_soma);
	pthread_mutex_destroy(&mutex_nthreads);
	pthread_mutex_destroy(&mutex_ladrao);
	for (i = 0 ; i < N_THREADS ; i++)	pthread_mutex_destroy(&mutex_buffer[i]); 
	return somathreads;
}
void printMenu(char &input) {
	cout << "Digite a função desejada" << endl 
	<< "\ta) f(x) = 1 + x" << endl 
	<< "\tb) f(x) = sqrt(1 - x^2)" 	<< endl 
	<< "\tc) f(x) = sqrt(1+x^4)" << endl 
	<< "\td) f(x) = sen(x^2)" << endl
	<< "\te) f(x) = cos(e^-x)" << endl
	<< "\tf) f(x) = cos(e^-x)*x" << endl
	<< "\tg) f(x) = cos(e^-x)*(0.005*x^3 + 1)" << endl;
	cin >> input;
}

void setIntervalo(double& x0, double &x1) {
	cout << "Digite o intervalo de integração. Ex: 0 10 para integrar de 0 a 10" << endl;
	cin >> x0 >> x1;
}

void setPrecisao() {
	cout << "Selecione a ordem de grandeza da precisão ex: -7 para 10e-7" << endl;
	cin >> Epsilon;
	Epsilon = pow(10,Epsilon);
}

void setNThreads() {
	
	cout << "Usar quantas threads?" << endl;
	cin >> N_THREADS;
}

int main () {
	
	char input;
	double intervalo1,intervalo2;
	setPrecisao();
	setNThreads();
	setIntervalo(intervalo1,intervalo2);
	printMenu(input);
	auto ini = chrono::high_resolution_clock::now();  //Mede o tempo de inicio
	switch (input){
		case 'a':    //  f(x) = 1 + x
			 IntegralConc(intervalo1,intervalo2,[](double x){return x+1;}) ;
			break;
			
		case 'b':  //  f(x) = sqrt(1 - x^2)
			 IntegralConc(intervalo1,intervalo2,[](double x){return sqrt(1-pow(x,2));}) ;
			break;
			
		case 'c': // f(x) = sqrt(1+x^4)
			 IntegralConc(intervalo1,intervalo2,[](double x){return sqrt(1+pow(x,4));}) ;
			break;
			
		case 'd': // f(x) = sen(x^2)
			  IntegralConc(intervalo1,intervalo2,[](double x){return sin(pow(x,2));}) ;
			break;
			
		case 'e': // f(x) = cos(e^-x)
			 IntegralConc(intervalo1,intervalo2,[](double x){return cos(exp(-x));}) ;
			break;
			
		case 'f': // f(x) = cos(e^-x)*x
			 IntegralConc(intervalo1,intervalo2,[](double x){return cos(exp(-x))*x;});
			break;
			
		case 'g': // f(x) = cos(e^-x)*(0.005*x^3 + 1)
			 IntegralConc(intervalo1,intervalo2,[](double x){return cos(exp(-x))*(0.005*pow(x,3)+1);});
			break;
			
		default: 
			cout << "Opção inválida, tente novamente" << endl << endl << "Voce escolheu: " << input << endl;
			break;
		}
	auto fim = chrono::high_resolution_clock::now(); // Mede o Tempo de Fim
	chrono::duration<double> tempo = fim - ini;      // Calcula o tempo de execução do programa
	cout << "Calculado em " << tempo.count() << " segundos." << endl;
	return 0;
}
