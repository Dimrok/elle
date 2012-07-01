#ifndef ELLE_STANDALONE_MORGUE_HH
# define ELLE_STANDALONE_MORGUE_HH

# include <elle/idiom/Close.hh>
#  include <list>
#  include <reactor/signal.hh>
#  include <reactor/thread.hh>
# include <elle/idiom/Open.hh>

#include <elle/types.hh>

# include <elle/types.hh>

# include <elle/radix/Entity.hh>
# include <elle/radix/Meta.hh>

namespace elle
{
  namespace standalone
  {

//
// ---------- classes ---------------------------------------------------------
//

    ///
    /// the morgue functionality provides a way for delaying the given
    /// objects' deletion.
    ///
    /// this functionality is especially useful for avoiding statements
    /// such as:
    ///
    ///   delete this;
    ///
    /// as such operations may induce side effects.
    ///
    class Morgue:
      public Entity
    {
    public:
      //
      // constants
      //
      const static Natural32                    Frequency;

      //
      // types
      //
      typedef std::list<Meta*>                          Container;
      typedef typename Container::iterator              Iterator;
      typedef typename Container::const_iterator        Scoutor;

      //
      // static methods
      //
      static Status     Initialize();
      static Status     Clean();

      static Status     Instance(Morgue*&);

      //
      // static attributes
      //
      static Morgue*    Current;

      //
      // constructors & destructors
      //
      Morgue();
      ~Morgue();

      //
      // methods
      //
      template <typename T>
      Status            Register(T*);

      //
      // callbacks
      //
      Status            Bury();
      void              GraveDigger();

      //
      // interfaces
      //

      // dumpable
      Status            Dump(const Natural32 = 0) const;

      //
      // attributes
      //
      Container         container;
      reactor::Signal   _corpses_available;
      reactor::Thread   _deleter;
    };

  }
}

//
// ---------- templates -------------------------------------------------------
//

#include <elle/standalone/Morgue.hxx>

#endif
