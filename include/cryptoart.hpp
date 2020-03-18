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

CONTRACT cryptoart : public contract {
public:
  using contract::contract;
  cryptoart(name receiver, name code, datastream<const char *> ds)
      : contract(receiver, code, ds), tokens(receiver, receiver.value),
        control_tokens(receiver, receiver.value) {}

  /**
   * Create a new non-fungible token.
   *
   * @param issuer - Issuer of the token
   * @param maximum_supply - Maximum supply. "0 ${symbol}" means infinite supply
   */
  ACTION create(name issuer, asset maximum_supply);

  /**
   * Transfer 1 nft with specified `id` from `from` to `to`.
   * Throws error if token with `id` does not exist, or `from` is no the token
   * owner.
   *
   * @param from - Account name of token owner
   * @param to - Account name of token receiver
   * @param token_iud - Unique ID of the token to transfer
   * @param memo - Action memo. Maximum 256 bytes
   */
  ACTION transfer(name from, name to, id_type token_id, string memo);

  /**
   * Burn 1 token with specified `id` owner by account `owner`.
   * Only tokens of this world can be burnt.
   *
   * @param owner - Token owner
   * @param uuid - Global ID of the token to burn
   * @param memo - Action memo. Maximum 256 bytes
   */
  ACTION burn(name owner, global_id uuid, string memo);

  ACTION setrampayer(name payer, id_type id);
  /**
   * Set up layer token after artist call `mintartwork`.
   * @param token_id - Token unique id
   * @param min_values - Minimum values list with index as lever_id
   * @param max_values - Maximum values list with index as lever_id
   * @param curr_values - Current values list with indnex as lever_id
   */
  ACTION setuptoken(id_type token_id, vector<int64_t> min_values,
                    vector<int64_t> max_values, vector<int64_t> curr_values);

  /**
   * Mint master layer of art work to `to` and multiple layer tokens to
   * given collaborators.
   * @param to - Target artist holding the master layer
   * @param uri - URI for master layer. e.g. url of image
   * @param collaborators - Collaborate artists holding multiple layer
   * tokens. Zero means normal crypto art. Non-zero means programmable art.
   */
  ACTION mintartwork(name to, string uri, vector<name> collaborators);

  /**
   * Update current value of a token with given lever ids.
   * @param token_id - Token unique id
   * @param lever_ids - Lever id list to update
   * @param new_values - New current value for levers indexed by `lever_ids`
   */
  ACTION updatetoken(id_type token_id, vector<int64_t> lever_ids,
                     vector<int64_t> new_values);

  /**
   * Get master token id of a given layer token.
   * If token is master, `master_token_id` is equal to its `id`.
   * @param token_id - Token unique id
   */
  id_type get_master(id_type token_id) {
    const auto &token = control_tokens.get(token_id, "token not found");
    return token.master_token_id;
  }

  /**
   * Get layer token ids with a given master id.
   */
  vector<id_type> get_layer_tokens(id_type master_id) {
    auto master_index = control_tokens.get_index<"bymasterid"_n>();
    auto itr = master_index.lower_bound(master_id);
    vector<id_type> layer_tokens;
    while (itr != master_index.end()) {
      layer_tokens.push_back(itr->id);
      itr++;
    }
    return layer_tokens;
  }

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

  name get_issuer(symbol_code sym) {
    stat_index stat(get_self(), get_self().value);
    auto info =
        stat.get(sym.raw(), "token stat not found, please create first");
    return info.issuer;
  }

  /**
   * Get token owner
   */
  name get_owner_by_uuid(global_id uuid) {
    auto index = tokens.get_index<"byuuid"_n>();
    auto it = index.find(uuid);
    if (it == index.end()) {
      return name{};
    } else {
      return it->owner;
    }
  }

  name get_owner_by_id(id_type token_id) {
    auto it = tokens.find(token_id);
    if (it == tokens.end()) {
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

    /* === You can add more fields below === */

    uint64_t primary_key() const { return supply.symbol.code().raw(); }
    uint64_t get_issuer() const { return issuer.value; }
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

  TABLE controltoken {
    // token id
    id_type id;
    // num of levers
    uint64_t levers_num;
    // is token setup
    bool isSetup;
    // master token id of the layer token
    // if it's master token, equal to the token id
    id_type master_token_id;
    // lever values
    vector<int64_t> min_values;
    vector<int64_t> max_values;
    vector<int64_t> curr_values;

    id_type primary_key() const { return id; }
    id_type get_master_id() const { return master_token_id; }
  };

  using control_token_table =
      multi_index<name("ctltokens"), controltoken,
                  indexed_by<name("bymasterid"),
                             const_mem_fun<controltoken, id_type,
                                           &controltoken::get_master_id>>>;

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

private:
  token_index tokens;
  control_token_table control_tokens;

  string normal_nft_sym = "NORMAL";
  string programmable_nft_sym = "PROGRAM";

  void sub_balance(name owner, asset value);
  void add_balance(name owner, asset value, name ram_payer);
  void sub_supply(asset quantity);
  void add_supply(asset quantity);
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
  id_type _safemint(name to, string symbol, string uri, string memo);
  id_type _mint(name owner, asset value, string uri);
};
