#include "restful.hpp"
using namespace std;

/**
 * @brief Example: common function
 */
Ret common_function(Ctx& ctx, int* a, double* b, std::string* c)
{
  cout << __FUNCTION__ << ": ";
  if (!a || !b || !c)
  {
    cout << "Missing args" << endl;
    return {};
  }

  cout << "a:" << *a << " b:" << *b << " c:" << *c;

  // Get all remain arguments
  while (ctx.has_rest_arg())
    cout << " arg:" << ctx.get_rest_arg();

  cout << endl;
  return {};
}

/**
 * @brief Example: common function with no arguments
 */
Ret common_function2(Ctx& ctx)
{
  cout << __FUNCTION__ << ": ";

  // Get all remain arguments
  while (ctx.has_rest_arg())
    cout << " arg:" << ctx.get_rest_arg();

  cout << endl;
  return {};
}

struct Handler
{
  Ret member_function(Ctx& ctx, int* code, std::string* msg)
  {
    cout << __FUNCTION__ << ": ";
    if (!code || !msg)
    {
      cout << "Missing args" << endl;
      return {};
    }
    cout << "code:" << *code << " msg:" << *msg << endl;
    return {};
  }

  static Ret static_member_function(Ctx& ctx, int* code, std::string* msg)
  {
    cout << __FUNCTION__ << ": ";
    if (!code || !msg)
    {
      cout << "Missing args" << endl;
      return {};
    }
    cout << "code:" << *code << " msg:" << *msg << endl;
    return {};
  }
};

/**
 * @brief Example: Convert to custom object or overload default convetor
 *
 */
struct CustomOrOverloadDefaultConvertObject
{
  int a;
};
/**
 * @brief Support for converting custom object
 */
namespace Restful
{
  namespace ArgConvertors
  {
    // "123,abc" -> {.a=123, .b="abc"}
    template<>
    inline void* convert<CustomOrOverloadDefaultConvertObject>(std::string&& src)
    {
      if (src.empty())
        return nullptr;

      // URL decode src here, eg "%20" -> " "

      auto ret = new CustomOrOverloadDefaultConvertObject();
      ret->a   = std::strtol(src.data(), nullptr, 10);
      // ret->a   = myAtoi(src.data());

      return ret;
    }

    template<>
    void clean<CustomOrOverloadDefaultConvertObject>(void* ptr)
    {
      if (ptr)
        delete (CustomOrOverloadDefaultConvertObject*)ptr;
    }
  } // namespace ArgConvertors
} // namespace Restful

int main()
{
  Restful::Apis apis;

  // Support common function, "/{}/{}" will be remove
  apis.RegisterRestful("/cm/{}/{}", common_function)

      .RegisterRestful("/cm2/{}/{}", common_function)

      // Support std::function
      .RegisterRestful("/cm3", std::function<decltype(common_function)>(common_function))

      // Support lambda
      .RegisterRestful("/lambda",
                       [&](Ctx& ctx, int* code, std::string* msg) -> Ret
                       {
                         cout << "lambda: ";
                         if (!code || !msg)
                         {
                           cout << "Missing args" << endl;
                           return {};
                         }
                         cout << "code:" << *code << " msg:" << *msg << endl;
                         return {};
                       });

  Handler h;
  // Please use lambda to wrap member function
  apis.RegisterRestful("/member",
                       [&](Ctx& ctx, int* code, std::string* msg) { return h.member_function(ctx, code, msg); })

      // Support static member function
      .RegisterRestful("/smember", Handler::static_member_function)

      // Support custom object
      .RegisterRestful("/custom",
                       [](Ctx& ctx, CustomOrOverloadDefaultConvertObject* obj) -> Ret
                       {
                         cout << "custom: ";
                         if (!obj)
                         {
                           cout << "Missing args" << endl;
                           return {};
                         }
                         cout << "a:" << obj->a << endl;
                         return {};
                       })

      // Support up to 15 parameters
      .RegisterRestful("/many",
                       [](Ctx& ctx, int* a, int* b, int* c, int* d, int* e, int* f, int* g, int* h, int* i, int* j,
                          int* k, int* l, int* m, int* n, int* o) -> Ret
                       {
                         cout << "many: ";
                         // clang-format off
                         cout
                         << "a:" << (a ? *a : 0) << ' '
                         << "b:" << (b ? *b : 0) << ' '
                         << "c:" << (c ? *c : 0) << ' '
                         << "d:" << (d ? *d : 0) << ' '
                         << "e:" << (e ? *e : 0) << ' '
                         << "d:" << (d ? *d : 0) << ' '
                         << "e:" << (e ? *e : 0) << ' '
                         << "f:" << (f ? *f : 0) << ' '
                         << "g:" << (g ? *g : 0) << ' '
                         << "h:" << (h ? *h : 0) << ' '
                         << "i:" << (i ? *i : 0) << ' '
                         << "j:" << (j ? *j : 0) << ' '
                         << "k:" << (k ? *k : 0) << ' '
                         << "l:" << (l ? *l : 0) << ' '
                         << "m:" << (m ? *m : 0) << ' '
                         << "n:" << (n ? *n : 0) << ' '
                         << "o:" << (o ? *o : 0) << ' '
                         << endl;
                         // clang-format on
                         return {};
                       });

  // Test
  apis.Test("/cm/1/2/text/ignore/ignore");
  cout << endl;
  apis.Test("/cm/1/2/text/arg3");
  cout << endl;
  apis.Test("/cm/1/text");
  cout << endl;
  apis.Test("/cm/1/");
  cout << endl;
  apis.Test("/cm/1");
  cout << endl;
  apis.Test("/cm/");
  cout << endl;
  apis.Test("/cm");
  cout << endl;
  apis.Test("/cm2/2/text2");
  cout << endl;
  apis.Test("/cm3/3/text3");
  cout << endl;
  apis.Test("/lambda/4/text4");
  cout << endl;
  apis.Test("/member/5/text5");
  cout << endl;
  apis.Test("/smember/6/ignore");
  cout << endl;
  apis.Test("/custom/6,text6/ignore");
  cout << endl;
  apis.Test("/many/1/2/3/4/5/6/7/8/9/-1");
  cout << endl;

  return 0;
}