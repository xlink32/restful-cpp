/**
 * @file restful.hpp
 * @author xlink32 (xlink32@foxmail.com)
 * @brief An example of Restful supporting variable parameters based on C++14
 * @version 0.1
 * @date 2023-02-20
 *
 * @copyright Copyright (c) 2023
 */

#ifndef __RESTFUL_H__
#define __RESTFUL_H__

#include <cstdlib>
#include <functional>
#include <map>
#include <string>
#include <tuple>
#include <vector>
#include <iostream>

#ifdef _MSC_VER
#define REST_MSVC 1
#elif defined(__clang__)
#define REST_CLANG 1
#elif defined(__GNUC__)
#define REST_GCC 1
#else
#error "Unknown compiler"
#endif

// Callback return type
struct Ret
{
};

// Callback arg0 type
struct Ctx
{
  std::string url;
  size_t      rest_begin = 0;
  Ctx() {}
  ~Ctx() {}

  std::string get_rest()
  {
    if (rest_begin == std::string::npos || rest_begin >= url.size())
      return {};

    std::string remain = url.substr(rest_begin);
    size_t      off    = remain.find('/');
    if (off == std::string::npos)
    {
      rest_begin = std::string::npos;
      return remain;
    }
    rest_begin += off + 1;
    auto ret = remain.substr(0, off);
    return ret;
  }
};

namespace Restful
{
  /**
   * @brief if wants to add custom object support, please implement "convert" function and "clean" function
   */
  namespace ArgConvertors
  {
    using Convertor_t = void* (*)(std::string&&);
    using Cleaner_t   = void (*)(void*);

    template<typename Result>
    inline void* convert(std::string&& src)
    {
      return nullptr;
    }

    template<>
    inline void* convert<int>(std::string&& src)
    {
      return src.empty() ? nullptr : new int(std::atoi(src.data()));
    }

    template<>
    inline void* convert<float>(std::string&& src)
    {
      return src.empty() ? nullptr : new float(std::atof(src.data()));
    }

    template<>
    inline void* convert<std::string>(std::string&& src)
    {
      return src.empty() ? nullptr : new std::string(std::move(src));
    }

    template<typename Result>
    void clean(void* ptr)
    {
    }

#define MAKE_DEFAULT_CLEANER(type)                                                                                     \
  template<>                                                                                                           \
  inline void clean<type>(void* ptr)                                                                                   \
  {                                                                                                                    \
    if (ptr)                                                                                                           \
      delete (type*)ptr;                                                                                               \
  }

    MAKE_DEFAULT_CLEANER(int);
    MAKE_DEFAULT_CLEANER(float);
    MAKE_DEFAULT_CLEANER(std::string);

#undef MAKE_DEFAULT_CLEANER

    template<typename T>
    Convertor_t make_convertor()
    {
      return convert<T>;
    }

    template<typename T>
    Cleaner_t make_cleaner()
    {
      return clean<T>;
    }
  } // namespace ArgConvertors

  class Apis
  {
  public:
    using Return_t = Ret;
    using Arg0_t   = Ctx&;

    template<typename... Args>
    using RestfulCallback_t = std::function<Return_t(Arg0_t, Args*...)>;

  private:
    struct ApiInfo
    {
      std::function<Return_t(Arg0_t)> invoker;
    };

    struct details
    {
      template<typename Func>
      struct function_traits: public function_traits<decltype(&Func::operator())>
      {
      };

      /**
       * @brief Convert Lambda type to std::function
       */
      template<typename ClassType, typename ReturnType, typename... Args>
      struct function_traits<ReturnType (ClassType::*)(Args...) const>
      {
        using pointer     = ReturnType (*)(Args...);
        using function    = std::function<ReturnType(Args...)>;
        using return_type = ReturnType;
        using args_type   = std::tuple<Args...>;
      };

      template<typename Tuple, int Begin, int End, bool Cond>
      struct check_tuple_impl
      {
      };

      template<typename Tuple, int Begin, int End>
      struct check_tuple_impl<Tuple, Begin, End, true>: std::true_type
      {
      };

      template<typename Tuple, int Begin, int End>
      struct check_tuple_impl<Tuple, Begin, End, false>
          : std::conditional<std::is_pointer<typename std::tuple_element<Begin, Tuple>::type>::value,
                             check_tuple_impl<Tuple, Begin + 1, End, Begin + 1 >= End>, std::false_type>::type
      {
      };

      template<typename Tuple, int Begin, int End = std::tuple_size<Tuple>::value - 1>
      struct check_tuple: check_tuple_impl<Tuple, Begin, End, Begin <= End - 1>
      {
      };

      template<int Target, typename Tuple, int Idx, bool Cond>
      struct get_args_impl
      {
      };

      template<int Target, typename Tuple, int Idx>
      struct get_args_impl<Target, Tuple, Idx, true>
      {
        using type = std::tuple_element<Idx, Tuple>;
      };

      template<int Target, typename Tuple, int Idx>
      struct get_args_impl<Target, Tuple, Idx, false>
          : get_args_impl<Target, Tuple, Idx + 1, Idx + 1 == std::tuple_size<Tuple>::value>
      {
      };

      template<int Target, typename... Args>
      struct get_args: get_args_impl<Target, std::tuple<Args...>, 0, 0 == Target>
      {
      };

      template<typename Tuple, int I>
      using get_arg_type = typename std::tuple_element<I, Tuple>::type*;

      template<int I, typename Callback, typename Tuple>
      struct invoker
      {
        Return_t operator()(Callback&& cb, Arg0_t ctx, std::vector<void*>& args) { return {}; }
      };

#define REST_ARG(i) (details::get_arg_type<Tuple, i>)args[i]

#if REST_GCC
#define REST_INVOKER(I, ...)                                                                                           \
  template<typename Callback, typename Tuple>                                                                          \
  struct invoker<I, Callback, Tuple>                                                                                   \
  {                                                                                                                    \
    Return_t operator()(Callback&& cb, Arg0_t ctx, std::vector<void*>& args) { return cb(ctx, __VA_ARGS__); }          \
  };
#else
#define REST_INVOKER(I, ...)                                                                                           \
  template<typename Sig, typename Tuple>                                                                               \
  struct invoker<I, std::function<Sig>, Tuple>                                                                         \
  {                                                                                                                    \
    Return_t operator()(const std::function<Sig>& cb, Arg0_t ctx, std::vector<void*>& args)                            \
    {                                                                                                                  \
      return cb(ctx, __VA_ARGS__);                                                                                     \
    }                                                                                                                  \
  };
#endif

      // Support up to 15 parameters
      REST_INVOKER(1, REST_ARG(0));
      REST_INVOKER(2, REST_ARG(0), REST_ARG(1));
      REST_INVOKER(3, REST_ARG(0), REST_ARG(1), REST_ARG(2));
      REST_INVOKER(4, REST_ARG(0), REST_ARG(1), REST_ARG(2), REST_ARG(3));
      REST_INVOKER(5, REST_ARG(0), REST_ARG(1), REST_ARG(2), REST_ARG(3), REST_ARG(4));
      REST_INVOKER(6, REST_ARG(0), REST_ARG(1), REST_ARG(2), REST_ARG(3), REST_ARG(4), REST_ARG(5));
      REST_INVOKER(7, REST_ARG(0), REST_ARG(1), REST_ARG(2), REST_ARG(3), REST_ARG(4), REST_ARG(5), REST_ARG(6));
      REST_INVOKER(8, REST_ARG(0), REST_ARG(1), REST_ARG(2), REST_ARG(3), REST_ARG(4), REST_ARG(5), REST_ARG(6),
                   REST_ARG(7));
      REST_INVOKER(9, REST_ARG(0), REST_ARG(1), REST_ARG(2), REST_ARG(3), REST_ARG(4), REST_ARG(5), REST_ARG(6),
                   REST_ARG(7), REST_ARG(8));
      REST_INVOKER(10, REST_ARG(0), REST_ARG(1), REST_ARG(2), REST_ARG(3), REST_ARG(4), REST_ARG(5), REST_ARG(6),
                   REST_ARG(7), REST_ARG(8), REST_ARG(9));
      REST_INVOKER(11, REST_ARG(0), REST_ARG(1), REST_ARG(2), REST_ARG(3), REST_ARG(4), REST_ARG(5), REST_ARG(6),
                   REST_ARG(7), REST_ARG(8), REST_ARG(9), REST_ARG(10));
      REST_INVOKER(12, REST_ARG(0), REST_ARG(1), REST_ARG(2), REST_ARG(3), REST_ARG(4), REST_ARG(5), REST_ARG(6),
                   REST_ARG(7), REST_ARG(8), REST_ARG(9), REST_ARG(10), REST_ARG(11));
      REST_INVOKER(13, REST_ARG(0), REST_ARG(1), REST_ARG(2), REST_ARG(3), REST_ARG(4), REST_ARG(5), REST_ARG(6),
                   REST_ARG(7), REST_ARG(8), REST_ARG(9), REST_ARG(10), REST_ARG(11), REST_ARG(12));
      REST_INVOKER(14, REST_ARG(0), REST_ARG(1), REST_ARG(2), REST_ARG(3), REST_ARG(4), REST_ARG(5), REST_ARG(6),
                   REST_ARG(7), REST_ARG(8), REST_ARG(9), REST_ARG(10), REST_ARG(11), REST_ARG(12), REST_ARG(13));
      REST_INVOKER(15, REST_ARG(0), REST_ARG(1), REST_ARG(2), REST_ARG(3), REST_ARG(4), REST_ARG(5), REST_ARG(6),
                   REST_ARG(7), REST_ARG(8), REST_ARG(9), REST_ARG(10), REST_ARG(11), REST_ARG(12), REST_ARG(13),
                   REST_ARG(14));

#undef TWS_REST_INVOKER
#undef REST_ARG

      template<typename... Args>
      static std::function<Return_t(Arg0_t ctx)> make_invoker(RestfulCallback_t<Args...>&&              callback,
                                                              std::vector<ArgConvertors::Convertor_t>&& convertors,
                                                              std::vector<ArgConvertors::Cleaner_t>&&   cleaners)
      {
        return [callback = std::move(callback), convertors = std::move(convertors),
                cleaners = std::move(cleaners)](Arg0_t ctx) -> Return_t
        {
          std::vector<void*> args;
          args.reserve(sizeof...(Args));
          for (int i = 0; i < sizeof...(Args); ++i)
            args.emplace_back(convertors[i](ctx.get_rest()));

#if REST_GCC
          Return_t ret = details::invoker<sizeof...(Args), decltype(callback), std::tuple<Args...>>()(
              std::move(callback), ctx, args);
#else
          Return_t ret =
              details::invoker<sizeof...(Args), decltype(callback), std::tuple<Args...>>()(callback, ctx, args);
#endif

          for (int i = 0; i < sizeof...(Args); ++i)
            cleaners[i](args[i]);

          return ret;
        };
      }
    };

  public:
    /**
     * @brief Support std::function
     */
    template<typename... Args>
    void RegisterRestful(const std::string& path, RestfulCallback_t<Args...>&& callback)
    {
      static_assert(sizeof...(Args) <= 15, "Arguments count must <= 15");

      if (path.empty() || path[0] != '/')
        throw std::logic_error("url should start with '/'");

      size_t pos = path.find("/{");
      // if (pos == std::string::npos)
      //   throw std::logic_error("");

      std::string _path = path.substr(0, pos);
      while (_path.size() > 1 && _path.back() == '/')
        _path.pop_back();
      _path.shrink_to_fit();

      // int argCount = 1;
      // while ((pos = path.find("/{", pos + 1)) != std::string::npos)
      //   ++argCount;

      // if (argCount != sizeof...(Args))
      //   throw std::logic_error("args count in path does not equal to which in callback");

      mRestfulCallbackMap[_path] = {
          .invoker = details::make_invoker(std::move(callback),
                                           {ArgConvertors::make_convertor<Args>()...},
                                           {ArgConvertors::make_cleaner<Args>()...}),
      };
    }

    /**
     * @brief Support common function
     */
    template<typename... Args>
    void RegisterRestful(const std::string& path, Return_t (*callback)(Arg0_t, Args*...))
    {
      return RegisterRestful(path, RestfulCallback_t<Args...>(callback));
    }

    /**
     * @brief Support lambda
     */
    template<typename Lambda>
    void RegisterRestful(const std::string& path, Lambda callback)
    {
      using func_t = details::function_traits<Lambda>;
      using args_t = typename func_t::args_type;

      // assert return type == Return_t
      static_assert(std::is_same<typename func_t::return_type, Return_t>::value,
                    "callback's return type must equal to Return_t");

      // assert arg0 type == Arg0_t
      static_assert(std::is_same<typename std::tuple_element<0, args_t>::type, Arg0_t>::value,
                    "callback's first arg type must equal to Arg0_t");

      // assert args count >= 2
      static_assert(std::tuple_size<args_t>::value >= 2,
                    "callback's require at least 1 arguments except arg0, eg (Arg0_t, int*)");

      // check all arguments are pointers
      static_assert(details::check_tuple<args_t, 1>::value, "callback's Args must be pointer");

      return RegisterRestful(path, typename func_t::function(callback));
    }

    void Test(const std::string& path)
    {
      if (path.empty() || path[0] != '/')
        return;

      typename std::decay<Arg0_t>::type ctx;
      ctx.url    = path;
      auto   it  = mRestfulCallbackMap.find(path);
      size_t pos = path.size();
      for (;;)
      {
        if (it != mRestfulCallbackMap.end())
        {
          std::cout << "url: [" << path << "] -> [" << it->first << "]  " << std::endl;
          ctx.rest_begin = pos + 1;
          it->second.invoker(ctx);
          return;
        }

        pos = path.rfind('/', pos - 1);
        if (pos == 0 || pos == std::string::npos)
          break;

        it = mRestfulCallbackMap.find(path.substr(0, pos));
      }
      std::cout << "Not found: " << path << std::endl;
    }

  private:
    std::map<std::string, ApiInfo> mRestfulCallbackMap;
  };
} // namespace Restful

#endif // !__RESTFUL_H__