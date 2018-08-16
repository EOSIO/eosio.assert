#include <eosiolib/dispatcher.hpp>
#include <eosiolib/types.hpp>

using namespace eosio;

struct asserter {
   name _self;

   void setchain() {}

   void apply(name contract, name act) {
      if (contract == _self) {
         auto& thiscontract = *this;
         switch (act) { EOSIO_API(asserter, (setchain)) };
      }
   }
};

[[noreturn]] extern "C" void apply(eosio::name receiver, eosio::name code, eosio::name action) {
   asserter{receiver}.apply(code, action);
   eosio_exit(0);
}
