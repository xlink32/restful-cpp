# restful-cpp

[English](./ReadMe.md)

基于C++20实现的类似于SpringBoot的参数转换的Restful框架


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


# 实现思路
[知乎 https://zhuanlan.zhihu.com/p/614595101](https://zhuanlan.zhihu.com/p/614595101)


# 要求
需要 ```C++20```

在 [godbolt compiler explorer](https://gcc.godbolt.org/) 平台上对以下编译器测试通过
* x86-64 GCC 12.2
* x86-64 GCC 11.1
* x86-64 Clang 13.0.0
* x86-64 Clang 15.0.0
* x86-64 MSVC 2022 (Unsupport due to error C4576)

对于版本<11的GCC和版本<13的Clang可以替换std::from_chars实现std::string_view到T类型的转换应该可以支持


# 使用方法
## 要求
1. 函数类型 ```Ret (*)(Ctx&, Args...)```
2. ```sizeof...(Args) >= 0```
3. ```Args...```必须为
   ```UrlParam<T,String>```或
   ```PostParam<T,String>```或
   ```PostBody<T,String>```或
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

## 自定义对象 或 重载默认转换行为 与 组合使用
```c++
struct CustomOrOverloadDefaultConvertObject
{
  int a;
};

// 请在Restful::ArgConvertors命名空间下实现"base_convertor"和"clean"函数
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

// 然后就可以使用了
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

## 默认支持最多15个参数


## 只支持PathParam的但只需C++14的可以看看[backup](./backup)中的老的实现版本
