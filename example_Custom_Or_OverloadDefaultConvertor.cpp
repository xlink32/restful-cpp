#include "restful.hpp"

using namespace std;
using namespace Restful;

struct CustomOrOberloadDefaultConvertor
{
  std::string x;

  friend std::ostream& operator<<(std::ostream& os, const CustomOrOberloadDefaultConvertor& o)
  {
    os << o.x;
    return os;
  }
};

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

int main()
{
  Apis apis;
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
}