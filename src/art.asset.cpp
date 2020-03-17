#include <art.asset.hpp>

// create new nft, if max_supply.amount == 0, means infinite supply.
ACTION artasset::create(name issuer, asset max_supply, int typ) {
  require_auth(get_self());

  // Check if issuer account exists
  check(is_account(issuer), "issuer account does not exist");

  // Valid maximum supply symbol
  check(max_supply.symbol.is_valid(), "invalid symbol name");

  // check token type
  check(typ == 0 || typ == 1, "invalid token type: should be `0` or `1`");
  if (typ == 1) {
    check(max_supply.symbol.precision() == 0,
          "max_supply must be a whole number");
  }

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
    row.type = typ;
  });
}

global_id artasset::mintnft(name to, string symbol, string uri, string memo) {
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
  check(st.type == 1, "this is fungible token, canno be issue as nft");
  // Ensure have issuer authorization and valid quantity
  require_auth(st.issuer);

  // Increase supply
  statstable.modify(st, same_payer, [&](auto &s) { s.issued += quantity; });

  add_supply(quantity);

  // Mint nfts
  global_id uuid = _mint(to, to, asset(1, sym), uri);
  // Add balance to account
  add_balance(to, quantity, to);
  return uuid;
}

ACTION artasset::transfernft(name from, name to, global_id uuid, string memo) {
  // Ensure authorized to send from account
  check(from != to, "cannot transfer to self");
  require_auth(from);

  // Ensure 'to' account exists
  check(is_account(to), "to account does not exist");

  // Check memo size and print
  check(memo.size() <= 256, "memo has more than 256 bytes");

  // Ensure token ID exists
  auto uuid_index = tokens.get_index<"byuuid"_n>();
  const auto &st =
      uuid_index.get(uuid, "token with specified ID does not exist");

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

global_id artasset::_mint(name owner, name ram_payer, asset value, string uri) {
  uint64_t tid = tokens.available_primary_key();
  global_id uuid = get_global_id(get_self(), tid);
  auto uuid_index = tokens.get_index<"byuuid"_n>();
  auto exist = uuid_index.find(uuid);
  check(exist == uuid_index.end(), "nft has exist");

  // Add token with creator paying for RAM
  tokens.emplace(ram_payer, [&](auto &token) {
    token.id = tid;
    token.uuid = uuid;
    token.uri = uri;
    token.owner = owner;
    token.value = value;
  });
  return uuid;
}

ACTION artasset::setrampayer(name payer, id_type id) {
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

ACTION artasset::burnnft(name owner, global_id uuid, string memo) {
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

void artasset::sub_balance(name owner, asset value) {
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

void artasset::add_balance(name owner, asset value, name ram_payer) {
  account_index to_accounts(get_self(), owner.value);
  auto to = to_accounts.find(value.symbol.code().raw());
  if (to == to_accounts.end()) {
    to_accounts.emplace(ram_payer, [&](auto &a) { a.balance = value; });
  } else {
    to_accounts.modify(to, ram_payer, [&](auto &a) { a.balance += value; });
  }
}

void artasset::sub_supply(asset quantity) {
  auto sym_code = quantity.symbol.code().raw();
  stat_index currency_table(get_self(), get_self().value);
  const auto &st = currency_table.get(sym_code, "asset dose not exist");
  check(st.supply >= quantity, "nft supply is not enough");
  currency_table.modify(st, same_payer,
                        [&](auto &currency) { currency.supply -= quantity; });
}

void artasset::add_supply(asset quantity) {
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

// EOSIO_DISPATCH(artasset, (create)(transfernft)(setrampayer)(burnnft))
