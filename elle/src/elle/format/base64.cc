#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>
#include <boost/archive/iterators/transform_width.hpp>

#include <elle/Exception.hh>
#include <elle/assert.hh>
#include <elle/format/base64.hh>
#include <elle/log.hh>

ELLE_LOG_COMPONENT("elle.format.base64")

namespace elle
{
  namespace format
  {
    namespace base64
    {
      using namespace boost::archive::iterators;

      Buffer
      encode(WeakBuffer input)
      {
        // XXX: pre-allocate the right size.
        // size_t size = (signed(input.size()) + 2) / 3 * 4;
        Buffer res;
        IOStream stream(new OutputStreamBuffer(res));
        Stream base64_stream(stream);
        base64_stream.write(reinterpret_cast<char*>(input.contents()),
                            input.size());
        return res;
      }

      static
      size_t
      _decoded_size(WeakBuffer encoded)
      {
        size_t size = encoded.size();
        int padding = 0;
        if (size >= 1 && encoded.mutable_contents()[size - 1] == '=')
          ++padding;
        if (size >= 2 && encoded.mutable_contents()[size - 2] == '=')
          ++padding;
        return size / 4 * 3 - padding;
      }

      Buffer
      decode(WeakBuffer input)
      {
        size_t size = _decoded_size(input);
        Buffer res(size);
        IOStream stream(new InputStreamBuffer<WeakBuffer>(input));
        Stream base64_stream(stream);
        base64_stream.read(reinterpret_cast<char*>(res.mutable_contents()),
                           size);
        ELLE_ASSERT_EQ(base64_stream.gcount(), signed(size));
        ELLE_ASSERT(
          (base64_stream.read(reinterpret_cast<char*>(
                                res.mutable_contents()), 1),
           base64_stream.gcount() == 0));

        return res;
      }

      typedef base64_from_binary<transform_width<const char *, 6, 8>> encoder;
      template <typename SourceIterator, typename DestinationIterator>
      void
      _stream_encode(SourceIterator begin,
                     SourceIterator end,
                     DestinationIterator output)
      {
        std::copy(encoder(begin), encoder(end), output);
      }

      typedef transform_width<binary_from_base64<char const *>, 8, 6> decoder;
      template <typename SourceIterator, typename DestinationIterator>
      void
      _stream_decode(SourceIterator begin,
                     SourceIterator end,
                     DestinationIterator output)
      {
        std::copy(decoder(begin), decoder(end), output);
      }

      class StreamBuffer:
        public elle::StreamBuffer,
        public elle::Printable
      {
      public:
        StreamBuffer(Stream& stream):
          _stream(stream),
          _remaining_write(0),
          _remaining_read(0)
        {}
        ELLE_ATTRIBUTE(Stream&, stream);

      protected:
        virtual
        WeakBuffer
        read_buffer()
        {
          static auto const size = sizeof(this->_buffer_read) / 3 * 4;
          ELLE_TRACE_SCOPE("%s: decode up to %s bytes", *this, size);
          char buffer[size];
          size_t read;
          ELLE_DEBUG("%s: read up to %s bytes from the backend", *this, size)
          {
            this->_stream._underlying.read(buffer, size);
            read = this->_stream._underlying.gcount();
            ELLE_DUMP("%s: got %s bytes", *this, read);
            if (read > 0)
              ELLE_DUMP("%s: encoded data: %s", *this,
                        elle::WeakBuffer(buffer, read));
          }
          this->_remaining_read = read % 4;
          read = read - this->_remaining_read;
          size_t decoded_size = read / 4 * 3;
          if (read > 0)
          {
            ELLE_DEBUG_SCOPE("%s: decode %s bytes", *this, read);
            bool end = buffer[read - 1] == '=';
            if (end)
            {
              read -= 4;
              decoded_size -= 3;
            }
            _stream_decode(buffer, buffer + read, this->_buffer_read);
            if (end)
            {
              ELLE_DEBUG_SCOPE("%s: decode last 4 bytes with padding", *this);
              ELLE_DUMP("%s: remaining bytes: \"%s\"", *this,
                        std::string(buffer + read, 4));
              auto source = decoder(buffer + read);
              auto destination = this->_buffer_read + (read / 4 * 3);
              if (buffer[read + 2] != '=')
              {
                *(destination++) = *(source++);
                ++decoded_size;
              }
              *(destination++) = *(source++);
              ++decoded_size;
            }
            if (decoded_size > 0)
              ELLE_DUMP("%s: decoded data: %s", *this,
                        elle::WeakBuffer(this->_buffer_read, decoded_size));
          }
          if (this->_remaining_read > 0)
          {
            ELLE_DEBUG_SCOPE("%s: store %s remaining bytes",
                             *this, this->_remaining_read);
          }
          return WeakBuffer(this->_buffer_read, decoded_size);
        }

        virtual
        WeakBuffer
        write_buffer()
        {
          return WeakBuffer(this->_buffer_write + this->_remaining_write,
                            sizeof(_buffer_write) - this->_remaining_write);
        }

        virtual
        void
        flush(Size size)
        {
          size += this->_remaining_write;
          ELLE_TRACE_SCOPE("%s: encode %s bytes", *this, size);
          this->_remaining_write = size % 3;
          size = size - size % 3;
          if (size > 0)
          {
            ELLE_DEBUG_SCOPE("%s: encode %s bytes to the backend",
                             *this, size);
            _stream_encode(this->_buffer_write,
                           this->_buffer_write + size,
                           std::ostream_iterator<char>(_stream._underlying));
          }
          if (this->_remaining_write > 0)
          {
            ELLE_DEBUG_SCOPE("%s: push back %s bytes in the buffer",
                             *this, this->_remaining_write);
            memcpy(this->_buffer_write,
                   this->_buffer_write + size,
                   this->_remaining_write);
          }
        }

        void
        finalize()
        {
          if (this->_remaining_write > 0)
          {
            ELLE_DEBUG_SCOPE("%s: encode last %s remaining bytes",
                       *this, this->_remaining_write);
            ELLE_ASSERT_LT(this->_remaining_write, 3);
            _stream_encode(
              this->_buffer_write,
              this->_buffer_write + this->_remaining_write,
              std::ostream_iterator<char>(this->_stream._underlying));
            switch (this->_remaining_write)
            {
              case 1:
                this->_stream._underlying << "==";
                break;
              case 2:
                this->_stream._underlying << "=";
                break;
              default:
                elle::unreachable();
            }
          }
        }

        virtual
        void
        print(std::ostream& output) const
        {
          output << "base64::StreamBuffer";
        }

      private:
        friend class Stream;
        int _remaining_write;
        char _buffer_write[1 << 12];
        int _remaining_read;
        char _buffer_read[1 << 12];
      };

      Stream::Stream(std::iostream& underlying):
        IOStream(this->_buffer = new StreamBuffer(*this)),
        _underlying(underlying)
      {}

      Stream::~Stream()
      {
        ELLE_TRACE_SCOPE("%s: finalize encoding", *this->_buffer);
        this->_buffer->sync();
        this->_buffer->finalize();
      }
    }
  }
}
