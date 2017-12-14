#include "args.hxx"
#include "imputation.h"
#include "random_forest.h"

#include <ctime>
#include <cstdlib>
#include <iostream>

#ifndef DATA_PATH
#define DATA_PATH "../Data/"
#endif

struct Options
{
  bool train;
  std::string load, save, dot_prefix, dataset;
  int verbose, ntrees, f, n, cv, rng;
  sel::Metric metric;
};

Options parse_argv(int argc, char* argv[]);

void print_options(const Options& options);

double evaluate_forest(const sel::RandomForest& forest, const sel::Dataframe& test);

void rank_features(const sel::RandomForest& forest);

double mean(const std::vector<double>& v);

double stdev(const std::vector<double>& v);

int main(int argc, char* argv[])
{
  Options options = parse_argv(argc, argv);
  srand(options.rng);
  if (options.verbose >= 3) print_options(options);
  
  std::string datafile = std::string(DATA_PATH)+options.dataset+'/'+options.dataset+".data";
  std::string metafile = std::string(DATA_PATH)+options.dataset+'/'+options.dataset+".meta";

  try
  {
    sel::Table table(datafile, metafile);
    if (options.verbose >= 2) std::cout << table << std::endl;

    table.shuffle();

    bool has_missing = table.get_nmissing() > 0;

    if (options.train)
    {
      if (options.cv > 1)
      {
        if (has_missing)
        {
          std::cout << "Data set has missing value. Using per-class "
                    << "median/mode imputation in train set and "
                    << "global median/mode imputation in test set." << std::endl;
        }
        std::vector<double> accuracies(options.cv);
        std::vector<double> elapsed(options.cv);
        int fold_size = table.get_nrecords() / options.cv;
        for (int fold = 0; fold < options.cv; ++fold)
        {
          if (options.verbose >= 1) std::cout << "Fold " << (fold+1) << ": ";
          sel::Table copy(table);
          sel::View train(copy, fold*fold_size, (fold+1)*fold_size, true);
          sel::View test(copy, fold*fold_size, (fold+1)*fold_size);
          if (has_missing)
          {
            sel::PerClass<sel::MedianModeImputation> imp1(train);
            sel::MedianModeImputation imp2(train);
            imp1(train);
            imp2(test);
          }
          std::clock_t start = std::clock();
          sel::RandomForest::Ptr forest(new sel::RandomForest(
                train, options.ntrees, options.f, options.n, options.metric));
          elapsed[fold] = (std::clock() - start)/(double)CLOCKS_PER_SEC;
          accuracies[fold] = evaluate_forest(*forest, test);
          if (options.verbose >= 1)
          {
            std::cout << "accuracy = " << (accuracies[fold]*100)
                      << "%; elapsed(CPU) = " << elapsed[fold] << "s" << std::endl;
          }
        }
        double avgacc = mean(accuracies);
        double stdacc = stdev(accuracies);
        double avgelapsed = mean(elapsed);
        double stdelapsed = stdev(elapsed);
        if (options.verbose >= 1)
        {
          std::cout << "Accuracy: " << avgacc*100 << "+-" << stdacc*100 << "%" << std::endl;
          std::cout << "Elapsed: " << avgelapsed << "+-" << stdelapsed << "s" << std::endl;
        }
      }
      else
      {
        if (has_missing)
        {
          if (options.verbose >= 2) std::cout << "Data set has missing values. Using per-class median/mode imputation..." << std::endl;
          sel::PerClass<sel::MedianModeImputation> imp(table);
          imp(table);
        }
        sel::RandomForest::Ptr forest(new sel::RandomForest(
              table, options.ntrees, options.f, options.n, options.metric));
        if (not options.save.empty())
        {
          forest->save(options.save);
        }
        if (not options.dot_prefix.empty())
        {
          forest->to_dot(options.dot_prefix);
        }
        if (options.verbose >= 1) rank_features(*forest);
      }
    }
    else
    {
      if (has_missing)
      {
        if (options.verbose >= 2) std::cout << "Data set has missing values. Using global median/mode imputation...";
        sel::MedianModeImputation imp(table);
      }
      if (options.verbose >= 2) std::cout << "Loading tree from JSON..." << std::endl;
      auto forest = sel::RandomForest::load(options.load);
      double acc = evaluate_forest(*forest, table);
      if (options.verbose >= 1) std::cout << "Accuracy: " << (acc*100) << "%" << std::endl;
    }
  }
  catch (sel::SelException& e)
  {
    std::cerr << e.what() << '\n';
    return 1;
  }
}

Options parse_argv(int argc, char* argv[])
{
  args::ArgumentParser parser("Train and/or test a random forest with an arbitrary data set");
  args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
  args::ValueFlag<int> verbose(parser, "verbose_level", "0: no info, 1: elapsed train time, accuracy and feature weights, if applicable (default); 2: stats; 3+: options", {'v', "verbose"}); 
  args::ValueFlag<int> rng(parser, "seed", "RNG seed", {'v', "verbose"}); 
  args::ValueFlag<std::string> load(parser, "filename", "Load forest from JSON, instead of training from scratch", {'l', "load"});
  args::Group train(parser, "Train parameters", args::Group::Validators::DontCare);
  args::ValueFlag<int> ntrees(train, "ntrees", "Number of trees in the ensemble (default 10)", {'M', "ntrees"});
  args::ValueFlag<int> f(train, "f", "Number of features evaluated randomly at each split (sqrt of the number of attributes if not specified)", {'F', "feature-bag"});
  args::ValueFlag<int> n(train, "n", "Minimum number of instances to split (default 2)", {'N', "min-split"});
  args::Group metric(train, "Metric", args::Group::Validators::AtMostOne);
  args::Flag gini(metric, "gini", "Gini impurity (default)", {'g', "gini"});
  args::Flag entropy(metric, "entropy", "Entropy (information gain)", {'i', "entropy"});
  args::Flag error(metric, "error", "Error (1 - pmax)", {'e', "error"});
  args::ValueFlag<int> cv(train, "cv", "Cross validation (by default, no cross validation is performed)", {"cv"});
  args::ValueFlag<std::string> json(train, "filename", "Store forest in JSON format", {'j', "json"});
  args::ValueFlag<std::string> dot(train, "prefix", "Create dot files", {'d', "dot"});
  args::Positional<std::string> dataset(parser, "datasetname", "Name of the data set (default iris).");
  Options options = {true, "", "", "", "iris", 1, 10, -1, 2, 0, 42, sel::gini};
  try
  {
    parser.ParseCLI(argc, argv);
    if (verbose) options.verbose = args::get(verbose);
    if (rng) options.rng = args::get(rng);
    if (load)
    {
      options.load = args::get(load);
      options.train = false;
    }
    else
    {
      if (ntrees) options.ntrees = args::get(ntrees);
      if (f) options.f = args::get(f);
      if (n) options.n = args::get(n);
      if (gini) options.metric = sel::gini;
      else if (entropy) options.metric = sel::entropy;
      else if (error) options.metric = sel::error;
      if (cv) options.cv = args::get(cv);
      if (json) options.save = args::get(json);
      if (dot) options.dot_prefix = args::get(dot);
      if (dataset) options.dataset = args::get(dataset);
    }
  }
  catch (args::Help)
  {
    std::cout << parser;
    std::exit(0);
  }
  catch (args::ParseError e)
  {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    std::exit(1);
  }
  catch (args::ValidationError e)
  {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    std::exit(1);
  }
  return options;
}

void print_options(const Options& options)
{
  std::cout << "Verbose: " << options.verbose << std::endl;
  std::cout << "Train: " << (options.train? "true" : "false") << std::endl;
  if (options.train)
  {
    std::string metric;
    if (options.metric == sel::gini) metric = "gini";
    else if (options.metric == sel::entropy) metric = "entropy";
    else metric = "error";
    std::cout << "ntrees: " << options.ntrees << std::endl;
    std::cout << "f (<= 0 means sqrt of #attributes): " << options.f << std::endl;
    std::cout << "n: " << options.n << std::endl;
    std::cout << "Metric: " << metric << std::endl;
    std::cout << "cv: " << options.cv << std::endl;
    std::cout << "save to json: " << options.save << std::endl;
    std::cout << "dot prefix: " << options.dot_prefix << std::endl;
    std::cout << "rng seed: " << options.rng << std::endl;
  }
  else
  {
    std::cout << "load from json: " << options.load << std::endl;
  }
  std::cout << "data set: " << options.dataset << std::endl;
}

double evaluate_forest(const sel::RandomForest& forest, const sel::Dataframe& test)
{
  double acc = 0;
  std::vector<std::string> guesses;
  forest.classify(test, guesses);
  int target_idx = test.get_target_idx();
  for (int idx = 0; idx < test.get_nrecords(); ++idx)
  {
    std::string truth = test.get_instance(idx).get(target_idx).get_category();
    if (truth == guesses[idx]) acc += 1;
  }
  acc /= test.get_nrecords();
  return acc;
}

void rank_features(const sel::RandomForest& forest)
{
  std::map<std::string,int> counts;
  forest.count_features(counts);
  double n_stumps = 0;
  std::vector<std::pair<double,std::string>> sorted;
  for (const auto& entry : counts) n_stumps += entry.second;
  for (const auto& entry : counts)
  {
    sorted.emplace_back(entry.second/n_stumps, entry.first);
  }
  std::greater<std::pair<double,std::string>> cmp;
  std::sort(sorted.begin(), sorted.end(), cmp);
  std::cout << "Feature ranking:" << std::endl;
  for (const auto& feature : sorted)
  {
    std::cout << feature.second << " (" << feature.first << ')' << std::endl;
  }
}

double mean(const std::vector<double>& v)
{
  double sum = 0;
  for (double x : v) sum += x;
  return sum/v.size();
}

double stdev(const std::vector<double>& v)
{
  double avg = mean(v);
  double sumsq = 0;
  for (double x : v) sumsq += x*x;
  return std::sqrt(sumsq/v.size() - avg*avg);
}

