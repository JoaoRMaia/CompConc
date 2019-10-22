#include <iostream>
#include <functional>
#include <cmath>
#include <chrono>

using namespace std;

static double Epsilon;   // Margem de erro

template <typename A , typename B>
double ptMed (A a, B b){     // Função que calcula o pt médio
	return (a+b)/2;
}

template<typename A>
double Modulo( A x){       // Função que retorna o módulo de um número
	if ( x < 0 ) return -x;
	else return x;
}

struct Retangulo{        // Struct usada para representar os retangulos usados para aproximar a integral
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
		// cout << "FIM " << soma <<endl;
	}
	
	
	return soma;
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

int main () {
	
	char input;
	double intervalo1,intervalo2;
	setPrecisao();
	setIntervalo(intervalo1,intervalo2);
	printMenu(input);
	auto ini = chrono::high_resolution_clock::now();  //Mede o tempo de inicio
	switch (input){
		case 'a':    //  f(x) = 1 + x
			cout << Integral(intervalo1,intervalo2,[](double x){return x+1;}) << endl;
			break;
			
		case 'b':  //  f(x) = sqrt(1 - x^2)
			cout << Integral(intervalo1,intervalo2,[](double x){return sqrt(1-pow(x,2));}) << endl;
			break;
			
		case 'c': // f(x) = sqrt(1+x^4)
			cout << Integral(intervalo1,intervalo2,[](double x){return sqrt(1+pow(x,4));}) << endl;
			break;
			
		case 'd': // f(x) = sen(x^2)
			cout << Integral(intervalo1,intervalo2,[](double x){return sin(pow(x,2));}) << endl;
			break;
			
		case 'e': // f(x) = cos(e^-x)
			cout << Integral(intervalo1,intervalo2,[](double x){return cos(exp(-x));}) << endl;
			break;
			
		case 'f': // f(x) = cos(e^-x)*x
			cout << Integral(intervalo1,intervalo2,[](double x){return cos(exp(-x))*x;}) << endl;
			break;
			
		case 'g': // f(x) = cos(e^-x)*(0.005*x^3 + 1)
			cout << Integral(intervalo1,intervalo2,[](double x){return cos(exp(-x))*(0.005*pow(x,3)+1);}) << endl ;
			break;
			
		default: 
			cout << "Opção inválida, tente novamente" << endl << endl << "Voce escolheu: " << input << endl;
		
	}
	auto fim = chrono::high_resolution_clock::now(); // Mede o Tempo de Fim
	chrono::duration<double> tempo = fim - ini;      // Calcula o tempo de execução do programa
	cout << "O tempo levado para calcular a integral foi de: " << tempo.count() <<" segundos." << endl;
	return 0;
}
