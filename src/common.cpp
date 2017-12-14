#include "common.h"

namespace sel
{

std::ostream& operator<<(std::ostream& os, const Stringifiable& strable)
{
  return os << strable.to_str();
}

Value::Ptr Value::create(const std::string& value, bool numeric)
{
  Value::Ptr ret;
  if (value == "?")
  {
    ret.reset(numeric? (Value*)new Number() : (Value*)new Category());
  }
  else if (numeric) ret.reset(new Number(std::stod(value)));
  else ret.reset(new Category(value));
  return ret;
}

std::string Number::to_str() const
{
  return is_missing()? std::string("?") : std::to_string(number_);
}

} /* end namespace sel */

