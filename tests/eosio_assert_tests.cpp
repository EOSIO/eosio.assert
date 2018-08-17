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
   using TESTER::push_transaction;
   void push_transaction(name signer, const std::string& s) {
      auto v = json::from_string(s);
      outfile << "push_transaction " << json::to_pretty_string(v) << "\n";

      signed_transaction trx;
      for (auto& a : v["actions"].get_array()) {
         variant_object data;
         from_variant(a["data"], data);
         action act;
         act.account       = a["account"].as<name>();
         act.name          = a["name"].as<name>();
         act.authorization = a["authorization"].as<vector<permission_level>>();
         act.data = abi_ser.variant_to_binary(abi_ser.get_action_type(act.name), data, abi_serializer_max_time);
         trx.actions.emplace_back(std::move(act));
      }

      try {
         set_transaction_headers(trx);
         trx.sign(get_private_key(signer, "active"), control->get_chain_id());
         push_transaction(trx);
         outfile << "transaction pushed\n";
      } catch (fc::exception& e) {
         outfile << "Exception: " << e.top_message() << "\n";
      }
   }

   assert_tester(const std::string& test_name)
       : TESTER(), test_name{test_name}, outfile{DATA_DIR + test_name + ".actual"}, abi{contracts::eosio_assert_abi()},
         abi_ser(json::from_string(std::string{abi.data(), abi.data() + abi.size()}).as<abi_def>(),
                 abi_serializer_max_time) {
      create_account(N(eosio.assert));
      set_code(N(eosio.assert), contracts::eosio_assert_wasm());
   }

   void dump_table(name account, name scope, name table) {
      outfile << account << "." << scope << "." << table << "\n";
      const auto& db  = control->db();
      const auto* tbl = db.find<table_id_object, by_code_scope_table>(boost::make_tuple(account, scope, table));
      if (!tbl)
         return;
      auto& idx = db.get_index<key_value_index, by_scope_primary>();
      for (auto it = idx.lower_bound(std::make_tuple(tbl->id, 0)); it != idx.end() && it->t_id == tbl->id; ++it) {
         outfile << "...\n";
      }
   }

   void heading(const std::string& s) { outfile << "\n" << s << "\n=========================\n"; }

   void check_file() {
      outfile.close();
      if (write_mode)
         BOOST_REQUIRE_EQUAL(
             0, system(("cp " DATA_DIR + test_name + ".actual " DATA_DIR + test_name + ".expected").c_str()));
      else
         BOOST_REQUIRE_EQUAL(
             0, system(("colordiff " DATA_DIR + test_name + ".expected " DATA_DIR + test_name + ".actual").c_str()));
   }

   std::string           test_name;
   mutable std::ofstream outfile;
   std::vector<char>     abi;
   abi_serializer        abi_ser;
};

BOOST_AUTO_TEST_SUITE(eosio_assert)

BOOST_AUTO_TEST_CASE(bootstrap) try {
   assert_tester t{"bootstrap"};
   t.create_account(N(dapp1));
   t.create_account(N(dapp2));

   t.heading("add.manifest: missing authority");
   t.push_transaction(N(dapp2), R"({
      "actions": [{
         "account":              "eosio.assert",
         "name":                 "add.manifest",
         "authorization": [{
            "actor":             "dapp2",
            "permission":        "active",
         }],
         "data": {
            "tables":            [],
            "ricardian_clauses": [],
            "abi_extensions":    [],
            "account":           "dapp1",
            "name":              "distributed app 1",
            "domain":            "https://nowhere",
            "icon":              "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF",
            "description":       "Something to try",
            "extra.json":        "",
            "whitelist":         [],
            "blacklist":         []
         },
      }]
   })");

   t.heading("add.manifest");
   t.push_transaction(N(dapp1), R"({
      "actions": [{
         "account":              "eosio.assert",
         "name":                 "add.manifest",
         "authorization": [{
            "actor":             "dapp1",
            "permission":        "active",
         }],
         "data": {
            "tables":            [],
            "ricardian_clauses": [],
            "abi_extensions":    [],
            "account":           "dapp1",
            "name":              "distributed app 1",
            "domain":            "https://nowhere",
            "icon":              "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF",
            "description":       "Something to try",
            "extra.json":        "",
            "whitelist":         [],
            "blacklist":         []
         },
      }]
   })");

   t.heading("add.manifest: duplicate");
   t.produce_block();
   t.push_transaction(N(dapp1), R"({
      "actions": [{
         "account":              "eosio.assert",
         "name":                 "add.manifest",
         "authorization": [{
            "actor":             "dapp1",
            "permission":        "active",
         }],
         "data": {
            "tables":            [],
            "ricardian_clauses": [],
            "abi_extensions":    [],
            "account":           "dapp1",
            "name":              "distributed app 1",
            "domain":            "https://nowhere",
            "icon":              "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF",
            "description":       "Something to try",
            "extra.json":        "",
            "whitelist":         [],
            "blacklist":         []
         },
      }]
   })");

   t.check_file();
}
FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
