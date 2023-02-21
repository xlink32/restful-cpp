#include "restful.hpp"
using namespace std;

Ret common_function(Ctx& ctx, int* a, double* b, std::string* c)
{
  cout << "[common_function] ";
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

struct Handler
{
  Ret member_function(Ctx& ctx, int* code, std::string* msg)
  {
    cout << "[member_function] ";
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
    cout << "[static_member_function] ";
    if (!code || !msg)
    {
      cout << "Missing args" << endl;
      return {};
    }
    cout << "code:" << *code << " msg:" << *msg << endl;
    return {};
  }
};

struct CustomObject
{
  int         a;
  std::string b;
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
    inline void* convert<CustomObject>(std::string&& src)
    {
      if (src.empty())
        return nullptr;

      // URL decode src here, eg "%20" -> " "

      auto ret = new CustomObject();
      ret->a   = std::atoi(src.data());
      size_t p = src.find(',');
      if (p != std::string::npos)
        ret->b = src.substr(p + 1);

      return ret;
    }

    template<>
    void clean<CustomObject>(void* ptr)
    {
      if (ptr)
        delete (CustomObject*)ptr;
    }
  } // namespace ArgConvertors
} // namespace Restful

int main()
{
  Restful::Apis apis;

  // Support common function, "/{}/{}" will be remove
  apis.RegisterRestful("/hello/{}/{}", common_function);

  // Support std::function
  apis.RegisterRestful("/hello2", std::function<decltype(common_function)>(common_function));

  // Support lambda
  apis.RegisterRestful("/hello3",
                       [&](Ctx& ctx, int* code, std::string* msg) -> Ret
                       {
                         cout << "[Lambda] ";
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
  apis.RegisterRestful("/hello4",
                       [&](Ctx& ctx, int* code, std::string* msg) { return h.member_function(ctx, code, msg); });

  // Support static member function
  apis.RegisterRestful("/hello5", Handler::static_member_function);

  // Support custom object
  apis.RegisterRestful("/hello6",
                       [](Ctx& ctx, CustomObject* obj) -> Ret
                       {
                         cout << "[custom] ";
                         if (!obj)
                         {
                           cout << "Missing args" << endl;
                           return {};
                         }
                         cout << "a:" << obj->a << " b:" << obj->b << endl;
                         return {};
                       });

  // Support up to 15 parameters
  apis.RegisterRestful("/hello7",
                       [](Ctx& ctx, int* a, int* b, int* c, int* d, int* e, int* f, int* g, int* h, int* i, int* j,
                          int* k, int* l, int* m, int* n, int* o) -> Ret
                       {
                         cout << "[hello 7] ";
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
  apis.Test("/hello/1/2/text/ignore/ignore");
  cout << endl;
  apis.Test("/hello/1/2/text/arg3");
  cout << endl;
  apis.Test("/hello/1/text");
  cout << endl;
  apis.Test("/hello/1/");
  cout << endl;
  apis.Test("/hello/1");
  cout << endl;
  apis.Test("/hello/");
  cout << endl;
  apis.Test("/hello");
  cout << endl;
  apis.Test("/hello2/2/text2");
  cout << endl;
  apis.Test("/hello3/3/text3");
  cout << endl;
  apis.Test("/hello4/4/text4");
  cout << endl;
  apis.Test("/hello5/5/text5");
  cout << endl;
  apis.Test("/hello6/6,text6/ignore");
  cout << endl;
  apis.Test("/hello7/1/2/3/4/5/6/7/8/9/-1");
  cout << endl;

  return 0;
}