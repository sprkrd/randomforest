#include "random_forest.h"

#include <fstream>

namespace sel
{

RandomForest::RandomForest(const Dataframe& data, int ntrees, int f, int n,
    Metric m) : forest_(ntrees)
{
  if (f <= 0)
  {
    f = (int)std::round(std::sqrt(data.get_nattributes()));
  }
  for (auto& tree : forest_)
  {
    tree.reset(new DecisionTree(data, n, f, m));
  }
}

RandomForest::Ptr RandomForest::load(const std::string& filename)
{
  json forest_json;
  std::ifstream file(filename);
  if (not file)
  {
    throw SelException(std::string("File ")+filename+" cannot be loaded");
  }
  file >> forest_json;
  Ptr ret(new RandomForest);
  for (json& tree : forest_json)
  {
    ret->forest_.emplace_back(new DecisionTree(tree));
  }
  return ret;
}

std::string RandomForest::classify(const Instance& instance) const
{
  std::map<std::string, int> votes;
  for (const auto& tree : forest_)
  {
    std::string guess = tree->classify(instance);
    ++votes[guess];
  }
  int max_votes = 0;
  std::string most_frequent;
  for (const auto& entry : votes)
  {
    if (entry.second > max_votes)
    {
      max_votes = entry.second;
      most_frequent = entry.first;
    }
  }
  return most_frequent;
}

void RandomForest::classify(const Dataframe& data,
    std::vector<std::string>& guesses) const
{
  guesses.resize(data.get_nrecords());
  for (int idx = 0; idx < data.get_nrecords(); ++idx)
  {
    guesses[idx] = classify(data.get_instance(idx));
  }
}

void RandomForest::to_json(json& forest) const
{
  for (const auto& tree : forest_)
  {
    json tree_json;
    tree->to_json(tree_json);
    forest.push_back(tree_json);
  }
}

void RandomForest::save(const std::string& filename) const
{
  json forest_json;
  to_json(forest_json);
  std::ofstream file(filename);
  if (not file)
  {
    throw SelException(std::string("File ")+filename+" cannot be written");
  }
  file << forest_json;
}

void RandomForest::to_dot(const std::string& prefix) const
{
  int idx = 1;
  for (const auto& tree : forest_)
  {
    std::string filename = prefix + std::to_string(idx) + ".dot";
    std::ofstream file(filename);
    if (not file)
    {
      throw SelException(std::string("File ")+filename+" cannot be written");
    }
    file << tree->to_dot();
    ++idx;
  }
}

void RandomForest::count_features(std::map<std::string,int>& counts) const
{
  for (const auto& tree : forest_)
  {
    tree->count_features(counts);
  }
}

}

