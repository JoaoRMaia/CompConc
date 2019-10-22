#include <iostream>
#include <functional>
#include <pthread.h>
#include <cmath>
#include <chrono>
#include <stack>
#include <utility>

using namespace std;

static double Epsilon;   // Margem de erro
static int N_THREADS;   // Número de threads

struct Retangulo;
template<typename T>
struct Pilha;

pthread_mutex_t mutex_soma;
pthread_mutex_t mutex_buffer;
double somathreads;   // Variavel global que guarda o resultado da integral

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

Pilha<Retangulo> buffer;    // Declaração da variável global buffer, que é uma Pilha de Retangulos.

// Função que processa os retangulos e realiza de fato a integral

void* IntegralAux( void* nada){
	Retangulo Maior;
	pair<Retangulo,Retangulo> Dividido;
	Retangulo MenorEsq;
	Retangulo MenorDir;
	double soma = 0;
	while (1){
		pthread_mutex_lock(&mutex_buffer);
		if (buffer.empty()){						// Se o buffer estiver vazio, adiciona a soma da thread ao total e retorna
			pthread_mutex_unlock(&mutex_buffer);
			pthread_mutex_lock(&mutex_soma);
			somathreads += soma;
			pthread_mutex_unlock(&mutex_soma);
			pthread_exit(0);
		}
		Maior = Retangulo{buffer.pop()};   // Cria um retangulo com o topo da pilha
		pthread_mutex_unlock(&mutex_buffer);
		MenorEsq = Retangulo(Maior.x0,ptMed(Maior.x0,Maior.x1),Maior.func);   // Divide o maior retangulo em dois
		MenorDir = Retangulo(ptMed(Maior.x0,Maior.x1),Maior.x1,Maior.func);
		
		if (Modulo((Maior-MenorEsq-MenorDir)) > Epsilon){     // Caso a diferença seja significativa, os dois retangulos são enviados ao buffer
			pthread_mutex_lock(&mutex_buffer);						 	  	// Para serem divididos mais ainda
			buffer.push(MenorEsq);
			buffer.push(MenorDir);
			pthread_mutex_unlock(&mutex_buffer);
		}
		else {
			soma += MenorEsq+MenorDir;
		}
	}
	return 0;
}

template<typename F>      // Função que divide a tarefa em blocos, passando os retangulos iniciais para o buffer
double IntegralConc(double x0, double x1, F func){
	int i;
	somathreads = 0;
	pthread_t tid[N_THREADS];
	pthread_mutex_init(&mutex_soma,0);
	pthread_mutex_init(&mutex_buffer,0);
	double incremento = (x1-x0)/N_THREADS;
	for ( i = 0 ; i < N_THREADS ; i++){
		cout << "Thread " << i+1 << " irá integrar de " << x0+incremento*i << " a " << x0+incremento*(i+1) << endl;
		buffer.push(Retangulo{x0+incremento*i,x0+incremento*(i+1),func});
	}
	for ( i = 0 ; i < N_THREADS ; i++){
		pthread_create(&tid[i],0,IntegralAux,0);
	}
	for ( int i = 0 ; i < N_THREADS ; i++){
		pthread_join(tid[i],0);
	}
	cout << "O valor da integral e de " << somathreads << " u.m." << endl;
	pthread_mutex_destroy(&mutex_soma);
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

void getIntervalo(double& x0, double &x1) {
	cout << "Digite o intervalo de integração. Ex: 0 10 para integrar de 0 a 10" << endl;
	cin >> x0 >> x1;
}

void selecionaPrecisao() {
	cout << "Selecione a ordem de grandeza da precisão ex: -7 para 10e-7" << endl;
	cin >> Epsilon;
	Epsilon = pow(10,Epsilon);
}

void selNThreads() {
	
	cout << "Usar quantas threads?" << endl;
	cin >> N_THREADS;
}


int main () {
	
	char input;
	double intervalo1,intervalo2;
	selecionaPrecisao();
	selNThreads();
	getIntervalo(intervalo1,intervalo2);
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
			IntegralConc(intervalo1,intervalo2,[](double x){return cos(exp(-x))*x;}) ;
			break;
			
		case 'g': // f(x) = cos(e^-x)*(0.005*x^3 + 1)
			IntegralConc(intervalo1,intervalo2,[](double x){return cos(exp(-x))*(0.005*pow(x,3)+1);}) ;
			break;
			
		default: 
			cout << "Opção inválida, tente novamente" << endl << endl << "Voce escolheu: " << input << endl;
	}
	auto fim = chrono::high_resolution_clock::now(); // Mede o Tempo de Fim
	chrono::duration<double> tempo = fim - ini;      // Calcula o tempo de execução do programa
	cout << "O tempo levado para calcular a integral foi de: " << tempo.count() <<" segundos." << endl;
	return 0;
}
