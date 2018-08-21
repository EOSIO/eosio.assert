#pragma once

#include <eosiolib/crypto.h>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/singleton.hpp>

namespace assert_contract {

using namespace eosio;
using boost::container::flat_set;
using std::string;
using std::vector;

/// When deserializing skip the stream's data for T.
template <typename T>
struct skip {};

template <typename DataStream>
DataStream& operator>>(DataStream& ds, skip<checksum256>&) {
   ds.skip(sizeof(checksum256::hash));
   return ds;
}

template <typename DataStream>
DataStream& operator>>(DataStream& ds, skip<std::string>&) {
   unsigned_int size;
   ds >> size;
   ds.skip(size);
   return ds;
}

inline key256 to_key256(const checksum256& cs) {
   key256 result;
   static_assert(sizeof(cs.hash) == sizeof(result.get_array()));
   memcpy(result.data(), cs.hash, sizeof(cs.hash));
   return result;
}

struct chain_params {
   checksum256 chain_id;
   string      chain_name;
   checksum256 icon;
};

struct stored_chain_params {
   checksum256 chain_id;
   string      chain_name;
   checksum256 icon;
   checksum256 hash;
   uint64_t    next_unique_id = 1;
};

using chains = singleton<"chain.params"_n, stored_chain_params>;

struct contract_action {
   name contract;
   name action;
};

inline bool operator<(const contract_action& a, const contract_action& b) {
   return std::tie(a.contract, a.action) < std::tie(b.contract, b.action);
}

struct manifest {
   name                    account;
   skip<std::string>       _name;
   std::string             domain;
   skip<checksum256>       icon;
   skip<std::string>       extra_json;
   vector<contract_action> whitelist;
   vector<contract_action> blacklist;
};

struct stored_manifest {
   uint64_t                  unique_id = 0;
   checksum256               id;
   name                      account;
   std::string               domain;
   flat_set<contract_action> whitelist;
   flat_set<contract_action> blacklist;

   uint64_t primary_key() const { return unique_id; }
   key256   id_key() const { return to_key256(id); }
};

using manifests =
    eosio::multi_index<"manifests"_n, stored_manifest,
                       indexed_by<"id"_n, eosio::const_mem_fun<stored_manifest, key256, &stored_manifest::id_key>>>;
using manifests_id_index = decltype(std::declval<manifests>().get_index<"id"_n>());

struct abi_hash {
   name        owner;
   checksum256 hash;

   uint64_t primary_key() const { return owner; }
};

using abi_hash_table = eosio::multi_index<"abihash"_n, abi_hash>;

} // namespace assert_contract
