#ifndef ARGS_LIST_HPP_
#define ARGS_LIST_HPP_

#include <cstdlib>
#include <utility>
#include <functional>
#include <type_traits>

namespace naive
{
  template<std::size_t... Indices>
  struct indices
  {
    using type = indices<Indices...>;
  };

  template<std::size_t, typename Indices>
  struct append_index;

  template<std::size_t I, std::size_t... Indices>
  struct append_index< I, indices<Indices...> > : indices< Indices..., I > {};

  template<std::size_t N>
  struct make_indices : append_index< N-1, typename make_indices<N-1>::type > {};

  template<>
  struct make_indices<1> : indices<0> {};

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
}

#endif // ARGS_LIST_HPP_

