#include <elle/log/CompositeLogger.hh>

#include <boost/algorithm/cxx11/any_of.hpp>

#include <elle/unreachable.hh>

namespace elle
{
  namespace log
  {
    CompositeLogger::CompositeLogger(Loggers l)
      : Logger("LOG")
      , _loggers{std::move(l)}
    {}

    CompositeLogger::CompositeLogger()
      : Logger("LOG")
    {}

    void
    CompositeLogger::_message(Message const& msg)
    {
      elle::unreachable();
    }

    void
    CompositeLogger::message(Level level,
                             Type type,
                             std::string const& component,
                             std::string const& message,
                             std::string const& file,
                             unsigned int line,
                             std::string const& function)
    {
      // We bounce to message and not _message so that each child logger can
      // have its own settings.
      // We must reproduce Send's behavior regarding indent and categories.
      for (auto& l: this->_loggers)
      {
        l->component_level(component); // for max size computation
        l->message(level, type,
                   component, message,
                   file, line, function);
      }
    }

    bool
    CompositeLogger::_component_is_active(std::string const& name, Level level)
    {
      return boost::algorithm::any_of(this->_loggers,
                                      [&name, level](auto& l)
                                      {
                                        return l->component_is_active(name, level);
                                      });
    }
  }
}
