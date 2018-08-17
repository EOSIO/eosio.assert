#include <eosiolib/crypto.h>
#include <eosiolib/dispatcher.hpp>
#include <eosiolib/multi_index.hpp>

using namespace eosio;
using boost::container::flat_set;
using std::string;
using std::vector;

using bytes = std::vector<char>;

/// When deserializing skip the stream's data for T.
template <typename T>
struct skip {};

template <typename DataStream>
DataStream& operator>>(DataStream& ds, skip<checksum256>&) {
   ds.skip(sizeof(checksum256::hash));
   return ds;
}

template <typename DataStream>
DataStream& operator>>(DataStream& ds, skip<string>&) {
   unsigned_int size;
   ds >> size;
   ds.skip(size);
   return ds;
}

key256 to_key256(const checksum256& cs) {
   key256 result;
   static_assert(sizeof(cs.hash) == sizeof(result.get_array()));
   memcpy(result.data(), cs.hash, sizeof(cs.hash));
   return result;
}

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

struct contract_action {
   name contract;
   name action;
};

bool operator<(const contract_action& a, const contract_action& b) {
   return std::tie(a.contract, a.action) < std::tie(b.contract, b.action);
}

struct manifest {
   name                    account;
   skip<string>            _name;
   skip<string>            domain;
   skip<checksum256>       icon;
   skip<string>            description;
   skip<string>            extra_json;
   vector<contract_action> whitelist;
   vector<contract_action> blacklist;
};

struct stored_manifest {
   uint64_t                  unique_id = 0;
   checksum256               id;
   name                      account;
   flat_set<contract_action> whitelist;
   flat_set<contract_action> blacklist;

   uint64_t primary_key() const { return unique_id; }
   key256   id_key() const { return to_key256(id); }
};

using manifests =
    eosio::multi_index<"manifests"_n, stored_manifest,
                       indexed_by<"id"_n, eosio::const_mem_fun<stored_manifest, key256, &stored_manifest::id_key>>>;
using manifests_id_index = decltype(std::declval<manifests>().get_index<"id"_n>());

struct asserter {
   name               _self;
   manifests          manifest_table{_self, _self};
   manifests_id_index manifest_id = manifest_table.get_index<"id"_n>();

   void add_manifest() {
      auto data     = get_action_bytes();
      auto hash     = get_hash(data);
      auto unpacked = unpack<manifest>(data);
      auto stored   = stored_manifest{
          .unique_id = 0, // todo
          .id        = hash,
          .account   = unpacked.account,
          .whitelist = {unpacked.whitelist.begin(), unpacked.whitelist.end()},
          .blacklist = {unpacked.blacklist.begin(), unpacked.blacklist.end()},
      };
      require_auth(stored.account);
      auto it = manifest_id.find(stored.id_key());
      eosio_assert(it == manifest_id.end(), "manifest already present");
      manifest_table.emplace(stored.account, [&](auto& x) { x = stored; });
   }

   void del_manifest(checksum256 id) {
      auto it = manifest_id.find(to_key256(id));
      eosio_assert(it != manifest_id.end(), "manifest not found");
      require_auth(it->account);
      manifest_id.erase(it);
   }

   void apply(name contract, name act) {
      if (contract == _self) {
         auto& thiscontract = *this;

         switch (act) {
         case "add.manifest"_n:
            return add_manifest();
         };

         switch (act) { EOSIO_API(asserter, (del_manifest)) };
      }
   }
};

[[noreturn]] extern "C" void apply(eosio::name receiver, eosio::name code, eosio::name action) {
   asserter{receiver}.apply(code, action);
   eosio_exit(0);
}
