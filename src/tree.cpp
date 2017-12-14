#include "tree.h"

//#include <iostream>

namespace sel
{

namespace /* utils for internal usage */
{

void normalize(const CategoryFrequency& counts, CategoryFrequency& density)
{
  double sum = 0;
  density = counts;
  for (const auto& entry : density) sum += entry.second;
  for (auto& entry : density) entry.second /= sum;
}

void shuffle(std::vector<int>& v, int f)
{
  for (int idx = 0; idx < f; ++idx)
  {
    int jdx = std::rand() % v.size();
    std::swap(v[idx], v[jdx]);
  }
}

}

double entropy(const CategoryFrequency& density)
{
  double h = 0;
  for (const auto& entry : density)
  {
    double p = entry.second;
    if (p > 0) h -= p*std::log2(p);
  }
  return h;
}

double gini(const CategoryFrequency& density)
{
  double g = 1;
  for (const auto& entry : density)
  {
    double p = entry.second;
    g -= p*p;
  }
  return g;
}

double error(const CategoryFrequency& density)
{
  double pmax = 0;
  for (const auto& entry : density)
  {
    pmax = std::max(pmax, entry.second);
  }
  return 1 - pmax;
}

DecisionStump* DecisionStump::from_json(json& stump)
{
  bool numeric = stump.at("attr").at("numeric").get<bool>();
  DecisionStump* ret = nullptr;
  if (numeric)
  {
    ret = new NumericDecisionStump(stump);
  }
  else
  {
    ret = new CategoricalDecisionStump(stump);
  }
  ret->split_ = stump.at("split");
  ret->attr_.name = stump.at("attr").at("name");
  return ret;
}

void DecisionStump::to_json(json& stump) const
{
  stump["attr"]["numeric"] = attr_.numeric;
  stump["attr"]["name"] = attr_.name;
  stump["split"] = split_;
}

NumericDecisionStump::NumericDecisionStump(json& stump)
{
  attr_.numeric = true;
  thr_ = stump.at("thr");
}

NumericDecisionStump::NumericDecisionStump(const Dataframe& data, int split,
    Metric metric, Dataframe::Ptr& left, Dataframe::Ptr& right) :
  DecisionStump(data, split)
{
  View sorted(const_cast<Dataframe&>(data)); // we won't modify the data
  sorted.sort_by_column(split_);
  int target_idx = sorted.get_target_idx();
  CategoryFrequency counts_i, counts_ip, probs_i, probs_ip;
  sorted.category_freq(target_idx, counts_ip, false);
  counts_i = counts_ip;
  m_lowest_ = inf;
  int idx_best = -1;
  for (auto& entry : counts_i) entry.second = 0;
  double previous = sorted.get_instance(0).get(split).get_number();
  for (int idx = 1; idx < sorted.get_nrecords(); ++idx)
  {
    double current = sorted.get_instance(idx).get(split).get_number();
    const std::string& class_ = sorted.get_instance(idx-1).get(
        target_idx).get_category();
    counts_i[class_] += 1;
    counts_ip[class_] -= 1;
    if (previous < current)
    {
      double p_i = ((double)idx)/sorted.get_nrecords();
      double p_ip = 1 - p_i;
      normalize(counts_i, probs_i);
      normalize(counts_ip, probs_ip);
      double m_i = metric(probs_i);
      double m_ip = metric(probs_ip);
      double m = p_i*m_i + p_ip*m_ip;
      if  (m < m_lowest_)
      {
        idx_best = idx;
        m_lowest_ = m;
      }
      //std::cout << "m: " << m << std::endl;
    }
    previous = current;
  }
  //std::cout << "m_lowest: " << m_lowest_ << std::endl;
  double x_l = sorted.get_instance(idx_best-1).get(split).get_number();
  double x_r = sorted.get_instance(idx_best).get(split).get_number();
  thr_ = (x_l + x_r)/2;
  left.reset(new View(sorted, 0, idx_best));
  right.reset(new View(sorted, idx_best));
}

bool NumericDecisionStump::send_left(const Instance& instance) const
{
  double x = instance.get(split_).get_number();
  return x < thr_;
}

void NumericDecisionStump::to_json(json& stump) const
{
  DecisionStump::to_json(stump);
  stump["thr"] = thr_;
}

std::string NumericDecisionStump::to_str() const
{
  std::ostringstream oss;
  oss << attr_.name << "<" << thr_;
  return oss.str();
}

CategoricalDecisionStump::CategoricalDecisionStump(json& stump)
{
  attr_.numeric = false;
  to_left_ = stump.at("to_left");
}

CategoricalDecisionStump::CategoricalDecisionStump(const Dataframe& data,
    int split, Metric metric, Dataframe::Ptr& left, Dataframe::Ptr& right) :
  DecisionStump(data, split)
{
  int target_idx = data.get_target_idx();
  Dataframe::Partition part;
  data.partition(split, part);
  CategoryFrequency counts, counts_l, counts_r, probs_l, probs_r;
  data.category_freq(target_idx, counts, false);
  m_lowest_ = inf;
  for (const auto& entry : part)
  {
    entry.second->category_freq(target_idx, counts_l, false);
    counts_r = counts;
    for (const auto& catfreq : counts_l)
    {
      counts_r[catfreq.first] -= catfreq.second;
    }
    normalize(counts_l, probs_l);
    normalize(counts_r, probs_r);
    double p_l = entry.second->get_nrecords() / (double)data.get_nrecords();
    double p_r = 1 - p_l;
    double m_l = metric(probs_l);
    double m_r = metric(probs_r);
    double m = p_l*m_l + p_r*m_r;
    //std::cout << "m: " << m << std::endl;
    if (m < m_lowest_)
    {
      to_left_ = entry.first;
      m_lowest_ = m;
    }
    if (part.size() == 2) break; // no need to continue
  }
  //std::cout << "m_lowest: " << m_lowest_ << std::endl;
  left = std::move(part[to_left_]);
  right.reset(new View(const_cast<Dataframe&>(data), split, to_left_));
}

bool CategoricalDecisionStump::send_left(const Instance& instance) const
{
  const std::string& cat = instance.get(split_).get_category();
  return cat == to_left_;
}

void CategoricalDecisionStump::to_json(json& stump) const
{
  DecisionStump::to_json(stump);
  stump["to_left"] = to_left_;
}

std::string CategoricalDecisionStump::to_str() const
{
  return attr_.name + "=" + to_left_;
}

DecisionTree::DecisionTree(json& tree) :
  stump_(nullptr), left_(nullptr), right_(nullptr)
{
  //std::cout << tree << std::endl;
  try
  {
    stump_ = DecisionStump::from_json(tree.at("stump"));
    left_ = new DecisionTree(tree.at("left"));
    right_ = new DecisionTree(tree.at("right"));
  }
  catch (json::out_of_range)
  {
    guess_ = tree.at("guess");
  }
}

DecisionTree::DecisionTree(const Dataframe& data, int n, int f, Metric m) :
  stump_(nullptr), left_(nullptr), right_(nullptr)
{
  int n_attr = data.get_nattributes();
  std::vector<int> candidate_features;
  for (int idx = 0; idx < n_attr; ++idx)
  {
    if (idx != data.get_target_idx()) candidate_features.push_back(idx);
  }
  fit(data, n, f, m, candidate_features);
}

DecisionTree::DecisionTree(const Dataframe& data, int n, int f, Metric m,
    const std::vector<int>& candidate_features) :
  stump_(nullptr), left_(nullptr), right_(nullptr)
{
  fit(data, n, f, m, candidate_features);
}

std::string DecisionTree::classify(const Instance& instance) const
{
  if (stump_)
  {
    if (stump_->send_left(instance)) return left_->classify(instance);
    else return right_->classify(instance);
  }
  return guess_;
}

void DecisionTree::classify(const Dataframe& data,
    std::vector<std::string>& guesses) const
{
  guesses.resize(data.get_nrecords());
  for (int idx = 0; idx < data.get_nrecords(); ++idx)
  {
    guesses[idx] = classify(data.get_instance(idx));
  }
}

std::string DecisionTree::to_str(int indent) const
{
  std::ostringstream oss;
  std::string pre(indent, ' ');
  if (stump_)
  {
     oss << pre << *stump_ << '\n' << left_->to_str(indent+2) << '\n';
     oss << pre << "not(" << *stump_ << ")\n" << right_->to_str(indent+2);
  }
  else oss << pre << guess_;
  return oss.str();
}

void DecisionTree::to_json(json& tree) const
{
  if (stump_)
  {
    stump_->to_json(tree["stump"]);
    left_->to_json(tree["left"]);
    right_->to_json(tree["right"]);
  }
  else
  {
    tree["guess"] = guess_;
  }
}

std::string DecisionTree::to_dot() const
{
  std::ostringstream oss;
  oss << "digraph {\n";
  oss << to_dot(1);
  oss << '}';
  return oss.str();
}

std::string DecisionTree::to_dot(int node) const
{
  std::ostringstream oss;
  if (stump_)
  {
    oss << node << "[shape=ellipse,label=\"" << *stump_ << "\"];\n";
    oss << node << " -> " << (2*node) << ";\n";
    oss << node << " -> " << (2*node+1) << ";\n";
    oss << left_->to_dot(2*node);
    oss << right_->to_dot(2*node+1);
  }
  else
  {
    oss << node << "[label=\"" << guess_ << "\",shape=box];\n";
  }
  return oss.str();
}

void DecisionTree::count_features(std::map<std::string,int>& counts) const
{
  if (stump_)
  {
    counts[stump_->get_attribute().name] += 1;
    left_->count_features(counts);
    right_->count_features(counts);
  }
}

DecisionTree::~DecisionTree()
{
  delete stump_;
  delete left_;
  delete right_;
}

void DecisionTree::extract_mode_as_guess(const Dataframe& data)
{
  CategoryFrequency freq;
  data.category_freq(data.get_target_idx(), freq, false);
  double max_occurrences = 0;
  for (const auto& entry : freq)
  {
    if (entry.second > max_occurrences)
    {
      guess_ = entry.first;
      max_occurrences = entry.second;
    }
  }
}

bool DecisionTree::all_equal(const Dataframe& data, int column)
{
  const Value* first = &data.get_instance(0).get(column);
  for (int idx = 1; idx < data.get_nrecords(); ++idx)
  {
    const Value* current = &data.get_instance(idx).get(column);
    if (*current != *first) return false;
  }
  return true;
}

void DecisionTree::filter_features(const Dataframe& data,
    const std::vector<int>& columns, std::vector<int>& filtered)
{
  filtered.clear();
  for (int column : columns)
  {
    if (not all_equal(data, column)) filtered.push_back(column);
  }
}

void DecisionTree::fit(const Dataframe& data, int n, int f, Metric m,
    const std::vector<int>& candidate_features)
{
  if (data.get_nrecords() < n)
  {
    // less than minimum number of instances to split
    extract_mode_as_guess(data);
    return;
  }
  if (all_equal(data, data.get_target_idx()))
  {
    // no variability in target attribute
    guess_ = data.get_instance(0).get(data.get_target_idx()).get_category();
    return;
  }
  std::vector<int> filtered;
  filter_features(data, candidate_features, filtered);
  if (filtered.empty())
  {
    // no candidate features
    extract_mode_as_guess(data);
    return;
  }
  int f_ = std::min(f, (int)filtered.size());
  shuffle(filtered, f_);

  DecisionStump* best = nullptr;
  Dataframe::Ptr left_data, right_data;

  for (int idx = 0; idx < f_; ++idx)
  {
    int column = filtered[idx];
    DecisionStump* stump;
    Dataframe::Ptr left, right;
    if (data.get_attribute(column).numeric)
    {
      stump = new NumericDecisionStump(data, column, m, left, right);
    }
    else
    {
      stump = new CategoricalDecisionStump(data, column, m, left, right);
    }
    //std::cout << "stump->get_m(): " << stump->get_m() << std::endl;
    if (not best or stump->get_m() < best->get_m())
    {
      delete best;
      best = stump;
      left_data = std::move(left);
      right_data = std::move(right);
    }
  }
  //std::cout << "best->get_m(): " << best->get_m() << std::endl;
  stump_ = best;
  left_ = new DecisionTree(*left_data, n, f, m, filtered);
  right_ = new DecisionTree(*right_data, n, f, m, filtered);
}

}

