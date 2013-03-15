#ifndef INFINIT_CRYPTOGRAPHY_INPUT_HH
# define INFINIT_CRYPTOGRAPHY_INPUT_HH

# include <cryptography/fwd.hh>

# include <elle/types.hh>
# include <elle/attribute.hh>
# include <elle/operator.hh>
# include <elle/Buffer.hh>
# include <elle/Printable.hh>

# include <boost/noncopyable.hpp>

namespace infinit
{
  namespace cryptography
  {
    /// Represent an input for an cryptographic operation. This class
    /// can be constructed from a weak buffer which is basically a wrapper
    /// around a memory address and size.
    class Input:
      public elle::Printable,
      private boost::noncopyable
    {
      /*-------------.
      | Construction |
      `-------------*/
    public:
      explicit
      Input(elle::WeakBuffer const& buffer);

      /*----------.
      | Operators |
      `----------*/
    public:
      elle::Boolean
      operator ==(Input const& other) const;
      elle::Boolean
      operator <(Input const& other) const;
      elle::Boolean
      operator <=(Input const& other) const;
      ELLE_OPERATOR_NEQ(Input);
      ELLE_OPERATOR_GT(Input);
      ELLE_OPERATOR_GTE(Input);
      ELLE_OPERATOR_NO_ASSIGNMENT(Input);

      elle::Boolean
      operator ==(Output const& other) const;
      elle::Boolean
      operator <(Output const& other) const;
      elle::Boolean
      operator <=(Output const& other) const;
      ELLE_OPERATOR_NEQ(Output);
      ELLE_OPERATOR_GT(Output);
      ELLE_OPERATOR_GTE(Output);

      /*-----------.
      | Interfaces |
      `-----------*/
    public:
      // printable
      virtual
      void
      print(std::ostream& stream) const;

      /*-----------.
      | Attributes |
      `-----------*/
    private:
      ELLE_ATTRIBUTE_R(elle::WeakBuffer, buffer);
    };
  }
}

#endif
