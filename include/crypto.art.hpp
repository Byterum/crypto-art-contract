#include <art.asset.hpp>
#include <vector>

using namespace std;
using namespace eosio;

CONTRACT cryptoart : public artasset {
public:
  using artasset::artasset;
  cryptoart(name receiver, name code, datastream<const char *> ds)
      : artasset(receiver, code, ds), control_tokens(receiver, receiver.value) {
  }

  /**
   * Set up layer token after artist call `mintartwork`.
   * @param uuid - Token global id
   * @param min_values - Minimum values list with index as lever_id
   * @param max_values - Maximum values list with index as lever_id
   * @param curr_values - Current values list with indnex as lever_id
   */
  ACTION setuptoken(global_id uuid, vector<int64_t> min_values,
                    vector<int64_t> max_values, vector<int64_t> curr_values);

  /**
   * Mint master layer of art work to `to` and multiple layer tokens to
   * given collaborators.
   * @param to - Target artist holding the master layer
   * @param uri - URI for master layer. e.g. url of image
   * @param collaborators - Collaborate artists holding multiple layer tokens
   */
  ACTION mintartwork(name to, string uri, vector<name> collaborators);

  /**
   * Update token current value with given lever ids.
   * @param uuid - Token global id
   * @param lever_ids - Lever id list to update
   * @param new_values - New current value for levers indexed by `lever_ids`
   */
  ACTION updatetoken(global_id uuid, vector<uint64_t> lever_ids,
                     vector<int64_t> new_values);
  TABLE ControlToken {
    uint64_t id;
    global_id uuid;
    uint64_t levers_num;
    bool isSetup;
    // lever values
    vector<int64_t> min_values;
    vector<int64_t> max_values;
    vector<int64_t> curr_values;

    auto primary_key() const { return id; }
    uint128_t get_uuid() const { return uuid; }
  };

  typedef multi_index<
      name("tokens"), ControlToken,
      indexed_by<name("byuuid"), const_mem_fun<ControlToken, uint128_t,
                                               &ControlToken::get_uuid>>>
      token_table;

private:
  string normal_nft_sym = "CRYPTOART";
  string programmable_nft_sym = "PROGRAMART";
  token_table control_tokens;
};
