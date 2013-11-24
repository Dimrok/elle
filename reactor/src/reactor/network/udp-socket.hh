#ifndef INFINIT_REACTOR_NETWORK_UDP_SOCKET_HH
# define INFINIT_REACTOR_NETWORK_UDP_SOCKET_HH

# include <reactor/asio.hh>
# include <reactor/network/socket.hh>
# include <reactor/signal.hh>

namespace reactor
{
  namespace network
  {
    class UDPSocket: public PlainSocket<boost::asio::ip::udp::socket>
    {
      /*---------.
      | Typedefs |
      `---------*/
      public:
        typedef PlainSocket<boost::asio::ip::udp::socket> Super;
        typedef boost::asio::ip::udp::resolver AsioResolver;

      /*-------------.
      | Construction |
      `-------------*/
      public:
        UDPSocket(Scheduler& sched,
                  const std::string& hostname,
                  const std::string& port);
        UDPSocket(Scheduler& sched,
                  const std::string& hostname,
                  int port);
        // UDPSocket(Scheduler& sched,
        //           int local_port,
        //           const std::string& hostname,
        //           int port);
        // UDPSocket(Scheduler& sched,
        //           int native_handle);
        // UDPSocket(Scheduler& sched);
        virtual ~UDPSocket();

      /*--------------.
      | Configuration |
      `--------------*/
      public:
        void bind(boost::asio::ip::udp::endpoint const& endpoint);

      /*-----.
      | Read |
      `-----*/
      public:
        virtual Size read_some(Buffer buffer,
                               DurationOpt timeout = DurationOpt());

        Size receive_from(Buffer buffer,
                          boost::asio::ip::udp::endpoint &endpoint,
                          DurationOpt timeout = DurationOpt());

      /*------.
      | Write |
      `------*/
      public:
        virtual void write(elle::ConstWeakBuffer buffer);
        void send_to(Buffer buffer,
                     EndPoint endpoint);

      /*----------------.
      | Pretty printing |
      `----------------*/
      public:
        void print(std::ostream& s) const;
    };
  }
}

#endif
