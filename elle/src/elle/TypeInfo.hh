#ifndef ELLE_TYPE_INFO_HH
# define ELLE_TYPE_INFO_HH

# include <string>
# include <typeinfo>

# include <elle/attribute.hh>

namespace elle
{
  class TypeInfo
  {
  public:
    std::string
    name() const;
    bool
    operator ==(TypeInfo const& rhs) const;
    bool
    operator <(TypeInfo const& rhs) const;

  private:
    TypeInfo(std::type_info const* info);
    template <typename T>
    friend TypeInfo type_info();
    template <typename T>
    friend TypeInfo type_info(T const&);
    ELLE_ATTRIBUTE(std::type_info const*, info)
  };

  std::ostream&
  operator << (std::ostream& s, TypeInfo const& ti);

  template <typename T>
  TypeInfo
  type_info();

  template <typename T>
  TypeInfo
  type_info(T const& v);
}

# include <elle/TypeInfo.hxx>

#endif