#include <hole/Hole.hh>
#include <hole/Holeable.hh>
#include <hole/implementations/remote/Manifest.hh>
#include <hole/implementations/remote/Server.hh>

#include <elle/log.hh>
#include <elle/network/Network.hh>
#include <elle/concurrency/Callback.hh>

#include <reactor/network/tcp-server.hh>

#include <nucleus/proton/ImmutableBlock.hh>
#include <nucleus/proton/MutableBlock.hh>
#include <nucleus/proton/Address.hh>
#include <nucleus/proton/Network.hh>
#include <nucleus/proton/Version.hh>
#include <nucleus/neutron/Component.hh>
#include <nucleus/neutron/Object.hh>
#include <nucleus/neutron/Access.hh>
#include <nucleus/Derivable.hh>

#include <lune/Lune.hh>

#include <Infinit.hh>

ELLE_LOG_COMPONENT("Infinit.Hole.Remote.Server");

namespace hole
{
  namespace implementations
  {
    namespace remote
    {
      /*-------------.
      | Construction |
      `-------------*/

      Server::Server(int port)
        : _server(new reactor::network::TCPServer
                  (elle::concurrency::scheduler()))
        , _acceptor(new reactor::Thread(elle::concurrency::scheduler(),
                                        "Remote Server accept",
                                        boost::bind(&Server::_accept, this)))
      {
        _server->listen(port);
      }

      Server::~Server()
      {
        this->_acceptor->terminate_now();
        for (Customer* customer: _customers)
          delete customer;
        this->_customers.clear();
      }

      void
      Server::_accept()
      {
        while (true)
          {
            std::unique_ptr<reactor::network::Socket> socket(_server->accept());
            ELLE_LOG_TRACE_SCOPE("accept");
            auto result = this->_customers.insert(new Customer(*this, std::move(socket)));
            assert(result.second);
          }
      }

      void
      Server::_remove(Customer* customer)
      {
        auto iterator = _customers.find(customer);
        assert(iterator != _customers.end());
        delete *iterator;
        this->_customers.erase(iterator);
      }

      void
      Server::put(const nucleus::proton::Address& address,
                  const nucleus::proton::ImmutableBlock& block)
      {
        ELLE_LOG_TRACE_SCOPE("Put[Immutable]");

        // does the block already exist.
        if (block.Exist(Hole::Implementation->network,
                        address) == elle::Status::True)
          throw reactor::Exception(elle::concurrency::scheduler(), "this immutable block seems to already exist");

        // store the block.
        if (block.Store(Hole::Implementation->network,
                        address) == elle::Status::Error)
          throw reactor::Exception(elle::concurrency::scheduler(), "unable to store the block");
      }

      void
      Server::put(const nucleus::proton::Address& address,
                  const nucleus::proton::MutableBlock& block)
      {
        ELLE_LOG_TRACE_SCOPE("Put[Mutable]");

        // Validate the block, depending on its component.
        //
        // Indeed, the Object component requires as additional block for
        // being validated.
        switch (address.component)
          {
          case nucleus::neutron::ComponentObject:
            {
              const nucleus::neutron::Object* object =
                static_cast<const nucleus::neutron::Object*>(&block);
              assert(dynamic_cast<const nucleus::neutron::Object*>(
                       &block) != nullptr);

              // validate the object according to the presence of
              // a referenced access block.
              if (object->meta.access != nucleus::proton::Address::Null)
                {
                  std::unique_ptr<nucleus::proton::Block> block
                    (Hole::Pull(object->meta.access, nucleus::proton::Version::Last));
                  std::unique_ptr<nucleus::neutron::Access> access
                    (dynamic_cast<nucleus::neutron::Access*>(block.release()));
                  if (access == nullptr)
                    throw reactor::Exception(elle::concurrency::scheduler(),
                                             "expected an access block");

                  // validate the object, providing the
                  if (object->Validate(address, *access) == elle::Status::Error)
                    throw reactor::Exception(elle::concurrency::scheduler(), "unable to validate the object");
                }
              else
                {
                  // validate the object.
                  if (object->Validate(
                        address,
                        nucleus::neutron::Access::Null) == elle::Status::Error)
                    throw reactor::Exception(elle::concurrency::scheduler(), "unable to validate the object");
                }

              break;
            }
          default:
            {
              // validate the block through the common interface.
              if (block.Validate(address) == elle::Status::Error)
                throw reactor::Exception(elle::concurrency::scheduler(), "the block seems to be invalid");

              break;
            }
          case nucleus::neutron::ComponentUnknown:
            {
              throw reactor::Exception(elle::concurrency::scheduler(),
                                       elle::sprintf("unknown component '%u'",
                                                     address.component));
            }
          }

        // store the block.
        if (block.Store(Hole::Implementation->network,
                        address) == elle::Status::Error)
          throw reactor::Exception(elle::concurrency::scheduler(), "unable to store the block");
      }

      void
      Server::get(const nucleus::proton::Address& address,
                  nucleus::proton::ImmutableBlock& block)
      {
        ELLE_LOG_TRACE_SCOPE("Get[Immutable]");

        // does the block exist.
        if (block.Exist(Hole::Implementation->network,
                        address) == elle::Status::False)
          throw reactor::Exception(elle::concurrency::scheduler(), "the block does not seem to exist");

        // load the block.
        if (block.Load(Hole::Implementation->network,
                       address) == elle::Status::Error)
          throw reactor::Exception(elle::concurrency::scheduler(), "unable to load the block");

        // validate the block.
        if (block.Validate(address) == elle::Status::Error)
          throw reactor::Exception(elle::concurrency::scheduler(), "the block seems to be invalid");
      }

      void
      Server::get(const nucleus::proton::Address& address,
                  const nucleus::proton::Version& version,
                  nucleus::proton::MutableBlock& block)
      {
        ELLE_LOG_TRACE_SCOPE("Get[Mutable]");

        // does the block exist.
        if (block.Exist(Hole::Implementation->network,
                        address, version) == elle::Status::False)
          throw reactor::Exception(elle::concurrency::scheduler(), "the block does not seem to exist");

        // load the block.
        if (block.Load(Hole::Implementation->network,
                       address, version) == elle::Status::Error)
          throw reactor::Exception(elle::concurrency::scheduler(), "unable to load the block");

        // validate the block, depending on its component.
        //
        // indeed, the Object component requires as additional block for
        // being validated.
        switch (address.component)
          {
          case nucleus::neutron::ComponentObject:
            {
              const nucleus::neutron::Object* object =
                static_cast<const nucleus::neutron::Object*>(&block);
              assert(dynamic_cast<const nucleus::neutron::Object*>(
                       &block) != nullptr);

              // validate the object according to the presence of
              // a referenced access block.
              if (object->meta.access != nucleus::proton::Address::Null)
                {
                  // Load the access block.
                  std::unique_ptr<nucleus::proton::Block> block
                    (Hole::Pull(object->meta.access, nucleus::proton::Version::Last));
                  std::unique_ptr<nucleus::neutron::Access> access
                    (dynamic_cast<nucleus::neutron::Access*>(block.release()));
                  if (!access)
                    throw reactor::Exception(elle::concurrency::scheduler(),
                                             "expected an access block");

                  // validate the object, providing the
                  if (object->Validate(address, *access) == elle::Status::Error)
                    throw reactor::Exception(elle::concurrency::scheduler(), "unable to validate the object");
                }
              else
                {
                  // validate the object.
                  if (object->Validate(
                        address,
                        nucleus::neutron::Access::Null) == elle::Status::Error)
                    throw reactor::Exception(elle::concurrency::scheduler(), "unable to validate the object");
                }

              break;
            }
          default:
            {
              // validate the block through the common interface.
              if (block.Validate(address) == elle::Status::Error)
                throw reactor::Exception(elle::concurrency::scheduler(), "the block seems to be invalid");

              break;
            }
          case nucleus::neutron::ComponentUnknown:
            {
              throw reactor::Exception(elle::concurrency::scheduler(),
                                       elle::sprintf("unknown component '%u'",
                                                     address.component));
            }
          }
      }

      void
      Server::kill(const nucleus::proton::Address& address)
      {
        ELLE_LOG_TRACE_SCOPE("Kill");

        // treat the request depending on the nature of the block which
        // the addres indicates.
        switch (address.family)
          {
          case nucleus::proton::FamilyContentHashBlock:
            {
              // erase the immutable block.
              if (nucleus::proton::ImmutableBlock::Erase(Hole::Implementation->network,
                           address) == elle::Status::Error)
                throw reactor::Exception(elle::concurrency::scheduler(), "unable to erase the block");

              break;
            }
          case nucleus::proton::FamilyPublicKeyBlock:
          case nucleus::proton::FamilyOwnerKeyBlock:
          case nucleus::proton::FamilyImprintBlock:
            {
              // retrieve the mutable block.
              if (nucleus::proton::MutableBlock::Erase(Hole::Implementation->network,
                           address) == elle::Status::Error)
                throw reactor::Exception(elle::concurrency::scheduler(), "unable to erase the block");

              break;
            }
          default:
            {
              throw reactor::Exception(elle::concurrency::scheduler(), "unknown block family");
            }
          }
      }

      elle::Status      Server::Dump(const elle::Natural32      margin) const
      {
        elle::String    alignment(margin, ' ');

        std::cout << alignment << "[Server]" << std::endl;

        std::cout << alignment << elle::io::Dumpable::Shift
                  << "[Customers] " << this->_customers.size() << std::endl;

        // go though the customer.
        for (Customer* customer: this->_customers)
          if (customer->Dump(margin + 4) == elle::Status::Error)
            throw reactor::Exception(elle::concurrency::scheduler(), "unable to dump the customer");

        return elle::Status::Ok;
      }

    }
  }
}
