#include "dataframe.h"
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
    table.shuffle();
    std::cout << table << std::endl;

    //sel::View v1(table, 130);
    //std::cout << v1 << std::endl;

    //sel::View v2(v1, 0, 10);
    //v2.sort_by_column("sepal-width");
    //sel::Dataframe::Partition part;
    //v2.partition("A1", part);
    //std::cout << v2 << std::endl;

    //for (const auto& p : part)
    //{
      //std::cout << "Partition " << p.first << ':' << std::endl;
      //std::cout << *p.second << std::endl;
    //}
  }
  catch (sel::SelException& ex)
  {
    std::cerr << ex.what() << '\n';
  }
}

