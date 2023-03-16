/**
 * @file restful.hpp
 * @author xlink32 (xlink32@foxmail.com)
 * @brief A Restful framework with automatic parameter conversion similar to SpringBoot implemented based on C++20
 * @version 0.2
 * @date 2023-03-16
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef __RESTFUL_H__
#define __RESTFUL_H__

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <functional>
#include <tuple>
#include <type_traits>
#include <map>
#include <unordered_map>
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
  std::string      url;
  std::string_view urlWithoutParams;
  std::string      contentBody;
  size_t           rest_begin = 0;

  std::unordered_map<std::string_view, std::string_view> parsedUrlParams;
  std::unordered_map<std::string_view, std::string_view> parsedContentParams;

  Ctx() {}
  ~Ctx() {}

  bool has_rest_arg() const { return rest_begin != std::string_view::npos; }

  std::string_view get_rest_arg()
  {
    if (rest_begin == std::string_view::npos)
      return {};

    if (rest_begin >= urlWithoutParams.size())
    {
      rest_begin = std::string_view::npos;
      return {};
    }

    std::string_view remain = urlWithoutParams.substr(rest_begin);
    size_t           off    = remain.find('/');
    if (off == std::string_view::npos)
    {
      rest_begin = std::string_view::npos;
      return remain;
    }
    rest_begin += off + 1;
    auto ret = remain.substr(0, off);
    return ret;
  }

  std::string_view get_url_params() const
  {
    auto pos = url.find_first_of('?');
    if (pos == std::string::npos)
      return {};
    return std::string_view(url).substr(pos + 1);
  }

  std::string_view get_content_body() const { return contentBody; }

  void parse_url_params()
  {
    std::string_view params = get_url_params();
    while (!params.empty())
    {
      auto pos1 = params.find_first_of('=');
      auto pos2 = params.find_first_of('&');
      if (pos1 == std::string_view::npos)
        break;
      if (pos2 > pos1 && pos1 != (size_t)0)
        parsedUrlParams[params.substr(0, pos1)] = params.substr(pos1 + 1, pos2 - pos1 - 1);
      if (pos2 == std::string_view::npos)
        break;
      params = params.substr(pos2 + 1);
    }
  }

  void parse_content_params()
  {
    std::string_view params = contentBody;
    while (!params.empty())
    {
      auto pos1 = params.find_first_of('=');
      auto pos2 = params.find_first_of('&');
      if (pos1 == std::string_view::npos)
        break;
      if (pos2 > pos1 && pos1 != (size_t)0)
        parsedContentParams[params.substr(0, pos1)] = params.substr(pos1 + 1, pos2 - pos1 - 1);
      if (pos2 == std::string_view::npos)
        break;
      params = params.substr(pos2 + 1);
    }
  }

  const std::unordered_map<std::string_view, std::string_view>& get_parse_url_params() const { return parsedUrlParams; }
  const std::unordered_map<std::string_view, std::string_view>& get_parse_content_params() const
  {
    return parsedContentParams;
  }
};

namespace Restful
{
  namespace details
  {
    template<size_t N>
    struct string_view_literal
    {
      char val[N];
      constexpr string_view_literal(const char (&s)[N]) { std::copy_n(s, N, val); }
      constexpr std::string_view view() const { return {val, N}; }
    };
  } // namespace details

  template<typename T>
  struct PathParam
  {
    using type      = T;
    using pointer   = T*;
    using reference = T&;

    pointer obj;

    operator bool() const { return obj != nullptr; }

    reference operator*() { return *obj; }

    pointer operator->() { return obj; }

    PathParam(void* pobj): obj((pointer)pobj) {}

    friend std::ostream& operator<<(std::ostream& os, PathParam<T>& o)
    {
      if (o.obj)
        os << *o;
      return os;
    }
  };

  template<typename T, details::string_view_literal Key>
  struct UrlParam
  {
    constexpr static details::string_view_literal key = Key;

    using type      = T;
    using pointer   = T*;
    using reference = T&;

    pointer obj;

    operator bool() const { return obj != nullptr; }

    reference operator*() { return *obj; }

    pointer operator->() { return obj; }

    UrlParam(void* pobj): obj((pointer)pobj) {}

    friend std::ostream& operator<<(std::ostream& os, UrlParam<T, Key>& o)
    {
      if (o.obj)
        os << *o;
      return os;
    }
  };

  template<typename T, details::string_view_literal Key>
  struct PostParam
  {
    constexpr static details::string_view_literal key = Key;

    using type      = T;
    using pointer   = T*;
    using reference = T&;

    pointer obj;

    operator bool() const { return obj != nullptr; }

    reference operator*() { return *obj; }

    pointer operator->() { return obj; }

    PostParam(void* pobj): obj((pointer)pobj) {}

    friend std::ostream& operator<<(std::ostream& os, PostParam<T, Key>& o)
    {
      if (o.obj)
        os << *o;
      return os;
    }
  };

  template<typename T>
  struct PostBody
  {
    using type      = T;
    using pointer   = T*;
    using reference = T&;

    pointer obj;

    operator bool() const { return obj != nullptr; }

    reference operator*() { return *obj; }

    pointer operator->() { return obj; }

    PostBody(void* pobj): obj((pointer)pobj) {}

    friend std::ostream& operator<<(std::ostream& os, PostBody<T>& o)
    {
      if (o.obj)
        os << *o;
      return os;
    }
  };

  namespace ArgConvertors
  {
    using Convertor_t = std::function<void(void*&, Ctx&)>;
    using Cleaner_t   = void (*)(void*);

    // [[ ******************** Base Convertor ********************
    template<typename T>
    inline void* base_convertor(const std::string_view& src)
    {
      throw std::logic_error("Unsupport convetor");
    }

    template<>
    inline void* base_convertor<char>(const std::string_view& src)
    {
      return src.empty() ? nullptr : new char(src[0]);
    }

    template<>
    inline void* base_convertor<unsigned char>(const std::string_view& src)
    {
      return src.empty() ? nullptr : new unsigned char(src[0]);
    }

#define REST_MAKE_BASE_CONVERTOR(Type)                                                                                 \
  template<>                                                                                                           \
  inline void* base_convertor<Type>(const std::string_view& src)                                                       \
  {                                                                                                                    \
    if (src.empty())                                                                                                   \
      return nullptr;                                                                                                  \
    auto ret = std::make_unique<Type>();                                                                               \
    auto ec  = std::from_chars(src.data(), src.data() + src.size(), *ret).ec;                                          \
    return (ec == std::errc()) ? (void*)ret.release() : nullptr;                                                       \
  }

    REST_MAKE_BASE_CONVERTOR(short);
    REST_MAKE_BASE_CONVERTOR(int);
    REST_MAKE_BASE_CONVERTOR(long);
    REST_MAKE_BASE_CONVERTOR(long long);
    REST_MAKE_BASE_CONVERTOR(unsigned short);
    REST_MAKE_BASE_CONVERTOR(unsigned int);
    REST_MAKE_BASE_CONVERTOR(unsigned long);
    REST_MAKE_BASE_CONVERTOR(unsigned long long);
    REST_MAKE_BASE_CONVERTOR(float);
    REST_MAKE_BASE_CONVERTOR(double);
    REST_MAKE_BASE_CONVERTOR(long double);

#undef REST_MAKE_BASE_CONVERTOR

    template<>
    inline void* base_convertor<std::string>(const std::string_view& src)
    {
      return src.empty() ? nullptr : new std::string(src);
    }

    // sp: string_view
    template<>
    inline void* base_convertor<std::string_view>(const std::string_view& src)
    {
      return src.empty() ? nullptr : new std::string_view(src);
    }

    // ]] ******************** Base Convertor ********************

    template<typename T>
    struct convertor
    {
      void operator()(void*&, Ctx&) { throw std::logic_error("Unsupport convetor"); }
    };

    template<typename T>
    struct convertor<PathParam<T>>
    {
      void operator()(void*& out, Ctx& ctx)
      {
        if (!ctx.has_rest_arg())
          return;
        out = base_convertor<T>(ctx.get_rest_arg());
      }
    };

    template<typename T, details::string_view_literal Key>
    struct convertor<UrlParam<T, Key>>
    {
      void operator()(void*& out, Ctx& ctx)
      {
        constexpr std::string_view key    = Key.view().substr(0, Key.view().size() - 1); // Remove Last '\0'
        auto&                      params = ctx.get_parse_url_params();
        auto                       it     = params.find(key);
        if (it == params.end())
          return;
        out = base_convertor<T>(it->second);
      }
    };

    template<typename T, details::string_view_literal Key>
    struct convertor<PostParam<T, Key>>
    {
      void operator()(void*& out, Ctx& ctx)
      {
        constexpr std::string_view key    = Key.view().substr(0, Key.view().size() - 1); // Remove Last '\0'
        auto&                      params = ctx.get_parse_content_params();
        auto                       it     = params.find(key);
        if (it == params.end())
          return;
        out = base_convertor<T>(it->second);
      }
    };

    template<typename T>
    struct convertor<PostBody<T>>
    {
      void operator()(void*& out, Ctx& ctx) { out = base_convertor<T>(ctx.get_content_body()); }
    };

    template<typename Result>
    void clean(void* ptr)
    {
    }

#define REST_MAKE_DEFAULT_CLEANER(type)                                                                                \
  template<>                                                                                                           \
  inline void clean<type>(void* ptr)                                                                                   \
  {                                                                                                                    \
    if (ptr)                                                                                                           \
      delete (type*)ptr;                                                                                               \
  }

    REST_MAKE_DEFAULT_CLEANER(char);
    REST_MAKE_DEFAULT_CLEANER(short);
    REST_MAKE_DEFAULT_CLEANER(int);
    REST_MAKE_DEFAULT_CLEANER(long);
    REST_MAKE_DEFAULT_CLEANER(long long);
    REST_MAKE_DEFAULT_CLEANER(unsigned char);
    REST_MAKE_DEFAULT_CLEANER(unsigned short);
    REST_MAKE_DEFAULT_CLEANER(unsigned int);
    REST_MAKE_DEFAULT_CLEANER(unsigned long);
    REST_MAKE_DEFAULT_CLEANER(unsigned long long);
    REST_MAKE_DEFAULT_CLEANER(float);
    REST_MAKE_DEFAULT_CLEANER(double);
    REST_MAKE_DEFAULT_CLEANER(long double);
    REST_MAKE_DEFAULT_CLEANER(std::string);
    REST_MAKE_DEFAULT_CLEANER(std::string_view);

#undef REST_MAKE_DEFAULT_CLEANER
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

      template<typename Tuple, int I>
      using get_arg_type = typename std::tuple_element<I, Tuple>::type;

      template<int I, typename Callback, typename Tuple>
      struct invoker
      {
        Return_t operator()(Callback&& cb, Arg0_t ctx, std::vector<void*>& args) { return {}; }
      };

#define REST_ARG(i)                                                                                                    \
  (details::get_arg_type<Tuple, i>) { args[i] }

#if REST_GCC
      template<typename Callback, typename Tuple>
      struct invoker<0, Callback, Tuple>
      {
        Return_t operator()(Callback&& cb, Arg0_t ctx, std::vector<void*>& args) { return cb(ctx); }
      };
#define REST_INVOKER(I, ...)                                                                                           \
  template<typename Callback, typename Tuple>                                                                          \
  struct invoker<I, Callback, Tuple>                                                                                   \
  {                                                                                                                    \
    Return_t operator()(Callback&& cb, Arg0_t ctx, std::vector<void*>& args) { return cb(ctx, __VA_ARGS__); }          \
  };
#else
      template<typename Sig, typename Tuple>
      struct invoker<0, std::function<Sig>, Tuple>
      {
        Return_t operator()(const std::function<Sig>& cb, Arg0_t ctx, std::vector<void*>& args) { return cb(ctx); }
      };
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

#undef REST_INVOKER
#undef REST_ARG

      template<typename... Args>
      static std::function<Return_t(Arg0_t ctx)> make_invoker(std::function<Return_t(Arg0_t, Args...)>&& callback,
                                                              std::vector<ArgConvertors::Convertor_t>&&  convertors,
                                                              std::vector<ArgConvertors::Cleaner_t>&&    cleaners)
      {
        return [callback = std::move(callback), convertors = std::move(convertors),
                cleaners = std::move(cleaners)](Arg0_t ctx) -> Return_t
        {
          std::vector<void*> args;
          args.resize(sizeof...(Args));
          for (int i = 0; i < sizeof...(Args); ++i)
            convertors[i](args[i], ctx);

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
    template<typename... Args>
    Apis& RegisterRestful(const std::string& path, std::function<Return_t(Arg0_t, Args...)>&& callback)
    {
      static_assert(sizeof...(Args) <= 15, "Arguments count must <= 15");

      if (path.empty() || path[0] != '/')
        throw std::logic_error("url should start with '/'");

      mRestfulCallbackMap[path] = {
          .invoker = details::make_invoker(std::move(callback), {ArgConvertors::convertor<Args>()...},
                                           {ArgConvertors::clean<typename Args::type>...}),
      };

      return *this;
    }

    template<typename... Args>
    Apis& RegisterRestful(const std::string& path, Return_t (*callback)(Arg0_t, Args...))
    {
      return RegisterRestful(path, RestfulCallback_t<Args...>(callback));
    }

    template<typename Lambda>
    Apis& RegisterRestful(const std::string& path, Lambda callback)
    {
      using func_t = details::function_traits<Lambda>;
      using args_t = typename func_t::args_type;

      // assert return type == Return_t
      static_assert(std::is_same<typename func_t::return_type, Return_t>::value,
                    "callback's return type must equal to Return_t");

      // assert arg0 type == Arg0_t
      static_assert(std::is_same<typename std::tuple_element<0, args_t>::type, Arg0_t>::value,
                    "callback's first arg type must equal to Arg0_t");

      return RegisterRestful(path, typename func_t::function(callback));
    }

    void Test(const std::string& path, const std::string& contentBody = "")
    {
      if (path.empty() || path[0] != '/')
        return;

      typename std::decay<Arg0_t>::type ctx;
      ctx.url              = path;
      ctx.urlWithoutParams = std::string_view(ctx.url).substr(0, ctx.url.find_first_of('?'));
      ctx.contentBody      = contentBody;
      ctx.parse_url_params();
      ctx.parse_content_params();

      std::string _path = std::string(ctx.urlWithoutParams);
      auto        it    = mRestfulCallbackMap.find(_path);
      size_t      pos   = path.size();
      for (;;)
      {
        if (it != mRestfulCallbackMap.end())
        {
          std::cout << "url: [" << path << "] -> [" << it->first << "]  " << std::endl;
          ctx.rest_begin = pos + 1;
          it->second.invoker(ctx);
          return;
        }

        pos = _path.rfind('/', pos - 1);
        if (pos == 0 || pos == std::string::npos)
          break;

        it = mRestfulCallbackMap.find(_path.substr(0, pos));
      }
      std::cout << "Not found: " << path << std::endl;
    }

  private:
    std::map<std::string, ApiInfo> mRestfulCallbackMap;
  };
} // namespace Restful

#endif // !__RESTFUL_H__