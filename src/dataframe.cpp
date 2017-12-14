#include "dataframe.h"

#include <algorithm>
#include <cstdlib>

namespace sel
{

namespace /* utils for internal usage */
{

int clip(int x, int lo, int up)
{
  if (x < lo) return lo;
  if (x > up) return up;
  return x;
}

} /* end anonymous namespace */

///////////////////
// Instance methods
///////////////////

Instance::Instance(const Instance& other)
{
  copy(other);
}

Instance& Instance::operator=(const Instance& other)
{
  copy(other);
  return *this;
}

const Value& Instance::get(const std::string& attr) const
{
  return get(dataframe_->get_attribute_idx(attr));
}

const Value& Instance::get(const Attribute& attr) const
{
  return get(dataframe_->get_attribute_idx(attr));
}

void Instance::set(int idx, Value::Ptr&& value)
{
  values_[idx] = std::move(value);
}

void Instance::set(const std::string& attr, Value::Ptr&& value)
{
  values_[dataframe_->get_attribute_idx(attr)] = std::move(value);
}

void Instance::set(const Attribute& attr, Value::Ptr&& value)
{
  values_[dataframe_->get_attribute_idx(attr)] = std::move(value);
}

void Instance::swap(Instance& other)
{
  std::swap(idx_, other.idx_);
  values_.swap(other.values_);
}

std::string Instance::to_str() const
{
  std::vector<std::string> out;
  out.reserve(values_.size());
  for (const Value::Ptr& value : values_)
  {
    out.push_back(value->to_str());
  }
  return std::to_string(idx_) + ": " + container2str(out);
}

void Instance::init(const Dataframe* dataframe, int idx, const CsvRow& row)
{
  dataframe_ = dataframe;
  idx_ = idx;
  values_.resize(row.size());
  for (int jdx = 0; jdx < dataframe_->get_nattributes(); ++jdx)
  {
    bool numeric = dataframe->get_attribute(jdx).numeric;
    values_[jdx] = std::move(Value::create(row[jdx], numeric));
  }
}

void Instance::copy(const Instance& other)
{
  idx_ = other.idx_;
  dataframe_ = other.dataframe_;
  values_.clear();
  for (const auto& value : other.values_) values_.push_back(value->clone());
}

////////////////////
// Dataframe methods
////////////////////

int Dataframe::get_attribute_idx(const std::string& name) const
{
  for (int idx = 0; idx < get_nattributes(); ++idx)
  {
    if (get_attribute(idx).name == name) return idx;
  }
  throw SelException(std::string("Non-existent attribute: ") + name);
}

int Dataframe::get_nmissing() const
{
  int count = 0;
  for (int idx = 0; idx < get_nrecords(); ++idx)
  {
    for (int jdx = 0; jdx < get_nattributes(); ++jdx)
    {
      if (get_instance(idx).get(jdx).is_missing()) ++count;
    }
  }
  return count;
}

double Dataframe::get_pmissing() const
{
  double nmissing = get_nmissing();
  int nvalues = get_nrecords()*(get_nattributes()-1);
  return nvalues > 0? nmissing / nvalues : 0;
}

void Dataframe::category_freq(int column, CategoryFrequency& density,
    bool normalize) const
{
  density.clear();
  for (int idx = 0; idx < get_nrecords(); ++idx)
  {
    const Value& curr = get_instance(idx).get(column);
    density[curr.get_category()] += 1;
  }
  if (normalize)
  {
    for (auto& entry : density)
    {
      entry.second /= get_nrecords();
    }
  }
}

void Dataframe::category_freq(const std::string& column,
    CategoryFrequency& density, bool normalize) const
{
  int column_idx = get_attribute_idx(column);
  category_freq(column_idx, density, normalize);
}

double Dataframe::aggregate(int column,
    std::function<double(double,double)> aggregator, double acc) const
{
  for (int idx = 0; idx < get_nrecords(); ++idx)
  {
    const Value& curr = get_instance(idx).get(column);
    if (not curr.is_missing()) acc = aggregator(acc, curr.get_number());
  }
  return acc;
}

double Dataframe::aggregate(const std::string& column,
    std::function<double(double,double)> aggregator, double acc) const
{
  int column_idx = get_attribute_idx(column);
  return aggregate(column_idx, aggregator, acc);
}

double Dataframe::mean(int column) const
{
  if (empty()) return 0;
  auto lambda = [](double acc, double x) { return acc + x; };
  double sum = aggregate(column, lambda);
  return sum / get_nrecords();
}

double Dataframe::mean(const std::string& column) const
{
  int column_idx = get_attribute_idx(column);
  return mean(column_idx);
}

double Dataframe::variance(int column) const
{
  if (empty()) return 0;
  double mean_ = mean(column);
  auto lambda = [](double acc, double x) { return acc + x*x; };
  double sumsq = aggregate(column, lambda);
  double meansq = sumsq / get_nrecords();
  return meansq - mean_*mean_;
}

double Dataframe::variance(const std::string& column) const
{
  int column_idx = get_attribute_idx(column);
  return variance(column_idx);
}

double Dataframe::max(int column) const
{
  auto lambda = [](double acc, double x) { return std::max(acc, x); };
  return aggregate(column, lambda);
}

double Dataframe::max(const std::string& column) const
{
  int column_idx = get_attribute_idx(column);
  return max(column_idx);
}

double Dataframe::min(int column) const
{
  auto lambda = [](double acc, double x) { return std::min(acc, x); };
  return aggregate(column, lambda, inf);
}

double Dataframe::min(const std::string& column) const
{
  int column_idx = get_attribute_idx(column);
  return min(column_idx);
}

void Dataframe::sort_by_column(const std::string& column)
{
  int column_idx = get_attribute_idx(column);
  sort_by_column(column_idx);
}

void Dataframe::partition(int column, Partition& part) const
{
  part.clear();
  if (empty()) return;
  View sorted(const_cast<Dataframe&>(*this)); /* It's OK! The view won't change this object. */
  sorted.sort_by_column(column);
  std::vector<int> transitions;
  for (int idx = 1; idx < sorted.get_nrecords(); ++idx)
  {
    const Value& prev = sorted.get_instance(idx-1).get(column);
    const Value& curr = sorted.get_instance(idx).get(column);
    if (prev != curr) transitions.push_back(idx);
  }
  int last_transition = 0;
  for (int transition : transitions)
  {
    const Value& val = sorted.get_instance(last_transition).get(column);
    std::string category = val.get_category();
    part[category] = Dataframe::Ptr(
        new View(sorted, last_transition, transition));
    last_transition = transition;
  }
  {
    const Value& val = sorted.get_instance(last_transition).get(column);
    std::string category = val.get_category();
    part[category] = Dataframe::Ptr(
        new View(sorted, last_transition, sorted.get_nrecords()));
  }
}

void Dataframe::partition(const std::string& column,
    Partition& part) const
{
  int column_idx = get_attribute_idx(column);
  partition(column_idx, part);
}

std::string Dataframe::to_str() const
{
  std::ostringstream oss;
  oss << "----------------------------------\n";
  oss << "#Columns: " << (get_nattributes()-1) << " (+ 1 target)"
      << "\n#Records: " << get_nrecords()
      << "\nMissing values: " << (get_pmissing()*100) << '%'
      << "\nTarget column: " << get_target_name()
      << "\nView? " << (is_view()? "yes" : "no");
  oss << "\nAttributes:";
  for (int idx = 0; idx < get_nattributes(); ++idx)
  {
    oss << "\n  ";
    print_attr_info(oss, idx);
  }
  if (get_nrecords() < 11)
  {
    for (int idx = 0; idx < get_nrecords(); ++idx)
    {
      oss << '\n' << get_instance(idx);
    }
  }
  else
  {
    oss << '\n' << get_instance(0)
        << '\n' << get_instance(1)
        << "\n..."
        << '\n' << get_instance(get_nrecords()-1);
  }
  oss << "\n----------------------------------\n";
  return oss.str();
}

void Dataframe::print_attr_info(std::ostream& os, int column) const
{
  const Attribute& attr = get_attribute(column);
  os << attr.name;
  if (attr.numeric)
  {
    double min_ = min(column);
    double max_ = max(column);
    double mean_ = mean(column);
    double variance_ = variance(column);
    double stdev = std::sqrt(variance_);
    os << " (numeric): min: " << min_ << ", max: " << max_
       << ", mean: " << mean_ << ", stdev: " << stdev;
  }
  else
  {
    CategoryFrequency density;
    category_freq(column, density);
    os << " (categoric): pdf: " << density2str(density);
  }
}

////////////////
// Table methods
////////////////

Table::Table(const std::string& csv, const std::string& meta)
{
  read_metadata(meta);
  read_csvdata(csv);
}

void Table::shuffle()
{
  for (int idx = 0; idx < instances_.size(); ++idx)
  {
    int jdx = std::rand() % instances_.size();
    instances_[idx].swap(instances_[jdx]);
  }
}

void Table::sort_by_column(int column)
{
  auto cmp = [column](const Instance& a, const Instance& b)
  {
    return a.get(column) < b.get(column);
  };
  std::sort(instances_.begin(), instances_.end(), cmp);
}

void Table::read_metadata(const std::string& meta)
{
  std::ifstream in(meta);
  if (not in)
  {
    throw SelException(std::string("Cannot read metafile: ") + meta);
  }
  int n_columns;
  if (not (in >> n_columns))
  {
    throw SelException("Cannot read number of columns from metafile");
  }
  if (n_columns < 2)
  {
    throw SelException("There must be at least two columns");
  }
  attributes_.resize(n_columns);
  for (int idx = 0; idx < n_columns; ++idx)
  {
    std::string attr_type;
    if (not (in >> attr_type >> attributes_[idx].name))
    {
      throw SelException("The metafile does not define all the attributes");
    }
    if (attr_type != "Real" and attr_type != "Nominal")
    {
      throw SelException(std::string("Unknown attribute type: ") + attr_type);
    }
    attributes_[idx].numeric = attr_type == "Real";
  }
  if (not (in >> target_name_))
  {
    throw SelException("Could not read target attribute");
  }
  target_idx_ = get_attribute_idx(target_name_);
}

void Table::read_csvdata(const std::string& csv)
{
  CsvReader reader(csv);
  CsvRow row;
  while (reader.next_row(row))
  {
    if (row.size() != attributes_.size())
    {
      throw SelException("Inconsistent number of columns");
    }
    instances_.push_back(Instance());
    instances_.back().init(this, instances_.size(), row);
  }
}

///////////////
// View methods
//7////////////

View::View(Dataframe& parent, int column, const std::string& exclude)
  : root_(parent.get_root())
{
  for (int idx = 0; idx < parent.get_nrecords(); ++idx)
  {
    Instance& instance = parent[idx];
    const std::string& cat = instance.get(column).get_category();
    if (cat != exclude) instances_.push_back(&instance);
  }
}

View::View(Dataframe& parent, int begin, int end, bool exclude)
  : root_(parent.get_root())
{
  begin = clip(begin, 0, parent.get_nrecords());
  end = clip(end, 0, parent.get_nrecords());
  int slice_size = std::max(0, end-begin);
  if (exclude)
  {
    instances_.resize(parent.get_nrecords() - slice_size);
    for (int idx = 0; idx < begin; ++idx)
    {
      instances_[idx] = &parent[idx];
    }
    for (int idx = end; idx < parent.get_nrecords(); ++idx)
    {
      instances_[idx-end+begin] = &parent[idx];
    }
  }
  else
  {
    instances_.resize(slice_size);
    for (int idx = begin; idx < end; ++idx)
    {
      instances_[idx-begin] = &parent[idx];
    }
  }
}

void View::shuffle()
{
  for (int idx = 0; idx < instances_.size(); ++idx)
  {
    int jdx = std::rand() % instances_.size();
    std::swap(instances_[idx], instances_[jdx]);
  }
}

void View::sort_by_column(int column)
{
  auto cmp = [column](const Instance* a, const Instance* b)
  {
    return a->get(column) < b->get(column);
  };
  std::sort(instances_.begin(), instances_.end(), cmp);
}

void View::filter(std::function<bool(const Instance&)> keep)
{
  std::vector<Instance*> instances;
  for (auto instance : instances_)
  {
    if (keep(*instance)) instances.push_back(instance);
  }
  instances_.swap(instances);
}

///////////////
// Free methods
///////////////

std::ostream& operator<<(std::ostream& os, const Attribute& attr)
{
  return os << attr.name << '(' << (attr.numeric? "num" : "cat") << ')';
}

std::string density2str(const CategoryFrequency& density)
{
  std::ostringstream oss;
  bool first = true;
  oss << '{';
  for (const auto& entry : density)
  {
    if (not first) oss << ',';
    oss << entry.first << ':' << (entry.second*100) << '%';
    first = false;
  }
  oss << '}';
  return oss.str();
}

} /* end namespace rise */
