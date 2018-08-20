#include <eosio.assert/eosio.assert.hpp>

#include <eosiolib/dispatcher.hpp>

namespace assert_contract {

using bytes = std::vector<char>;

bytes get_action_bytes() {
   bytes result(action_data_size());
   read_action_data(result.data(), result.size());
   return result;
}

checksum256 get_hash(const bytes& data) {
   checksum256 result;
   sha256(data.data(), data.size(), &result);
   return result;
}

struct asserter {
   name                _self;
   manifests           manifest_table{_self, _self};
   manifests_id_index  manifest_id_idx = manifest_table.get_index<"id"_n>();
   chains              chain_table{_self, _self};
   stored_chain_params chain = chain_table.get_or_default();

   void setchain() {
      require_auth("eosio"_n);
      auto data        = get_action_bytes();
      auto hash        = get_hash(data);
      auto unpacked    = unpack<chain_params>(data);
      chain.chain_id   = unpacked.chain_id;
      chain.chain_name = unpacked.chain_name;
      chain.icon       = unpacked.icon;
      chain.hash       = hash;
      chain_table.set(chain, _self);
   }

   void add_manifest() {
      auto data     = get_action_bytes();
      auto hash     = get_hash(data);
      auto unpacked = unpack<manifest>(data);
      auto stored   = stored_manifest{
          .unique_id = chain.next_unique_id++,
          .id        = hash,
          .account   = unpacked.account,
          .whitelist = {unpacked.whitelist.begin(), unpacked.whitelist.end()},
          .blacklist = {unpacked.blacklist.begin(), unpacked.blacklist.end()},
      };
      require_auth(stored.account);
      auto it = manifest_id_idx.find(stored.id_key());
      eosio_assert(it == manifest_id_idx.end(), "manifest already present");
      manifest_table.emplace(stored.account, [&](auto& x) { x = stored; });
      chain_table.set(chain, _self);
   }

   void del_manifest(checksum256 id) {
      auto it = manifest_id_idx.find(to_key256(id));
      eosio_assert(it != manifest_id_idx.end(), "manifest not found");
      require_auth(it->account);
      manifest_id_idx.erase(it);
   }

   bool in(contract_action action, const flat_set<contract_action>& actions) {
      return actions.find(action) != actions.end() || //
             actions.find({action.contract, name{0}}) != actions.end() ||
             actions.find({name{0}, action.action}) != actions.end() ||
             actions.find({name{0}, name{0}}) != actions.end();
   }

   void require(const checksum256& chain_params_hash, const checksum256& manifest_id,
                const vector<contract_action>& actions) {
      eosio_assert(chain_params_hash == chain.hash, "Incorrect chain");
      auto it = manifest_id_idx.find(to_key256(manifest_id));
      eosio_assert(it != manifest_id_idx.end(), "manifest not found");
      for (auto& action : actions) {
         if (!in(action, it->whitelist))
            eosio_assert(
                false,
                (action.action.to_string() + "@" + action.contract.to_string() + " is not in whitelist").c_str());
         if (in(action, it->blacklist))
            eosio_assert(
                false,
                (action.action.to_string() + "@" + action.contract.to_string() + " is in blacklist").c_str());
      }
   }

   void apply(name contract, name act) {
      if (contract == _self) {
         auto& thiscontract = *this;

         switch (act) {
         case "setchain"_n:
            return setchain();
         case "add.manifest"_n:
            return add_manifest();
         };

         switch (act) { EOSIO_API(asserter, (del_manifest)(require)) };
      }
   }
};

[[noreturn]] extern "C" void apply(eosio::name receiver, eosio::name code, eosio::name action) {
   asserter{receiver}.apply(code, action);
   eosio_exit(0);
}

} // namespace assert_contract
