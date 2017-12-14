#include "csv_reader.h"

#include <cctype>
#include <iostream>

namespace sel
{

namespace /* utils for internal usage */
{

// parsing is easier if we make sure that there are no whitespaces
void remove_blanks(const std::string& line, std::string& clean_line)
{
  clean_line.clear();
  clean_line.reserve(line.length());
  for (char c : line)
  {
    if (not isblank(c)) clean_line.push_back(c);
  }
}

void split(const std::string& line, CsvRow& row, char delim=',')
{
  std::size_t pos = 0;
  std::size_t next;
  while ((next = line.find_first_of(delim, pos)) != std::string::npos)
  {
    row.push_back(line.substr(pos, next-pos));
    pos = next + 1;
  }
  row.push_back(line.substr(pos));
}

} /* end anonymous namespace */

CsvReader::CsvReader(const std::string& filename, char delim)
  : in_(filename), delim_(delim)
{
  if (not in_)
  {
    throw SelException(std::string("Error opening ") + filename);
  }
}

bool CsvReader::next_row(CsvRow& row)
{
  row.clear();
  std::string line, clean_line;
  while (row.empty() and std::getline(in_, line))
  {
    remove_blanks(line, clean_line);
    if (not clean_line.empty()) split(clean_line, row, delim_);
  }
  return (bool) in_;
}

} /* end namespace rise */


