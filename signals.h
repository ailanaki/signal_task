#pragma once

#include <functional>
#include <boost/intrusive/list.hpp>

using namespace boost::intrusive;
namespace signals {

    template<typename T>
    struct signal;

    template<typename... Args>
    struct signal<void(Args...)> {

        using slot_t = std::function<void(Args...)>;
        using auto_unlink_hook = list_base_hook<link_mode<auto_unlink> >;


        struct connection : public auto_unlink_hook {
            connection() = default;

            explicit connection(signal *sig, slot_t slot) :
                    sig(sig), slot(std::move(slot)) {
                sig->connections.push_front(*this);
            }

            connection(connection &&other) noexcept : slot(std::move(other.slot)), sig(other.sig) {
                if (other.is_linked()) {
                    swap_nodes(other);
                    other.unlink();

                }
            }

            connection &operator=(connection &&other) noexcept {
                this->slot = other.slot;
                if (other.is_linked()) {
                    this->swap_nodes(other);
                    other.unlink();
                }
                return *this;

            }

            void disconnect() {
                if (is_linked()) {
                    unlink();
                    if (&*sig->current_t->current == this){
                        sig->current_t->current++;
                    }
                    sig = nullptr;
                }

            };
            slot_t slot = {};
            signal *sig = nullptr;
        };

        using connection_t=boost::intrusive::list<connection, constant_time_size<false>>;

        signal(){
            current_t = new (iteration_token);
        };

        signal(signal const &) = delete;

        signal &operator=(signal const &) = delete;

        ~signal() {
        };

        connection connect(slot_t slot) noexcept {
            return connection(this, slot);
        };
        using con_i = typename connection_t::const_iterator;

        struct iteration_token {
            con_i current;
        };

        void operator()(Args... args) const{
            iteration_token* tok = current_t;
            tok->current = connections.cbegin();
            try {
                while (tok->current != connections.end()) {
                    auto copy = tok->current;
                    ++tok->current;
                    copy->slot(args...);
                    if (!tok->current.pointed_node()){
                        return;
                    }
                }

            } catch(std::exception &e){
                throw;

            }
        };
    private:
        connection_t connections;
        iteration_token* current_t;
    };
}
