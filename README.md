Здравствуй, читающий!

<i>Введение</i>
Наверное, трудно найти такого программиста на с++, который никогда не применял в своем коде
<source lang="cpp">
std::bind // до выхода с++11(а возможно, и после) boost::bind
</source>
В новом стандарте с++11 bind и function стали частью стандартной библиотеки(их можно найти в заголовочном файле functional). По сути bind не что иное, как обертка(по факту шаблонная функция) над callable-объектом(т.е. объектом, который можно вызвать, передав ему необходимое число аргументов в круглых скобочках), позволяющая связать/забиндить любые аргументы последнего с конкретными значениями, либо указать какие из входных аргументов на какие позиции callable-объекта следует подставить. Кому интересно, как это может быть реализовано, прошу под кат.

<cut />
<i>Собственно реализация</i>
Вот простейший (надуманный) пример использования bind-a:
<source lang="cpp">
int sum( int lhs, int rhs )
{
  return lhs + rhs;
}

auto f_sum = std::bind( sum, 3, std::placeholders::_2 );
f_sum( 5, 7 ); // f_sum вернет 10
</source>
В данном примере std::bind берет за основу указатель на функцию sum и в качестве ее аргумента lhs использует 3, а в качестве rhs - второй аргумент из переданных на вход callable-объекта f_sum. Таким образом результатом вызова f_sum будет 10. Все очень просто. Понятно, что подобным же образом std::bind можно использовать и с функциями-членами. Отличие будет только в том, что в этом случае в качестве первого аргумента в конструктор callable-объекта необходимо передать объект соответствующего класса, у которого будет вызвана функция-член.
<spoiler title="Пример">
<source lang="cpp">
struct A
{
  void Print() const
  {
    std::cout << "A::Print()" << std::endl;
  }
};

A a;
auto f = std::bind(&A::Print, a);
f(); // "напечатает" A::Print()
</source>
</spoiler>
Итак, std::bind - шаблонная функция, которая принимает на вход указатель на callable-объект и аргументы, которые могут быть константами, переменными или плейсхолдерами (placeholder). В рамках нового (с++11) стандарта это может быть записано так:
<source lang="cpp">
namespace naive
{
template<typename Func, typename... BinderArgs>
binder<Func, BinderArgs...> bind( Func const & func, BinderArgs &&... args )
{
  return binder<Func, BinderArgs...>( func, std::forward<BinderArgs>( args )... );
}
}
</source>
В данном случае binder - шаблонный класс, который хранит в себе все переданные ему на вход аргументы, хранит по значению (поведение std::bind по умолчанию).
<source lang="cpp">
// ...
  template<typename Func, typename... BinderArgs>
  struct binder
  {
    binder( Func const & func, BinderArgs &&... binderArgs )
      : m_func{ func}
      , m_args{ std::forward<BinderArgs>(binderArgs)... }
    {}
    // ...
  private:
    invoker_t m_invoker;
    Func m_func;
    args_list<BinderArgs...> m_args;
  };
// ...
</source>
Для этих целей была реализована структура naive::args_list (очень грубо можно сравнить с std::tuple).
<spoiler title="args_list">
<source lang="cpp">
  // arg далее используется в качестве базового класса для args_list.
  // дополнительный шаблонный параметр  std::size_t нужен, чтобы имелась возможность различать
  // аргументы с одинаковым типом
  template<std::size_t, typename T>
  struct arg
  {
    explicit arg( T val ) : value( val ) {}

    T const value;
  };
  template<typename,typename...>
  struct args_list_impl;

  template<std::size_t... Indices, typename... Args>
  struct args_list_impl<indices<Indices...>, Args...> : arg<Indices, Args>...
  {
    template<typename... OtherArgs>
    args_list_impl( OtherArgs &&... args ) : arg<Indices, Args>( std::forward<OtherArgs>(args) )... {}
  };

  template<typename... Args>
  struct args_list : args_list_impl< typename make_indices< sizeof...( Args )>::type, Args... >
  {
    using base_t = args_list_impl< typename make_indices< sizeof...( Args ) >::type, Args... >;

    template<typename... OtherArgs>
    args_list( OtherArgs &&... args ) : base_t( std::forward<OtherArgs>(args)... ) {}
  };
</source>
</spoiler>
Кстати, аргументы в биндер при вызове его оператора "круглые скобки" передаются с использованием args_list (правда, там имеется небольшая хитрость на случай пустого списка).

Движемся далее. В теле класса naive::binder объявлен оператор "круглые скобки", который перенаправляет вызов в соответствующий invoker("вызыватель") в зависимости от переданного на вход callable-объекта. Здесь возможны два варианта: либо была передана "обычная" функция и тогда ее нужно вызвать так (free_function_invoker):
<source lang="cpp">
template<typename...Args>
void invoke( Func const & func, Args &&... args ) const
{
  return func( std::forward<Args>(args)... );
}
</source>
либо была передана функция-член класса и тогда вызов будет таким (member_function_invoker):
<source lang="cpp">
template<typename ObjType, typename...Args>
void invoke( Func const & func, ObjType && obj, Args &&... args ) const
{
  return (obj.*func)( std::forward<Args>(args)... );
}
</source>
Какой тип "вызывателя" использовать определяем на этапе конструирования биндера
<source lang="cpp">
using invoker_t =  conditional_t< std::is_member_function_pointer<Func>::value, member_function_invoker, free_function_invoker >;
</source>
Теперь самое интересное. Как же реализовать оператор "круглые скобки" у биндера.
<source lang="cpp">
template<typename Func, typename... BinderArgs>
struct binder
{
  // ...
  template<typename... Args>
  void operator()( Args &&... args ) const
  {
    // need check: sizeof...(Args) should not be less than max placeholder value
    call_function( make_indices< sizeof...(BinderArgs) >{}, std::forward<Args>(args)... );
  }

private:
  template< std::size_t... Indices, typename... Args >
  void call_function( indices<Indices...> const &, Args &&... args ) const
  {
    struct empty_list
    {
      empty_list( Args &&... args ) {}
    };

    using args_t = conditional_t< sizeof...(Args) == 0, empty_list, args_list<Args...> >;
    args_t const argsList{ std::forward<Args>(args)... };

    m_invoker.invoke( m_func, take_argument( get_arg<Indices,BinderArgs...>( m_args ), argsList )... );
  }
  // ...
};
</source>
Функция get_arg<I, Args...> просто возвращает I-й элемент из списка args_list<Args...>, полученного при конструировании naive::binder.
<spoiler title="get_arg">
<source lang="cpp">
  template<std::size_t I, typename Head, typename... Tail>
  struct type_at_index
  {
    using type = typename type_at_index<I-1, Tail...>::type;
  };

  template<typename Head, typename... Tail>
  struct type_at_index<0, Head, Tail...>
  {
    using type = Head;
  };

  template<std::size_t I, typename... Args>
  using type_at_index_t = typename type_at_index<I, Args...>::type;

  template<std::size_t I, typename... Args>
  type_at_index_t<I, Args...> get_arg( args_list<Args...> const & args )
  {
    arg< I, type_at_index_t<I, Args...> > const & argument = args;
    return argument.value;
  };
</source>
</spoiler>
Теперь рассмотрим take_arg. Имеется две перегрузки этой функции, одна из которых необходима для работы с плейсхолдерами, другая - для всех остальных случаев. Например, если при создании биндера в его конструктор был передан std::placeholders::_N, то при вызове оператора "круглые скобки" биндер подставит вместо std::placeholders::_N N-ый аргумент переданный ему на вход. Во всех остальных случаях биндер проигнорирует аргументы, переданные в оператор "круглые скобки" и подставит соответствующие значения, полученные им в конструкторе.
<spoiler title="take_argument">
<source lang="cpp">
  template<typename T, typename S>
  T take_argument( T const & arg, S const & args )
  {
    return arg;
  }

  template<typename T, typename... Args,
    std::size_t I = std::is_placeholder<T>::value,
    typename = typename std::enable_if< I != 0 >::type >
  type_at_index_t<I-1, Args...> take_argument( T const & ph, args_list<Args...> const & args )
  {
    return get_arg< I-1, Args... >( args );
  }
</source>
</spoiler>
Вот собственно и всё. Ниже даны примеры использования naive::bind.
<spoiler title="Примеры использования">
<source lang="cpp">
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
</source>
</spoiler>

Понятно, что naive::bind является наивной (см. название поста) реализацией std::bind и не претендует на включение в стандарт. Многие вещи можно было реализовать по-другому: например, вместо args_list использовать std::tuple, по-иному реализовать invoker (он же в тексте "вызыватель") и т.п. Целью статьи было попытаться разобраться в том, как устроен std::bind под капотом, посмотреть на его простейшую реализацию. Надеюсь, что это получилось. Спасибо за внимание.

Все исходники можно найти на
<a href="github">https://github.com/black-tomato/bind</a>
Компилировалось все g++-6.2.
