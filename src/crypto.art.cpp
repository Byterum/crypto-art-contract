#include <crypto.art.hpp>

ACTION cryptoart::setuptoken(global_id uuid, vector<int64_t> min_values,
                             vector<int64_t> max_values,
                             vector<int64_t> curr_values) {
  // require owner's auth
  name owner = get_owner(uuid);
  require_auth(owner);

  check(min_values.size() == max_values.size() &&
            max_values.size() == curr_values.size(),
        "values array size should be equal");
  auto levers_num = min_values.size();
  auto uuid_index = control_tokens.get_index<"byuuid"_n>();
  auto token = uuid_index.find(uuid);
  check(token != uuid_index.end() && token->isSetup == false,
        "token has been setup");
  uuid_index.modify(token, same_payer, [&](auto &r) {
    r.isSetup = true;
    r.levers_num = levers_num;
    r.min_values = min_values;
    r.max_values = max_values;
    r.curr_values = curr_values;
  });
}

ACTION cryptoart::mintartwork(name to, string uri, vector<name> collaborators) {
  require_auth(get_self());
  // issue master layer token
  global_id master_token_id =
      mintnft(to, programmable_nft_sym, uri, string(""));
  control_tokens.emplace(get_self(), [&](auto &r) {
    r.id = control_tokens.available_primary_key();
    r.uuid = master_token_id;
    r.levers_num = 0;
    r.isSetup = true;
  });
  // issue layer token to collaborators
  for (int i = 0; i < collaborators.size(); i++) {
    name collaborator = collaborators[i];
    global_id uuid = mintnft(collaborator, programmable_nft_sym,
                             "oasis://crypto.art/ART/programmable?master=" +
                                 uuid_to_string(master_token_id),
                             string(""));
    control_tokens.emplace(get_self(), [&](auto &r) {
      r.id = control_tokens.available_primary_key();
      r.uuid = uuid;
      r.isSetup = false;
    });
  }
}

ACTION cryptoart::updatetoken(global_id uuid, vector<uint64_t> lever_ids,
                              vector<int64_t> new_values) {
  name owner = get_owner(uuid);
  require_auth(owner);

  check(lever_ids.size() == new_values.size(),
        "length of lever_ids should be equal to new_values");
  token_table tokens(get_self(), get_self().value);
  auto index = tokens.get_index<"byuuid"_n>();
  auto token = index.get(uuid, "token not found");

  vector<int64_t> values = token.curr_values;
  for (int i = 0; i < lever_ids.size(); i++) {
    auto lever_id = lever_ids[i];
    check(new_values[lever_id] >= token.min_values[lever_id] &&
              new_values[lever_id] <= token.max_values[lever_id],
          "new value should be at the range of [" +
              to_string(token.min_values[lever_id]) + "," +
              to_string(token.max_values[lever_id]) + "]");
    values[lever_id] = new_values[lever_id];
  }
  tokens.modify(token, same_payer, [&](auto &r) { r.curr_values = values; });
}

EOSIO_DISPATCH(cryptoart, (setuptoken)(updatetoken)(mintartwork)(create)(
                              transfernft)(setrampayer)(burnnft))
