#ifndef RANDOM_FOREST_H
#define RANDOM_FOREST_H

#include "tree.h"

namespace sel
{

class RandomForest;

class RandomForest
{
  public:

    typedef std::unique_ptr<RandomForest> Ptr;

    static Ptr load(const std::string& filename);

    RandomForest(const Dataframe& data, int ntrees, int f, int n, Metric m);

    RandomForest(const RandomForest&) = delete;

    RandomForest& operator=(const RandomForest&) = delete;

    std::string classify(const Instance& instance) const;

    void classify(const Dataframe& data, std::vector<std::string>& guesses) const;

    void to_json(json& forest) const;

    void save(const std::string& filename) const;

    void to_dot(const std::string& prefix) const;
 
    void count_features(std::map<std::string,int>& counts) const;

  private:

    RandomForest() {}

    std::vector<DecisionTree::Ptr> forest_;

};

}

#endif
