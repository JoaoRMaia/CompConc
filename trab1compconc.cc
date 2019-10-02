#include <iostream>
#include <functional>

using namespace std;

#define Epsilon 10e-5


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

int main () {
	
	
	cout << Integral(1,22,[](double x){ return x*x+1;	}) << endl;
	
	return 0;
}
