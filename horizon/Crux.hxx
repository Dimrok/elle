//
// ---------- header ----------------------------------------------------------
//
// project       pig
//
// license       infinit
//
// file          /home/mycure/infinit/pig/Crux.hxx
//
// created       julien quintard   [wed jun  1 09:50:53 2011]
// updated       julien quintard   [wed jun  1 09:57:15 2011]
//

namespace pig
{

//
// ---------- template methods ------------------------------------------------
//

  ///
  /// this method discards modifications applied onto the given identified
  /// context.
  ///
  template <typename T>
  elle::Void		Crux::Release(T&		identifier)
  {
    // XXX etoile::wall::Object::Discard(identifier);
  }

  ///
  /// this method releases a set of identified contexts.
  ///
  template <typename T,
	    typename... TT>
  elle::Void		Crux::Release(T&		identifier,
				      TT&...		identifiers)
  {
    Crux::Release(identifier);
    Crux::Release(identifiers...);
  }

}
