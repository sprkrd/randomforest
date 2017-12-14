#include "imputation.h"
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

    std::cout << table << std::endl;

  }
  catch (sel::SelException& ex)
  {
    std::cerr << ex.what() << '\n';
  }
}
