#include "contracts.hpp"
#include "eosio.system_tester.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/test/unit_test.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/testing/tester.hpp>

#include <Runtime/Runtime.h>

#include <fc/io/json.hpp>
#include <fc/static_variant.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

#define CHECK_ASSERT(S, M)                                                                                             \
   try {                                                                                                               \
      S;                                                                                                               \
      BOOST_ERROR("exception eosio_assert_message_exception is expected");                                             \
   } catch (eosio_assert_message_exception & e) {                                                                      \
      if (e.top_message() != "assertion failure with message: " M)                                                     \
         BOOST_ERROR("expected \"assertion failure with message: " M "\" got \"" + e.top_message() + "\"");            \
   }

static const fc::microseconds abi_serializer_max_time{1'000'000};

class assert_tester : public TESTER {
 public:
   auto push_action(name contract, const name& signer, const action_name& name, const variant_object& data) {
      string action_type_name = abi_ser.get_action_type(name);

      action act;
      act.account = contract;
      act.name = name;
      act.authorization = vector<permission_level>{{signer, config::active_name}};
      act.data = abi_ser.variant_to_binary(action_type_name, data, abi_serializer_max_time);

      signed_transaction trx;
      trx.actions.emplace_back(std::move(act));
      set_transaction_headers(trx);
      trx.sign(get_private_key(signer, "active"), control->get_chain_id());
      return push_transaction(trx);
   }

   assert_tester()
       : TESTER(), abi{contracts::eosio_assert_abi()},
         abi_ser(json::from_string(std::string{abi.data(), abi.data() + abi.size()}).as<abi_def>(),
                 abi_serializer_max_time) {
      create_account(N(eosio.assert));
      set_code(N(eosio.assert), contracts::eosio_assert_wasm());
   }

   std::vector<char> abi;
   abi_serializer abi_ser;
};

BOOST_AUTO_TEST_SUITE(eosio_assert)

BOOST_AUTO_TEST_CASE(bootstrap) try { assert_tester t{}; }
FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
