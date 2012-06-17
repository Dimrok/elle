
#include <elle/system/Platform.hh>

#include <elle/concurrency/Callback.hh>
#include <elle/concurrency/Program.hh>
#include <elle/concurrency/Scheduler.hh>

#include <elle/standalone/Maid.hh>
#include <elle/standalone/Report.hh>

#include <elle/idiom/Close.hh>
# include <reactor/scheduler.hh>
# include <reactor/signal.hh>
# include <reactor/thread.hh>
#include <elle/idiom/Open.hh>

#include <hole/Hole.hh>

#include <elle/log.hh>

ELLE_LOG_TRACE_COMPONENT("Infinit.Elle.Program");

namespace elle
{
  namespace concurrency
  {

//
// ---------- globals ---------------------------------------------------------
//

    ///
    /// this variable represents the program.
    ///
    Program*                    program = NULL;

//
// ---------- static methods --------------------------------------------------
//

    ///
    /// this method initializes the program.
    ///
    Status              Program::Initialize()
    {
      // allocate a new program.
      program = new Program;

      return Status::Ok;
    }

    ///
    /// this method cleans the program.
    ///
    Status              Program::Clean()
    {
      // delete the program.
      if (program != NULL)
        delete program;

      return Status::Ok;
    }

    ///
    /// this method sets up the program for startup.
    ///
    Status              Program::Setup()
    {
#if defined(INFINIT_LINUX) || defined(INFINIT_MACOSX)
      // set the signal handlers.
      ::signal(SIGINT, &Program::Exception);
      ::signal(SIGQUIT, &Program::Exception);
      ::signal(SIGABRT, &Program::Exception);
      ::signal(SIGTERM, &Program::Exception);
#elif defined(INFINIT_WINDOWS)
      // XXX to implement
#else
# error "unsupported platform"
#endif

      return Status::Ok;
    }

    void
    Program::Exit()
    {
      ELLE_LOG_TRACE_SCOPE("Exit");

      _exit.signal();
    }

    void
    Program::Launch()
    {
      ELLE_LOG_TRACE_SCOPE("Launch");

      elle::concurrency::scheduler().current()->wait(_exit);
    }

    ///
    /// this method is triggered whenever a POSIX signal is received.
    ///
    Void                Program::Exception(int                  signal)
    {
      ELLE_LOG_TRACE_SCOPE("Exception");

      // stop the program depending on the signal.
      switch (signal)
        {
#if defined(INFINIT_LINUX) || defined(INFINIT_MACOSX)
        case SIGQUIT:
#elif defined(INFINIT_WINDOWS)
	// nothing
#else
# error "unsupported platform"
#endif
        case SIGINT:
        case SIGABRT:
        case SIGTERM:
          {
            // exit properly by finishing processing the last events.
            Program::Exit();

            break;
          }
        }
    }

//
// ---------- constructors & destructors --------------------------------------
//

    ///
    /// default constructor.
    ///
    Program::Program()
    {}

    ///
    /// destructor.
    ///
    Program::~Program()
    {}

    reactor::Signal Program::_exit;
  }
}
