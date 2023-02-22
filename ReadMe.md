# restful-cpp

An example of Restful supporting variable parameters based on C++14. Example:

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

                      // Get all remain arguments
                      while (ctx.has_rest_arg())
                         std::cout << " arg:" << ctx.get_rest_arg();
                      std::cout << std::endl;
                      ...
                    })

apis.Test("/hello/1/2/3/4"); // out "a:1 b:2"
apis.Test("/world/1/2/3/4"); // out "a:1 b:2 arg:3 arg:4"
```



# Requirements
Require at least ```C++14```

Test on follow compiler in [godbolt compiler explorer](https://gcc.godbolt.org/)
* x86-64 GCC 4.9
* x86-64 GCC 12.1
* x86-64 Clang 4.0.0
* x86-64 Clang 11.0.0


# Usage
## require
1. function type such as ```Ret (*)(Ctx&, Args*...)```
2. ```sizeof...(Args) >= 0```
3. or not, ```static_assert``` in ```restful.hpp@RegisterRestful``` will be triggered

## common funtion
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

// for member function
struct{
  Ret func(Ctx& ctx, int* a);
} s;

apis.RegisterRestful("/hello2", [&](Ctx& ctx, int* a) -> Ret{
  return s.func(ctx, a);
});
```

## custom object or overload default convertor
```c++
struct CustomOrOverloadDefaultConvertObject
{
  int a;
};

// implement "convert" and "clean" function in namespace Restful::ArgConvertors
namespace Restful
{
  namespace ArgConvertors
  {
    template<>
    inline void* convert<CustomOrOverloadDefaultConvertObject>(std::string&& src)
    {
      if (src.empty())
        return nullptr;

      // URL decode src here, eg "%20" -> " "

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

// Now you can register with CustomOrOverloadDefaultConvertObject
Restful::Apis apis;
apis.RegisterRestful("/custom",
                       [](Ctx& ctx, CustomOrOverloadDefaultConvertObject* obj) -> Ret
                       {
                         ...
                       });
```

## Support up to 15 parameters
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
