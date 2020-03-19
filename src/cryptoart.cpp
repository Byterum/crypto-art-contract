#include <cryptoart.hpp>

ACTION cryptoart::create(name issuer, asset max_supply) {
  require_auth(get_self());

  // Check if issuer account exists
  check(is_account(issuer), "issuer account does not exist");

  // Valid maximum supply symbol
  check(max_supply.symbol.is_valid(), "invalid symbol name");

  // check token type
  check(max_supply.symbol.precision() == 0,
        "max_supply must be a whole number");

  // Check if currency with symbol already exists
  auto sym_code = max_supply.symbol.code().raw();
  stat_index currency_table(get_self(), get_self().value);
  auto existing_currency = currency_table.find(sym_code);
  check(existing_currency == currency_table.end(),
        "token with symbol already exists");

  // Create new currency
  currency_table.emplace(get_self(), [&](auto &row) {
    row.supply = asset(0, max_supply.symbol);
    // if max_supply.amount == 0, means infinite supply.
    if (max_supply.amount == 0) {
      row.infinite = true;
    }
    row.issued = row.supply;
    row.max_supply = max_supply;
    row.issuer = issuer;
  });
}

id_type cryptoart::_safemint(name to, string symbol, string uri, string memo) {
  check(is_account(to), "to account does not exist");
  // e,g, Get EOS from 3 EOS
  auto sym = eosio::symbol(symbol.c_str(), 0);
  auto quantity = asset(1, sym);
  check(sym.is_valid(), "invalid symbol name");
  check(sym.precision() == 0, "quantity must be a whole number");
  check(memo.size() <= 256, "memo has more than 256 bytes");

  // Ensure currency has been created
  auto sym_code = sym.code().raw();
  stat_index statstable(get_self(), get_self().value);
  const auto &st = statstable.get(
      sym_code, "token with symbol does not exist. create token before issue");
  // Ensure have issuer authorization and valid quantity
  require_auth(st.issuer);

  // Increase supply
  statstable.modify(st, same_payer, [&](auto &s) { s.issued += quantity; });

  add_supply(quantity);
  // Add balance to account
  add_balance(to, quantity, to);
  // Mint nfts
  return _mint(to, asset(1, sym), uri);
}

ACTION cryptoart::transfer(name from, name to, id_type token_id, string memo) {
  // Ensure authorized to send from account
  check(from != to, "cannot transfer to self");
  require_auth(from);

  // Ensure 'to' account exists
  check(is_account(to), "to account does not exist");

  // Check memo size and print
  check(memo.size() <= 256, "memo has more than 256 bytes");

  // Ensure token ID exists
  const auto &st = tokens.get(token_id, "token does not exist");

  // Ensure owner owns token
  check(st.owner == from, "sender does not own token with specified ID");

  // Notify both recipients
  require_recipient(from);
  require_recipient(to);

  // Transfer NFT from sender to receiver
  tokens.modify(st, from, [&](auto &token) { token.owner = to; });

  // Change balance of both accounts
  sub_balance(from, st.value);
  add_balance(to, st.value, from);
}

id_type cryptoart::_mint(name owner, asset value, string uri) {
  id_type token_id = tokens.available_primary_key();
  // Add token with creator paying for RAM
  tokens.emplace(get_issuer(value.symbol.code()), [&](auto &token) {
    token.id = token_id;
    token.uuid = get_global_id(get_self(), token_id);
    token.uri = uri;
    token.owner = owner;
    token.value = value;
  });
  return token_id;
}

ACTION cryptoart::setrampayer(name payer, id_type id) {
  require_auth(payer);

  // Ensure token ID exists
  auto payer_token = tokens.find(id);
  check(payer_token != tokens.end(), "token with specified ID does not exist");

  // Ensure payer owns token
  check(payer_token->owner == payer,
        "payer does not own token with specified ID");

  const auto &st = *payer_token;

  // Notify payer
  require_recipient(payer);

  // Set owner as a RAM payer
  tokens.modify(payer_token, payer, [&](auto &token) {
    token.id = st.id;
    token.uri = st.uri;
    token.owner = st.owner;
    token.value = st.value;
  });

  sub_balance(payer, st.value);
  add_balance(payer, st.value, payer);
}

ACTION cryptoart::burn(name owner, global_id uuid, string memo) {
  require_auth(owner);

  // Find token to burn
  auto uuid_index = tokens.get_index<"byuuid"_n>();
  const auto &burn_token = uuid_index.get(uuid, "token with id does not exist");
  check(burn_token.owner == owner, "token not owned by account");

  asset burnt_supply = burn_token.value;

  // Remove token from tokens table
  tokens.erase(burn_token);
  // Lower balance from owner
  sub_balance(owner, burnt_supply);
  // Lower supply from currency
  sub_supply(burnt_supply);
}

void cryptoart::sub_balance(name owner, asset value) {
  account_index from_acnts(get_self(), owner.value);
  const auto &from =
      from_acnts.get(value.symbol.code().raw(), "no balance object found");
  check(from.balance.amount >= value.amount, "overdrawn balance");

  if (from.balance.amount == value.amount) {
    from_acnts.erase(from);
  } else {
    from_acnts.modify(from, same_payer, [&](auto &a) { a.balance -= value; });
  }
}

void cryptoart::add_balance(name owner, asset value, name ram_payer) {
  account_index to_accounts(get_self(), owner.value);
  auto to = to_accounts.find(value.symbol.code().raw());
  if (to == to_accounts.end()) {
    to_accounts.emplace(ram_payer, [&](auto &a) { a.balance = value; });
  } else {
    to_accounts.modify(to, ram_payer, [&](auto &a) { a.balance += value; });
  }
}

void cryptoart::sub_supply(asset quantity) {
  auto sym_code = quantity.symbol.code().raw();
  stat_index currency_table(get_self(), get_self().value);
  const auto &st = currency_table.get(sym_code, "asset dose not exist");
  check(st.supply >= quantity, "nft supply is not enough");
  currency_table.modify(st, same_payer,
                        [&](auto &currency) { currency.supply -= quantity; });
}

void cryptoart::add_supply(asset quantity) {
  auto sym_code = quantity.symbol.code().raw();
  check(quantity.is_valid(), "invalid quantity");
  check(quantity.amount > 0, "must issue positive quantity");

  stat_index currency_table(get_self(), get_self().value);
  const auto &st = currency_table.get(sym_code, "asset does not exist");

  check(st.infinite || st.issued + quantity < st.max_supply,
        "quantity should not be more than maximum supply");
  currency_table.modify(st, same_payer,
                        [&](auto &currency) { currency.supply += quantity; });
}

ACTION cryptoart::setuptoken(id_type token_id, vector<int64_t> min_values,
                             vector<int64_t> max_values,
                             vector<int64_t> curr_values) {
  // require owner's auth
  name owner = get_owner_by_id(token_id);
  require_auth(owner);

  // check the length of values lists are equal or not
  check(min_values.size() == max_values.size() &&
            max_values.size() == curr_values.size(),
        "values array size should be equal");
  auto levers_num = min_values.size();
  // get the token that is not setup
  const auto &token = control_tokens.get(token_id, "token not found");
  check(token.isSetup == false, "token was setup");
  // modify token structure to setup initial values
  control_tokens.modify(token, same_payer, [&](auto &r) {
    r.isSetup = true;
    r.levers_num = levers_num;
    r.min_values = min_values;
    r.max_values = max_values;
    r.curr_values = curr_values;
  });
}

ACTION cryptoart::mintartwork(name to, string uri, vector<name> collaborators) {
  name issuer = get_issuer(symbol_code(art_symbol));
  require_auth(issuer);
  // issue master layer token
  id_type master_token_id = _safemint(
      to, art_symbol, "mobius://crypto.art/ART/master?ipfs=" + uri, string(""));
  control_tokens.emplace(issuer, [&](auto &r) {
    // `token_id` and `master_token_id` are the same in master token
    r.id = master_token_id;
    r.master_token_id = master_token_id;
    r.levers_num = 0;
    r.isSetup = true;
  });
  // issue layer token to initial collaborators
  for (int i = 0; i < collaborators.size(); i++) {
    name collaborator = collaborators[i];
    id_type token_id = _safemint(collaborator, art_symbol,
                                 "mobius://crypto.art/ART/layer?master=" +
                                     to_string(master_token_id),
                                 string(""));
    control_tokens.emplace(issuer, [&](auto &r) {
      r.id = token_id;
      r.isSetup = false;
      r.master_token_id = master_token_id;
    });
  }
}

ACTION cryptoart::updatetoken(id_type token_id, vector<int64_t> lever_ids,
                              vector<int64_t> new_values) {
  name owner = get_owner_by_id(token_id);
  require_auth(owner);

  check(lever_ids.size() == new_values.size(),
        "length of lever_ids should be equal to new_values");
  const auto &token = control_tokens.get(token_id, "token not found");
  check(token.isSetup, "token is not setup");
  vector<int64_t> values = token.curr_values;
  for (int i = 0; i < lever_ids.size(); i++) {
    int lever_id = lever_ids[i];
    auto new_value = new_values[i];
    check(lever_id < token.curr_values.size(),
          "lever id should be lower than values length");
    check(new_value >= token.min_values[lever_id] &&
              new_value <= token.max_values[lever_id],
          "new value should be at the range of [" +
              to_string(token.min_values[lever_id]) + "," +
              to_string(token.max_values[lever_id]) + "]");
    values[lever_id] = new_value;
  }
  control_tokens.modify(token, same_payer,
                        [&](auto &r) { r.curr_values = values; });
}

EOSIO_DISPATCH(cryptoart,
               (create)(transfer)(burn)(setuptoken)(updatetoken)(mintartwork))
