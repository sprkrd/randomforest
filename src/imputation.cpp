#include "imputation.h"

#include <algorithm>

namespace sel
{

MedianModeImputation::MedianModeImputation(const Dataframe& data)
{
  substitutes_.reserve(data.get_nattributes());
  for (int idx = 0; idx < data.get_nattributes(); ++idx)
  {
    if (idx == data.get_target_idx())
    {
      /* No need to impute the target column. */
      substitutes_.push_back(Value::Ptr(nullptr));
      continue;
    }
    const Attribute& attr = data.get_attribute(idx);
    if (attr.numeric)
    {
      std::vector<double> all_values;
      all_values.reserve(data.get_nrecords());
      for (int jdx = 0; jdx < data.get_nrecords(); ++jdx)
      {
        const Value& value = data.get_instance(jdx).get(idx);
        if (not value.is_missing()) all_values.push_back(value.get_number());
      }
      std::sort(all_values.begin(), all_values.end());
      double median = all_values[all_values.size()/2];
      substitutes_.push_back(Value::Ptr(new Number(median)));
    }
    else
    {
      CategoryFrequency density;
      data.category_freq(idx, density);
      std::string mode;
      double max_density = 0;
      for (const auto& entry : density)
      {
        if (entry.first != "?" and entry.second > max_density)
        {
          mode = entry.first;
          max_density = entry.second;
        }
      }
      substitutes_.push_back(Value::Ptr(new Category(mode)));
    }
  }
}

void MedianModeImputation::operator()(Dataframe& data)
{
  for (int idx = 0; idx < data.get_nrecords(); ++idx)
  {
    for (int jdx = 0; jdx < data.get_nattributes(); ++jdx)
    {
      const Value& val = data.get_instance(idx).get(jdx);
      if (val.is_missing())
      {
        data[idx].set(jdx, substitutes_[jdx]->clone());
      }
    }
  }
}

} /* end namespace sel */

