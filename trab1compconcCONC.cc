#include <iostream>
#include <functional>
#include <pthread.h>
#include <cmath>
#include <chrono>

using namespace std;

static double Epsilon;   // Margem de erro
static int N_THREADS;   // Número de threads

pthread_mutex_t mutex;
double somathreads;

// Função que retorna o ponto médio de dois pontos

template <typename A , typename B>
double ptMed (A a, B b){
	return (a+b)/2;
}

// Função que retorna o modulo de um número

template<typename A>
double Modulo( A x){
	if ( x < 0 ) return -x;
	else return x;
}

// Struct usada para representar os retangulos usados para aproximar a integral

struct Retangulo{
	Retangulo(double x0, double x1, double y){
		this->x0 = x0;
		this->x1 = x1;
		this->y = y;
		area = (x1-x0)*y;
	}
	Retangulo(double x0, double x1, function<double(double)> y){
		this->x0 = x0;
		this->x1 = x1;
		this->y = y(ptMed(x0,x1));
		area = (x1-x0)*this->y;
	}
	
	double getArea(){
		return this->area;
	}

	double x0;
	double x1;
	double y;
	double area;
};

// Struct usada para ser passada como argumento para a função IntegralAux

struct Arg{
	
	Arg() {
		this->x0 = 0;
		this->x1 = 0;
		this->func = 0;
	}
	template <typename F>
	Arg(double x0,double x1, F func){
		this->x0 = x0;
		this->x1 = x1;
		this->func = func;
	}
	double x0;
	double x1;
	function<double(double)> func;	
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


// Função que realiza integral

template <typename F>
double Integral(double x0, double x1, F func){
	double soma;
	
	Retangulo Maior{x0,x1,func};
	Retangulo MenorEsq{x0,ptMed(x0,x1),func};
	Retangulo MenorDir{ptMed(x0,x1),x1,func};
	
	if (Modulo((Maior-MenorEsq-MenorDir)) > Epsilon){
		soma = Integral(MenorEsq.x0,MenorEsq.x1,func) + Integral(MenorDir.x0,MenorDir.x1,func);
	}
	else {
		soma = MenorEsq+MenorDir;
		//cout << "FIM " << soma <<endl;
	}
	return soma;
}

// Função que cria as threads com os blocos criados pela IntegralConc

void* IntegralAux( void* arg){
	Arg aux = *(Arg*) arg;
	double x0 = aux.x0;
	double x1 = aux.x1;
	function<double(double)> func = aux.func;
	double soma;
	
	Retangulo Maior{x0,x1,func};
	Retangulo MenorEsq{x0,ptMed(x0,x1),func};
	Retangulo MenorDir{ptMed(x0,x1),x1,func};
	
	if (Modulo((Maior-MenorEsq-MenorDir)) > Epsilon){
		soma = Integral(MenorEsq.x0,MenorEsq.x1,func) + Integral(MenorDir.x0,MenorDir.x1,func);
	}
	else {
		soma = MenorEsq+MenorDir;
		//cout << "FIM " << soma <<endl;
	}
	
	pthread_mutex_lock(&mutex);
	somathreads += soma;
	pthread_mutex_unlock(&mutex);
	
	pthread_exit(0);
	return 0;
}


// Função que divide a tarefa em blocos

template<typename F>
double IntegralConc(double x0, double x1, F func){
	Arg arg[N_THREADS];
	int i;
	somathreads = 0;
	pthread_t tid[N_THREADS];
	pthread_mutex_init(&mutex,0);
	double incremento = (x1-x0)/N_THREADS;
	for ( i = 0 ; i < N_THREADS ; i++){
		arg[i] = Arg(x0+incremento*i,x0+incremento*(i+1),func);
		//cout << "Calculando de " << x0+incremento*i << " ate " << x0+incremento*(i+1) << endl;
		pthread_create(&tid[i],0,IntegralAux,(void*)&arg[i]);
	}
	for ( int i = 0 ; i < N_THREADS ; i++){
		pthread_join(tid[i],0);
		free(&tid[i]);
	}
	cout << "O valor da integral e de " << somathreads << "u.m." << endl;
	pthread_mutex_destroy(&mutex);
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
	cout << "Digite o intervalo de integração" << endl;
	cin >> x0 >> x1;
}

void selecionaPrecisao() {
	cout << "Selecione a ordem de grandeza da precisão ex: -7 para 10e-7" << endl;
	cin >> Epsilon;
	Epsilon = pow(10,Epsilon);
}

void selNThreads() {
	cout << "Usar quantas threads? (0 para sequencial)" << endl;
	cin >> N_THREADS;
}
int main () {
	
	char input;
	double intervalo1,intervalo2;
	selecionaPrecisao();
	selNThreads();
	getIntervalo(intervalo1,intervalo2);
	printMenu(input);
	auto ini = chrono::high_resolution_clock::now();
	switch (input){
		case 'a':    //  f(x) = 1 + x
			if (N_THREADS > 0 && N_THREADS < 256)  IntegralConc(intervalo1,intervalo2,[](double x){return x+1;}) ;
			else     Integral(intervalo1,intervalo2,[](double x){return x+1;}) ;
			break;
			
		case 'b':  //  f(x) = sqrt(1 - x^2)
			if (N_THREADS > 0 && N_THREADS < 256)  IntegralConc(intervalo1,intervalo2,[](double x){return sqrt(1-pow(x,2));}) ;
			else  Integral(intervalo1,intervalo2,[](double x){return sqrt(1-pow(x,2));}) ;
			break;
			
		case 'c': // f(x) = sqrt(1+x^4)
			if (N_THREADS > 0 && N_THREADS < 256)  IntegralConc(intervalo1,intervalo2,[](double x){return sqrt(1+pow(x,4));}) ;
			else  Integral(intervalo1,intervalo2,[](double x){return sqrt(1+pow(x,4));}) ; 
			break;
			
		case 'd': // f(x) = sen(x^2)
			if (N_THREADS > 0 && N_THREADS < 256)  IntegralConc(intervalo1,intervalo2,[](double x){return sin(pow(x,2));}) ;
			else  Integral(intervalo1,intervalo2,[](double x){return sin(pow(x,2));}) ;
		case 'e': // f(x) = cos(e^-x)
			if (N_THREADS > 0 && N_THREADS < 256)  IntegralConc(intervalo1,intervalo2,[](double x){return cos(exp(-x));}) ;
			else  Integral(intervalo1,intervalo2,[](double x){return cos(exp(-x));}) ;
			break;
			
		case 'f': // f(x) = cos(e^-x)*x
			if (N_THREADS > 0 && N_THREADS < 256)  IntegralConc(intervalo1,intervalo2,[](double x){return cos(exp(-x))*x;}) ;
			else  Integral(intervalo1,intervalo2,[](double x){return cos(exp(-x))*x;}) ;
			break;
			
		case 'g': // f(x) = cos(e^-x)*(0.005*x^3 + 1)
			if (N_THREADS > 0 && N_THREADS < 256)  IntegralConc(intervalo1,intervalo2,[](double x){return cos(exp(-x))*(0.005*pow(x,3)+1);}) ;
			else  Integral(intervalo1,intervalo2,[](double x){return cos(exp(-x))*(0.005*pow(x,3)+1);}) ;
			break;
			
		default: 
			cout << "Opção inválida, tente novamente" << endl << endl << "Voce escolheu: " << input << endl;
		
	}
	auto fim = chrono::high_resolution_clock::now();
	chrono::duration<double> tempo = fim - ini;
	cout << "O tempo levado para calcular a integral foi de: " << tempo.count() <<" segundos." << endl;
	return 0;
}
