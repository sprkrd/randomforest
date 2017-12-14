/**
 * @author Alejandro Suarez Hernandez
 * @file common.h
 * Contains the declaration of basic types and methods.
 */

#ifndef COMMON_H
#define COMMON_H

#include <cmath>
#include <exception>
#include <unordered_map>
#include <memory>
#include <limits>
#include <sstream>
#include <string>

/** 
 * @brief Main namespace used to hold all the methods and classes of this
 * project.
 */
namespace sel
{

const double inf = std::numeric_limits<double>::infinity();

class Stringifiable;
class SelException;
class Value;
class Number;
class Category;
class Instance;

/** 
 * @brief Abstract type to identify classes that implement the to_str method.
 *
 * Classes that implement this "interface" can be easily output to a stream.
 */
class Stringifiable
{
  public:

    /** 
     * @return string representing the object
     */
    virtual std::string to_str() const = 0;

    virtual ~Stringifiable() {}
};

/** 
 * @brief Very simple exception that just stores a custom message with the
 * reason of the error.
 */
class SelException
{
  public:

    /** 
     * @param msg Error message
     */
    SelException(const std::string& msg) : msg_(msg) {}

    virtual const char* what() const throw() { return msg_.c_str(); }

  private:

    std::string msg_;

};

/** 
 * @brief Represents attribute values (either number or categories).
 */
class Value : public Stringifiable
{
  public:

    typedef std::unique_ptr<const Value> Ptr;

    /** 
     * @brief Factory-like method to parse a string and return either a Number
     * or a Category.
     * 
     * @param value String to parse. A "?" represents a missing value.
     * @param numeric Whether the string represents a numeric input (if false,
     * it is a category).
     * 
     * @return A pointer to a new Value.
     */
    static Ptr create(const std::string& value, bool numeric);



    /** 
     * @return Whether this is a missing value (?).
     */
    virtual bool is_missing() const = 0;

    /** 
     * @return Whether this is a number.
     */
    virtual bool is_numeric() const = 0;

    /** 
     * @return This value in numeric form (double).
     *
     * @throw SelException if this value is not a number.
     */
    virtual double get_number() const { throw SelException("Not a number"); }

    /** 
     * @return This value in category form (string).
     *
     * @throw SelException if this value is not a category.
     */
    virtual const std::string& get_category() const
    {
      throw SelException("Not a category");
    }

    /** 
     * @brief LT operator, implemented for sorting values of the same type.
     *
     * IMPORTANT! Does not support heterogeneous comparisons-
     * 
     * @param other
     * 
     * @return true if this value is smaller (lexicographically or numerically)
     * than other given value).
     *
     * @throw SelException if comparing heterogeneous values.
     */
    virtual bool operator<(const Value& other) const = 0;

    /** 
     * @brief Standard equality test.
     *
     * IMPORTANT! Does not support heterogeneous comparisons.
     * 
     * @param other
     * 
     * @return true if values share the same number or category.
     *
     * @throw SelException when trying to compare heterogeneous values.
     */
    virtual bool operator==(const Value& other) const = 0;

    /** 
     * @brief Just performs !(*this == other).
     *
     * IMPORTANT! Does not support heterogeneous values.
     * 
     * @param other
     * 
     * @return true if values are not equal.
     *
     * @throw SelException when trying to compare heterogeneous values.
     */
    bool operator!=(const Value& other) const
    {
      return not(*this == other);
    }

    /** 
     * @brief Copies this object and returns a pointer to the copy.
     * 
     * @return pointer to a copy of this object.
     */
    virtual Ptr clone() const = 0;

    virtual ~Value() { }

};

class Number : public Value
{
  public:

    /** 
     * @brief Empty (missing) value constructor.
     */
    Number() : number_(std::numeric_limits<double>::quiet_NaN()) { }

    /** 
     * @param number Number stored by this object.
     */
    Number(double number) : number_(number) { }

    virtual bool is_missing() const override { return std::isnan(number_); }

    virtual bool is_numeric() const override { return true; }

    virtual double get_number() const override { return number_; }

    virtual bool operator<(const Value& other) const
    {
      return number_ < other.get_number();
    }

    virtual bool operator==(const Value& other) const
    {
      return number_ == other.get_number();
    }

    virtual Value::Ptr clone() const { return Value::Ptr(new Number(*this)); }

    virtual std::string to_str() const override;

  private:

    double number_;
};

class Category : public Value
{
  public:

    /** 
     * @brief Empty (missing) value constructor.
     */
    Category() : category_("?") {}

    /** 
     * @param category Category stored by this object.
     */
    Category(const std::string& category) : category_(category) { }

    virtual bool is_missing() const override { return category_ == "?"; }

    virtual bool is_numeric() const override { return false; }

    virtual const std::string& get_category() const override { return category_; }

    virtual bool operator<(const Value& other) const
    {
      return category_ < other.get_category();
    }

    virtual bool operator==(const Value& other) const
    {
      return category_ == other.get_category();
    }

    virtual Value::Ptr clone() const { return Value::Ptr(new Category(*this)); }

    virtual std::string to_str() const override { return category_; };

  private:

    std::string category_;
  
};


// Templatized classes/methods

/** 
 * @brief Helper templatized method to output the content of a Container (e.g.
 * a vector) as a string.
 *
 * The method assumes that there is an implementation of the insert (<<)
 * operator between an stream an the contained objects.
 * 
 * @param ctn Container object
 * @param open Sequence used to mark the beginning of the container (default:
 * the left squared bracket "[").
 * @param close Sequence used to mark the end of the container (default: the
 * right squared bracket "]").
 * @param sep separator between consecutive elements (by default, the comma)
 * 
 * @return string representation of the container.
 */
template <class Container>
std::string container2str(const Container& ctn, const std::string& open="[",
                          const std::string& close="]",
                          const std::string& sep=",")
{
  std::ostringstream oss;
  oss << open;
  bool first = true;
  for (const auto& obj : ctn)
  {
    if (not first) oss << ',';
    oss << obj;
    first = false;
  }
  oss << close;
  return oss.str();
}

/** 
 * @brief Method to put directly the string representation of a Stringifiable
 * into a stream.
 * 
 * @param os Output stream.
 * @param strable Input stringifiable.
 * 
 * @return os after the insertion.
 */
std::ostream& operator<<(std::ostream& os, const Stringifiable& strable);

} /* end namespace sel*/

#endif
