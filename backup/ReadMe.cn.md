# restful-cpp

基于C++14实现的Restful路径参数自动转换的例子

```c++
Restful::Apis apis;
apis.RegisterRestful("/hello",
                    [](Ctx& ctx, int* a, double* b) -> Ret
                    {
                      if (!a || !b)
                        ...

                      std::cout << "a:" << *a << " b:" << *b << std::endl;
                      ...
                    })

    .RegisterRestful("/world",
                    [](Ctx& ctx, long* a, std::string* b) -> Ret
                    {
                      if (!a || !b)
                        ...

                      std::cout << "a:" << *a << " b:" << *b ;

                      // 获取所有剩余的参数
                      while (ctx.has_rest_arg())
                         std::cout << " arg:" << ctx.get_rest_arg();
                      std::cout << std::endl;
                      ...
                    })

apis.Test("/hello/1/2/3/4"); // out "a:1 b:2"
apis.Test("/world/1/2/3/4"); // out "a:1 b:2 arg:3 arg:4"
```



# 要求
需要 ```C++14```

在 [godbolt compiler explorer](https://gcc.godbolt.org/) 平台上对以下编译器测试通过
* x86-64 GCC 4.9
* x86-64 GCC 12.1
* x86-64 Clang 4.0.0
* x86-64 Clang 11.0.0


# 使用方法
## 要求
1. 函数类型 ```Ret (*)(Ctx&, Args*...)```
2. ```sizeof...(Args) >= 0```
3. 否则, ```restful.hpp@RegisterRestful``` 中的 ```static_assert``` 会报编译器错误

## 普通函数
```c++
Ret callback(Ctx& ctx, int* a)
{
  ...
}

Restful::Apis apis;
apis.RegisterRestful("/rest", callback);
// or
apis.RegisterRestful("/rest", std::function<decltype(callback)>(callback));
```

## lambda
```c++
Restful::Apis apis;
apis.RegisterRestful("/rest", [](Ctx& ctx, int* a) -> Ret{
  ...
});

// 成员函数
struct{
  Ret func(Ctx& ctx, int* a);
} s;

apis.RegisterRestful("/hello2", [&](Ctx& ctx, int* a) -> Ret{
  return s.func(ctx, a);
});
```

## 自定义对象 或 重载默认转换行为
```c++
struct CustomOrOverloadDefaultConvertObject
{
  int a;
};

// 请在Restful::ArgConvertors命名空间下实现"convert"和"clean"函数
namespace Restful
{
  namespace ArgConvertors
  {
    template<>
    inline void* convert<CustomOrOverloadDefaultConvertObject>(std::string&& src)
    {
      if (src.empty())
        return nullptr;

      // 可能需要对 URL 进行解码

      auto ret = new CustomOrOverloadDefaultConvertObject();
      ret->a   = MyAtoi(src.data());

      return ret;
    }

    template<>
    void clean<CustomOrOverloadDefaultConvertObject>(void* ptr)
    {
      if (ptr)
        delete (CustomObject*)ptr;
    }
  } // namespace ArgConvertors
} // namespace Restful

// 然后就可以使用了
Restful::Apis apis;
apis.RegisterRestful("/custom",
                       [](Ctx& ctx, CustomOrOverloadDefaultConvertObject* obj) -> Ret
                       {
                         ...
                       });
```

## 默认支持最多15个参数
```c++
Restful::Apis apis;
apis.RegisterRestful("/many",
                       [](Ctx& ctx, 
                          int* a, int* b, int* c,
                          int* d, int* e, int* f,
                          int* g, int* h, int* i,
                          int* j, int* k, int* l,
                          int* m, int* n, int* o
                       ) -> Ret
                       {
                         ...
                       });
```
