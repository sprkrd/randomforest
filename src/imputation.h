#ifndef IMPUTATION_H
#define IMPUTATION_H

#include "dataframe.h"

namespace sel
{

class ImputationMethod
{
  public:

    virtual void operator()(Dataframe& data) = 0;

    virtual ~ImputationMethod() {}
};

class MedianModeImputation : public ImputationMethod
{
  public:

    MedianModeImputation(const Dataframe& data);

    virtual void operator()(Dataframe& data) override;

  private:

    std::vector<Value::Ptr> substitutes_;
};

template<class Method>
class PerClass : public ImputationMethod
{
  public:

    PerClass(const Dataframe& data)
    {
      Dataframe::Partition part;
      data.partition(data.get_target_idx(), part);
      for (const auto& entry : part)
      {
        per_class_methods_[entry.first] = new Method(*entry.second);
      }
    }

    virtual void operator()(Dataframe& data) override
    {
      Dataframe::Partition part;
      data.partition(data.get_target_idx(), part);
      for (const auto& entry : part)
      {
        (*per_class_methods_[entry.first])(*entry.second);
      }
    }

    virtual ~PerClass()
    {
      for (const auto& entry : per_class_methods_)
      {
        delete entry.second;
      }
    }

  private:

    std::map<std::string, Method*> per_class_methods_;
};

} /* end namespace sel */

#endif
