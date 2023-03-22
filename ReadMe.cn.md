# restful-cpp

[English](./ReadMe.md)

基于C++20实现的类似于SpringBoot的参数转换的Restful框架

### Example：
```c++
  Apis apis;
  struct DefaultUserId: DefaultValue<int>
  {
    static int get_default_value() { return -1; }
  };
  apis.RegisterRestful("/urlparam",
                       [](Ctx& ctx, UrlParam<int, "uid", DefaultUserId> userId, UrlParam<std::string, "pass", Require> pass) -> Ret
                       {
                         cout << "uid: " << userId << endl;
                         cout << "pass: " << pass << endl;
                         return {};
                       });
  apis.Test("/urlparam?uid=123&pass=dfsfd");
  /**
      uid: 123
      pass: dfsfd
  */
  apis.Test("/urlparam?pass=dfsfd");
  /**
      uid: -1
      pass: dfsfd
  */
  apis.Test("/urlparam?uid=123");
  /**
      Require url param: pass
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

# Changelog
2023/3/22:
  添加 Require Tag 和 Default Value Tag 的支持


# 使用方法
## 要求
1. 函数类型 ```Ret (*)(Ctx&, Args...)```
2. ```sizeof...(Args) >= 0```
3. ```Args...```必须为
   ```UrlParam<T,String>```或
   ```PostParam<T,String>```或
   ```PostBody<T>```或
   ```PathParam<T>```
4. 对于 ```UrlParam/PostParam/PostBody/PathParam```, ```...```可以为 ```Restful::Require``` 或者 继承自 ```Restful::DefaultValue<T>``` 且有以下方法 ```static T get_default_value()``` 的结构体, @详见下面的例子
   
## Example
[UrlParam Example](./example_UrlParam.cpp)

[PostParam Example](./example_PostParam.cpp)

[PostBody Example](./example_PostBody.cpp)

[PathParam Example](./example_PathParam.cpp)

---

[Require Tag Example](./example_Require.cpp)
```c++
  struct MyDefault_Int1: DefaultValue<int>
  {
    static int get_default_value() { return 1; }
  };
  apis.RegisterRestful("/1",
                       [](Ctx& ctx, UrlParam<int, "a", Require> a) -> Ret
                       {
                         cout << "a: " << a << endl;
                         return {};
                       });

  // !!! Require will overwrite DefaultValue
  apis.RegisterRestful("/2",
                       [](Ctx& ctx, PostParam<int, "a", MyDefault_Int1, Require> a) -> Ret
                       {
                         cout << "a: " << a << endl;
                         return {};
                       });

  apis.Test("/1?a=123");
  /**
      a: 123
  */

  apis.Test("/1");
  /**
      Require url param: a
  */

  apis.Test("/2");
  /**
      Require post param: a
  */
```

---

[Default Value Tag Example](./example_DefaultValue.cpp)
```c++
  /* template
    struct MyDefault_T: DefaultValue<T>
    {
      static T get_default_value() { return T(); }
    };
  */
  struct MyDefault_Int1: DefaultValue<int>
  {
    static int get_default_value() { return 1; }
  };
  struct MyDefault_Int2: DefaultValue<int>
  {
    static int get_default_value() { return 2; }
  };
  apis.RegisterRestful("/1",
                       [](Ctx& ctx, UrlParam<int, "a", MyDefault_Int1> a, UrlParam<int, "b", MyDefault_Int2> b) -> Ret
                       {
                         cout << "a: " << a << endl;
                         cout << "b: " << b << endl;
                         return {};
                       });

  apis.RegisterRestful("/2",
                       [](Ctx& ctx, PostParam<int, "a", MyDefault_Int1> a, PostParam<int, "b", MyDefault_Int2> b) -> Ret
                       {
                         cout << "a: " << a << endl;
                         cout << "b: " << b << endl;
                         return {};
                       });

  apis.Test("/1?a=123&b=456");
  /**
      a: 123
      b: 456
  */

  apis.Test("/1?b=456");
  /**
      a: 1
      b: 456
  */

  apis.Test("/2", "a=123");
  /**
      a: 123
      b: 2
  */
```

## 自定义对象 或 重载默认转换行为 与 组合使用
[Example](./example_Custom_Or_OverloadDefaultConvertor.cpp)
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
