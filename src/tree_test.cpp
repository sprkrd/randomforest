#include "imputation.h"
#include "tree.h"
#include <cstdlib>
#include <iostream>

#ifndef DATA_PATH
#define DATA_PATH "../Data/"
#endif

int main(int argc, char* argv[])
{
  srand(42);
  if (argc != 2)
  {
    std::cerr << "Usage: " << argv[0] << " datasetname\n";
    return -1;
  }
  try
  {
    std::string datafile = std::string(DATA_PATH) + argv[1] + '/' + argv[1] + ".data";
    std::string metafile = std::string(DATA_PATH) + argv[1] + '/' + argv[1] + ".meta";

    std::srand(42);

    sel::Table table(datafile, metafile);
    std::cout << table << std::endl;

    //sel::MedianModeImputation imp(table);
    sel::PerClass<sel::MedianModeImputation> imp(table);
    imp(table);

    //sel::Dataframe::Ptr left, right;

    //sel::NumericDecisionStump stump(table, 1, sel::entropy, left, right);
    //sel::CategoricalDecisionStump stump(table, 14, sel::entropy, left, right);

    //std::cout << stump << std::endl;
    //std::cout << *left << std::endl;
    //std::cout << *right << std::endl;
    
    sel::DecisionTree tree(table, 5, 3, sel::gini);

    //std::cout << tree << std::endl;

    //std::cout << tree.to_dot() << std::endl;
    
    sel::json tree_json;

    tree.to_json(tree_json);

    //std::cout << tree_json << std::endl;
    
    sel::DecisionTree tree2(tree_json);

    std::cout << tree << std::endl;
    std::cout << tree2 << std::endl;
  }
  catch (sel::SelException& ex)
  {
    std::cerr << ex.what() << '\n';
  }
}

