#ifndef DATAFRAME_H
#define DATAFRAME_H

#include "common.h"
#include "csv_reader.h"

#include <functional>
#include <map>

namespace sel
{

struct Attribute;
class Instance;
class Dataframe;
class Table;
class View;

typedef std::map<std::string, double> CategoryFrequency;

struct Attribute
{
  std::string name;
  bool numeric;
};

class Instance : public Stringifiable
{
  friend Table; /* The only type capable of initializing Instances. */

  public:

    Instance(const Instance& other);

    Instance& operator=(const Instance& other);

    int get_index() const { return idx_; }

    const Value& get(int idx) const { return *values_.at(idx); }

    const Value& get(const std::string& attr) const;

    const Value& get(const Attribute& attr) const;

    void set(int idx, Value::Ptr&& value);

    void set(const std::string& attr, Value::Ptr&& value);

    void set(const Attribute& attr, Value::Ptr&& value);

    virtual std::string to_str() const override;

  private:

    Instance() {}

    /** 
     * @brief Instances can be only initialized by Table members when loading
     * the data set from a file.
     * 
     * @param dataframe Parent data frame
     * @param idx Index of the instance
     * @param row Raw data
     */
    void init(const Dataframe* dataframe, int idx, const CsvRow& row);

    void swap(Instance& other);

    void copy(const Instance& other);

    const Dataframe* dataframe_;
    int idx_;
    std::vector<Value::Ptr> values_;

};

class Dataframe : public Stringifiable
{
  public:

    typedef std::unique_ptr<Dataframe> Ptr;
    typedef std::map<std::string, Ptr> Partition;

    virtual int get_nrecords() const = 0;

    bool empty() const { return not (bool)get_nrecords(); }

    operator bool() const { return empty(); }

    virtual int get_nattributes() const = 0;

    int get_attribute_idx(const std::string& name) const;

    int get_attribute_idx(const Attribute& attr) const
    {
      return get_attribute_idx(attr.name);
    }

    virtual const std::string& get_target_name() const = 0;

    virtual int get_target_idx() const = 0;

    virtual const Attribute& get_attribute(int idx) const = 0;

    virtual Instance& operator[](int idx) = 0;

    virtual const Instance& get_instance(int idx) const = 0;

    virtual const Table& get_root() const = 0;

    bool is_view() const { return (Dataframe*)&get_root() != this; }

    int get_nmissing() const;

    double get_pmissing() const;

    void category_freq(int column, CategoryFrequency& density,
        bool normalize=true) const;

    void category_freq(const std::string& column,
        CategoryFrequency& density, bool normalize=true) const;

    void category_freq(const Attribute& column,
        CategoryFrequency& density, bool normalize=true) const
    {
      category_freq(column.name, density, normalize);
    }

    double aggregate(int column,
        std::function<double(double,double)> aggregator, double acc=0.0) const;

    double aggregate(const std::string& column,
        std::function<double(double,double)> aggregator, double acc=0.0) const;

    double aggregate(const Attribute& column,
        std::function<double(double,double)> aggregator, double acc=0.0) const
    {
      return aggregate(column.name, aggregator, acc);
    }

    double mean(int column) const;

    double mean(const std::string& column) const;

    double mean(const Attribute& column) const { return mean(column.name); }

    double variance(int column) const;

    double variance(const std::string& column) const;

    double variance(const Attribute& column) const
    {
      return variance(column.name);
    }

    double max(int column) const;

    double max(const std::string& column) const;

    double max(const Attribute& column) const { return max(column.name); }

    double min(int column) const;

    double min(const std::string& column) const;

    double min(const Attribute& column) const { return min(column.name); }

    virtual void shuffle() = 0;

    virtual void sort_by_column(int column) = 0;

    void sort_by_column(const std::string& column);

    void sort_by_column(const Attribute& column)
    {
      sort_by_column(column.name);
    }

    void partition(int column, Partition& part) const;

    void partition(const std::string& column, Partition& part) const;

    void partition(const Attribute& column, Partition& part) const
    {
      partition(column.name, part);
    }

    virtual std::string to_str() const override;

    virtual ~Dataframe() {}

  private:

    void print_attr_info(std::ostream& os, int column) const;

};

class Table : public Dataframe
{
  public:

    Table(const std::string& csv, const std::string& meta);

    virtual int get_nrecords() const override { return instances_.size(); }

    virtual int get_nattributes() const override { return attributes_.size(); }
    
    virtual const std::string& get_target_name() const { return target_name_; }

    virtual int get_target_idx() const override { return target_idx_; }

    virtual const Attribute& get_attribute(int idx) const override
    {
      return attributes_[idx];
    }

    virtual Instance& operator[](int idx) { return instances_[idx]; }

    virtual const Instance& get_instance(int idx) const
    {
      return instances_[idx];
    }

    virtual const Table& get_root() const override { return *this; }

    virtual void shuffle() override;

    virtual void sort_by_column(int column) override;

    using Dataframe::sort_by_column;


  private:

    void read_metadata(const std::string& meta);

    void read_csvdata(const std::string& csv);

    void copy(const Table& other);

    std::vector<Attribute> attributes_;
    std::vector<Instance> instances_;
    std::string target_name_;
    int target_idx_;
};

class View : public Dataframe
{
  public:

    View(Dataframe& parent, int column, const std::string& exclude);

    View(Dataframe& parent, int begin=0,
        int end=std::numeric_limits<int>::max(), bool exclude=false);

    virtual int get_nrecords() const override { return instances_.size(); }

    virtual int get_nattributes() const override
    {
      return root_.get_nattributes();
    }

    virtual const std::string& get_target_name() const
    {
      return root_.get_target_name();
    }

    virtual int get_target_idx() const
    {
      return root_.get_target_idx();
    }

    virtual const Attribute& get_attribute(int idx) const override
    {
      return root_.get_attribute(idx);
    }

    virtual Instance& operator[](int idx)
    {
      return *instances_[idx];
    }

    virtual const Instance& get_instance(int idx) const
    {
      return *instances_[idx];
    }

    virtual const Table& get_root() const override { return root_; }

    virtual void shuffle() override;

    virtual void sort_by_column(int column) override;

    void filter(std::function<bool(const Instance&)> keep);

    using Dataframe::sort_by_column;

  private:

    const Table& root_;
    std::vector<Instance*> instances_;

};

std::ostream& operator<<(std::ostream& os, const Attribute& attr);

std::string density2str(const CategoryFrequency& density);

} /* end namespace rise */


#endif


