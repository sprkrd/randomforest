#include "common.h"
#include <iostream>

void print_val(const std::string& name, const sel::Value::Ptr& val)
{
  std::cout << name << ": is missing? " << (val->is_missing()? "yes" : "no")
            << ", str: " << *val << std::endl;
}

int main()
{
  auto val1 = sel::Value::create("3.14", true);
  auto val2 = sel::Value::create("yes", false);
  auto val3 = sel::Value::create("?", true);
  auto val4 = sel::Value::create("?", false);
  print_val("val1", val1);
  print_val("val2", val2);
  print_val("val3", val3);
  print_val("val4", val4);
}


