# restful-cpp

[中文ReadMe](./ReadMe.cn.md)


A Restful framework with automatic parameter conversion similar to SpringBoot implemented based on C++20

###Example：
```c++
  Apis apis;
  apis.RegisterRestful("/urlparam",
                       [](Ctx& ctx, UrlParam<int, "uid"> userId, UrlParam<std::string, "pass"> pass) -> Ret
                       {
                         cout << "uid: " << (userId ? *userId : -1) << endl;
                         cout << "pass: " << (pass ? *pass : "") << endl;
                         return {};
                       });
  apis.Test("/urlparam?uid=123&pass=dfsfd");
  /**
      uid: 123
      pass: dfsfd
  */

  apis.RegisterRestful("/postparam",
                       [](Ctx& ctx, UrlParam<float, "val1"> val1, PostParam<float, "val2"> val2, PostParam<float, "val3"> val3) -> Ret
                       {
                         if (!val1 || !val2 || !val3)
                           return {};

                         float result = *val1 + *val2 + *val3;
                         cout << val1 << " + " << val2 << " + " << val3 << " = " << result << endl;
                         return {};
                       });
  apis.Test("/postparam?val1=124", "val2=53.6&val3=123");
  /**
      124 + 53.6 + 123 = 300.6
  */

  apis.RegisterRestful("/postbody",
                       [](Ctx& ctx, PostBody<std::string_view /* JsonObject */> body) -> Ret
                       {
                         if (!body)
                           return {};

                         cout << body << endl;
                         return {};
                       });
  apis.Test("/postbody", R"({"a":1, "b":[1, false]})");
  /**
      {"a":1, "b":[1, false]}
  */

  apis.RegisterRestful("/pathparam",
                       [](Ctx& ctx, PathParam<int> a, PathParam<std::string> b, PathParam<float> c) -> Ret
                       {
                         cout << "a: " << a << endl;
                         cout << "b: " << b << endl;
                         cout << "c: " << c << endl;
                         while (ctx.has_rest_arg())
                           cout << "arg: " << ctx.get_rest_arg() << endl;
                         return {};
                       });
  apis.Test("/pathparam/1/2/3/4/5");
  /**
      a: 1
      b: 2
      c: 3
      arg: 4
      arg: 5
    */
```

# Implementation ideas
[知乎(Chinese) https://zhuanlan.zhihu.com/p/614595101](https://zhuanlan.zhihu.com/p/614595101)


# Require
Require ```C++20```

The following compiler tests passed on the platform [godbolt compiler explorer](https://gcc.godbolt.org/)
* x86-64 GCC 12.2
* x86-64 GCC 11.1
* x86-64 Clang 13.0.0
* x86-64 Clang 15.0.0
* x86-64 MSVC 2022 (Unsupport due to error C4576)

To GCC version < 11 or Clang version < 13, you can replace default std::string_view to T convertor
because which are using std::from_chars


# Usage
## Require
1. Function type like ```Ret (*)(Ctx&, Args...)```
2. ```sizeof...(Args) >= 0```
3. ```Args...```must be
   ```UrlParam<T,String>```or
   ```PostParam<T,String>```or
   ```PostBody<T,String>```or
   ```PathParam<T,String>```

## UrlParam
```c++
  apis.RegisterRestful("/login",
                       [](Ctx& ctx, UrlParam<int, "a"> a, UrlParam<std::string, "b"> b) -> Ret
                       {
                          if(!a || !b)
                            ...
                          ...
                          return {};
                       });


  apis.Test("/login?a=123&b=asd"); // -> a:123 b:asd
```

## PostParam
```c++
  apis.RegisterRestful("/mul",
                       [](Ctx& ctx, PostParam<float, "val1"> val1, PostParam<float, "val2"> val2) -> Ret
                       {
                         if (!val1 || !val2)
                           ...

                         float result = *val1 * *val2;
                         cout << val1 << " * " << val2 << " = " << result << endl;
                         return {};
                       });

  apis.Test("/mul", "val1=124&val2=53.6"); // -> 124 * 53.6 = 6646.4
```

## PostBody
```c++
  apis.RegisterRestful("/body",
                       [](Ctx& ctx, PostBody<std::string /* JsonObject */> body) -> Ret
                       {
                         if (!body)
                           ...
                        ...
                       });

  apis.Test("/body", R"({"a":1, "b":[1, false]})"); // -> {"a":1, "b":[1, false]}
```

## PathParam
```c++
  apis.RegisterRestful("/path2",
                       [](Ctx& ctx, PathParam<int> a, PathParam<std::string> b, PathParam<float> c) -> Ret
                       {
                         cout << "a: " << a << endl;
                         cout << "b: " << b << endl;
                         cout << "c: " << c << endl;
                         while (ctx.has_rest_arg())
                           cout << "arg: " << ctx.get_rest_arg() << endl;
                         return {};
                       });

  apis.Test("/path2/1/2/3/4/5");
  /**
      a: 1
      b: 2
      c: 3
      arg: 4
      arg: 5
    */
```

## Custom object or Overload default convertor and Combined use
```c++
struct CustomOrOverloadDefaultConvertObject
{
  int a;
};

// Plz implement "base_convertor" and "clean" functions in Restful::ArgConvertors namespace
namespace Restful::ArgConvertors
{
  template<>
  inline void* base_convertor<CustomOrOberloadDefaultConvertor>(const std::string_view& src)
  {
    if (src.empty())
      return nullptr;

    auto ret = std::make_unique<CustomOrOberloadDefaultConvertor>();
    ret->x   = src;
    return ret.release();

    // return success_convert ? ret.release() : nullptr;
  }

  template<>
  inline void clean<CustomOrOberloadDefaultConvertor>(void* ptr)
  {
    if (ptr)
      delete (CustomOrOberloadDefaultConvertor*)ptr;
  }
} // namespace Restful::ArgConvertors

  // and now you can use
  apis.RegisterRestful("/post",
                       [](Ctx& ctx,
                          PathParam<CustomOrOberloadDefaultConvertor>
                              a,
                          PostParam<CustomOrOberloadDefaultConvertor, "x">
                              b,
                          UrlParam<CustomOrOberloadDefaultConvertor, "x">
                              c) -> Ret
                       {
                         cout << "a: " << a << endl;
                         cout << "b: " << b << endl;
                         cout << "c: " << c << endl;
                         return {};
                       });

  apis.Test("/post/hello?x=123", "x=xyz");
  /**
    a: hello
    b: xyz
    c: 123
   */
```

## Up to 15 parameters are supported by default


## For those who only support PathParam but only need C++14, see the old implementation version in [backup](/backup)
