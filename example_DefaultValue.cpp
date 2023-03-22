#include "restful.hpp"

using namespace std;
using namespace Restful;

int main()
{
  Apis apis;

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
}