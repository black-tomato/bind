#ifndef BINDER_HPP_
#define BINDER_HPP_

#include "args_list.hpp"
#include <type_traits>
#include <utility>
#include <cstdlib>

namespace naive
{
  template<bool B, typename T, typename F >
  using conditional_t = typename std::conditional<B, T, F>::type;

  template<typename Func, typename... BinderArgs>
  struct binder
  {
    binder( Func const & func, BinderArgs &&... binderArgs )
      : m_func{ func}
      , m_args{ std::forward<BinderArgs>(binderArgs)... }
    {}

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

      struct free_function_invoker
      {
        template<typename...Args>
        void invoke( Func const & func, Args &&... args ) const
        {
          return func( std::forward<Args>(args)... );
        }
      };

      struct member_function_invoker
      {
        template<typename ObjType, typename...Args>
        void invoke( Func const & func, ObjType && obj, Args &&... args ) const
        {
          return (obj.*func)( std::forward<Args>(args)... );
        }
      };

      using invoker_t =
        conditional_t< std::is_member_function_pointer<Func>::value, member_function_invoker, free_function_invoker >;

      invoker_t m_invoker;
      Func m_func;
      args_list<BinderArgs...> m_args;
  };

  template<typename Func, typename... BinderArgs>
  binder<Func, BinderArgs...> bind( Func const & func, BinderArgs &&... args )
  {
    return binder<Func, BinderArgs...>( func, std::forward<BinderArgs>( args )... );
  }
}

#endif // BINDER_HPP_

