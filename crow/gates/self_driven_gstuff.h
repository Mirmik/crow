#ifndef CROW_SELF_DRIVEN_GSTUFF_H
#define CROW_SELF_DRIVEN_GSTUFF_H

#include <crow/gateway.h>
#include <crow/tower.h>
#include <igris/protocols/gstuff.h>
#include <igris/sync/semaphore.h>

#include <nos/print.h>

namespace crow
{
    class self_driven_gstuff : public crow::gateway
    {
        struct semaphore sem = SEMAPHORE_INIT(sem, 1);

        dlist_head to_send = DLIST_HEAD_INIT(to_send);
        crow::packet *insend = nullptr;
        char *send_buffer = nullptr;
        char *send_it = nullptr;
        char *send_eit = nullptr;

        int received_maxpack_size = 48;
        crow::packet *recvpack = nullptr;
        struct gstuff_autorecv recver;

        int (*write_callback)(void *, const char *data, unsigned int size);
        void *write_privdata;

      public:
        void init(char *send_buffer,
                  int (*write_callback)(void *, const char *data,
                                        unsigned int size),
                  void *write_privdata)
        {
            this->send_buffer = send_buffer;
            init_receiver();
            invalidate_sender();

            this->write_callback = write_callback;
            this->write_privdata = write_privdata;
        }

        void newdata(char c)
        {
            int status;

            status = gstuff_autorecv_newchar(&recver, c);

            switch (status)
            {
            case GSTUFF_NEWPACKAGE:
                newline_handler();
                break;

            default:
                break;
            }
        }

        void newline_handler()
        {
            struct crow::packet *pack = recvpack;
            recvpack = NULL;

            pack->revert_gate(this->id);

            crow::packet_initialization(pack, this);

            crow::nocontrol_travel(pack, false);

            init_receiver();
        }

        void init_receiver()
        {
            recvpack = crow::allocate_packet(received_maxpack_size);
            gstuff_autorecv_setbuf(&recver, (char *)&recvpack->header,
                                   received_maxpack_size);
        }

        void start_send()
        {
            if (dlist_empty(&to_send))
                return;

            insend = dlist_first_entry(&to_send, crow::packet, lnk);
            dlist_del_init(&insend->lnk);

            int size = gstuffing((const char *)&insend->header,
                                 insend->fullsize(), send_buffer);

            send_it = send_buffer;
            send_eit = send_buffer + size;

            continue_send();
        }

        void invalidate_sender()
        {
            insend = nullptr;
            send_it = nullptr;
            send_eit = nullptr;
        }

        void finish_send()
        {
            system_lock();
            crow::return_to_tower(insend, CROW_SENDED);
            invalidate_sender();
            system_unlock();

            start_send();
        }

        void continue_send()
        {
            int needwrite = send_eit - send_it;
            int writed = write_callback(write_privdata, send_it, needwrite);

            send_it += writed;

            if (send_it == send_eit)
            {
                finish_send();
            }
        }

        void send(crow::packet *pack) override
        {
            system_lock();
            dlist_move(&pack->lnk, &to_send);
            system_unlock();

            if (sem_try_down(&sem))
                return;

            if (insend == nullptr)
            {
                start_send();
            }

            sem_up(&sem);
        }

        void nblock_onestep() override
        {
            if (sem_try_down(&sem))
                return;

            if (send_it != send_eit)
            {
                continue_send();
            }

            sem_up(&sem);
        }

        void finish() override
        {
            if (recvpack)
            {
                crow::deallocate_packet(recvpack);
                recvpack = nullptr;
            }
        }

        ~self_driven_gstuff() { finish(); }
    };
}

#endif