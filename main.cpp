#include "binder.hpp"
#include <iostream>
#include <string>

void Print( std::string const & msg )
{
  std::cout << "Print(): " << msg << std::endl;
}

struct A
{
  void Print( std::string const & msg )
  {
    std::cout << "A::Print(): " << msg << std::endl;
  }
};

int main()
{
  std::string const hello {"hello"};
  auto f = naive::bind( &Print, hello );
  auto f2 = naive::bind( &Print, std::placeholders::_1 );

  f();
  f2( hello );

  A a;
  auto f3 = naive::bind( &A::Print, std::placeholders::_2, std::placeholders::_1 );
  auto f4 = naive::bind( &A::Print, std::placeholders::_1, hello );
  auto f5 = naive::bind( &A::Print, a, std::placeholders::_1 );
  auto f6 = naive::bind( &A::Print, a, hello );

  f3( hello, a );
  f4( a );
  f5( hello );
  f6();

  return 0;
}

