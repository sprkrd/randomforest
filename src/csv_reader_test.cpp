#include "csv_reader.h"
#include <iostream>

#ifndef DATA_PATH
#define DATA_PATH "../Data/"
#endif

int main(int argc, char* argv[])
{
  if (argc != 2)
  {
    std::cerr << "Usage: " << argv[0] << " filename\n";
    return -1;
  }

  int columns = -1;

  try
  {
    std::string path = std::string(DATA_PATH) + argv[1] + '/' + argv[1] + ".data";
    sel::CsvReader reader(path);
    sel::CsvRow row;
    int nrecords = 0;
    while (reader.next_row(row))
    {
      nrecords += 1;
      if (columns == -1) columns = row.size();
      else if (columns != (int)row.size()) std::cerr << "Warning! number of columns not consistent\n";
      std::cout << sel::container2str(row) << std::endl;
    }
    std::cout << "#Records: " << nrecords << "; #Attributes: " << columns << std::endl;
  }
  catch (sel::SelException& ex)
  {
    std::cerr << ex.what() << '\n';
  }
}
