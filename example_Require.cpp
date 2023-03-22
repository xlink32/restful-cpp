#include "restful.hpp"

using namespace std;
using namespace Restful;

int main()
{
  Apis apis;
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

  // Require will overwrite DefaultValue
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
}