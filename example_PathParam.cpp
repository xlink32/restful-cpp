#include "restful.hpp"

using namespace std;
using namespace Restful;

int main()
{
  Apis apis;
  apis.RegisterRestful("/path",
                       [](Ctx& ctx, PathParam<int> a, PathParam<int> b) -> Ret
                       {
                         cout << "a: " << a << endl;
                         cout << "b: " << b << endl;
                         return {};
                       });

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

  apis.Test("/path/1/2");
  /**
      a: 1
      b: 2
   */

  apis.Test("/path2/1/2/3/4/5");
  /**
      a: 1
      b: 2
      c: 3
      arg: 4
      arg: 5
    */
}