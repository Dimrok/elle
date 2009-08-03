//
// ---------- header ----------------------------------------------------------
//
// project       elle
//
// license       infinit (c)
//
// file          /home/mycure/infinit/elle/crypto/Cipher.cc
//
// created       julien quintard   [mon feb  2 22:22:12 2009]
// updated       julien quintard   [mon aug  3 21:01:12 2009]
//

//
// ---------- includes --------------------------------------------------------
//

#include <elle/crypto/Cipher.hh>

namespace elle
{
  using namespace core;
  using namespace misc;
  using namespace archive;

  namespace crypto
  {

//
// ---------- entity ----------------------------------------------------------
//

    ///
    /// assign the given cipher by duplicating the attributes.
    ///
    Cipher&		Cipher::operator=(const Cipher&		element)
    {
      // self-check
      if (this == &element)
	return (*this);

      // recycle the cipher.
      if (this->Recycle<Cipher>(&element) == StatusError)
	yield("unable to recycle the cipher", *this);

      return (*this);
    }

    ///
    /// this method check if two ciphers match.
    ///
    Boolean		Cipher::operator==(const Cipher&	element) const
    {
      // compare the regions.
      return (this->region == element.region);
    }

    ///
    /// this method checks if two ciphers dis-match.
    ///
    Boolean		Cipher::operator!=(const Cipher&	element) const
    {
      return (!(*this == element));
    }

//
// ---------- dumpable --------------------------------------------------------
//

    ///
    /// this method dumps the cipher.
    ///
    Status		Cipher::Dump(const Natural32		margin)
    {
      String		alignment(margin, ' ');

      std::cout << alignment << "[Cipher]" << std::endl;

      if (this->region.Dump(margin + 2) == StatusError)
	escape("unable to dump the region");

      leave();
    }

//
// ---------- archivable ------------------------------------------------------
//

    ///
    /// this method serializes a cipher object.
    ///
    Status		Cipher::Serialize(Archive&		archive) const
    {
      // serialize the region.
      if (archive.Serialize(this->region) == StatusError)
	escape("unable to serialize the region");

      leave();
    }

    ///
    /// this method extracts a cipher object.
    ///
    Status		Cipher::Extract(Archive&		archive)
    {
      // extract the content.
      if (archive.Extract(this->region) == StatusError)
	escape("unable to extract the region");

      leave();
    }

  }
}
