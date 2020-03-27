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
   * Add a token to auction list. This action can be called only by token
  holder.
   * @param token_id - Token unique id
   * @param min_price - Minimum price
   */
  ACTION auctiontoken(id_type token_id, asset min_price);

  /**
   * Bid a control token. Internal function
   * @param bidder - Token bidder account
   * @param token_id - Token unique id
   * @param price - Bid value
   */
  void bid(name bidder, id_type token_id, asset price);

  /**
   * Add qualification of bidding with one time.
   * @param bidder - Token bidder account
   */
  void addbidqual(name bidder, asset quantity);

  /**
   * Accept the final bid and sell token.
   * @param token_id - Token unique id
   */
  ACTION acceptbid(id_type token_id);

  [[eosio::on_notify("eosio.token::transfer")]] void payeos(
      name from, name to, asset quantity, string memo);

  [[eosio::on_notify("pandaheroast::transfer")]] void paypdh(
      name from, name to, asset quantity, string memo);

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

  name get_issuer(symbol_code sym) {
    stat_index stat(get_self(), get_self().value);
    auto info =
        stat.get(sym.raw(), "token stat not found, please create first");
    return info.issuer;
  }

  name get_owner_by_id(id_type token_id) {
    auto it = tokens.find(token_id);
    if (it == tokens.end()) {
      return name{};
    } else {
      return it->owner;
    }
  }

  TABLE account {
    asset balance; // DON'T MODIFIED.
    // You can add more fields here...

    uint64_t primary_key() const { return balance.symbol.code().raw(); }
  };

  TABLE bid_qualification {
    name owner;
    uint64_t avail_bid_time; // available bid time

    uint64_t primary_key() const { return owner.value; }
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

  TABLE auction {
    // token id
    id_type id;
    // top bidder
    name bidder;
    // current bid price
    asset curr_price;
    // auction status. 0 auction. 1 end.
    int status;

    id_type primary_key() const { return id; }
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

  using auction_index = multi_index<"auction"_n, auction>;

  using bid_qual = multi_index<"bidqual"_n, bid_qualification>;

  // generated token global uuid based on token id and
  // contract name, passed in the argument
  global_id get_global_id(name contract, id_type id) const {
    uint128_t self_128 = static_cast<uint128_t>(contract.value);
    uint128_t id_128 = static_cast<uint128_t>(id);
    uint128_t res = (self_128 << 64) | (id_128);
    return res;
  }

private:
  token_index tokens;
  control_token_table control_tokens;

  string art_symbol = "ART";

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
