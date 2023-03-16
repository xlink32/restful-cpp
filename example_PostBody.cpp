#include "restful.hpp"

using namespace std;
using namespace Restful;

int main()
{
  Apis apis;
  apis.RegisterRestful("/post",
                       [](Ctx& ctx, PostBody<std::string /* JsonObject */> body) -> Ret
                       {
                         if (!body)
                           return {};

                         cout << body << endl;
                         return {};
                       });

  apis.RegisterRestful("/post2",
                       [](Ctx& ctx, PostBody<std::string_view> body) -> Ret
                       {
                         if (!body)
                           return {};

                         cout << body << endl;
                         return {};
                       });

  apis.Test("/post", "hello%20world");
  /**
      hello%20world
  */

  apis.Test("/post2", R"({"a":1, "b":[1, false]})");
  /**
      {"a":1, "b":[1, false]}
  */
}