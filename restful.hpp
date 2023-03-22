/**
 * @file restful.hpp
 * @author xlink32 (xlink32@foxmail.com)
 * @brief A Restful framework with automatic parameter conversion similar to SpringBoot implemented based on C++20
 * @version 0.3
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
#include <cstdint>
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

namespace Restful
{
  class Apis;
}

// Callback arg0 type
struct Ctx
{
  friend class Restful::Apis;

  Ctx(const std::string& _url, const std::string& _contentBody): url(_url), contentBody(_contentBody)
  {
    urlParamBegin = url.find_first_of('?');
    if (urlParamBegin == std::string::npos)
      urlWithoutParams = std::string_view(url);
    else
      urlWithoutParams = std::string_view(url).substr(0, urlParamBegin++);
  }
  ~Ctx() {}

  bool HasRestArg() const { return restBegin != std::string_view::npos; }

  std::string_view GetRestArg()
  {
    if (restBegin == std::string_view::npos)
      return {};

    if (restBegin >= urlWithoutParams.size())
    {
      restBegin = std::string_view::npos;
      return {};
    }

    std::string_view remain = urlWithoutParams.substr(restBegin);
    size_t           off    = remain.find('/');
    if (off == std::string_view::npos)
    {
      restBegin = std::string_view::npos;
      return remain;
    }
    restBegin += off + 1;
    auto ret = remain.substr(0, off);
    return ret;
  }

  std::string_view GetUrlWithoutParams() const { return urlWithoutParams; }

  std::string_view GetRawUrlParams() const
  {
    if (urlWithoutParams.size() < url.size())
      return std::string_view(url).substr(urlWithoutParams.size() + 1);
    return {};
  }

  std::string_view GetRawContentBody() const { return contentBody; }

  std::string_view GetUrlParam(const std::string_view& key)
  {
    auto it = parsedParams.find({ParamKey::Url, key});
    if (it != parsedParams.end())
      return it->second;

    if (urlParamBegin >= url.size())
      return {};

    std::string_view params = std::string_view(url).substr(urlParamBegin);
    while (!params.empty())
    {
      auto pos1 = params.find_first_of('=');
      auto pos2 = params.find_first_of('&');
      if (pos1 == std::string_view::npos)
      {
        urlParamBegin = std::string_view::npos;
        return {};
      }
      if (pos2 > pos1 && pos1 != (size_t)0)
      {
        std::string_view _key   = params.substr(0, pos1);
        std::string_view _value = params.substr(pos1 + 1, pos2 - pos1 - 1);
        parsedParams.insert({
            {ParamKey::Url, _key},
            _value
        });
        if (key == _key)
          return _value;
      }
      if (pos2 == std::string_view::npos)
      {
        urlParamBegin = std::string_view::npos;
        return {};
      }
      else
        urlParamBegin += pos2 + 1;

      params = params.substr(pos2 + 1);
    }
    return {};
  }

  std::string_view GetContentParam(const std::string_view& key)
  {
    auto it = parsedParams.find({ParamKey::Content, key});
    if (it != parsedParams.end())
      return it->second;

    if (contentParamBegin >= contentBody.size())
      return {};

    std::string_view params = std::string_view(contentBody).substr(contentParamBegin);
    while (!params.empty())
    {
      auto pos1 = params.find_first_of('=');
      auto pos2 = params.find_first_of('&');
      if (pos1 == std::string_view::npos)
      {
        contentParamBegin = std::string_view::npos;
        return {};
      }
      if (pos2 > pos1 && pos1 != (size_t)0)
      {
        std::string_view _key   = params.substr(0, pos1);
        std::string_view _value = params.substr(pos1 + 1, pos2 - pos1 - 1);
        parsedParams.insert({
            {ParamKey::Content, _key},
            _value
        });
        if (key == _key)
          return _value;
      }
      if (pos2 == std::string_view::npos)
      {
        contentParamBegin = std::string_view::npos;
        return {};
      }
      else
        contentParamBegin += pos2 + 1;

      params = params.substr(pos2 + 1);
    }
    return {};
  }

protected:
  void adjustRestBegin(size_t pos) { restBegin = pos; }

  std::string      url;
  std::string_view urlWithoutParams;
  std::string      contentBody;
  size_t           restBegin         = 0;
  size_t           urlParamBegin     = 0;
  size_t           contentParamBegin = 0;

  struct ParamKey
  {
    enum EParam : std::uint8_t
    {
      Url,
      Content,
    } type;
    std::string_view key;

    struct hash
    {
      size_t operator()(const ParamKey& key) const noexcept
      {
        size_t seed = std::hash<std::uint8_t>()(key.type);
        seed ^= std::hash<std::string_view>()(key.key) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
      }
    };

    friend bool operator==(const ParamKey& a, const ParamKey& b) noexcept { return a.type == b.type && a.key == b.key; }
  };

  std::unordered_map<ParamKey, std::string_view, ParamKey::hash> parsedParams;
};

namespace Restful
{
  /**
   * @brief Require Tag
   * @brief use to declare a Param is require
   */
  struct Require
  {
  };

  template<typename T, typename _ = void>
  struct DefaultValue
  {
  };

  /**
   * @brief Default Value Tag
   * @tparam T Value type
   *
   * @example
    struct MyDefaultInt: DefaultValue<int>
    {
      static int get_default_value() { return -1; }
    };
   */
  template<typename T>
  struct DefaultValue<T>
  {
  };

  namespace details
  {
    template<size_t N>
    struct string_literal
    {
      static_assert(N > 1, "string_literal should not be empty");

      char val[N];
      constexpr string_literal(const char (&s)[N]) { std::copy_n(s, N, val); }
      constexpr std::string_view view() const { return {val, N - 1}; }
    };

    template<typename T, typename Tuple, size_t Begin, size_t End, bool Stop>
    struct get_default_value_impl
    {
    };
    template<typename T, typename Tuple, size_t Begin, size_t End>
    struct get_default_value_impl<T, Tuple, Begin, End, true>
    {
      using type = void;
    };
    template<typename T, typename Tuple, size_t Begin, size_t End>
    struct get_default_value_impl<T, Tuple, Begin, End, false>
        : std::conditional_t<std::is_base_of_v<DefaultValue<T>, std::tuple_element_t<Begin, Tuple>>,
                             std::tuple_element<Begin, Tuple>,
                             get_default_value_impl<T, Tuple, Begin + 1, End, Begin + 1 >= End>>
    {
    };
    template<typename T, typename Tuple, size_t Begin = 0, size_t End = std::tuple_size_v<Tuple>>
    struct get_default_value: get_default_value_impl<T, Tuple, Begin, End, Begin >= End>
    {
    };
  } // namespace details

  template<typename T, typename... Args>
  struct PathParam
  {
    using type      = T;
    using pointer   = T*;
    using reference = T&;

    static constexpr bool isRequire = (std::is_same<Args, Require>::value || ...);
    pointer               obj;

    operator bool() const { return obj != nullptr; }

    reference operator*() { return *obj; }

    pointer operator->() { return obj; }

    PathParam(void* pobj): obj((pointer)pobj) {}

    friend std::ostream& operator<<(std::ostream& os, PathParam<T, Args...>& o)
    {
      if (o.obj)
        os << *o;
      return os;
    }

    static T* MakeDefaultValue()
    {
      using type = typename details::get_default_value<T, std::tuple<Args...>>::type;
      if constexpr (std::is_same_v<type, void>)
        return nullptr;
      else
        return new T(type::get_default_value());
    }
  };

  template<typename T, details::string_literal Key, typename... Args>
  struct UrlParam
  {
    constexpr static details::string_literal key = Key;

    using type      = T;
    using pointer   = T*;
    using reference = T&;

    static constexpr bool isRequire = (std::is_same<Args, Require>::value || ...);
    pointer               obj;

    operator bool() const { return obj != nullptr; }

    reference operator*() { return *obj; }

    pointer operator->() { return obj; }

    UrlParam(void* pobj): obj((pointer)pobj) {}

    friend std::ostream& operator<<(std::ostream& os, UrlParam<T, Key, Args...>& o)
    {
      if (o.obj)
        os << *o;
      return os;
    }

    static T* MakeDefaultValue()
    {
      using type = typename details::get_default_value<T, std::tuple<Args...>>::type;
      if constexpr (std::is_same_v<type, void>)
        return nullptr;
      else
        return new T(type::get_default_value());
    }
  };

  template<typename T, details::string_literal Key, typename... Args>
  struct PostParam
  {
    constexpr static details::string_literal key = Key;

    using type      = T;
    using pointer   = T*;
    using reference = T&;

    static constexpr bool isRequire = (std::is_same<Args, Require>::value || ...);
    pointer               obj;

    operator bool() const { return obj != nullptr; }

    reference operator*() { return *obj; }

    pointer operator->() { return obj; }

    PostParam(void* pobj): obj((pointer)pobj) {}

    friend std::ostream& operator<<(std::ostream& os, PostParam<T, Key, Args...>& o)
    {
      if (o.obj)
        os << *o;
      return os;
    }

    static T* MakeDefaultValue()
    {
      using type = typename details::get_default_value<T, std::tuple<Args...>>::type;
      if constexpr (std::is_same_v<type, void>)
        return nullptr;
      else
        return new T(type::get_default_value());
    }
  };

  template<typename T, typename... Args>
  struct PostBody
  {
    using type      = T;
    using pointer   = T*;
    using reference = T&;

    static constexpr bool isRequire = (std::is_same<Args, Require>::value || ...);
    pointer               obj;

    operator bool() const { return obj != nullptr; }

    reference operator*() { return *obj; }

    pointer operator->() { return obj; }

    PostBody(void* pobj): obj((pointer)pobj) {}

    friend std::ostream& operator<<(std::ostream& os, PostBody<T, Args...>& o)
    {
      if (o.obj)
        os << *o;
      return os;
    }

    static T* MakeDefaultValue()
    {
      using type = typename details::get_default_value<T, std::tuple<Args...>>::type;
      if constexpr (std::is_same_v<type, void>)
        return nullptr;
      else
        return new T(type::get_default_value());
    }
  };

  namespace ArgConvertors
  {
    using Convertor_t = std::function<bool(void*&, Ctx&, int)>;
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
      bool operator()(void*&, Ctx&, int) { throw std::logic_error("Unsupport convetor"); }
    };

    template<typename T, typename... Args>
    struct convertor<PathParam<T, Args...>>
    {
      bool operator()(void*& out, Ctx& ctx, int idx)
      {
        if constexpr (PathParam<T, Args...>::isRequire)
        {
          if (!ctx.HasRestArg())
            return false;
          out = base_convertor<T>(ctx.GetRestArg());

          if (out == nullptr)
            std::cout << "Require path param: " << idx << std::endl;

          return out != nullptr;
        }
        else // optional
        {
          if (!ctx.HasRestArg())
            return true;
          void* ptr = base_convertor<T>(ctx.GetRestArg());
          out       = ptr ? ptr : (void*)PathParam<T, Args...>::MakeDefaultValue();
          return true;
        }
      }
    };

    template<typename T, details::string_literal Key, typename... Args>
    struct convertor<UrlParam<T, Key, Args...>>
    {
      bool operator()(void*& out, Ctx& ctx, int idx)
      {
        constexpr std::string_view key = Key.view();
        if constexpr (UrlParam<T, Key, Args...>::isRequire)
        {
          out = base_convertor<T>(ctx.GetUrlParam(key));

          if (out == nullptr)
            std::cout << "Require url param: " << key << std::endl;

          return out != nullptr;
        }
        else // optional
        {
          void* ptr = base_convertor<T>(ctx.GetUrlParam(key));
          out       = ptr ? ptr : (void*)UrlParam<T, Key, Args...>::MakeDefaultValue();
          return true;
        }
      }
    };

    template<typename T, details::string_literal Key, typename... Args>
    struct convertor<PostParam<T, Key, Args...>>
    {
      bool operator()(void*& out, Ctx& ctx, int idx)
      {
        constexpr std::string_view key = Key.view();
        if constexpr (PostParam<T, Key, Args...>::isRequire)
        {
          out = base_convertor<T>(ctx.GetContentParam(key));

          if (out == nullptr)
            std::cout << "Require post param: " << key << std::endl;

          return out != nullptr;
        }
        else // optional
        {
          void* ptr = base_convertor<T>(ctx.GetContentParam(key));
          out       = ptr ? ptr : (void*)PostParam<T, Key, Args...>::MakeDefaultValue();
          return true;
        }
      }
    };

    template<typename T, typename... Args>
    struct convertor<PostBody<T, Args...>>
    {
      bool operator()(void*& out, Ctx& ctx, int idx)
      {
        if constexpr (PostBody<T, Args...>::isRequire)
        {
          out = base_convertor<T>(ctx.GetRawContentBody());

          if (out == nullptr)
            std::cout << "Require post body" << std::endl;

          return out != nullptr;
        }
        else // optional
        {
          void* ptr = base_convertor<T>(ctx.GetRawContentBody());
          out       = ptr ? ptr : (void*)PostBody<T, Args...>::MakeDefaultValue();
          return true;
        }
      }
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
          {
            if (!convertors[i](args[i], ctx, i))
            {
              for (int j = 0; j <= i; ++j)
                cleaners[j](args[j]);
              return {}; // "Require is not satisfied" -> HTTP/400 Bad Request
            }
          }

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
      return RegisterRestful(path, std::function<Return_t(Arg0_t, Args...)>(callback));
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

      typename std::decay<Arg0_t>::type ctx(path, contentBody);

      std::string _path = std::string(ctx.GetUrlWithoutParams());
      auto        it    = mRestfulCallbackMap.find(_path);
      size_t      pos   = path.size();
      for (;;)
      {
        if (it != mRestfulCallbackMap.end())
        {
          std::cout << "url: [" << path << "] -> [" << it->first << "]  " << std::endl;
          ctx.adjustRestBegin(pos + 1);
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