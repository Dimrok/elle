#include <elle/assert.hh>

#include <reactor/FDStream.hh>
#include <reactor/scheduler.hh>

namespace reactor
{
  FDStream::EOF::EOF()
    : elle::Error("end of file")
  {}

  FDStream::StreamBuffer::StreamBuffer(boost::asio::io_service& service,
                                       Handle handle)
    : _stream(service, handle)
    , _handle(handle)
  {}

  elle::PlainStreamBuffer::Size
  FDStream::StreamBuffer::read(char* buffer, elle::PlainStreamBuffer::Size size)
  {
    elle::Size read = 0;
    boost::system::error_code error;
    reactor::Barrier done;
    this->_stream.async_read_some(
      boost::asio::buffer(buffer, size),
      [&] (boost::system::error_code const& e, std::size_t s)
      {
        error = e;
        read = s;
        done.open();
      });
    reactor::wait(done);
    if (error == boost::asio::error::eof)
      throw EOF();
    else if (error)
      throw elle::Error(
        elle::sprintf("unable to read from %s: %s",
                      this->_handle, error.message()));
    else
      return read;
  }

  void
  FDStream::StreamBuffer::write(char* buffer, Size size)
  {
    elle::unreachable();
  }

  FDStream::FDStream(boost::asio::io_service& service, Handle handle)
    : elle::IOStream(new StreamBuffer(service, handle))
  {}

  FDStream::FDStream(Handle handle)
    : FDStream(reactor::scheduler().io_service(), handle)
  {}
}