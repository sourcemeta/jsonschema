#ifndef SOURCEMETA_CORE_JSON_VALUE_H_
#define SOURCEMETA_CORE_JSON_VALUE_H_

#ifndef SOURCEMETA_CORE_JSON_EXPORT
#include <sourcemeta/core/json_export.h>
#endif

#include <sourcemeta/core/json_array.h>
#include <sourcemeta/core/json_hash.h>
#include <sourcemeta/core/json_object.h>

#include <sourcemeta/core/numeric.h>

#include <algorithm>        // std::any_of
#include <bitset>           // std::bitset
#include <cassert>          // assert
#include <cstddef>          // std::size_t
#include <cstdint>          // std::int64_t, std::uint8_t
#include <functional>       // std::less, std::reference_wrapper, std::function
#include <initializer_list> // std::initializer_list
#include <memory>           // std::allocator
#include <set>              // std::set
#include <sstream>          // std::basic_istringstream
#include <string>           // std::basic_string, std::char_traits
#include <string_view>      // std::basic_string_view
#include <type_traits>      // std::enable_if_t, std::is_same_v
#include <utility>          // std::pair

namespace sourcemeta::core {

/// @ingroup json
class SOURCEMETA_CORE_JSON_EXPORT JSON {
public:
  /// The character type used by the JSON document.
  using Char = char;
  /// The character traits used by the JSON document.
  using CharTraits = std::char_traits<Char>;
  /// The integer type used by the JSON document.
  using Integer = std::int64_t;
  /// The real type used by the JSON document.
  using Real = double;
  /// The allocator used by the JSON document.
  template <typename T> using Allocator = std::allocator<T>;
  /// The string type used by the JSON document.
  using String = std::basic_string<Char, CharTraits, Allocator<Char>>;
  /// The array type used by the JSON document.
  using Array = JSONArray<JSON>;
  /// The object type used by the JSON document.
  using Object = JSONObject<String, JSON, PropertyHashJSON<JSON::String>>;
  /// The parsing phase of a JSON document.
  enum class ParsePhase : std::uint8_t { Pre, Post };

  // The enumeration indexes must stay in sync with the internal variant
  /// The different types of a JSON instance.
  enum class Type : std::uint8_t {
    Null = 0,
    Boolean = 1,
    Integer = 2,
    Real = 3,
    String = 4,
    Array = 5,
    Object = 6,
    Decimal = 7
  };

  /// A set of types
  using TypeSet = std::bitset<8>;

  /// An optional callback that can be passed to parsing functions to obtain
  /// metadata during the parsing process. Each subdocument will emit 2 events:
  /// a "pre" and a "post". When parsing object and arrays, during the "pre"
  /// event, the value corresponds to the property name or index, respectively.
  using ParseCallback =
      std::function<void(const ParsePhase phase, const Type type,
                         const std::uint64_t line, const std::uint64_t column,
                         // TODO: Instead of taking a JSON value, we should take
                         // either an index or a string view to the key
                         const JSON &value)>;
  /// A comparison function between object property keys.
  /// See https://en.cppreference.com/w/cpp/named_req/Compare
  using KeyComparison = std::function<bool(const String &, const String &)>;

  /*
    Constructors
   */

  /// This constructor creates a JSON document from an integer type. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  ///
  /// const sourcemeta::core::JSON my_integer{4};
  /// ```
  explicit JSON(const std::int64_t value);

  /// This constructor creates a JSON document from an integer type. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  ///
  /// const sourcemeta::core::JSON my_integer{4};
  /// ```
  explicit JSON(const std::size_t value);

  /// This constructor creates a JSON document from an integer type. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  ///
  /// const sourcemeta::core::JSON my_integer{4};
  /// ```
  explicit JSON(const int value);

  // On some systems, `std::int64_t` might be equal to `long`
  template <typename T = std::int64_t>
  explicit JSON(const long value)
    requires(!std::is_same_v<T, std::int64_t>)
      : current_type{Type::Integer} {
    this->data_integer = value;
  }

  /// This constructor creates a JSON document from an real number type. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  ///
  /// const sourcemeta::core::JSON my_real{3.14};
  /// ```
  explicit JSON(const double value);

  /// This constructor creates a JSON document from an real number type. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  ///
  /// const sourcemeta::core::JSON my_real{3.14};
  /// ```
  explicit JSON(const float value);

  /// This constructor creates a JSON document from a boolean type. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  ///
  /// const sourcemeta::core::JSON my_boolean{true};
  /// ```
  explicit JSON(const bool value);

  /// This constructor creates a JSON document from a null type. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  ///
  /// const sourcemeta::core::JSON my_null{nullptr};
  /// ```
  explicit JSON(const std::nullptr_t);

  /// This constructor creates a JSON document from a string type. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  ///
  /// const sourcemeta::core::JSON my_string{"foo"};
  /// ```
  explicit JSON(const String &value);

  /// This constructor creates a JSON document from a string type. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  ///
  /// const sourcemeta::core::JSON my_string{"foo"};
  /// ```
  explicit JSON(const std::basic_string_view<Char, CharTraits> &value);

  /// This constructor creates a JSON document from a string type. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  ///
  /// const sourcemeta::core::JSON my_string{"foo"};
  /// ```
  explicit JSON(const Char *const value);

  /// This constructor creates a JSON array from a set of other JSON documents.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON my_array{
  ///   sourcemeta::core::JSON{1},
  ///   sourcemeta::core::JSON{2},
  ///   sourcemeta::core::JSON{3}};
  ///
  /// assert(my_array.is_array());
  /// ```
  explicit JSON(std::initializer_list<JSON> values);

  /// A copy constructor for the array type.
  explicit JSON(const Array &value);

  /// This constructor creates a JSON object from a pair of other JSON
  /// documents. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON my_object{
  ///   {"foo", sourcemeta::core::JSON{1}},
  ///   {"bar", sourcemeta::core::JSON{1}},
  ///   {"baz", sourcemeta::core::JSON{1}}};
  ///
  /// assert(my_object.is_object());
  /// ```
  explicit JSON(std::initializer_list<typename Object::pair_value_type> values);

  /// A copy constructor for the object type.
  explicit JSON(const Object &value);

  /// A copy constructor for the decimal type.
  explicit JSON(const Decimal &value);

  /// A move constructor for the decimal type.
  explicit JSON(Decimal &&value);

  /// Misc constructors
  JSON(const JSON &);
  JSON(JSON &&) noexcept;
  auto operator=(const JSON &) -> JSON &;
  auto operator=(JSON &&) noexcept -> JSON &;

  /// Destructor
  ~JSON();

  /// This function creates an empty JSON array. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document =
  ///   sourcemeta::core::JSON::make_array();
  /// assert(document.is_array());
  /// assert(document.empty());
  /// ```
  ///
  /// This function is particularly handy for programatically constructing
  /// arrays.
  static auto make_array() -> JSON;

  /// This function creates an empty JSON object. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document =
  ///   sourcemeta::core::JSON::make_object();
  /// assert(document.is_object());
  /// assert(document.empty());
  /// ```
  ///
  /// This function is particularly handy for programatically constructing
  /// objects.
  static auto make_object() -> JSON;

  /// This function calculates the logical size of a string according to the
  /// JSON specification. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON::String value{"foo"};
  /// assert(sourcemeta::core::JSON::size(value) == 3);
  /// ```
  static auto size(const String &value) noexcept -> std::size_t;

  /*
   * Operators
   */

  auto operator<(const JSON &other) const noexcept -> bool;
  auto operator<=(const JSON &other) const noexcept -> bool;
  auto operator>(const JSON &other) const noexcept -> bool;
  auto operator>=(const JSON &other) const noexcept -> bool;
  auto operator==(const JSON &other) const noexcept -> bool;
  auto operator!=(const JSON &) const noexcept -> bool = default;

  /// Add two numeric JSON instances and get a new instance with the result. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON left{5};
  /// const sourcemeta::core::JSON right{3};
  /// const sourcemeta::core::JSON result{left + right};
  /// assert(result.is_integer());
  /// assert(document.to_integer() == 8);
  /// ```
  auto operator+(const JSON &other) const -> JSON;

  /// Substract two numeric JSON instances and get a new instance with the
  /// result. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON left{5};
  /// const sourcemeta::core::JSON right{3};
  /// const sourcemeta::core::JSON result{left - right};
  /// assert(result.is_integer());
  /// assert(document.to_integer() == 2);
  /// ```
  auto operator-(const JSON &other) const -> JSON;

  /// This operator adds a numeric JSON instance to another numeric JSON
  /// instance. For example, a numeric JSON instance 3.2 can be added to a
  /// numeric JSON instance 5 as follows:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON document{5};
  /// const sourcemeta::core::JSON additive{3.2};
  /// document += additive;
  /// assert(document.is_real());
  /// assert(document.to_real() == 8.2);
  /// ```
  auto operator+=(const JSON &additive) -> JSON &;

  /// This operator substracts a numeric JSON instance from another numeric JSON
  /// instance. For example, a numeric JSON instance 3.2 can be substracted from
  /// a numeric JSON instance 5 as follows:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON document{5};
  /// const sourcemeta::core::JSON substractive{3.2};
  /// document -= substractive;
  /// assert(document.is_real());
  /// assert(document.to_real() == 1.8);
  /// ```
  auto operator-=(const JSON &substractive) -> JSON &;

  /*
   * Type checking
   */

  /// Check if the input JSON document is a boolean. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document{true};
  /// assert(document.is_boolean());
  /// ```
  [[nodiscard]] auto is_boolean() const noexcept -> bool;

  /// Check if the input JSON document is null. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document{nullptr};
  /// assert(document.is_null());
  /// ```
  [[nodiscard]] auto is_null() const noexcept -> bool;

  /// Check if the input JSON document is an integer. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document{5};
  /// assert(document.is_integer());
  /// ```
  [[nodiscard]] auto is_integer() const noexcept -> bool;

  /// Check if the input JSON document is a real type. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document{3.14};
  /// assert(document.is_real());
  /// ```
  [[nodiscard]] auto is_real() const noexcept -> bool;

  /// Check if the input JSON document is an integer, a real number that
  /// represents an integer, or an integer decimal. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document{5.0};
  /// assert(document.is_integral());
  /// ```
  [[nodiscard]] auto is_integral() const noexcept -> bool;

  /// Check if the input JSON document is either an integer or a real type. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON real{3.14};
  /// const sourcemeta::core::JSON integer{5};
  /// assert(real.is_number());
  /// assert(integer.is_number());
  /// ```
  [[nodiscard]] auto is_number() const noexcept -> bool;

  /// Check if the input JSON document is either a positive integer or a
  /// positive real number. Zero is considered to be positive. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON real{3.14};
  /// const sourcemeta::core::JSON integer{-5};
  /// assert(real.is_positive());
  /// assert(!integer.is_positive());
  /// ```
  [[nodiscard]] auto is_positive() const noexcept -> bool;

  /// Check if the input JSON document is a string. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document{"foo"};
  /// assert(document.is_string());
  /// ```
  [[nodiscard]] auto is_string() const noexcept -> bool;

  /// Check if the input JSON document is an array. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON
  /// document=sourcemeta::core::parse_json("[ 1, 2, 3 ]");
  /// assert(document.is_array());
  /// ```
  [[nodiscard]] auto is_array() const noexcept -> bool;

  /// Check if the input JSON document is an object. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON
  /// document=sourcemeta::core::parse_json("{ \"foo\": 1 }");
  /// assert(document.is_object());
  /// ```
  [[nodiscard]] auto is_object() const noexcept -> bool;

  /// Check if the input JSON document is an arbitrary precision decimal value.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Decimal value{1234567890};
  /// const sourcemeta::core::JSON document{value};
  /// assert(document.is_decimal());
  /// ```
  [[nodiscard]] auto is_decimal() const noexcept -> bool;

  /// Get the type of the JSON document. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document{true};
  /// assert(document.type() == sourcemeta::core::JSON::Type::Boolean);
  /// ```
  [[nodiscard]] auto type() const noexcept -> Type;

  /*
   * Type conversion
   */

  /// Convert a JSON instance into a boolean value. The result of this method is
  /// undefined unless the JSON instance holds a boolean value. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document{true};
  /// assert(document.is_boolean());
  /// assert(document.to_boolean());
  /// ```
  [[nodiscard]] auto to_boolean() const noexcept -> bool;

  /// Convert a JSON instance into a signed integer value. The result of this
  /// method is undefined unless the JSON instance holds an integer value. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document{5};
  /// assert(document.is_integer());
  /// assert(document.to_integer() == 5);
  /// ```
  [[nodiscard]] auto to_integer() const noexcept -> Integer;

  /// Convert a JSON instance into an IEEE 64-bit floating-point value. The
  /// result of this method is undefined unless the JSON instance holds a real
  /// value. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document{3.14};
  /// assert(document.is_real());
  /// assert(document.to_real() == 3.14);
  /// ```
  [[nodiscard]] auto to_real() const noexcept -> Real;

  /// Convert a JSON instance into a decimal value. The result of this method
  /// is undefined unless the JSON instance holds a decimal value. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Decimal value{1234567890};
  /// const sourcemeta::core::JSON document{value};
  /// assert(document.is_decimal());
  /// assert(document.to_decimal().to_int64() == 1234567890);
  /// ```
  [[nodiscard]] auto to_decimal() const noexcept -> const Decimal &;

  /// Convert a JSON instance into a standard string value. The result of this
  /// method is undefined unless the JSON instance holds a string value. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document{"foo"};
  /// assert(document.is_string());
  /// assert(document.to_string() == "foo");
  /// ```
  [[nodiscard]] auto to_string() const noexcept -> const String &;

  /// Get a standard input string stream from a JSON string. The result of this
  /// method is undefined unless the JSON instance holds a string value. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document{"foo"};
  /// assert(document.is_string());
  /// auto stream{document.to_stringstream()};
  /// assert(stream.get() == 'f');
  /// ```
  [[nodiscard]] auto to_stringstream() const
      -> std::basic_istringstream<Char, CharTraits, Allocator<Char>>;

  /// Get the JSON document as an array instance. This is convenient
  /// for using constant iterators on the array. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <algorithm>
  /// #include <iostream>
  ///
  /// const sourcemeta::core::JSON document =
  ///   sourcemeta::core::parse_json("[ 1, 2, 3 ]");
  /// std::for_each(document.as_array().cbegin(),
  ///               document.as_array().cend(),
  ///               [](const auto &element) {
  ///                 std::cout << "Element: "
  ///                           << element.to_integer()
  ///                           << "\n";
  ///               });
  /// ```
  [[nodiscard]] auto as_array() const noexcept -> const Array &;

  /// Get the JSON document as an array instance. This is convenient
  /// for using mutable iterators on the array. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <algorithm>
  /// #include <iostream>
  ///
  /// const sourcemeta::core::JSON document =
  ///   sourcemeta::core::parse_json("[ 1, 2, 3 ]");
  /// std::sort(document.as_array().begin(), document.as_array().end());
  /// ```
  [[nodiscard]] auto as_array() noexcept -> Array &;

  /// Get the JSON document as an object instance. This is convenient
  /// for using constant iterators on the object. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <algorithm>
  /// #include <iostream>
  ///
  /// sourcemeta::core::JSON document =
  ///   sourcemeta::core::JSON::make_object();
  /// document.assign("foo", sourcemeta::core::JSON{1});
  /// document.assign("bar", sourcemeta::core::JSON{2});
  /// document.assign("baz", sourcemeta::core::JSON{3});
  ///
  /// std::for_each(document.as_object().cbegin(),
  ///               document.as_object().cend(),
  ///               [](const auto &pair) {
  ///                 std::cout << "Value: "
  ///                           << pair.second.to_integer()
  ///                           << "\n";
  ///               });
  /// ```
  [[nodiscard]] auto as_object() noexcept -> Object &;

  /// Get the JSON document as an object instance. This is convenient
  /// for using mutable iterators on the object. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <algorithm>
  /// #include <iostream>
  ///
  /// sourcemeta::core::JSON document =
  ///   sourcemeta::core::JSON::make_object();
  /// document.assign("foo", sourcemeta::core::JSON{1});
  /// document.assign("bar", sourcemeta::core::JSON{2});
  /// document.assign("baz", sourcemeta::core::JSON{3});
  ///
  /// for (auto &[key, value] : document.as_object()) {
  ///   value += sourcemeta::core::JSON{1};
  /// }
  /// ```
  [[nodiscard]] auto as_object() const noexcept -> const Object &;

  /// Get the JSON numeric document as a real number if it is not one already.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document{5};
  /// assert(document.as_real() == 5.0);
  /// ```
  [[nodiscard]] auto as_real() const noexcept -> Real;

  /// Get the JSON numeric document as an integer number if it is not one
  /// already. If the number is a real number, truncation will take place. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document{5.3};
  /// assert(document.as_integer() == 5);
  /// ```
  [[nodiscard]] auto as_integer() const noexcept -> Integer;

  /*
   * Getters
   */

  /// This method retrieves a element by its index. If the input JSON instance
  /// is an object, a property that corresponds to the stringified integer will
  /// be accessed.
  ///
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON my_array =
  ///   sourcemeta::core::parse_json("[ 1, 2 ]");
  /// assert(my_array.at(1).to_integer() == 2);
  ///
  /// const sourcemeta::core::JSON my_object =
  ///   sourcemeta::core::parse_json("{ \"1\": "foo" }");
  /// assert(my_array.at(1).to_string() == "foo");
  /// ```
  [[nodiscard]] auto at(const typename Array::size_type index) const
      -> const JSON &;

  /// This method retrieves a element by its index. If the input JSON instance
  /// is an object, a property that corresponds to the stringified integer will
  /// be accessed.
  ///
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON my_array =
  ///   sourcemeta::core::parse_json("[ 1, 2 ]");
  /// assert(my_array.at(1).to_integer() == 2);
  ///
  /// sourcemeta::core::JSON my_object =
  ///   sourcemeta::core::parse_json("{ \"1\": "foo" }");
  /// assert(my_array.at(1).to_string() == "foo");
  /// ```
  [[nodiscard]] auto at(const typename Array::size_type index) -> JSON &;

  /// This method retrieves an object element.
  ///
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON my_object =
  ///   sourcemeta::core::parse_json("{ \"foo\": 1, \"bar\": 2 }");
  /// assert(my_object.at("bar").to_integer() == 2);
  /// ```
  [[nodiscard]] auto at(const String &key) const -> const JSON &;

  /// This method retrieves an object element given a pre-calculated property
  /// hash.
  ///
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON my_object =
  ///   sourcemeta::core::parse_json("{ \"foo\": 1, \"bar\": 2 }");
  /// assert(my_object.at("bar",
  ///  my_object.as_object().hash("bar")).to_integer() == 2);
  /// ```
  [[nodiscard]] auto at(const String &key,
                        const typename Object::hash_type hash) const
      -> const JSON &;

  /// This method retrieves an object element.
  ///
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON my_object =
  ///   sourcemeta::core::parse_json("{ \"foo\": 1, \"bar\": 2 }");
  /// assert(my_object.at("bar").to_integer() == 2);
  /// ```
  [[nodiscard]] auto at(const String &key) -> JSON &;

  /// This method retrieves an object element given a pre-calculated property
  /// hash.
  ///
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON my_object =
  ///   sourcemeta::core::parse_json("{ \"foo\": 1, \"bar\": 2 }");
  /// assert(my_object.at("bar",
  ///   my_object.as_object().hash("bar")).to_integer() == 2);
  /// ```
  [[nodiscard]] auto at(const String &key,
                        const typename Object::hash_type hash) -> JSON &;

  /// This method retrieves an object property or a user provided value if such
  /// property is not defined.
  ///
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON my_object =
  ///   sourcemeta::core::parse_json("{ \"foo\": 1, \"bar\": 2 }");
  /// const sourcemeta::core::JSON default_value{3};
  /// assert(my_object.at_or("baz", default_value).to_integer() == 3);
  /// ```
  [[nodiscard]] auto at_or(const String &key, const JSON &otherwise) const
      -> const JSON &;

  /// This method retrieves an object property given a pre-calculated property
  /// hash, or a user provided value if such property is not defined.
  ///
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON my_object =
  ///   sourcemeta::core::parse_json("{ \"foo\": 1, \"bar\": 2 }");
  /// const sourcemeta::core::JSON default_value{3};
  /// assert(my_object.at_or("foo",
  ///   my_object.as_object().hash("foo"),
  ///   default_value).to_integer() == 1);
  /// ```
  [[nodiscard]] auto at_or(const String &key,
                           const typename Object::hash_type hash,
                           const JSON &otherwise) const -> const JSON &;

  // Constant reference parameters can accept xvalues which will be destructed
  // after the call. When the function returns such a parameter also as constant
  // reference, then the returned reference can be used after the object it
  // refers to has been destroyed.
  // https://clang.llvm.org/extra/clang-tidy/checks/bugprone/return-const-ref-from-parameter.html
  // This overload avoids mis-uses of retuning const reference parameter as
  // constant reference.
  [[nodiscard]] auto at_or(const String &key,
                           const typename Object::hash_type hash,
                           JSON &&otherwise) const -> const JSON & = delete;

  /// This method retrieves a reference to the first element of a JSON array.
  /// This method is undefined if the input JSON instance is an empty array. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON document =
  ///   sourcemeta::core::parse_json("[ 1, 2, 3 ]");
  /// assert(document.front().to_integer() == 1);
  /// ```
  [[nodiscard]] auto front() -> JSON &;

  /// This method retrieves a reference to the first element of a JSON array.
  /// This method is undefined if the input JSON instance is an empty array. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document =
  ///   sourcemeta::core::parse_json("[ 1, 2, 3 ]");
  /// assert(document.front().to_integer() == 1);
  /// ```
  [[nodiscard]] auto front() const -> const JSON &;

  /// This method retrieves a reference to the last element of a JSON array.
  /// This method is undefined if the input JSON instance is an empty array. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON document =
  ///   sourcemeta::core::parse_json("[ 1, 2, 3 ]");
  /// assert(document.back().to_integer() == 3);
  /// ```
  [[nodiscard]] auto back() -> JSON &;

  /// This method retrieves a reference to the last element of a JSON array.
  /// This method is undefined if the input JSON instance is an empty array. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document =
  ///   sourcemeta::core::parse_json("[ 1, 2, 3 ]");
  /// assert(document.back().to_integer() == 3);
  /// ```
  [[nodiscard]] auto back() const -> const JSON &;

  /*
   * Read operations
   */

  /// If the input JSON instance is an object, return its number of pairs. If
  /// the input JSON instance is an array, return its number of elements. If the
  /// input JSON instance is a string, return its logical length.
  ///
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON my_object =
  ///   sourcemeta::core::parse_json("{ \"foo\": 1 }");
  /// const sourcemeta::core::JSON my_array =
  ///   sourcemeta::core::parse_json("[ 1, 2 ]");
  /// const sourcemeta::core::JSON my_string{"foo"};
  ///
  /// assert(my_object.size() == 1);
  /// assert(my_array.size() == 2);
  /// assert(my_string.size() == 3);
  /// ```
  [[nodiscard]] auto size() const -> std::size_t;

  /// If the input JSON instance is a string, return its logical length.
  ///
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON my_string{"foo"};
  /// assert(my_string.string_size() == 3);
  /// ```
  [[nodiscard]] auto string_size() const -> std::size_t;

  /// If the input JSON instance is an array, return its number of elements.
  ///
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON my_array =
  ///   sourcemeta::core::parse_json("[ 1, 2 ]");
  /// assert(my_array.array_size() == 2);
  /// ```
  [[nodiscard]] auto array_size() const -> std::size_t;

  /// If the input JSON instance is an object, return its number of pairs.
  ///
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON my_object =
  ///   sourcemeta::core::parse_json("{ \"foo\": 1 }");
  /// assert(my_object.object_size() == 1);
  /// ```
  [[nodiscard]] auto object_size() const -> std::size_t;

  /// If the input JSON instance is string, input JSON instance is a string,
  /// return its number of bytes. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON my_string{
  ///   sourcemeta::core::parse_json("\"\\uD83D\\uDCA9\"")};
  /// assert(my_string.size() == 2);
  /// ```
  [[nodiscard]] auto byte_size() const -> std::size_t;

  /// Estimate the byte size occupied by the given parsed JSON instance (not its
  /// stringified representation). Keep in mind that as the method name implies,
  /// this is just a rough estimate. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON value =
  ///   sourcemeta::core::parse_json("{ \"foo\": 1 }");
  ///
  /// // Byte length of "foo" (3) + byte length of 1 (8)
  /// assert(value.estimated_byte_size() == 11);
  /// ```
  [[nodiscard]] auto estimated_byte_size() const -> std::uint64_t;

  /// Produce a simple hash for the JSON value. Note the hash is fast to produce
  /// but might have a higher chance of collisions. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON value_1 =
  ///   sourcemeta::core::parse_json("{ \"foo\": 1 }");
  ///
  /// const sourcemeta::core::JSON value_2 =
  ///   sourcemeta::core::parse_json("{ \"foo\": 1 }");
  ///
  /// assert(value_1.fast_hash() == value_2.fast_hash());
  /// ```
  [[nodiscard]] auto fast_hash() const -> std::uint64_t;

  /// Check whether a numeric instance is divisible by another numeric instance.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON dividend{6};
  /// const sourcemeta::core::JSON divisor{1.5};
  ///
  /// assert(dividend.divisible_by(divisor));
  /// ```
  [[nodiscard]] auto divisible_by(const JSON &divisor) const -> bool;

  /// A convenience method to check whether the input JSON document is an empty
  /// object, empty array or empty string.
  ///
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON my_object =
  ///   sourcemeta::core::parse_json("{}");
  /// const sourcemeta::core::JSON my_array =
  ///   sourcemeta::core::parse_json("[]");
  /// const sourcemeta::core::JSON my_string{""};
  ///
  /// assert(my_object.empty());
  /// assert(my_array.empty());
  /// assert(my_string.empty());
  /// ```
  [[nodiscard]] auto empty() const -> bool;

  /// This method checks whether an input JSON object defines a specific key
  /// and returns the value if it does. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document =
  ///   sourcemeta::core::parse_json("{ \"foo\": 1 }");
  /// EXPECT_TRUE(document.is_object());
  /// const auto result = document.try_at("foo");
  /// EXPECT_TRUE(result);
  /// EXPECT_EQ(result->to_integer(), 1);
  [[nodiscard]] auto try_at(const String &key) const -> const JSON *;

  /// This method checks, given a pre-calculated hash, whether an input JSON
  /// object defines a specific key and returns the value if it does. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document =
  ///   sourcemeta::core::parse_json("{ \"foo\": 1 }");
  /// EXPECT_TRUE(document.is_object());
  /// const auto result = document.try_at("foo",
  ///   document.as_object().hash("foo"));
  /// EXPECT_TRUE(result);
  /// EXPECT_EQ(result->to_integer(), 1);
  [[nodiscard]] auto try_at(const String &key,
                            const typename Object::hash_type hash) const
      -> const JSON *;

  /// This method checks whether an input JSON object defines a specific key.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document =
  ///   sourcemeta::core::parse_json("{ \"foo\": 1 }");
  /// assert(document.defines("foo"));
  /// assert(!document.defines("bar"));
  /// ```
  [[nodiscard]] auto defines(const String &key) const -> bool;

  /// This method checks whether an input JSON object defines a specific key
  /// given a pre-calculated property hash. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document =
  ///   sourcemeta::core::parse_json("{ \"foo\": 1 }");
  /// assert(document.defines("foo",
  ///   document.as_object().hash("foo")));
  /// assert(document.defines("bar",
  ///   document.as_object().hash("bar")));
  /// ```
  [[nodiscard]] auto defines(const String &key,
                             const typename Object::hash_type hash) const
      -> bool;

  /// This method checks whether an input JSON object defines a specific integer
  /// key. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document =
  ///   sourcemeta::core::parse_json("{ \"0\": 1 }");
  /// assert(document.defines(0));
  /// assert(!document.defines(1));
  /// ```
  [[nodiscard]] auto defines(const typename Array::size_type index) const
      -> bool;

  /// This method checks whether an input JSON object defines at least one given
  /// key.
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  /// #include <string>
  /// #include <vector>
  ///
  /// const sourcemeta::core::JSON document =
  ///   sourcemeta::core::parse_json("{ \"foo\": true, \"bar\": false }");
  ///
  /// const std::vector<std::string> keys{"foo", "qux"};
  /// assert(document.defines_any(keys.cbegin(), keys.cend()));
  /// ```
  template <typename Iterator>
  [[nodiscard]] auto defines_any(Iterator begin, Iterator end) const -> bool {
    assert(this->is_object());
    return std::any_of(begin, end,
                       [this](const auto &key) { return this->defines(key); });
  }

  /// This method checks whether an input JSON object defines at least one given
  /// key.
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document =
  ///   sourcemeta::core::parse_json("{ \"foo\": true, \"bar\": false }");
  ///
  /// assert(document.defines_any({ "foo", "qux" }));
  /// ```
  [[nodiscard]] auto defines_any(std::initializer_list<String> keys) const
      -> bool;

  /// This method checks if an JSON array contains a given JSON instance. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document =
  ///   sourcemeta::core::parse_json("[ 1, 2, 3 ]");
  /// assert(document.contains(sourcemeta::core::JSON{2}));
  /// assert(!document.contains(sourcemeta::core::JSON{4}));
  /// ```
  [[nodiscard]] auto contains(const JSON &element) const -> bool;

  /// This method checks if an JSON string contains a given string. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document{"foo bar baz"};
  /// assert(document.contains("bar"));
  /// assert(!document.contains("baz"));
  /// ```
  [[nodiscard]] auto contains(const String &input) const -> bool;

  /// This method checks if an JSON string contains a given character. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document{"foo"};
  /// assert(document.contains('f'));
  /// assert(!document.contains('b'));
  /// ```
  [[nodiscard]] auto contains(const String::value_type input) const -> bool;

  /// This method checks if an JSON array does not contain duplicated items. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document =
  ///   sourcemeta::core::parse_json("[ 1, 2, 3 ]");
  /// assert(document.unique());
  /// ```
  [[nodiscard]] auto unique() const -> bool;

  /*
   * Write operations
   */

  /// This method inserts a new element to the end of the given array. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON document =
  ///   sourcemeta::core::parse_json("[ 1, 2, 3 ]");
  /// const sourcemeta::core::JSON value{4};
  /// document.push_back(value);
  /// assert(document.size() == 4);
  /// assert(document.back().to_integer() == 4);
  /// ```
  auto push_back(const JSON &value) -> void;

  /// This method inserts a new element to the end of the given array. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON document =
  ///   sourcemeta::core::parse_json("[ 1, 2, 3 ]");
  /// document.push_back(sourcemeta::core::JSON{4});
  /// assert(document.size() == 4);
  /// assert(document.back().to_integer() == 4);
  /// ```
  auto push_back(JSON &&value) -> void;

  /// This method inserts a new element to the end of the given array if an
  /// equal element is not already present in the array. The return value is a
  /// pair consisting of a reference to the element in question and whether the
  /// element was inserted or not. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON document =
  ///   sourcemeta::core::parse_json("[ 1, 2, 3 ]");
  /// const sourcemeta::core::JSON new_element{3};
  /// const auto result{document.push_back_if_unique(new_element)};
  /// assert(result.first.get().to_integer() == 3);
  /// assert(!result.second);
  /// ```
  auto push_back_if_unique(const JSON &value)
      -> std::pair<std::reference_wrapper<const JSON>, bool>;

  /// This method inserts a new element to the end of the given array if an
  /// equal element is not already present in the array. The return value is a
  /// pair consisting of a reference to the element in question and whether the
  /// element was inserted or not. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  /// #include <utility>
  ///
  /// sourcemeta::core::JSON document =
  ///   sourcemeta::core::parse_json("[ 1, 2, 3 ]");
  /// sourcemeta::core::JSON new_element{3};
  /// const auto result{document.push_back_if_unique(std::move(new_element)};
  /// assert(result.first.get().to_integer() == 3);
  /// assert(!result.second);
  /// ```
  auto push_back_if_unique(JSON &&value)
      -> std::pair<std::reference_wrapper<const JSON>, bool>;

  /// This method sets or updates an object key. For example, an object can be
  /// updated to contain a new `bar` boolean member as follows:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON document =
  ///   sourcemeta::core::parse_json("{ \"foo\": true }");
  /// const sourcemeta::core::JSON value{false};
  /// document.assign("bar", value);
  /// assert(document.defines("foo"));
  /// assert(document.defines("bar"));
  /// ```
  auto assign(const String &key, const JSON &value) -> void;

  /// This method sets or updates an object key. For example, an object can be
  /// updated to contain a new `bar` boolean member as follows:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON document =
  ///   sourcemeta::core::parse_json("{ \"foo\": true }");
  /// document.assign("bar", sourcemeta::core::JSON{false});
  /// assert(document.defines("foo"));
  /// assert(document.defines("bar"));
  /// ```
  auto assign(const String &key, JSON &&value) -> void;

  /// This method sets or updates an object key. However, it will try to insert
  /// the key _before_ the given one if possible.
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON document =
  ///   sourcemeta::core::parse_json("{ \"foo\": true }");
  /// const sourcemeta::core::JSON value{false};
  /// document.try_assign_before("bar", value, "foo");
  /// assert(document.as_object().cbegin()->first == "bar");
  /// ```
  auto try_assign_before(const String &key, const JSON &value,
                         const String &other) -> void;

  /// This method sets an object key if it is not already defined. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON document =
  ///   sourcemeta::core::parse_json("{ \"foo\": true }");
  ///
  /// const sourcemeta::core::JSON value_1{1};
  /// const sourcemeta::core::JSON value_2{2};
  ///
  /// document.assign_if_missing("foo", value_1);
  /// document.assign_if_missing("bar", value_2);
  ///
  /// assert(document.defines("foo"));
  /// assert(document.at("foo").is_boolean());
  /// assert(document.defines("bar"));
  /// assert(document.at("bar").is_integer());
  /// ```
  auto assign_if_missing(const String &key, const JSON &value) -> void;

  /// This method sets an object key if it is not already defined. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON document =
  ///   sourcemeta::core::parse_json("{ \"foo\": true }");
  ///
  /// document.assign_if_missing("foo", sourcemeta::core::JSON{1});
  /// document.assign_if_missing("bar", sourcemeta::core::JSON{2});
  ///
  /// assert(document.defines("foo"));
  /// assert(document.at("foo").is_boolean());
  /// assert(document.defines("bar"));
  /// assert(document.at("bar").is_integer());
  /// ```
  auto assign_if_missing(const String &key, JSON &&value) -> void;

  /// This method sets an object key, assuming the key does not already exist.
  /// If the key already exists, behavior is undefined. This variant is faster
  /// than `assign` when building objects with keys known to be unique. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON document = sourcemeta::core::JSON::make_object();
  /// document.assign_assume_new("foo", sourcemeta::core::JSON{1});
  /// assert(document.defines("foo"));
  /// assert(document.at("foo").to_integer() == 1);
  /// ```
  auto assign_assume_new(const String &key, JSON &&value) -> void;

  /// This method sets an object key, assuming the key does not already exist.
  /// If the key already exists, behavior is undefined. This variant is faster
  /// than `assign` when building objects with keys known to be unique, and
  /// allows moving the key. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON document = sourcemeta::core::JSON::make_object();
  /// std::string key{"foo"};
  /// document.assign_assume_new(std::move(key), sourcemeta::core::JSON{1});
  /// assert(document.defines("foo"));
  /// assert(document.at("foo").to_integer() == 1);
  /// ```
  auto assign_assume_new(String &&key, JSON &&value) -> void;

  /// This method deletes an object key. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON document =
  ///   sourcemeta::core::parse_json("{ \"foo\": true }");
  /// document.erase("foo");
  /// assert(!document.defines("foo"));
  /// ```
  auto erase(const String &key) -> typename Object::size_type;

  /// This method deletes a set of object keys. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  /// #include <string>
  /// #include <vector>
  ///
  /// sourcemeta::core::JSON document =
  ///   sourcemeta::core::JSON::make_object();
  /// document.assign("foo", sourcemeta::core::JSON{true});
  /// document.assign("bar", sourcemeta::core::JSON{false});
  /// document.assign("baz", sourcemeta::core::JSON{true});
  ///
  /// const std::vector<std::string> keys{"foo", "bar"};
  /// document.erase_keys(keys.cbegin(), keys.cend());
  ///
  /// assert(!document.defines("foo"));
  /// assert(!document.defines("bar"));
  /// assert(document.defines("baz"));
  /// ```
  template <typename Iterator>
  auto erase_keys(Iterator first, Iterator last) -> void {
    assert(this->is_object());
    for (auto iterator = first; iterator != last; ++iterator) {
      this->data_object.erase(*iterator);
    }
  }

  /// This method deletes a set of object keys. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON document =
  ///   sourcemeta::core::JSON::make_object();
  /// document.assign("foo", sourcemeta::core::JSON{true});
  /// document.assign("bar", sourcemeta::core::JSON{false});
  /// document.assign("baz", sourcemeta::core::JSON{true});
  ///
  /// document.erase_keys({ "foo", "bar" });
  ///
  /// assert(!document.defines("foo"));
  /// assert(!document.defines("bar"));
  /// assert(document.defines("baz"));
  /// ```
  auto erase_keys(std::initializer_list<String> keys) -> void;

  /// This method deletes an array element using an iterator. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  /// #include <iterator>
  ///
  /// sourcemeta::core::JSON array =
  ///   sourcemeta::core::parse_json("[ 1, 2, 3 ]");
  /// array.erase(std::next(array.begin()));
  /// assert(array.size(), 2);
  /// assert(array.at(0), 1);
  /// assert(array.at(1), 3);
  /// ```
  auto erase(typename Array::const_iterator position) ->
      typename Array::iterator;

  /// This method deletes a set of array elements using iterators. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  /// #include <iterator>
  ///
  /// sourcemeta::core::JSON array =
  ///   sourcemeta::core::parse_json("[ 1, 2, 3 ]");
  /// array.erase(std::next(array.begin()), array.end());
  /// assert(array.size(), 1);
  /// assert(array.at(0), 1);
  /// ```
  auto erase(typename Array::const_iterator first,
             typename Array::const_iterator last) -> typename Array::iterator;

  /// This method deletes a set of array elements given a predicate. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON array =
  ///   sourcemeta::core::parse_json("[ 1, 2, 3 ]");
  /// array.erase_if(array,
  ///   [](const auto &item) { return item.to_integer() % 2 == 0; });
  /// assert(array.size(), 2);
  /// assert(array.at(0), 1);
  /// assert(array.at(1), 3);
  /// ```
  auto erase_if(const std::function<bool(const JSON &)> &predicate) -> void;

  /// This method deletes all members of an object or all elements of an array,
  /// leaving them empty. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON my_object =
  ///   sourcemeta::core::parse_json("{ \"foo\": true }");
  /// sourcemeta::core::JSON my_array =
  ///   sourcemeta::core::parse_json("[ 1, 2, 3 ]");
  /// my_object.clear();
  /// my_array.clear();
  /// assert(my_object.empty());
  /// assert(my_array.empty());
  /// ```
  auto clear() -> void;

  /// This method deletes all members of an object except for the JSON keys
  /// declares as the second argument. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  /// #include <string>
  /// #include <vector>
  ///
  /// sourcemeta::core::JSON document =
  ///   sourcemeta::core::JSON::make_object();
  /// document.assign("foo", sourcemeta::core::JSON{true});
  /// document.assign("bar", sourcemeta::core::JSON{false});
  /// document.assign("baz", sourcemeta::core::JSON{true});
  ///
  /// const std::vector<std::string> keys{"foo"};
  /// document.clear_except(keys.cbegin(), keys.cend());
  ///
  /// assert(document.defines("foo"));
  /// assert(!document.defines("bar"));
  /// assert(!document.defines("baz"));
  /// ```
  template <typename Iterator>
  auto clear_except(Iterator first, Iterator last) -> void {
    assert(this->is_object());
    std::set<String, std::less<>, Allocator<String>> whitelist;
    for (auto iterator = first; iterator != last; ++iterator) {
      whitelist.insert(*iterator);
    }

    std::set<String, std::less<>, Allocator<String>> blacklist;
    for (const auto &pair : this->as_object()) {
      if (!whitelist.contains(pair.first)) {
        blacklist.insert(pair.first);
      }
    }

    this->erase_keys(blacklist.cbegin(), blacklist.cend());
  }

  /// This method deletes all members of an object except for the JSON keys
  /// declares as the second argument. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON document =
  ///   sourcemeta::core::JSON::make_object();
  /// document.assign("foo", sourcemeta::core::JSON{true});
  /// document.assign("bar", sourcemeta::core::JSON{false});
  /// document.assign("baz", sourcemeta::core::JSON{true});
  ///
  /// document.clear_except({ "foo" });
  ///
  /// assert(document.defines("foo"));
  /// assert(!document.defines("bar"));
  /// assert(!document.defines("baz"));
  /// ```
  auto clear_except(std::initializer_list<String> keys) -> void;

  /// This method assigns every property of another object into the current
  /// object. Overriding existing properties if they are already defined. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON document =
  ///   sourcemeta::core::JSON::make_object();
  /// document.assign("foo", sourcemeta::core::JSON{1});
  /// document.assign("bar", sourcemeta::core::JSON{2});
  ///
  /// sourcemeta::core::JSON other =
  ///   sourcemeta::core::JSON::make_object();
  /// other.assign("bar", sourcemeta::core::JSON{1});
  /// other.assign("baz", sourcemeta::core::JSON{2});
  ///
  /// document.merge(other.as_object());
  ///
  /// assert(document.size() == 3);
  ///
  /// assert(document.at("foo").to_integer() == 1);
  /// assert(document.at("bar").to_integer() == 1);
  /// assert(document.at("baz").to_integer() == 2);
  /// ```
  auto merge(const JSON::Object &other) -> void;

  /// Return a trimmed version of the string. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document{" \r\t  Hello World\n\v   \f"};
  /// assert(document.trim() == "Hello World");
  /// ```
  [[nodiscard]] auto trim() const -> JSON::String;

  /// Trim the string in-place. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON document{" \r\t  Hello World\n\v   \f"};
  /// document.trim();
  /// assert(document.to_string() == "Hello World");
  /// ```
  auto trim() -> const JSON::String &;

  /// Check if the string has no leading or trailing whitespace. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON trimmed{"Hello World"};
  /// assert(trimmed.is_trimmed());
  ///
  /// const sourcemeta::core::JSON untrimmed{" Hello World "};
  /// assert(!untrimmed.is_trimmed());
  /// ```
  [[nodiscard]] auto is_trimmed() const noexcept -> bool;

  /// Reorder the properties of an object by sorting keys according to a
  /// comparator function. The object is modified in-place. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON document =
  ///   sourcemeta::core::JSON::make_object();
  /// document.assign("zebra", sourcemeta::core::JSON{1});
  /// document.assign("apple", sourcemeta::core::JSON{2});
  /// document.assign("banana", sourcemeta::core::JSON{3});
  ///
  /// document.reorder([](const auto &left, const auto &right) {
  ///   return left < right;
  /// });
  ///
  /// auto iterator = document.as_object().cbegin();
  /// assert(iterator->first == "apple");
  /// ++iterator;
  /// assert(iterator->first == "banana");
  /// ++iterator;
  /// assert(iterator->first == "zebra");
  /// ```
  auto reorder(const KeyComparison &compare) -> void;

  /*
   * Transform operations
   */

  /// This method moves a JSON property value from one property name to another,
  /// potentially deleting the destination property name if it already exists.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON document =
  ///   sourcemeta::core::parse_json("{ \"foo\": true }");
  /// document.rename("foo", "bar");
  ///
  /// assert(!document.defines("foo"));
  /// assert(document.defines("bar"));
  /// assert(document.at("bar").is_boolean());
  /// assert(document.at("bar").to_boolean());
  /// ```
  auto rename(const JSON::String &key, JSON::String &&to) -> void;

  /// This method sets a value to another JSON value by copying it. For example,
  /// the member of a JSON document can be transformed from a boolean to an
  /// integer as follows:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON document =
  ///   sourcemeta::core::parse_json("{ \"foo\": true }");
  /// const sourcemeta::core::JSON value{2};
  /// document.at("foo").into(value);
  /// assert(document.at("foo").is_integer());
  /// ```
  auto into(const JSON &other) -> void;

  /// This method sets a value to another JSON value. For example, the member of
  /// a JSON document can be transformed from a boolean to an integer as
  /// follows:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON document =
  ///   sourcemeta::core::parse_json("{ \"foo\": true }");
  /// document.at("foo").into(sourcemeta::core::JSON{2});
  /// assert(document.at("foo").is_integer());
  /// ```
  auto into(JSON &&other) noexcept -> void;

  /// This method converts an existing JSON instance into an empty array. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON document{true};
  /// assert(document.is_boolean());
  /// document.into_array();
  /// assert(document.is_array());
  /// assert(document.empty());
  /// ```
  auto into_array() -> void;

  /// This method converts an existing JSON instance into an empty object. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::JSON document{true};
  /// assert(document.is_boolean());
  /// document.into_object();
  /// assert(document.is_object());
  /// assert(document.empty());
  /// ```
  auto into_object() -> void;

private:
  Type current_type = Type::Null;

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  union {
    bool data_boolean;
    Integer data_integer;
    Real data_real;
    String data_string;
    Array data_array;
    Object data_object;
    // Move Decimal to the heap to reduce the size of the JSON class.
    // Dealing with arbitrary precision numbers is not common, so we pay the
    // indirection cost only when needed.
    Decimal *data_decimal;
  };
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
  auto maybe_destruct_union() -> void;
};

} // namespace sourcemeta::core

#endif
