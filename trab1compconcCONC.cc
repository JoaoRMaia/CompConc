#include <iostream>
#include <functional>
#include <pthread.h>

static const double Epsilon = 10e-5;   // Margem de erro

static const int N_THREADS = 8;   // Número de threads

using namespace std;


pthread_mutex_t mutex;
double somathreads = 0;

template <typename A , typename B>
double ptMed (A a, B b){
	return (a+b)/2;
}

template<typename A>
double Modulo( A x){
	if ( x < 0 ) return -x;
	else return x;
}

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

// Redefinição de operadores + e - para somar a area dos retangulos

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

// Função que cria as threads

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
		cout << "FIM " << soma <<endl;
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
		pthread_create(&tid[i],NULL,IntegralAux,(void*)&arg[i]);
	}
	
	for ( int i = 0 ; i < N_THREADS ; i++){
		pthread_join(tid[i],0);
	}
	
	return somathreads;
}



int main () {
	
	
	
	cout << IntegralConc(1,1000000,[](double x){ return x*x+1;	}) << endl;   
	
	return 0;
}
