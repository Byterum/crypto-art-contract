#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <eosio/system.hpp>
#include <string>
#include <vector>

using namespace eosio;
using namespace std;
typedef uint128_t global_id;
typedef uint64_t id_type;
typedef string uri_type;

CONTRACT artasset : public contract {
public:
  using contract::contract;
  artasset(name receiver, name code, datastream<const char *> ds)
      : contract(receiver, code, ds), tokens(receiver, receiver.value) {}

  /**
   * Create a new non-fungible token.
   *
   * @param issuer - Issuer of the token
   * @param maximum_supply - Maximum supply. "0 ${symbol}" means infinite supply
   * @param typ - token type. `0` coin, `1` nft
   */
  ACTION create(name issuer, asset maximum_supply, int typ);

  /**
   * Transfer 1 nft with specified `id` from `from` to `to`.
   * Throws error if token with `id` does not exist, or `from` is no the token
   * owner.
   *
   * @param from - Account name of token owner
   * @param to - Account name of token receiver
   * @param uuid - Global ID of the token to transfer
   * @param memo - Action memo. Maximum 256 bytes
   */
  ACTION transfernft(name from, name to, global_id uuid, string memo);

  /**
   * Burn 1 token with specified `id` owner by account `owner`.
   * Only tokens of this world can be burnt.
   *
   * @param owner - Token owner
   * @param uuid - Global ID of the token to burn
   * @param memo - Action memo. Maximum 256 bytes
   */
  ACTION burnnft(name owner, global_id uuid, string memo);

  ACTION setrampayer(name payer, id_type id);

  /**
   * Get symbol code of token with specified `id`.
   *
   * @param contract - Account name of contract
   * @param token_id - Unique ID of the token
   */
  static symbol_code get_symbol(name contract, id_type token_id) {
    token_index tokens(contract, contract.value);
    auto it = tokens.find(token_id);
    if (it == tokens.end()) {
      return symbol_code{};
    } else {
      return it->value.symbol.code();
    }
  }

  /**
   * Get balance of token with specified symbol code.
   *
   * @param contract - Account name of contract
   * @param owner - Token owner
   * @param sym_code - Token symbol code
   */
  static asset get_balance(name contract, name owner, symbol_code sym_code) {
    account_index acnts(contract, owner.value);
    auto t = acnts.find(sym_code.raw());
    if (t == acnts.end()) {
      return asset(0, symbol(sym_code, 0));
    } else {
      return t->balance;
    }
  }

  /**
   * Get token owner
   */
  name get_owner(global_id uuid) {
    token_index tokens(get_self(), get_self().value);
    auto index = tokens.get_index<"byuuid"_n>();
    auto it = index.find(uuid);
    if (it == index.end()) {
      return name{};
    } else {
      return it->owner;
    }
  }

  asset get_balance(name owner) {
    account_index acnts(get_self(), owner.value);
    auto it = acnts.find(owner.value);
    if (it == acnts.end()) {
      return asset{};
    } else {
      return it->balance;
    }
  }

  TABLE account {
    asset balance; // DON'T MODIFIED.

    // You can add more fields here...

    uint64_t primary_key() const { return balance.symbol.code().raw(); }
  };

  TABLE stat {
    /* === DON'T MODIFIED fields === */
    name issuer;      // Token issuer.
    asset supply;     // Token supply.
    asset issued;     // Token amount has been issued.
    asset max_supply; // Token maximum supply.
    bool infinite;    // Is token infinitely supply or not.
    int type;         // Token type: `0` ft; `1` nft.

    /* === You can add more fields below === */

    uint64_t primary_key() const { return supply.symbol.code().raw(); }
    uint64_t get_issuer() const { return issuer.value; }

    EOSLIB_SERIALIZE(stat,
                     (issuer)(supply)(issued)(max_supply)(infinite)(type));
  };

  // nft definition. YOU CAN ONLY ADD FIELDS.
  TABLE token {
    /* === DON'T MODIFIED fields === */
    id_type id;     // Unique 64 bit identifier.
    global_id uuid; // Global token id.
    uri_type uri;   // RFC 3986.
    name owner;     // token owner.
    asset value;    // token value (e.g. 1 EOS).

    /* === You can add more fields below === */

    id_type primary_key() const { return id; }
    global_id get_uuid() const { return uuid; }
    uint64_t get_owner() const { return owner.value; }
    string get_uri() const { return uri; }
    asset get_value() const { return value; }
    uint64_t get_symbol() const { return value.symbol.code().raw(); }
  };

  using account_index = eosio::multi_index<"accounts"_n, account>;

  using stat_index = eosio::multi_index<
      "stats"_n, stat,
      indexed_by<"byissuer"_n,
                 const_mem_fun<stat, uint64_t, &stat::get_issuer>>>;

  using token_index = eosio::multi_index<
      "tokens"_n, token,
      indexed_by<"byuuid"_n, const_mem_fun<token, uint128_t, &token::get_uuid>>,
      indexed_by<"byowner"_n,
                 const_mem_fun<token, uint64_t, &token::get_owner>>,
      indexed_by<"bysymbol"_n,
                 const_mem_fun<token, uint64_t, &token::get_symbol>>>;

  // generated token global uuid based on token id and
  // contract name, passed in the argument
  global_id get_global_id(name contract, id_type id) const {
    uint128_t self_128 = static_cast<uint128_t>(contract.value);
    uint128_t id_128 = static_cast<uint128_t>(id);
    uint128_t res = (self_128 << 64) | (id_128);
    return res;
  }

  string uuid_to_string(global_id uuid) {
    const char *charmap = "0123456789";
    string result;
    result.reserve(40); // max. 40 digits possible ( uint64_t has 20)
    uint128_t helper = uuid;

    do {
      result += charmap[helper % 10];
      helper /= 10;
    } while (helper);
    std::reverse(result.begin(), result.end());
    return result;
  }

  /**
   * Mint specified number of tokens with previously created symbol to the
   * account name "to".
   * Each token is generated with an unique token_id asiigned to it, and a URI
   * format string.
   *
   * @param to - Issuer of the token
   * @param symbol - Token symbol
   * @param uri - URI string of token. Seed the RFC 3986
   * @param memo - Action memo. Maximum 256 bytes
   */
  global_id mintnft(name to, string symbol, string uri, string memo);

private:
  token_index tokens;

  void sub_balance(name owner, asset value);
  void add_balance(name owner, asset value, name ram_payer);
  void sub_supply(asset quantity);
  void add_supply(asset quantity);
  global_id _mint(name owner, name ram_payer, asset value, string uri);
};
