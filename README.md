# cryptoart

Crypto and programmable artwork, using non-fungible token to represent and release in EOSIO blockchain.

Inspired by [async.art](https://async.art/).

## How to mint artwork

### 1. Create two basic artwork token symbol

We design two different token symbol to tell the normal artworks and programmable artworks.

- `NORMAL` - means normal artwork token with only 1 **master** layer.
- `PROGRAM` - means programmable artwork token with 1 master layer and multiple sub layers controlled by other token-holders.

### 2. Call `mintartwork` to issue your art

The artist need to specify the uri of master token(usually IPFS cid of config data in JSON) to tell the [render](https://github.com/MobiusGame/crypto-art-render) how to cooperate with other layers to paint the final artwork at frontend.

The artist alse have to pass multiple collaborators to hold sub-layer tokens.

This action will issue NFTs to the specifed account, and after issuing, token holder can make effect on artwork by updating their layer token values.

At mobius crypto art, **we force all artworks only support IPFS storage network** to make more **decentralized**. The upper layers can also cache the buffer from IPFS for more efficient loading.

The valid uri format follows:

- Master Token: `mobius://crypto.art/ART/master?ipfs=${cid}`
- Layer Token: `mobius://crypto.art/ART/layer?master=${master_token_id}`, which will be generated automatically when mint artwork.

### 3. Call `setuptoken` to setup the control token

Only token holders can setup and pass basic values set to their layer tokens.

- `min_values` - minimum value this token can have
- `max_values` - maximum value this token can have
- `curr_values` - current value the render load from

We map layer token id as the index of value array of master token.

### 4. Call `updatetoken` to update current value of your layer token

After setup tokens, you can also update current value of each layer token you hold. **This will make effect on the final artwork.**

## How to bid artwork

### 0. Token holders call `auctiontoken` to make token auctionable.

Artists need to add their tokens to auction list and set a specific end time. Artist can also accept any current prices from bidders before the auction ends by calling `acceptbid`. If do so, the auction will be closed in advance.

### 1. Pay by PDH and get the qualification of bidding.

To get the qualification of bidding, you need to transfer a special token `PDH` to artwork contract account to get 1 chance of bidding.

### 2. 3rd-party call `bid` to bid the token.

Users that are interested in this artwork token can bid it. Each bidding will charge 1 chance as bidding qualification, and will give price larger than those before.

### 3. Make auction end and transfer tokens by calling `auctionend` or `acceptbid`.

We have two ways to end an auction and trigger token transferring:

- Token holders can call `acceptbid` to accept the final price.
- Bidders can call `auctionend` only after auction ends.

## License

MIT
