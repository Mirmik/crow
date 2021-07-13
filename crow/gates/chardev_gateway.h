/**
@file
*/

#ifndef CROW_GATES_CHANNEL_GATE_H
#define CROW_GATES_CHANNEL_GATE_H

#include <crow/gateway.h>
#include <crow/packet.h>
#include <igris/buffer.h>

namespace crow
{
    class chardev_protocol;
    class chardev_driver;
    class chardev_gateway;

    class chardev_protocol
    {
      public:
        chardev_gateway *gate;
        chardev_driver *driver; // fast access

      public:
        virtual void send_automate_reset(crow::packet *pack) = 0;

        virtual int receive_automate_newdata(char c) = 0;
        virtual void receive_automate_reset() = 0;
        virtual void receive_automate_setbuf(char *data, unsigned int size) = 0;
    };

    class chardev_driver
    {
      public:
        chardev_gateway *gate;

        // virtual int room() = 0; // количество символов, которые устройство
        // может
        // принять. на передачу
        // virtual void send(const char *data, unsigned int size) = 0;

        // bool ready_to_send();
        virtual bool ready_for_recv() = 0;
        virtual void start_send() = 0;

        virtual void read(char *data, int size) = 0;
        // virtual void nonblock_tryread() = 0;
    };

    class chardev_gateway : public crow::gateway
    {
        chardev_driver *driver;
        chardev_protocol *protocol;

        dlist_head to_send = DLIST_HEAD_INIT(to_send);
        int packet_dataaddr_size = 48;
        crow::packet *insend;
        crow::packet *rpack;

        bool send_by_symbol_mode = false;
        bool recv_by_symbol_mode = false;
        bool self_driven_receive = false;

      public:
        void set_send_symbol_mode(bool en) { send_by_symbol_mode = en; }
        void set_recv_symbol_mode(bool en) { recv_by_symbol_mode = en; }

        chardev_gateway(chardev_driver *driver, chardev_protocol *protocol);

        void dosend(struct crow::packet *pack);
        void init_recv();

        void send(crow::packet *) override;
        void nblock_onestep() override;
        void finish() override;

      public: // driver callback
        void newdata(char c);
    };
}

#endif