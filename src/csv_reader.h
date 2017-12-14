/**
 * @author Alejandro Suarez Hernandez
 * @file csv_reader.h
 * Definition of a straightforward CSV reader.
 */

#ifndef CSV_READER_H
#define CSV_READER_H

#include "common.h"

#include <fstream>
#include <vector>

namespace sel
{

typedef std::vector<std::string> CsvRow;
class CsvReader;

/** 
 * @brief Provides a very simple interface to parse CSV files.
 */
class CsvReader
{
  public:

    /** 
     * @brief The reader must be constructed for a particular filename.
     * 
     * @param filename File to be read.
     * @param delim Delimiter char (default, ",").
     */
    CsvReader(const std::string& filename, char delim=',');

    /** 
     * @brief Main method to read rows from the CSV file line by line.
     * 
     * @param row The new row (if any) will be stored here.
     * 
     * @return Whether the method could read a new row or, on the contrary,
     * has encountered the EOF symbol.
     */
    bool next_row(CsvRow& row);

  private:

    std::ifstream in_;
    char delim_;
};


} /* end namespace rise */

#endif

