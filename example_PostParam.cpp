#include "restful.hpp"

using namespace std;
using namespace Restful;

int main()
{
  Apis apis;
  apis.RegisterRestful("/login",
                       [](Ctx& ctx, PostParam<int, "uid"> userId, PostParam<std::string, "pass"> pass) -> Ret
                       {
                         cout << "uid: " << (userId ? *userId : -1) << endl;
                         cout << "pass: " << (pass ? *pass : "") << endl;
                         return {};
                       });

  apis.RegisterRestful("/mul",
                       [](Ctx& ctx, PostParam<float, "val1"> val1, PostParam<float, "val2"> val2) -> Ret
                       {
                         if (!val1 || !val2)
                           return {};

                         float result = *val1 * *val2;
                         cout << val1 << " * " << val2 << " = " << result << endl;
                         return {};
                       });

  apis.Test("/login", "uid=123&pass=dfsfd");
  /**
      uid: 123
      pass: dfsfd
  */

  apis.Test("/login", "uid=456&pass=cvbcvb");
  /**
      uid: 456
      pass: cvbcvb
  */

  apis.Test("/mul", "val1=124&val2=53.6");
  /**
      124 * 53.6 = 6646.4
  */
}