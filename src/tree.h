#ifndef TREE_H
#define TREE_H

#include "dataframe.h"
#include "json.hpp"

#include <cmath>


namespace sel
{

using nlohmann::json;

class DecisionStump;
class NumericDecisionStump;
class CategoricalDecisionStump;
class DecisionTree;

typedef double (*Metric)(const CategoryFrequency&);

double entropy(const CategoryFrequency& density);

double gini(const CategoryFrequency& density);

double error(const CategoryFrequency& density);

class DecisionStump : public Stringifiable
{
  public:

    static DecisionStump* from_json(json& stump);

    typedef std::unique_ptr<DecisionStump> Ptr;

    DecisionStump(const Dataframe& data, int split) :
      attr_(data.get_attribute(split)), split_(split) {}

    virtual bool send_left(const Instance& instance) const = 0;

    const Attribute& get_attribute() const { return attr_; }

    double get_m() const { return m_lowest_; }

    virtual void to_json(json& stump) const;

    virtual ~DecisionStump() {}

  protected:

    DecisionStump() {}

    Attribute attr_;
    int split_;
    double m_lowest_;

};

class NumericDecisionStump : public DecisionStump
{
  public:

    NumericDecisionStump(json& stump);

    NumericDecisionStump(const Dataframe& data, int split, Metric metric,
        Dataframe::Ptr& left, Dataframe::Ptr& right);

    virtual bool send_left(const Instance& instance) const override;
    
    virtual void to_json(json& stump) const override;

    virtual std::string to_str() const override;

  private:

    double thr_;

};

class CategoricalDecisionStump : public DecisionStump
{
  public:

    CategoricalDecisionStump(json& stump);

    CategoricalDecisionStump(const Dataframe& data, int split, Metric metric,
        Dataframe::Ptr& left, Dataframe::Ptr& right);

    virtual bool send_left(const Instance& instance) const override;

    virtual void to_json(json& stump) const override;

    virtual std::string to_str() const override;

  private:

    std::string to_left_;

};

class DecisionTree : public Stringifiable
{
  public:

    typedef std::unique_ptr<DecisionTree> Ptr;

    DecisionTree(json& tree);

    DecisionTree(const Dataframe& data, int n, int f, Metric m);

    DecisionTree(const Dataframe& data, int n, int f, Metric m,
        const std::vector<int>& candidate_features);

    /* We do not need to copy trees. Delete default constructor so it is
     * not accidentally used. */
    DecisionTree(const DecisionTree&) = delete;

    /* We do not need to copy trees. Delete default assignment operator so it
     * is not accidentally used. */
    DecisionTree& operator=(const DecisionTree&) = delete;

    std::string classify(const Instance& instance) const;

    void classify(const Dataframe& data, std::vector<std::string>& guesses) const;

    virtual std::string to_str() const override { return to_str(0); }

    std::string to_str(int indent) const;

    void count_features(std::map<std::string,int>& counts) const;

    void to_json(json& tree) const;

    std::string to_dot() const;

    ~DecisionTree();

  private:

    std::string to_dot(int node) const;

    void extract_mode_as_guess(const Dataframe& data);

    static bool all_equal(const Dataframe& data, int column);

    static void filter_features(const Dataframe& data,
        const std::vector<int>& columns, std::vector<int>& filtered);

    void fit(const Dataframe& data, int n, int f, Metric m,
        const std::vector<int>& candidate_features);

    std::string guess_;
    DecisionStump* stump_;
    DecisionTree* left_, *right_;

};

}

#endif

